// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "fusion/gst/gstFormat.h"

#include <stdlib.h>
#include <alloca.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/khstl.h"
#include "common/khConstants.h"
#include "common/khFileUtils.h"
#include "common/notify.h"
#include "common/khTileAddr.h"
#include "common/khException.h"
#include "fusion/autoingest/AssetVersionRef.h"
#include "fusion/gst/gstFormatManager.h"
#include "fusion/gst/gstSpatialRef.h"

// remove trailing spaces
char* getStr(const char *buf, int nbuf, char *obuf) {
  memcpy(obuf, buf, nbuf);
  char *p;
  for (p = obuf + nbuf - 1; *p == ' ' && p >= obuf; --*p);
  *(p+1) = 0;
  return obuf;
}

// shift string to left, skipping past leading spaces
void strLeftShift(char *buf) {
  char *p = buf;
  while (*p == ' ')
    p++;

  strcpy(buf, p);
}

int getInt(const char *buf, int nbuf) {
  char *tbuf = static_cast<char*>(alloca(nbuf + 1));
  memcpy(tbuf, buf, nbuf);
  tbuf[nbuf] = 0;
  while (*tbuf && *tbuf == ' ')
    tbuf++;
  return atoi(tbuf);
}

 std::uint64_t getUInt64(const char *buf, int nbuf) {
  char *tbuf = static_cast<char*>(alloca(nbuf + 1));
  memcpy(tbuf, buf, nbuf);
  tbuf[nbuf] = 0;
  while (*tbuf && *tbuf == ' ')
    tbuf++;
  return strtoull(tbuf, NULL, 10);
}

double getDouble(const char *buf, int nbuf) {
  char *tbuf = static_cast<char*>(alloca(nbuf + 1));
  memcpy(tbuf, buf, nbuf);
  tbuf[nbuf] = 0;

  char *df;
  if ((df = strchr(tbuf, 'D')))
    *df = 'E';
  return strtod(tbuf, NULL);
}


int getIntB_le(const unsigned char *buf, int nbuf) {
  int val = 0;
  while (nbuf) {
    val <<= 8;
    val += buf[nbuf-1];
    nbuf--;
  }
  return val;
}

int getIntB(const char *buf, int nbuf) {
  int val = 0;
  while (nbuf) {
    val <<= 8;
    val += *buf;
    nbuf--;
    buf++;
  }
  return val;
}

////////////////////////////////////////////////////////////////////////////

gstLayerDef::gstLayerDef(gstPrimType type, std::uint32_t f, const gstHeaderHandle& h)
    : type_(type),
      num_features_(f),
      attrib_defs_(h),
      bounding_box_(),
      average_feature_diameter_(),
      min_resolution_level_(),
      max_resolution_level_(),
      efficient_resolution_level_() {
}

gstLayerDef::gstLayerDef(std::uint32_t f, const gstHeaderHandle& h)
    : type_(gstUnknown),
      num_features_(f),
      attrib_defs_(h),
      bounding_box_(),
      average_feature_diameter_(),
      min_resolution_level_(),
      max_resolution_level_(),
      efficient_resolution_level_() {
}

gstLayerDef::~gstLayerDef() {
}

void gstLayerDef::SetBoundingBox(const gstBBox& b) {
  bounding_box_ = b;
}

/////////////////////////////////////////////////////////////////////////////

gstFormat::gstFormat(const char *n)
    : gstMemory(n),
      spatial_ref_(NULL),
      nofileok(false),
      noXForm(false),
      is_mercator_imagery_(false) {
  seqReadLayer = 0;
  seqReadCurrID = 0;
  seqReadIsDone = true;
}

gstFormat::gstFormat(const char *n, bool is_mercator_imagery)
    : gstMemory(n),
      spatial_ref_(NULL),
      nofileok(false),
      noXForm(false),
      is_mercator_imagery_(is_mercator_imagery) {
  seqReadLayer = 0;
  seqReadCurrID = 0;
  seqReadIsDone = true;
}

gstFormat::~gstFormat() {
  delete spatial_ref_;
}

