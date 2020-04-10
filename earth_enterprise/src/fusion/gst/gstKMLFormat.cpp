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


#include "fusion/gst/gstKMLFormat.h"

#include <stdlib.h>
#include <string>
#include <vector>

#include "common/notify.h"
#include "common/khFileUtils.h"
#include "common/khStringUtils.h"
#include "fusion/gst/gstRecord.h"
#include "fusion/gst/unzip/unzip.h"

// TODO: make KML parser more robust:
// - if any feature is invalid stop parsing and report unable to load.
// (e.g. Point with more then one vertex, Polygon with invalid inner boundary)
// - support multi-polygon features;

const int WRITEBUFFERSIZE = 8192;

const std::string gstKMLFormat::ZKMLFILE = "doc.kml";
const std::string gstKMLFormat::TMPDIR = khTmpDirPath();

const unsigned int kMinKMLPolylineVertices = 2;
const unsigned int kMinKMLPolygonCycleVertices = 4;   // first vertex == last vertex;

gstKMLFormat::gstKMLFormat(const char *n)
    : gstFormat(n),
      tmp_kml_file_(),
      is_kmz_(false),
      type_(kKmlUnknown),
      header_(),
      geodes_(),
      records_() {
  header_ = gstHeaderImpl::Create();
  header_->addSpec("name", gstTagString);
  header_->addSpec("description", gstTagString);
  header_->addSpec("LookAt", gstTagString);

  //    noXForm = true;
}

gstKMLFormat::~gstKMLFormat() {
}

void gstKMLFormat::GetElementsByTagName(khxml::DOMNode* parent,
                                        const std::string& tagname,
                                        khDOMElemList& elements) const {
  khxml::DOMNode *node = parent->getFirstChild();
  while (node) {
    if (node->getNodeType() == khxml::DOMNode::ELEMENT_NODE) {
      khxml::DOMElement *elem = (khxml::DOMElement*)node;
      if (FromXMLStr(elem->getTagName()) == tagname) {
        elements.push_back(elem);
      } else {
        GetElementsByTagName(node, tagname, elements);
      }
    }
    node = node->getNextSibling();
  }
}

//  QString gstKMLFormat::GetAllChildren(khxml::DOMNode* node) {
//  QString str = QString("");
//  if (!node)
//    return str;
//
//  khxml::DOMNode* child = node->getFirstChild();
//  while (child) {
//    str += XMLStr2QString(((khxml::DOMText*)child)->getData());
//    child = child->getNextSibling();
//  }
//  return str;
//  }

void gstKMLFormat::ParseKml(khxml::DOMNode* root) {
  const unsigned int kNameIdx = 0;
  const unsigned int kDescIdx = 1;
  const unsigned int kLookatIdx = 2;
  khDOMElemList placemarkList[3];
  gstKmlGeomType tags[3] = {kKmlPoint, kKmlLineString, kKmlPolygon};
  GetElementsByTagName(root, "Point", placemarkList[0]);
  GetElementsByTagName(root, "LineString", placemarkList[1]);
  GetElementsByTagName(root, "Polygon", placemarkList[2]);

  unsigned int max_size = 0;
  for (unsigned int p = 0; p < 3; p++) {
    khDOMElemList& placemarks = placemarkList[p];
    gstKmlGeomType type = tags[p];

    // get the dominant type in the KML
    if (placemarks.size() > max_size) {
      max_size = placemarks.size();
      type_ = type;
    }

    for (unsigned int i = 0; i < placemarks.size(); i++) {
      gstGeodeHandle geode;
      gstRecordHandle record = header_->NewRecord();
      ExtractFeature(type, placemarks[i], geode);
      if (geode) {
        khxml::DOMElement* parent
            = (khxml::DOMElement *) placemarks[i]->getParentNode();
        if (khxml::DOMElement* el_name = GetFirstNamedChild(parent, "name")) {
          QString name;
          FromElement(el_name, name);
          record->Field(kNameIdx)->set(name);
        }
        if (khxml::DOMElement* el_desc
            = GetFirstNamedChild(parent, "description")) {
          QString desc;
          FromElement(el_desc, desc);
          record->Field(kDescIdx)->set(desc);
        }
        if (khxml::DOMElement* el_lookAt
            = GetFirstNamedChild(parent, "LookAt")) {
          const unsigned int kTagsSize = 5;
          const QString tags[kTagsSize] = {"longitude", "latitude",
                                     "range", "tilt", "heading"};
          QString lookat;
          QString delimeter = "";
          for (unsigned int j = 0; j < kTagsSize; ++j) {
            if (khxml::DOMElement* el_tag
                = GetFirstNamedChild(el_lookAt,
                                     tags[j].toUtf8().constData())) {
              QString tag_data;
              FromElement(el_tag, tag_data);
              lookat += delimeter;
              lookat += tag_data;
              delimeter = "|";
            }
          }
          record->Field(kLookatIdx)->set(lookat);
        }
        geodes_.push_back(geode);
        records_.push_back(record);
      }
    }
  }
}

