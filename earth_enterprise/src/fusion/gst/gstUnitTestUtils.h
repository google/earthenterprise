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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTUNITTESTUTILS_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTUNITTESTUTILS_H_

#include <string>
#include <ostream>

#include "fusion/gst/gstVertex.h"
#include "fusion/gst/gstGeode.h"
#include "fusion/gst/gstMathUtils.h"

// gtest needs to be able to print out objects we're comparing.
std::ostream& operator<<(std::ostream& os, const gstVertex& v);

std::ostream& operator<<(std::ostream& os, const gstGeodeHandle &geodeh);

namespace fusion_gst {

class TestUtils {
 public:
  // Auxiliary function to use as predicate in gtest ASSERT_PRED.
  static bool gstVertexEqualXY(const gstVertex &l, const gstVertex &r) {
    const double kEpsilonTest = 1.0e-15;
    return (Equals(l.x, r.x, kEpsilonTest) &&
            Equals(l.y, r.y, kEpsilonTest));
  }

  static void PrintGeode(const gstGeodeHandle &geodeh);

  static void CompareGeodes(const gstGeodeHandle &geodeh,
                            const gstGeodeHandle &expected_geodeh,
                            const std::string &message);
};

}  // namespace fusion_gst

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTUNITTESTUTILS_H_