////////////////////////////////////////////////////////////////////////////////
//
// provide a default implementation of sequential
// reading for any loader that doesn't have native support
//
void gstFormat::DefaultResetReadingImpl(std::uint32_t layer) {
  seqReadLayer = layer;
  seqReadCurrID = 0;
  seqReadIsDone = false;
}

gstGeodeHandle gstFormat::GetNormCurrFeatureImpl(void) {
  gstGeodeHandle g = GetCurrentFeatureImpl();
  if (!g) {
    throw khSoftException("Unable to get current feature");
  }
  if (g->IsEmpty()) {
    throw khSoftException(kh::tr("Empty feature"));
  }
  if (!noXForm)
    TransformGeode(g);
  return g;
}

gstGeodeHandle gstFormat::DefaultGetCurrentFeatureImpl(void) {
  return GetFeatureImpl(seqReadLayer, seqReadCurrID);
}

gstRecordHandle gstFormat::DefaultGetCurrentAttributeImpl() {
  return GetAttributeImpl(seqReadLayer, seqReadCurrID);
}

void gstFormat::DefaultIncrementReadingImpl() {
  ++seqReadCurrID;
  seqReadIsDone = (seqReadCurrID >= NumFeatures(seqReadLayer));
}

bool gstFormat::DefaultIsReadingDoneImpl() {
  return seqReadIsDone;
}


////////////////////////////////////////////////////////////////////////////////

std::vector<gstLayerDef> gstFormat::GetNormLayerDefs() const {
  std::vector<gstLayerDef> ldefs = layer_defs_;

  if (noXForm)
    return ldefs;

  for (std::vector<gstLayerDef>::iterator it = ldefs.begin();
       it != ldefs.end(); ++it) {
    if ((*it).BoundingBox().Valid()) {
      double w = (*it).BoundingBox().w;
      double s = (*it).BoundingBox().s;
      VertexTransform(&w, &s);
      double e = (*it).BoundingBox().e;
      double n = (*it).BoundingBox().n;
      VertexTransform(&e, &n);
      (*it).SetBoundingBox(gstBBox(w, e, s, n));
    }
  }

  return ldefs;
}

namespace {

void RenormalizeForMercator(gstGeode *geode);

void RenormalizeForMercator(gstGeodeHandle geodeh) {
  switch (geodeh->PrimType()) {
    case gstUnknown:
      break;

    case gstPoint:
    case gstPoint25D:
    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
    case gstPolygon:
    case gstPolygon25D:
    case gstPolygon3D:
      {
        gstGeode* geode = static_cast<gstGeode*>(&(*geodeh));
        RenormalizeForMercator(geode);
      }
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      {
        gstGeodeCollection *multi_geode =
            static_cast<gstGeodeCollection*>(&(*geodeh));

        // Invalidate cached data of multi-polygon because
        // RenormalizeForMercator() through ModifyVertex() invalidates
        // internal data of geode(center and bounding box).
        // true means invalidate bounding box too.
        multi_geode->InvalidateCachedData(true);

        for (unsigned int p = 0; p < multi_geode->NumParts(); ++p) {
          gstGeode* geode =
              static_cast<gstGeode*>(&(*multi_geode->GetGeode(p)));
          RenormalizeForMercator(geode);
        }
      }
      break;
  }
}

void RenormalizeForMercator(gstGeode *geode) {
  for (unsigned int p = 0; p < geode->NumParts(); ++p) {
    for (unsigned int v = 0; v < geode->VertexCount(p); ++v) {
      gstVertex vert = geode->GetVertex(p, v);
      vert.y = MercatorProjection::
          FromNormalizedLatitudeToMercatorNormalizedLatitude(vert.y);
      geode->ModifyVertex(p, v, vert);
    }
  }
}

}  // end namespace

////////////////////////////////////////////////////////////////////////////////

