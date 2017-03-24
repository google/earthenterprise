/*
 * Copyright 2017 Google Inc.
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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTPOLYGONCLEANER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTPOLYGONCLEANER_H_

#include "fusion/gst/gstPolygonBuilder.h"
#include "fusion/gst/gstGeode.h"

#include "common/base/macros.h"

namespace fusion_gst {

// Class PolygonCleaner checks/fixes polygon's cycles orientation and
// detects/removes completely overlapped edges. e.g. the edges that join
// donut-hole with outer polygon cycle (hole-cutting edges).
class PolygonCleaner {
 public:
  PolygonCleaner();
  ~PolygonCleaner();

  // Runs all checks and fixes: cycle orientation, completely overlapped edges.
  void Run(gstGeodeHandle* const geode);

 private:
  void ProcessPolygon(gstGeodeHandle* const geodeh);

  // Auxiliary processor to clean completely overlapped edges and fix cycle
  // orientation.
  PolygonBuilder polygon_builder_;

  // Auxiliary processor to create geode objects. It is required because
  // in general case for single source polygon the polygon builder can return
  // some polygons. In such case we pack it into one multi-polygon.
  GeodeCreator geode_creator_;

  DISALLOW_COPY_AND_ASSIGN(PolygonCleaner);
};

}  // namespace fusion_gst

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTPOLYGONCLEANER_H_
