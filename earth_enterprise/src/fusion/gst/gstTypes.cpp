// Copyright 2017 Google Inc.
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

#include "fusion/gst/gstTypes.h"

#include <string>

std::string PrettyPrimType(gstPrimType type) {
  switch (type) {
    case gstUnknown:
      return "Unknown";
    case gstPoint:
      return "Point";
    case gstPoint25D:
      return "Point25D";
    case gstPolyLine:
      return "Polyline";
    case gstPolyLine25D:
      return "Polyline25D";
    case gstStreet:
      return "Street";
    case gstStreet25D:
      return "Street25D";
    case gstPolygon:
      return "Polygon2D";
    case gstPolygon25D:
      return "Polygon25D";
    case gstPolygon3D:
      return "Polygon3D";
    case gstMultiPolygon:
      return "MultiPolygon2D";
    case gstMultiPolygon25D:
      return "MultiPolygon25D";
    case gstMultiPolygon3D:
      return "MultiPolygon3D";
  }
  // unreached but silences warnings
  return std::string();
}

gstPrimType ToFlatPrimType(gstPrimType type) {
  switch (type) {
    case gstUnknown:
    case gstPoint:
    case gstPolyLine:
    case gstStreet:
    case gstPolygon:
      return type;

    case gstPolygon25D:
    case gstPolygon3D:
      return gstPolygon;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      return gstMultiPolygon;

    case gstPoint25D:
    case gstPolyLine25D:
    case gstStreet25D:
      return static_cast<gstPrimType>(type & 0x7f);
  }
  // unreached but silences warnings
  return gstUnknown;
}

bool IsMultiGeodeType(gstPrimType type) {
  switch (type) {
    case gstUnknown:
    case gstPoint:
    case gstPolyLine:
    case gstStreet:
    case gstPoint25D:
    case gstPolyLine25D:
    case gstStreet25D:
    case gstPolygon:
    case gstPolygon25D:
    case gstPolygon3D:
      return false;

    case gstMultiPolygon:
    case gstMultiPolygon25D:
    case gstMultiPolygon3D:
      return true;
  }
  // unreached but silences warnings
  return false;
}

gstPrimType GetMultiGeodeTypeFromSingle(gstPrimType type) {
  gstPrimType multi_type = gstMultiPolygon;
  if (gstPolygon == type) {
    multi_type = gstMultiPolygon;
  } else if (gstPolygon25D == type) {
    multi_type = gstMultiPolygon25D;
  } else if (gstPolygon3D == type) {
    multi_type = gstMultiPolygon3D;
  } else {
    notify(NFY_FATAL, "%s: improper single-geometry type %d.", __func__, type);
  }
  return multi_type;
}

gstPrimType GetSingleGeodeTypeFromMulti(gstPrimType type) {
  gstPrimType single_type = gstPolygon;
  if (gstMultiPolygon == type) {
    single_type = gstPolygon;
  } else if (gstMultiPolygon25D == type) {
    single_type = gstPolygon25D;
  } else if (gstMultiPolygon3D == type) {
    single_type = gstPolygon3D;
  } else {
    notify(NFY_FATAL, "%s: improper multi-geometry type %d.", __func__, type);
  }

  return single_type;
}