gstGeodeHandle gstFormat::GetNormFeatureImpl(std::uint32_t layer, std::uint32_t id,
                                             bool is_mercator_preview) {
  gstGeodeHandle g = GetFeatureImpl(layer, id);
  if (!g) {
    throw khSoftException("Unable to get feature");
  }
  if (g->IsEmpty()) {
    throw khSoftException(kh::tr("Empty feature"));
  }
  if (!noXForm) {
    TransformGeode(g);
  }
  // in case of Mercator-Preview, features need renormalization.
  if (is_mercator_preview) {
    RenormalizeForMercator(g);
  }

  return g;
}

void gstFormat::TransformGeode(gstGeodeHandle geodeh) const {
  switch (geodeh->PrimType()) {
    case gstUnknown:
      break;

    case gstPoint:
    case gstPoint25D:
    case gstPolyLine:
    case gstPolyLine25D:
    case gstStreet:
    case gstStreet25D:
    case gstPolygon:
    case gstPolygon25D:
    case gstPolygon3D:
      {
        gstGeode* geode = static_cast<gstGeode*>(&(*geodeh));
        TransformGeode(geode);
      }
      break;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      {
        gstGeodeCollection *multi_geode =
            static_cast<gstGeodeCollection*>(&(*geodeh));

        // Invalidate cached data of multi-polygon because TransformGeode()
        // through ModifyVertex() invalidates internal data of geode(center
        // and bounding box).
        // true means invalidate bounding box too.
        multi_geode->InvalidateCachedData(true);

        for (unsigned int p = 0; p < multi_geode->NumParts(); ++p) {
          gstGeode* geode =
              static_cast<gstGeode*>(&(*multi_geode->GetGeode(p)));
          TransformGeode(geode);
        }
      }
      break;
  }
}

void gstFormat::TransformGeode(gstGeode *geode) const {
  for (unsigned int p = 0; p < geode->NumParts(); ++p) {
    for (unsigned int v = 0; v < geode->VertexCount(p); ++v) {
      const gstVertex& vert = geode->GetVertex(p, v);
      geode->ModifyVertex(p, v, TransformVertex(vert));
    }
  }
}

////////////////////////////////////////////////////////////////////////////////

void gstFormat::SetSpatialRef(gstSpatialRef* srs) {
  gstSpatialRef* old = spatial_ref_;
  spatial_ref_ = srs;
  delete old;
}

gstFormat* gstFormat::Open(const std::string& fname) {
  // search for a valid loader
  gstFormat* format = theFormatManager()->FindFormat(fname.c_str());
  if (format == NULL) {
    notify(NFY_WARN, "Unsupported file format");
    return NULL;
  }

  // call format-specific openFile function, which is
  // responsible for opening file
  gstStatus ostat = format->OpenFile();
  if (ostat != GST_OKAY) {
    if (ostat == GST_PERMISSION_DENIED)
      notify(NFY_WARN, "Permission denied");
    delete format;
    return NULL;
  }
  notify(NFY_DEBUG, "Format Type: %s (%d)",
         PrettyPrimType(format->layer_defs_[0].Type()).c_str(),
         format->layer_defs_[0].Type());

  //
  // look for spatial reference file (*.prj) only if format didn't provide one
  //
  if (format->SpatialRef() == NULL) {
    std::string prjfile = khReplaceExtension(fname, ".prj");
    if (!khExists(prjfile))
      prjfile = khReplaceExtension(fname, ".PRJ");
    if (khExists(prjfile)) {
      OGRSpatialReference *poSRS = new OGRSpatialReference();
      char **papszLines = CSLLoad(prjfile.c_str());
      if (poSRS->importFromESRI(papszLines) != OGRERR_NONE) {
        delete poSRS;
        notify(NFY_WARN, "Unable to properly parse projection file: %s",
               prjfile.c_str());
      } else {
        format->SetSpatialRef(new gstSpatialRef(poSRS));
      }
      CSLDestroy(papszLines);
    }
  }

  return format;
}

void gstFormat::SetCodec(QTextCodec* codec) {
  for (std::vector<gstLayerDef>::iterator it = layer_defs_.begin();
       it != layer_defs_.end(); ++it) {
    it->SetCodec(codec);
  }
}

