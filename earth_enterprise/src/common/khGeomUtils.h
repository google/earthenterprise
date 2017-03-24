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

// This class implements commonly used geometric operations.

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_KHGEOMUTILS_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_KHGEOMUTILS_H_

class khGeomUtils {
 public:
  static double DegreesToMeters(double degrees)
  {
    return degrees * khEarthCircumferencePerDegree;
  }

  static const double khEarthCircumference;
  static const double khEarthCircumferencePerDegree;

  // Update the buffer (originally representing n1-s1-e1-w1) to represent the
  // bounding box n2-s2-e2-w2 which is inside n1-s1-e1-w1.
  // The original buffer is of tile_width * tile_height pixel (row-wise starting
  // from upper-left). Each pixel is represented by bytes_per_pixel
  // (e.g 3 for jpeg and 4 for png) in buffer.
  static void CutTileOptimized(
      unsigned char * buffer, const int bytes_per_pixel,
      int *tile_width, int *tile_height,
      const double n1, const double s1, const double e1, const double w1,
      const double n2, const double s2, const double e2, const double w2);

  // Similar to above, but n2-s2-e2-w2 may be outside n1-s1-e1-w1.
  static void CutTile(
      unsigned char * buffer, const int bytes_per_pixel,
      int *tile_width, int *tile_height,
      const double n1, const double s1, const double e1, const double w1,
      const double n2, const double s2, const double e2, const double w2) {
    CutTileOptimized(buffer, bytes_per_pixel, tile_width, tile_height,
                     n1, s1, e1, w1, (n1 < n2 ? n1 : n2), (s1 > s2 ? s1 : s2),
                     (e1 < e2 ? e1 : e2), (w1 > w2 ? w1 : w2));
  }
};


class khGeomUtilsMercator {
 public:
  static double DegreesToMeters(double degrees) {
    return degrees * khEarthCircumferencePerDegree;
  }

  static const double khEarthCircumference;
  static const double khEarthCircumferencePerDegree;
  static const double khMaxLatitude;
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_KHGEOMUTILS_H_
