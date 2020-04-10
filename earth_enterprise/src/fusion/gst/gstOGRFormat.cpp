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


#include "fusion/gst/gstOGRFormat.h"

#include "fusion/gst/.idl/gstDBSource.h"

#include "common/notify.h"
#include "common/khGuard.h"
#include "common/khException.h"

#include "fusion/khgdal/khgdal.h"
#include "fusion/gst/gstTypes.h"
#include "fusion/gst/gstGeode.h"
#include "fusion/gst/gstMisc.h"
#include "fusion/gst/gstSpatialRef.h"
#include "fusion/gst/gstRecord.h"

static bool RegisterOnce = true;

gstOGRFormat::gstOGRFormat(const char *n)
    : gstFormat(n),
      currentLayer(0),
      currentFeature(0),
      layers() {
  // register OGR Formats
  if (RegisterOnce) {
    RegisterOnce = false;

    GDALAllRegister();

    if (getNotifyLevel() >= NFY_DEBUG) {
      GDALDriverManager *poR = GetGDALDriverManager();
      for (int iDriver = 0; iDriver < poR->GetDriverCount(); ++iDriver)
        notify(NFY_DEBUG,
               "  OGR Supports:  %s", poR->GetDriver(iDriver)->GetDescription());
    }
  }

  data_source_ = NULL;
}

gstOGRFormat::~gstOGRFormat() {
  if (currentFeature)
    OGRFeature::DestroyFeature(currentFeature);

  CloseFile();
}

gstStatus gstOGRFormat::CloseFile() {
  if (data_source_) {
    delete data_source_;
    data_source_ = NULL;
  }
  return GST_OKAY;
}

gstStatus gstOGRFormat::OpenFile() {
  notify(NFY_DEBUG, "OGR Loader opening file %s", name());

  data_source_ = (GDALDataset *) GDALOpenEx(name(),
                                            GDAL_OF_VECTOR | GDAL_OF_READONLY,
                                            NULL, NULL, NULL);

  if (data_source_ == NULL)
    return GST_OPEN_FAIL;

  if (data_source_->GetLayerCount() == 0) {
    notify(NFY_WARN, "data source has no layers!");
    return GST_UNKNOWN;
  }

  notify(NFY_INFO, "OGR Successfully opened file: %s", name());

  //
  // get spatial reference from first layer.
  //
  // this must be the same for all layers of file.
  // XXX this might not be a true statement XXX
  //
  OGRLayer* poLayer = data_source_->GetLayer(0);
  assert(poLayer != NULL);
  OGRSpatialReference* srs = poLayer->GetSpatialRef();
  if (srs != NULL)
    SetSpatialRef(new gstSpatialRef(srs));

  //
  // find attribute header
  //

  layers.reserve(data_source_->GetLayerCount());
  for (int layer = 0; layer < data_source_->GetLayerCount(); ++layer) {
    OGRLayer *poLayer = data_source_->GetLayer(layer);

    if (poLayer == NULL) {
      notify(NFY_WARN, "Couldn't fetch layer %d!", layer);
      continue;
    }

    AddOGRLayer(layer, poLayer);

    // XXX need to figure out what is the starting feature ID
    // XXX this is wrong according to GDAL dev.
    // XXX apparently the feature id's don't have to be sequential
    // XXX but for now, let's make this assumption since it seems
    // XXX to work for the few data formats that we use OGR for
    // XXX (SHP, MapInfo & Tiger)

    int off = 0;

    poLayer->ResetReading();
    OGRFeature* first = poLayer->GetNextFeature();
    if (first != NULL) {
      off = first->GetFID();
      OGRFeature::DestroyFeature(first);
    }

    feature_id_offset_.push_back(off);
  }
  return GST_OKAY;
}

OGRLayer* gstOGRFormat::GetOGRLayer(int layer) const {
  if (layer < static_cast<int>(layers.size())) {
    return layers[layer];
  }
  return 0;
}

