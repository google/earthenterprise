/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTGEOMETRY_H__
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTGEOMETRY_H__

#include "common/khRefCounter.h"
#include "fusion/gst/gstGeode.h"

class gstFeaturePreviewConfig;
class gstGeometryImpl;
typedef khRefGuard<gstGeometryImpl> gstGeometryHandle;

class gstGeometryImpl : public khRefCounter {
 public:
  class Point {
   public:
    float x;
    float y;
    Point() : x(.0f), y(.0f) {}
    Point(float _x, float _y) : x(_x), y(_y) {}
  };

  // determine amount of memory used by gstGeometryImpl
  std::uint64_t GetSize() {
    return sizeof(type_)
    + sizeof(origin_)
    + sizeof(points_)
    + sizeof(lengths_);
  }

  static gstGeometryHandle Create(gstGeodeHandle g) {
    return khRefGuardFromNew(new gstGeometryImpl(g));
  }

  void Draw(const gstBBox& frust,
            const gstFeaturePreviewConfig &preview_config);

 private:
  gstGeometryImpl(const gstGeodeHandle &geodeh);

  void Init(const gstGeodeHandle &geodeh);

  void ConvertGeode(const gstGeode *geode,
                    const double dx, const double dy,
                    const float f_error_x, const float f_error_y);

  gstPrimType type_;
  Point origin_;
  std::vector<Point> points_;
  std::vector< unsigned int>  lengths_;

  DISALLOW_COPY_AND_ASSIGN(gstGeometryImpl);
};

#endif  // !GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTGEOMETRY_H__