gstBBox gstFormat::DefaultGetFeatureBoxImpl(std::uint32_t layer, std::uint32_t id) {
  gstGeodeHandle g = GetNormFeatureImpl(layer, id, false);
  if (!g) {
    throw khSoftException("Unable to get feature");
  }
  if (g->IsEmpty()) {
    throw khSoftException(kh::tr("Empty feature"));
  }
  return g->BoundingBox();
}

gstLayerDef& gstFormat::AddLayer(gstPrimType type, std::uint32_t n,
                                 const gstHeaderHandle& h) {
  layer_defs_.push_back(gstLayerDef(type, n, h));
  return layer_defs_[layer_defs_.size() - 1];
}

gstLayerDef& gstFormat::AddLayer(std::uint32_t n, const gstHeaderHandle& h) {
  layer_defs_.push_back(gstLayerDef(n, h));
  return layer_defs_[layer_defs_.size() - 1];
}

void gstFormat::AddLayer(const gstLayerDef& l) {
  layer_defs_.push_back(l);
}

gstVertex gstFormat::TransformVertex(const gstVertex& v) const {
  double x = v.x;
  double y = v.y;
  double z = 1;

  if (spatial_ref_)
    spatial_ref_->Transform(&x, &y, &z);

  // clamp values that are extremely close to the edges of max domain
  // anything further outside the domain should be a failure
  // this is to account for minor rounding errors that could easily
  // occur during many transformations of the geometry before we get it
  const double epsilon = 1e-5;
  const double min_x = 0.0;
  const double max_x = 1.0;
  const double min_y = is_mercator_imagery_ ? 0.0 : 0.25;
  const double max_y = is_mercator_imagery_ ? 1.0 : 0.75;
  double norm_x = NormalizeLongitude(x);
  if (norm_x < min_x && (min_x - norm_x < epsilon)) {
    notify(NFY_DEBUG, "Clamped x: %16.16lf", norm_x);
    norm_x = min_x;
  } else if (norm_x > max_x && (norm_x - max_x < epsilon)) {
    notify(NFY_DEBUG, "Clamped x: %16.16lf", norm_x);
    norm_x = max_x;
  }
  double norm_y = NormalizeLatitude(y);
  if (norm_y < min_y && (min_y - norm_y < epsilon)) {
    notify(NFY_DEBUG, "Clamped y: %16.16lf", norm_y);
    norm_y = min_y;
  } else if (norm_y > max_y && (norm_y - max_y < epsilon)) {
    notify(NFY_DEBUG, "Clamped y: %16.16lf", norm_y);
    norm_y = max_y;
  }

  return gstVertex(norm_x, norm_y, v.z);
}

void gstFormat::VertexTransform(double* x, double* y) const {
  double z;     // XXX ignore z
  if (spatial_ref_)
    spatial_ref_->Transform(x, y, &z);

  *x = NormalizeLongitude(*x);
  *y = NormalizeLatitude(*y);
}


int gstFormat::ParseToken(char *buf, char **val, gstFormatToken *token) {
  if (buf == NULL || *buf == 0)
    return -1;

  while (token->tag != -1) {
    if (!strncmp(token->desc, buf, strlen(token->desc))) {
      *val = buf + strlen(token->desc);
      while (**val && isspace(**val))
        (*val)++;
      return token->tag;
    }
    token++;
  }

  return -1;
}


//
// convenience functions for derived classes
//
char *gstFormat::getNextLine(FILE *fp) {
  if (fp <= 0)
    return NULL;

  // read a new line of text
  // ignore blank lines and leading white space


  while (!feof(fp)) {
    fgets(_lineBuf, FMT_LINEBUF, fp);
    if (feof(fp))
      return NULL;
    char *ptr = _lineBuf;

    // remove leading white-space
    while (*ptr && isspace(*ptr))
      ptr++;

    // remove trailing white-space and '\n'
    while (*ptr && (ptr[strlen(ptr) - 1] == '\n' ||
                    isspace(ptr[strlen(ptr) - 1])))
      ptr[strlen(ptr) -1] = 0;

    if (*ptr)
      return ptr;
  }

  return NULL;
}