gstStatus gstOGRFormat::AddOGRLayer(int layer, OGRLayer* poLayer) {
  gstHeaderHandle header = gstHeaderImpl::Create();

  if (static_cast<int>(layers.size()) <= layer) {
    layers.resize(layer+1);
    layers[layer] = poLayer;
  }

  OGRFeatureDefn* featureDefn = poLayer->GetLayerDefn();

  notify(NFY_DEBUG, " [ %d ] %s", layer, featureDefn->GetName());

  if (featureDefn && featureDefn->GetFieldCount() != 0) {
    notify(NFY_DEBUG,
           "  Retrieving %d field definitions", featureDefn->GetFieldCount());
    for (int def = 0; def < featureDefn->GetFieldCount(); ++def) {
      OGRFieldDefn* fieldDefn = featureDefn->GetFieldDefn(def);
      const char* name = fieldDefn->GetNameRef();
      std::uint32_t type = 0;
      switch (fieldDefn->GetType()) {
        case OFTInteger:
        case OFTIntegerList:
          type = gstTagInt;
          break;
        case OFTReal:
        case OFTRealList:
          type = gstTagDouble;
          break;
        case OFTString:
        case OFTStringList:
          type = gstTagString;
          break;
        case OFTWideString:
        case OFTWideStringList:
        case OFTBinary:
        default:
          notify(NFY_WARN,
                 "attribute type %d not supported, converting to string.",
                 fieldDefn->GetType());
          type = gstTagString;
          break;
      }
      header->addSpec(name, type);
      notify(NFY_DEBUG, "    [ %d ] %s (%d)", def, name, type);
    }
  } else {
    notify(NFY_WARN, "Can't find attribute definitions.");
  }

  gstPrimType type = gstUnknown;
  switch (featureDefn->GetGeomType()) {
    case wkbNone:
    case wkbLinearRing:
    case wkbGeometryCollection25D:
    case wkbUnknown:
    case wkbGeometryCollection:
    case wkbCircularString:
    case wkbCompoundCurve:
    case wkbCurvePolygon:
    case wkbMultiCurve:
    case wkbMultiSurface:
    case wkbCurve:
    case wkbSurface:
    case wkbPolyhedralSurface:
    case wkbTIN:
    case wkbTriangle:
    case wkbCircularStringZ:
    case wkbCompoundCurveZ:
    case wkbCurvePolygonZ:
    case wkbMultiCurveZ:
    case wkbMultiSurfaceZ:
    case wkbCurveZ:
    case wkbSurfaceZ:
    case wkbPolyhedralSurfaceZ:
    case wkbTINZ:
    case wkbTriangleZ:
    case wkbPointM:
    case wkbLineStringM:
    case wkbPolygonM:
    case wkbMultiPointM:
    case wkbMultiLineStringM:
    case wkbMultiPolygonM:
    case wkbGeometryCollectionM:
    case wkbCircularStringM:
    case wkbCompoundCurveM:
    case wkbCurvePolygonM:
    case wkbMultiCurveM:
    case wkbMultiSurfaceM:
    case wkbCurveM:
    case wkbSurfaceM:
    case wkbPolyhedralSurfaceM:
    case wkbTINM:
    case wkbTriangleM:
    case wkbPointZM:
    case wkbLineStringZM:
    case wkbPolygonZM:
    case wkbMultiPointZM:
    case wkbMultiLineStringZM:
    case wkbMultiPolygonZM:
    case wkbGeometryCollectionZM:
    case wkbCircularStringZM:
    case wkbCompoundCurveZM:
    case wkbCurvePolygonZM:
    case wkbMultiCurveZM:
    case wkbMultiSurfaceZM:
    case wkbCurveZM:
    case wkbSurfaceZM:
    case wkbPolyhedralSurfaceZM:
    case wkbTINZM:
    case wkbTriangleZM:
      if (featureDefn->GetGeomType() != wkbUnknown) {
        notify(NFY_DEBUG, "Mapping OGR feature type %s to Unknown",
               OGRGeometryTypeToName(featureDefn->GetGeomType()));
      }
      type = gstUnknown;
      break;

    case wkbPoint:
    case wkbPoint25D:
    case wkbMultiPoint:
    case wkbMultiPoint25D:
      type = gstPoint;
      break;

    case wkbLineString:
    case wkbLineString25D:
    case wkbMultiLineString:
    case wkbMultiLineString25D:
      type = gstPolyLine;
      break;

    case wkbPolygon:
    case wkbPolygon25D:
    case wkbMultiPolygon:
    case wkbMultiPolygon25D:
      type = gstPolygon;
      break;
  }

  if (type == gstUnknown) {
    notify(NFY_WARN, "Unknown feature type: %u ", type);
    // return GST_UNKNOWN;
  }

  std::uint32_t featureCount = poLayer->GetFeatureCount();
  gstLayerDef& ldef = AddLayer(type, featureCount, header);

  if (featureCount == 0) {
    notify(NFY_WARN, "Layer %d has no features", layer);
    return GST_OKAY;
  }

  // compute bounding box
  // OGR returns the box in local coordinates, so we may need to reproject them
  OGREnvelope extents;
  if (poLayer->GetExtent(&extents, true) == OGRERR_NONE) {
    // get all four corners
    gstVertex ll(extents.MinX, extents.MinY);
    gstVertex ul(extents.MinX, extents.MaxY);
    gstVertex lr(extents.MaxX, extents.MaxY);
    gstVertex ur(extents.MaxX, extents.MinY);


    // expand bbox out by all four corners
    gstBBox box(ll.x, ur.x, ll.y, ur.y);    // lower-left, upper-right box
    box.Grow(ul.x, ul.y);                   // grow by upper-left
    box.Grow(lr.x, lr.y);                   // grow by lower-right

    ldef.SetBoundingBox(box);
  }

  return GST_OKAY;
}

