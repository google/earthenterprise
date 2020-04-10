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

// Description: GeometryChecker for checking and fixing different geometry
// invalidities in polygon-features: coincident vertices, spikes,
// incorrect cycle orientation.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTGEOMETRYCHECKER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTGEOMETRYCHECKER_H_

#include "fusion/gst/gstGeode.h"
#include "fusion/gst/gstMathUtils.h"

#include "common/base/macros.h"


namespace fusion_gst {

// Types of cycle orientation.
enum CycleOrientation {
  kClockWiseCycle = 0,
  kCounterClockWiseCycle = 1
};

// Class GeometryChecker checks and fixes different geometry invalidities
// (coincident vertices, spikes, incorrect cycle orientation).
// Note: EdgeFlag of Geode is not supported. GeometryChecker should be only
// used to check source geometry.
// TODO: support edge flag?!
class GeometryChecker {
  friend class GeometryCheckerTest;

 public:
  GeometryChecker();
  ~GeometryChecker();

  // Runs all checks and fixes: coincident vertices, spikes.
  // Check and fix incorrect cycle orientation functionality is switched off.
  // Now we fix it in PolygonCleaner.
  void Run(gstGeodeHandle* const geode);

 private:
  // Processes 2D/25D polygons.
  void ProcessPolygon(gstGeodeHandle* const geodeh);

  // Processes 3D polygons.
  void ProcessPolygon3D(gstGeodeHandle* const geodeh);

  // Checks for consecutive coincident vertexes and removes duplicates.
  // Note: do not accept multi-geometry types.
  void RemoveCoincidentVertices(gstGeodeHandle* const geodeh) const;

  // Checks for spikes and removes them.
  // Note: do not accept multi-geometry types.
  void RemoveSpikes(gstGeodeHandle* const geodeh) const;

  void RemoveSpikesInPart(const gstGeode &geode,
                          unsigned int part,
                          gstGeodePart *res_part) const;

  // Checks points for collinearity.
  bool IsSpike(const gstVertex &a, const gstVertex &b,
               const gstVertex &c) const;

  // Checks and Fixes self intersection cycles.
  void CheckForSelfIntersection(gstGeodeHandle* const geodeh) const;

  // Checks and Fixes cycle orientation.
  // Note: input polygon should not have coincident vertices and spikes;
  // Note: do not accept multi-geometry types.
  void CheckAndFixCycleOrientation(gstGeodeHandle* const geodeh) const;

  // Calculates cycle orientation.
  CycleOrientation CalcCycleOrientation(gstGeode* const geode, unsigned int part) const;

  // Less XY-functor for gstVertex without tolerance.
  XYLexicographicLess<gstVertex> less_xy_;

  DISALLOW_COPY_AND_ASSIGN(GeometryChecker);
};

}  // namespace fusion_gst

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTGEOMETRYCHECKER_H_
