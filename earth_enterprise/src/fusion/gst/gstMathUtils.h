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

// Contains different math utilities.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTMATHUTILS_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTMATHUTILS_H_

#include <cmath>
#include <functional>

#include "fusion/gst/gstConstants.h"

namespace fusion_gst {

// Compares equality of double values with tolerance.
inline bool Equals(double left, double right,
                   double epsilon = kEpsilon) {
  return (std::abs(left - right) <= std::abs(left + right) * epsilon / 2);
}

// "Greater than" inequality of double values with tolerance.
inline bool Greater(double left, double right,
                    double epsilon = kEpsilon) {
  return ((left - right) > std::abs(left + right) * epsilon / 2);
}

// "Less than" inequality of double values with tolerance.
inline bool Less(double left, double right,
                 double epsilon = kEpsilon) {
  return ((right - left) > std::abs(left + right) * epsilon / 2);
}

// "Greater or equal" compare of double values with tolerance.
inline bool GreaterOrEquals(double left, double right,
                            double epsilon = kEpsilon) {
  return ((left - right) >= -std::abs(left + right) * epsilon / 2);
}

// "Less or equal" compare of double values with tolerance.
inline bool LessOrEquals(double left, double right,
                         double epsilon = kEpsilon) {
  return ((left - right) <= std::abs(left + right) * epsilon / 2);
}

// Compares points for equality by X,Y coordinates with tolerance.
template <typename Point>
bool EqualsXY(const Point &left, const Point &right,
              double epsilon = kEpsilon) {
  return (Equals(left.x, right.x, epsilon) && Equals(left.y, right.y, epsilon));
}

// Function object for "less than" inequality comparison operation
// by X,Y coordinates in accordance with vertical sweep line.
template <typename Point>
struct XYLexicographicLess
    : public std::binary_function<Point, Point, bool> {
  bool operator()(const Point &left, const Point &right) const {
    return ((left.x < right.x)
            || ((left.x == right.x) && (left.y < right.y)));
  }
};

// Function object for "less than" with tolerance inequality comparison
// operation by X,Y coordinates in accordance with vertical sweep line .
template <typename Point>
struct XYLexicographicLessWithTolerance
    : public std::binary_function<Point, Point, bool> {
  explicit XYLexicographicLessWithTolerance(double epsilon = kEpsilon)
      : epsilon_(epsilon) {
  }

  bool operator()(const Point &left, const Point &right) const {
    return (Less(left.x, right.x, epsilon_) ||
            (Equals(left.x, right.x, epsilon_) &&
             Less(left.y, right.y, epsilon_)));
  }

 private:
  double epsilon_;
};

}  // namespace fusion_gst

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTMATHUTILS_H_