gstStatus gstKMLFormat::ParseKml(const std::string& kml_file) {
  gstStatus result = GST_OPEN_FAIL;
    std::unique_ptr<GEDocument> doc = ReadDocument(kml_file);
    if (doc) {
      try {
        khxml::DOMNode *root = (khxml::DOMNode*) doc->getDocumentElement();
        if (root) {
          ParseKml(root);
          if (geodes_.size() > 0) {
            result = GST_OKAY;
          } else {
            notify(NFY_WARN,
                   "Unable to load geometry from %s", kml_file.c_str());
            result = GST_PARSE_FAIL;
          }
        } else {
          notify(NFY_WARN,
                 "No document element loading %s", kml_file.c_str());
        }
      } catch(const std::exception &e) {
        notify(NFY_WARN, "%s while loading %s", e.what(), kml_file.c_str());
      } catch(...) {
        notify(NFY_WARN, "Unable to load %s", kml_file.c_str());
      }
    } else {
      notify(NFY_WARN, "Unable to read %s", kml_file.c_str());
    }

  return result;
}


int gstKMLFormat::ExtractKml(const std::string &kmz_file) {
  unzFile uf = unzOpen(kmz_file.c_str());
  if (NULL == uf) {
    notify(NFY_WARN, "Cannot open %s", kmz_file.c_str());
    return -1;
  }

  if (unzLocateFile(uf, ZKMLFILE.c_str(), 0) != UNZ_OK) {
    notify(NFY_WARN, "File %s not found in the zipfile", ZKMLFILE.c_str());
    return -1;
  }

  int result = unzOpenCurrentFile(uf);
  if (result != UNZ_OK) {
    notify(NFY_WARN, "Error %d with zipfile in unzOpenCurrentFile\n", result);
  }

  tmp_kml_file_ = TMPDIR + "/" + ZKMLFILE;

  tmp_kml_file_ = khTmpFilename(tmp_kml_file_);

  FILE* fout = ::fopen(tmp_kml_file_.c_str(), "wb");
  int errno_fopen = errno;
  if (NULL == fout) {
    notify(NFY_WARN, "Unable to open temporary file %s for unzip %s: %s",
           tmp_kml_file_.c_str(), kmz_file.c_str(),
           khstrerror(errno_fopen).c_str());
    return -1;
  }

  char buf[WRITEBUFFERSIZE];
  do {
    result = unzReadCurrentFile(uf, buf, WRITEBUFFERSIZE);
    if (result < 0) {
      notify(NFY_WARN, "Error %d with zipfile in unzReadCurrentFile", result);
      break;
    }
    if (result > 0) {
      if (fwrite(buf, result, 1, fout) != 1) {
        notify(NFY_WARN, "Error in writing extracted file");
        result = UNZ_ERRNO;
        break;
      }
    }
  }
  while (result > 0);

  fclose(fout);
  return result;
}

