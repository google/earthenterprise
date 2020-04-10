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

// This header is the central place for all the constants used in fusion/gst.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTCONSTANTS_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTCONSTANTS_H_

#include <float.h>
//#include "common/khTypes.h"
#include <cstdint>
#include "common/khConstants.h"

// namespace fusion_gst {

// Minimal number of vertices in polyline.
const unsigned int kMinPolylineVertices = 2;

// Minimal number of vertices in cycle of polygon.
const unsigned int kMinCycleVertices = 4;   // first vertex == last vertex;

// Max level at which we build presence mask for optimizing quads processing.
const std::uint32_t kMaxLevelForBuildPresenceMask = 14;


// Epsilon tolerance value to compare floating-point (double) values.
const double kEpsilon = 1.0e-30;

const double kDblEpsilon = DBL_EPSILON;

// Zero value for floating-point operations.
const double kAlmostZero = 1.0e-30;


// XY-tolerance. It is used to set min distance when points are considered
// identical. Min distance is sqrt(2)*kXYTolerance.
const double kXYTolerance = 3e-4;  // specified in meters.

// Normalized XY-tolerance. (7.494334952e-12)
const double kXYToleranceNorm = kXYTolerance/(2*M_PI*khEarthRadius);

// }  // namespace fusion_gst

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTCONSTANTS_H_