void gstOGRFormat::ResetReadingImpl(std::uint32_t layer) {
  seqReadIsDone = false;
  currentLayer = layer;
  OGRLayer* poLayer = GetOGRLayer(layer);
  if (!poLayer) {
    throw khException(kh::tr("Cannot get layer"));
  }

  poLayer->ResetReading();
  if (currentFeature)
    OGRFeature::DestroyFeature(currentFeature);
  currentFeature = poLayer->GetNextFeature();

  seqReadIsDone = (currentFeature == NULL);
}

gstGeodeHandle gstOGRFormat::GetCurrentFeatureImpl(void) {
  if (currentFeature)
    return TranslateGeometry(currentFeature);

  throw khException(kh::tr("No current features (past end?)"));
}

gstRecordHandle gstOGRFormat::GetCurrentAttributeImpl() {
  if (currentFeature) {
    return TranslateAttribute(currentFeature, currentLayer);
  }

  throw khException(kh::tr("No current attributes (past end?)"));
}

void gstOGRFormat::IncrementReadingImpl() {
  OGRLayer* poLayer = GetOGRLayer(currentLayer);
  if (!poLayer) {
    throw khException(kh::tr("Cannot get layer"));
  }

  if (currentFeature)
    OGRFeature::DestroyFeature(currentFeature);

  currentFeature = poLayer->GetNextFeature();

  seqReadIsDone = (currentFeature == NULL);
}

gstGeodeHandle gstOGRFormat::GetFeatureImpl(std::uint32_t layer, std::uint32_t fidx) {
  // should be checked by gstSource before calling me
  assert(layer < NumLayers());
  assert(fidx < NumFeatures(layer));

  // fidx adjustment
  fidx += feature_id_offset_[layer];

  // get the OGR layer
  OGRLayer* poLayer = GetOGRLayer(layer);
  if (!poLayer) {
    throw khException(kh::tr("Cannot get layer"));
  }

  // pull the feature from the current layer
  OGRFeature* poFeature = poLayer->GetFeature(fidx);
  if (poFeature == NULL) {
    throw khSoftException(kh::tr("Cannot get feature"));
  }

  // make sure the temporary gets cleaned up (no matter what)
  khCallGuard<OGRFeature*, void> destroyer(OGRFeature::DestroyFeature,
                                           poFeature);

  // convert to our type and return
  return TranslateGeometry(poFeature);
}


gstRecordHandle gstOGRFormat::GetAttributeImpl(std::uint32_t layer, std::uint32_t fidx) {
  // should be checked by gstSource before calling me
  assert(layer < NumLayers());
  assert(fidx < NumFeatures(layer));

  // fidx adjustment
  fidx += feature_id_offset_[layer];

  // get the OGR layer
  OGRLayer* poLayer = GetOGRLayer(layer);
  if (!poLayer) {
    throw khException(kh::tr("Cannot get layer"));
  }

  // pull the feature from the current layer
  OGRFeature* poFeature = poLayer->GetFeature(fidx);
  if (poFeature == NULL) {
    throw khSoftException(kh::tr("Cannot get attribute"));
  }

  // make sure the temporary gets cleaned up (no matter what)
  khCallGuard<OGRFeature*, void> destroyer(OGRFeature::DestroyFeature,
                                           poFeature);

  return TranslateAttribute(poFeature, layer);
}