gstStatus gstKMLFormat::OpenFile() {
  gstStatus result = GST_OPEN_FAIL;

  std::string kml_file(name());
  is_kmz_ = false;

  if (kml_file.find(".kmz", 0) != std::string::npos
      || kml_file.find(".KMZ", 0) != std::string::npos) {
    if (ExtractKml(kml_file) >= 0) {
      kml_file = tmp_kml_file_;  // tmp_kml_file_ is created in ExtractKml();
      is_kmz_ = true;
    }
  }

  result = ParseKml(kml_file);
  if (GST_OKAY == result) {
    AddLayer(GetPrimType(), geodes_.size(), header_);
  }

  // Note: we parse kml-file into geodes and we can close
  // file here.
  CloseFile();

  return result;
}

gstStatus gstKMLFormat::CloseFile() {
  if (is_kmz_) {
    unlink(tmp_kml_file_.c_str());
  }
  return GST_OKAY;
}

bool gstKMLFormat::ExtractCoordinates(khxml::DOMElement* elem,
                                      gstVertexVector& verts) const {
  verts.clear();

  bool is_25d_extension = false;

  if (khxml::DOMElement* coordinates
      = GetFirstNamedChild(elem, "coordinates") ) {
    if (khxml::DOMText* text
        = (khxml::DOMText*) coordinates->getFirstChild()) {
      std::string xyz_str = FromXMLStr(text->getData());
      std::vector<std::string> xyz_tokens;
      TokenizeString(xyz_str, xyz_tokens, " \n\t");
      verts.reserve(xyz_tokens.size());
      for (unsigned int i = 0; i < xyz_tokens.size(); i++) {
        gstVertex v;
        std::vector<std::string> xyz;
        TokenizeString(xyz_tokens[i], xyz, ",");
        if (xyz.size() > 1) {
          const unsigned int kXIdx = 0;
          const unsigned int kYIdx = 1;
          const unsigned int kZIdx = 2;
          char *end_ptr;
          v.x = ::strtod(xyz[kXIdx].c_str(), &end_ptr);
          v.y = ::strtod(xyz[kYIdx].c_str(), &end_ptr);
          if (xyz.size() > 2) {
            is_25d_extension = true;
            v.z = ::strtod(xyz[kZIdx].c_str(), &end_ptr);
          } else {
            v.z = .0;
          }
          verts.push_back(v);
        } else {
          verts.clear();
          notify(NFY_WARN, "Invalid coordinates");
          break;
        }
      }
    } else {
      notify(NFY_WARN, "Invalid coordinates");
    }
  } else {
    notify(NFY_WARN, "Invalid coordinates");
  }

  return is_25d_extension;
}

void gstKMLFormat::ExtractFeature(const gstKmlGeomType type,
                                  khxml::DOMElement* elem,
                                  gstGeodeHandle& geode) {
  switch (type) {
    case kKmlUnknown:
      assert(0);
      break;

    case kKmlPoint:
      AddPoint(elem, geode);
      break;

    case kKmlLineString:
      AddLineString(elem, geode);
      break;

    case kKmlPolygon:
      AddPolygon(elem, geode);
      break;

    default:
      notify(NFY_FATAL, "Invalid KML geometry type.");
      break;
  }
}

void gstKMLFormat::AddPoint(khxml::DOMElement* elem,
                            gstGeodeHandle& geodeh) {
  gstVertexVector verts;
  bool is_25d_extension = ExtractCoordinates(elem, verts);

  if (verts.empty()) {
    notify(NFY_WARN, "Invalid point feature.");
    return;
  }

  gstPrimType prim_type = is_25d_extension ? gstPoint25D : gstPoint;

  if (!geodeh) {
    geodeh = gstGeodeImpl::Create(prim_type);
  }

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));

  for (unsigned int i = 0; i < verts.size(); i++) {
    // NOTE: Each part of point feature contains exactly one point.
    // See gstGeode::ToRaw(). So we add part here.
    // TODO: refactor - use one part for all points?!
    geode->AddPart(1);
    geode->AddVertex(verts[i]);
  }
}

void gstKMLFormat::AddLineString(khxml::DOMElement* elem,
                                 gstGeodeHandle& geodeh) {
  gstVertexVector verts;
  bool is_25d_extension = ExtractCoordinates(elem, verts);

  if (verts.size() < kMinKMLPolylineVertices) {
    notify(NFY_WARN, "Invalid polyline feature.");
    return;
  }

  gstPrimType prim_type = is_25d_extension ? gstPolyLine25D : gstPolyLine;

  if (!geodeh) {
    geodeh = gstGeodeImpl::Create(prim_type);
  }

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
  geode->AddPart(verts.size());

  for (unsigned int i = 0; i < verts.size(); i++) {
    geode->AddVertex(verts[i]);
  }
}

