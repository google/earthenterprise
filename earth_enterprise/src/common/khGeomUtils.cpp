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


#include "khGeomUtils.h"
#include <string.h>

const double khGeomUtils::khEarthCircumference = 40075160.00;
const double khGeomUtils::khEarthCircumferencePerDegree =
    khGeomUtils::khEarthCircumference / 360.00;

// This value is defined because GDAL gives 40075016.6855784 / 2.0 for 180 deg
// longitude for Mercator_1SP projection.
const double khGeomUtilsMercator::khEarthCircumference = 40075016.6855784;
const double khGeomUtilsMercator::khEarthCircumferencePerDegree =
    khGeomUtilsMercator::khEarthCircumference / 360.00;

// The maximum absolute latitude (in degrees) used on Google Maps. This is
// chosen to cause the fully zoomed out map to be a square.
const double khGeomUtilsMercator::khMaxLatitude = 85.051128779806575;

void khGeomUtils::CutTileOptimized(
    unsigned char * buffer, const int bytes_per_pixel, int *width, int *height,
    const double n1, const double s1, const double e1, const double w1,
    const double n2, const double s2, const double e2, const double w2) {
  const double pxl_width = (e1 - w1) / (*width);
  const double pxl_height = (n1 - s1) / (*height);

  // Start from {w_pxl(col), n_pxl(col)} (inclusive) and
  // end at {e_pxl, s_pxl} (exclusive) Count the following pixels from top to
  // bottom and left to right.
  const int n_pxl = static_cast<int>((n1 - n2) / pxl_height + 0.5);
  const int s_pxl = static_cast<int>((n1 - s2) / pxl_height + 0.5);
  const int w_pxl = static_cast<int>((w2 - w1) / pxl_width + 0.5);
  const int e_pxl = static_cast<int>((e2 - w1) / pxl_width + 0.5);
  const int cut_height = s_pxl - n_pxl;
  const int cut_width = e_pxl - w_pxl;
  const int orig_row_size = (*width) * bytes_per_pixel;
  const int new_row_size = cut_width * bytes_per_pixel;

  unsigned char * start_at = buffer + (n_pxl * orig_row_size +
                                       w_pxl * bytes_per_pixel);
  for (int i = 0; i < cut_height; ++i) {
    memmove(buffer, start_at, new_row_size);
    buffer += new_row_size;
    start_at += orig_row_size;
  }
  *width = cut_width;
  *height = cut_height;
}