gstGeodeHandle gstOGRFormat::TranslateGeometry(OGRFeature* feature) const {
  OGRGeometry* geom = feature->GetGeometryRef();

  // confirm geometry exists
  if (!geom) {
    throw khSoftException(kh::tr("Cannot get feature geometry"));
  }

  if (geom->IsEmpty()) {
    throw khSoftException(kh::tr("Empty feature"));
  }


  gstGeodeHandle geode;
  OGRwkbGeometryType ogr_geometry_type = geom->getGeometryType();
  const char* wkb_type_string = NULL;
  int geometry_count = 1;
  switch (ogr_geometry_type) {
    case wkbUnknown:
      throw khSoftException(kh::tr("Unknown geometry type"));

    case wkbPoint25D:
      wkb_type_string = "wkbPoint25D";
      AddPoint(static_cast<OGRPoint*>(geom), gstPoint25D, &geode);
      break;

    case wkbPoint:
      wkb_type_string = "wkbPoint";
      AddPoint(static_cast<OGRPoint*>(geom), gstPoint, &geode);
      break;

    case wkbLineString25D:
      wkb_type_string = "wkbLineString25D";
      AddString(static_cast<OGRLineString*>(geom), gstPolyLine25D, &geode);
      break;

    case wkbLineString:
      wkb_type_string = "wkbLineString";
      AddString(static_cast<OGRLineString*>(geom), gstPolyLine, &geode);
      break;

    case wkbPolygon:
      {
        wkb_type_string = "wkbPolygon";
        OGRPolygon *poly = static_cast<OGRPolygon*>(geom);
        geode = GetPolygon(poly, gstPolygon);
      }
      break;

    case wkbPolygon25D:
      {
        wkb_type_string = "wkbPolygon25D";
        OGRPolygon *poly = static_cast<OGRPolygon*>(geom);
        geode = GetPolygon(poly, gstPolygon25D);
      }
      break;

    case wkbMultiPoint25D:
      wkb_type_string = "wkbMultiPoint25D";
      geometry_count = AddMultiPoint(geom, gstPoint25D, &geode);
      break;

    case wkbMultiPoint:
      wkb_type_string = "wkbMultiPoint";
      geometry_count = AddMultiPoint(geom, gstPoint, &geode);
      break;

    case wkbMultiLineString25D:
      wkb_type_string = "wkbMultiLineString25D";
      geometry_count = AddMultiString(geom, gstPolyLine25D, &geode);
      break;

    case wkbMultiLineString:
      wkb_type_string = "wkbMultiLineString";
      geometry_count = AddMultiString(geom, gstPolyLine, &geode);
      break;

    case wkbMultiPolygon:
      {
        wkb_type_string = "wkbMultiPolygon";

        OGRMultiPolygon *multi_poly = static_cast<OGRMultiPolygon*>(geom);
        geometry_count = multi_poly->getNumGeometries();
        geode = GetMultiPolygon(multi_poly, gstMultiPolygon);
      }
      break;

    case wkbMultiPolygon25D:
      {
        wkb_type_string = "wkbMultiPolygon25D";

        OGRMultiPolygon *multi_poly = static_cast<OGRMultiPolygon*>(geom);
        geometry_count = multi_poly->getNumGeometries();
        geode = GetMultiPolygon(multi_poly, gstMultiPolygon25D);
      }
      break;

    case wkbGeometryCollection:
    case wkbGeometryCollection25D:
    case wkbNone:
    case wkbLinearRing:
  case wkbCircularString:
  case wkbCompoundCurve:
  case wkbCurvePolygon:
  case wkbMultiCurve:
  case wkbMultiSurface:
  case wkbCurve:
  case wkbSurface:
  case wkbPolyhedralSurface:
  case wkbTIN:
  case wkbTriangle:
  case wkbCircularStringZ:
  case wkbCompoundCurveZ:
  case wkbCurvePolygonZ:
  case wkbMultiCurveZ:
  case wkbMultiSurfaceZ:
  case wkbCurveZ:
  case wkbSurfaceZ:
  case wkbPolyhedralSurfaceZ:
  case wkbTINZ:
  case wkbTriangleZ:
  case wkbPointM:
  case wkbLineStringM:
  case wkbPolygonM:
  case wkbMultiPointM:
  case wkbMultiLineStringM:
  case wkbMultiPolygonM:
  case wkbGeometryCollectionM:
  case wkbCircularStringM:
  case wkbCompoundCurveM:
  case wkbCurvePolygonM:
  case wkbMultiCurveM:
  case wkbMultiSurfaceM:
  case wkbCurveM:
  case wkbSurfaceM:
  case wkbPolyhedralSurfaceM:
  case wkbTINM:
  case wkbTriangleM:
  case wkbPointZM:
  case wkbLineStringZM:
  case wkbPolygonZM:
  case wkbMultiPointZM:
  case wkbMultiLineStringZM:
  case wkbMultiPolygonZM:
  case wkbGeometryCollectionZM:
  case wkbCircularStringZM:
  case wkbCompoundCurveZM:
  case wkbCurvePolygonZM:
  case wkbMultiCurveZM:
  case wkbMultiSurfaceZM:
  case wkbCurveZM:
  case wkbSurfaceZM:
  case wkbPolyhedralSurfaceZM:
  case wkbTINZM:
  case wkbTriangleZM:
      throw khSoftException(
          kh::tr("Unsupported OGR geometry type: %1")
          .arg(OGRGeometryTypeToName(ogr_geometry_type)));
  }
  notify(NFY_VERBOSE, "%s %d", wkb_type_string, geometry_count);

  return geode;
}