void gstKMLFormat::AddPolygon(khxml::DOMElement* elem,
                              gstGeodeHandle& geodeh) {
  if (khxml::DOMElement* outerBoundaryIs
      = GetFirstNamedChild(elem, "outerBoundaryIs")) {
    if (khxml::DOMElement* linearRing
        = GetFirstNamedChild(outerBoundaryIs, "LinearRing") ) {
      // Get outer boundary.
      gstVertexVector verts;
      bool is_25d_extension = ExtractCoordinates(linearRing, verts);

      gstPrimType prim_type = is_25d_extension ? gstPolygon25D : gstPolygon;
      AddOuterBoundary(verts, prim_type, geodeh);

      // Check if outer boundary is valid.
      if (geodeh && (!geodeh->IsEmpty())) {
        // Get inner boundaries.
        khDOMElemList inners = GetChildrenByTagName(elem, "innerBoundaryIs");
        for (unsigned int i = 0; i < inners.size(); i++) {
          if (khxml::DOMElement* linearRing
              = GetFirstNamedChild(inners[i], "LinearRing")) {
            gstVertexVector verts;
            ExtractCoordinates(linearRing, verts);
            AddInnerBoundary(verts, prim_type, geodeh);
          }
        }
      }
    }
  }
}

void gstKMLFormat::AddOuterBoundary(const gstVertexVector& verts,
                                    gstPrimType type,
                                    gstGeodeHandle& geodeh) const {
  assert(!geodeh);
  assert(gstPolygon == type || gstPolygon25D == type);
  if (verts.size() < kMinKMLPolygonCycleVertices) {
    notify(NFY_WARN, "Invalid polygon(outer boundary): skipped.");
    return;
  }

  geodeh = gstGeodeImpl::Create(type);

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
  geode->AddPart(verts.size());

  // The <coordinates> in KML polygon must be specified in counterclockwise
  // order.
  for (unsigned int i = 0; i < verts.size(); i++) {
    geode->AddVertex(verts[i]);
  }
}

void gstKMLFormat::AddInnerBoundary(const gstVertexVector& verts,
                                    gstPrimType type,
                                    gstGeodeHandle& geodeh) const {
  assert(geodeh);
  assert(gstPolygon == type || gstPolygon25D == type);
  if (verts.size() < kMinKMLPolygonCycleVertices) {
    notify(NFY_WARN, "Invalid inner boundary of polygon: skipped.");
    return;
  }

  gstGeode *geode = static_cast<gstGeode*>(&(*geodeh));
  geode->AddPart(verts.size());
  // The <coordinates> in KML polygon must be specified in counterclockwise
  // order. We need to revert it for inner boundaries for further processing.
  for (int i = verts.size() - 1; i >= 0; --i) {
    geode->AddVertex(verts[i]);
  }
}

gstGeodeHandle gstKMLFormat::GetFeatureImpl(std::uint32_t layer, std::uint32_t fidx) {
  // should be checked by gstSource before calling me
  assert(layer < NumLayers());
  assert(0 == layer);
  assert(fidx < NumFeatures(layer));

  return geodes_[fidx]->Duplicate();
}

gstRecordHandle gstKMLFormat::GetAttributeImpl(std::uint32_t layer, std::uint32_t fidx) {
  // should be checked by gstSource before calling me
  assert(layer < NumLayers());
  assert(0 == layer);
  assert(fidx < NumFeatures(layer));

  return records_[fidx];
}

gstPrimType gstKMLFormat::GetPrimType() const {
  switch (type_) {
    case kKmlUnknown:
      return gstUnknown;

    case kKmlPoint:
      return gstPoint;

    case kKmlLineString:
      return gstPolyLine;

    case kKmlPolygon:
      return gstPolygon;

    default:
      notify(NFY_FATAL, "Invalid KML geometry type.");
      return gstUnknown;
  }
}