gstRecordHandle gstOGRFormat::TranslateAttribute(
    OGRFeature *feature, unsigned int layerid) {
  // get the header for this layer
  gstHeaderHandle header = layer_defs_[layerid].AttrDefs();

  // make a new record
  gstRecordHandle attrib = header->NewRecord();

  int fcnt = header->numColumns();
  for (int def = 0; def < fcnt; ++def) {
    switch (header->ftype(def)) {
      case gstTagInt:
        attrib->Field(def)->set(feature->GetFieldAsInteger(def));
        break;
      case gstTagDouble:
        attrib->Field(def)->set(feature->GetFieldAsDouble(def));
        break;
      case gstTagString:
        attrib->Field(def)->set(feature->GetFieldAsString(def));
        break;
    }
  }
  return attrib;
}


void gstOGRFormat::AddPoint(OGRPoint* point,
                            gstPrimType prim_type,
                            gstGeodeHandle *baseh) const {
  if (!(*baseh)) {
    *baseh = gstGeodeImpl::Create(prim_type);
  }

  gstGeode *base = static_cast<gstGeode*>(&(**baseh));
  // NOTE: Each part of point feature contains exactly one point.
  // See gstGeode::ToRaw(). So we add part here.
  // TODO: make re-factoring - use one part for all points?!
  base->AddPart(1);
  base->AddVertex(gstVertex(point->getX(), point->getY(), point->getZ()));
}

int gstOGRFormat::AddMultiPoint(OGRGeometry* geom,
                                 gstPrimType prim_type,
                                 gstGeodeHandle *baseh) const {
  OGRGeometryCollection *multi =
      static_cast<OGRGeometryCollection*>(geom);
  int num_geometries = multi->getNumGeometries();
  if (num_geometries == 0) {
    notify(NFY_WARN, "Empty MultiPoint feature, skipping.");
  } else {
    for (int count = 0; count < num_geometries; ++count) {
      AddPoint(static_cast<OGRPoint*>(multi->getGeometryRef(count)),
               prim_type, baseh);
    }
  }
  return num_geometries;
}


void gstOGRFormat::AddString(OGRLineString* string, gstPrimType type,
                             gstGeodeHandle *baseh) const {
  if (!(*baseh)) {
    *baseh = gstGeodeImpl::Create(type);
  }

  gstGeode *base = static_cast<gstGeode*>(&(**baseh));
  base->AddPart(string->getNumPoints());

  OGRPoint point;
  for (int ptNum = 0; ptNum < string->getNumPoints(); ++ptNum) {
    string->getPoint(ptNum, &point);  // sets the x/y pair of supplied point
    base->AddVertex(gstVertex(point.getX(), point.getY(), point.getZ()));
  }
}

int gstOGRFormat::AddMultiString(OGRGeometry* geom, gstPrimType type,
                                 gstGeodeHandle *baseh) const {
  OGRGeometryCollection *multi =
      static_cast<OGRGeometryCollection*>(geom);
  int num_geometries = multi->getNumGeometries();

  if (num_geometries == 0) {
    notify(NFY_WARN, "Empty MultiLineString feature, skipping.");
  } else {
    for (int count = 0; count < num_geometries; ++count) {
      AddString(static_cast<OGRLineString*>(
          multi->getGeometryRef(count)), type, baseh);
    }
  }

  return num_geometries;
}

gstGeodeHandle gstOGRFormat::GetPolygon(OGRPolygon *poly,
                                        gstPrimType type) const {
  gstGeodeHandle geode;
#ifdef MULTI_POLYGON_ON
  AddExteriorRing(poly->getExteriorRing(), type, &geode);
#else
  AddString(poly->getExteriorRing(), type, &geode);
#endif

  for (int interior = 0;
       interior < poly->getNumInteriorRings(); ++interior) {
#ifdef MULTI_POLYGON_ON
    AddInteriorRing(poly->getInteriorRing(interior), type, &geode);
#else
    AddString(poly->getInteriorRing(interior), type, &geode);
#endif
  }

  return geode;
}

gstGeodeHandle gstOGRFormat::GetMultiPolygon(
     OGRMultiPolygon *multi_poly, gstPrimType multi_type) const {
#ifdef MULTI_POLYGON_ON
  gstGeodeHandle multi_geodeh = gstGeodeImpl::Create(multi_type);
  gstMultiPoly *multi_geode = static_cast<gstMultiPoly*>(&(*multi_geodeh));

  gstPrimType single_type = GetSingleGeodeTypeFromMulti(multi_type);

  for (int geomIdx = 0;
       geomIdx < multi_poly->getNumGeometries(); ++geomIdx) {
    OGRPolygon *poly =
        static_cast<OGRPolygon*>(multi_poly->getGeometryRef(geomIdx));
    gstGeodeHandle geodeh;
    geodeh = GetPolygon(poly, single_type);

    multi_geode->AddGeode(geodeh);
  }
  return multi_geodeh;
#else
  gstGeodeHandle geode;
  for (int geomIdx = 0; geomIdx < multi_poly->getNumGeometries(); ++geomIdx) {
    OGRPolygon *poly =
        static_cast<OGRPolygon*>(multi_poly->getGeometryRef(geomIdx));

    AddString(poly->getExteriorRing(), gstPolygon, &geode);
    for (int interior = 0;
         interior < poly->getNumInteriorRings(); ++interior) {
      AddString(poly->getInteriorRing(interior), gstPolygon, &geode);
    }
  }
  return geode;
#endif
}

void gstOGRFormat::AddExteriorRing(
    OGRLinearRing* ring, gstPrimType type, gstGeodeHandle *baseh) const {
  assert(!(*baseh));
  *baseh = gstGeodeImpl::Create(type);

  gstGeode *base = static_cast<gstGeode*>(&(**baseh));
  base->AddPart(ring->getNumPoints());

  if (ring->isClockwise()) {
    AddInvertedRing(ring, base);
  } else {
    AddRing(ring, base);
  }
}

void gstOGRFormat::AddInteriorRing(
    OGRLinearRing* ring, gstPrimType type, gstGeodeHandle *baseh) const {
  assert(*baseh);

  gstGeode *base = static_cast<gstGeode*>(&(**baseh));
  base->AddPart(ring->getNumPoints());

  OGRPoint point;
  if (ring->isClockwise()) {
    AddRing(ring, base);
  } else {
    AddInvertedRing(ring, base);
  }
}

void gstOGRFormat::AddRing(OGRLinearRing* ring, gstGeode *base) const {
  OGRPoint point;
  for (int ptNum = 0; ptNum < ring->getNumPoints(); ++ptNum) {
    ring->getPoint(ptNum, &point);  // sets the x/y pair of supplied point
    base->AddVertex(gstVertex(point.getX(), point.getY(), point.getZ()));
  }
}

void gstOGRFormat::AddInvertedRing(
    OGRLinearRing* ring, gstGeode *base) const {
  OGRPoint point;
  for (int ptNum = (ring->getNumPoints() - 1); ptNum >= 0; --ptNum) {
    ring->getPoint(ptNum, &point);  // sets the x/y pair of supplied point
    base->AddVertex(gstVertex(point.getX(), point.getY(), point.getZ()));
  }
}

// ****************************************************************************
// ***  gstOGRSQLFormat
// ****************************************************************************
gstOGRSQLFormat::gstOGRSQLFormat(const char *fname)
    : gstOGRFormat(fname), sqlOGRLayer(0) {
}

gstOGRSQLFormat::~gstOGRSQLFormat(void) {
  if (data_source_ && sqlOGRLayer) {
    data_source_->ReleaseResultSet(sqlOGRLayer);
  }
}


gstStatus gstOGRSQLFormat::OpenFile(void) {
  gstDBSource dbSource;
  if (!dbSource.Load(name())) {
    // more specific error already emitted to console
    return GST_OPEN_FAIL;
  }

  // use OGR to create source dataset
  data_source_ = (GDALDataset *) GDALOpenEx((const char*)dbSource.
                                            OGRDataSource.utf8(),
                                            GDAL_OF_READONLY, NULL, NULL, NULL);
  if (!data_source_) {
    // more specific error already emitted to console
    // but it won't have the filename in it, so lets add another messge with
    // our filename
    notify(NFY_WARN, "Unable to create SQL reader for DBSource (%s)", name());
    return GST_OPEN_FAIL;
  }

  // See if we should use an override SRS
  khDeleteGuard<OGRSpatialReference> newsrs;
  if (!dbSource.srsOverride.isEmpty()) {
    newsrs = TransferOwnership(new OGRSpatialReference());
    InterpretSRSString((const char *)dbSource.srsOverride.utf8(), *newsrs);
  }

  if (!dbSource.sql.isEmpty()) {
    // we have a sql query - run it and use its results as the only
    // layer
    sqlOGRLayer = data_source_->ExecuteSQL((const char *)dbSource.sql.utf8(),
                                           0, /* spatialFilter */
                                           0  /* sqlDialect */);
    if (!sqlOGRLayer) {
      notify(NFY_WARN, "Unable to execute SQL (%s) for DBSource (%s)",
             (const char *)dbSource.sql.utf8(), name());
      return GST_OPEN_FAIL;
    }

    AddOGRLayer(0, sqlOGRLayer);

    if (!newsrs) {
      OGRSpatialReference* srs = sqlOGRLayer->GetSpatialRef();
      if (srs != NULL) {
        newsrs = TransferOwnership(srs->Clone());
      }
    }
  } else {
    // we have no sql query - get our layers from the data source
    if (data_source_->GetLayerCount() == 0) {
      notify(NFY_WARN, "DBSource (%s) has no layers", name());
      return GST_OPEN_FAIL;
    }

    bool needSRSCheck = false;
    layers.reserve(data_source_->GetLayerCount());
    for (int layer = 0; layer < data_source_->GetLayerCount(); ++layer) {
      OGRLayer *poLayer = data_source_->GetLayer(layer);

      if (poLayer == NULL) {
        notify(NFY_WARN, "Couldn't fetch layer %d!", layer);
        continue;
      }

      AddOGRLayer(layer, poLayer);

      if (!newsrs) {
        OGRSpatialReference* srs = poLayer->GetSpatialRef();
        if (srs) {
          if (layer > 0) {
            notify(NFY_WARN, "Layers have different SRS in %s", name());
            return GST_OPEN_FAIL;
          }
          newsrs = TransferOwnership(srs->Clone());
        }
        needSRSCheck = true;
      } else if (needSRSCheck) {
        OGRSpatialReference* srs = poLayer->GetSpatialRef();
        if (!srs || (!srs->IsSame(newsrs)))
          notify(NFY_WARN, "Layers have different SRS in %s", name());
        return GST_OPEN_FAIL;
      }
    }
  }

  SetSpatialRef(new gstSpatialRef(newsrs));
  return GST_OKAY;
}

gstGeodeHandle gstOGRSQLFormat::GetFeatureImpl(std::uint32_t, std::uint32_t) {
  throw khException(kh::tr("Random access not supported for SQL sources"));
}

gstRecordHandle gstOGRSQLFormat::GetAttributeImpl(std::uint32_t, std::uint32_t) {
  throw khException(kh::tr("Random access not supported for SQL sources"));
}
