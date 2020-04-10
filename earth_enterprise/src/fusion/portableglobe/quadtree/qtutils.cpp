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


// Methods for converting a polygon from a kml file into a
// set of quadtree nodes that the polygon encompasses.

#include "fusion/portableglobe/quadtree/qtutils.h"

#include <math.h>
#include <algorithm>
#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include <vector>
#include "common/khTypes.h"
#include <cstdint>
#include "common/notify.h"

namespace fusion_portableglobe {

/**
 * Fills in all of the mercator addresses that correspond to nodes that touch
 * the lat/lng bounding box of the node specifed by the given
 * plate carree address. This could be optimized more by doing
 * more of the work inline.
 */
void ConvertFlatToMercatorQtAddresses(
    std::string flat_qtaddress,
    std::vector<std::string>* mercator_qtaddresses) {
  // x, y, z correspond to their Maps query usage (column, row, and LOD).
  std::uint32_t x;
  std::uint32_t y;
  std::uint32_t z;

  ConvertFromQtNode(flat_qtaddress, &x, &y, &z);
  // Dimension of the grid is 2^z by 2^z.
  double max_ypos = static_cast<double>(1 << z);
  double y_value = static_cast<double>(y);

  // Calculate top and bottom latitude of tile.
  // We use a 360 degree span because only the middle section of
  // the grid is used for plate carree. The entire grid actually
  // spans (-180,180), but (-180,-90) and (90,180) are black.
  double min_lat = 180.0 - (360.0 * y_value / max_ypos);
  double max_lat = 180.0 - (360.0 * (y_value + 1.0) / max_ypos);

  // Find the corresponding y range on a Mercator map.
  // Be aggressive in inclusion, and allow unfound tiles
  // to just be ignored.
  std::uint32_t y_bottom = LatToYPos(min_lat, z, true);
  std::uint32_t y_top = LatToYPos(max_lat, z, true);
  // Add a Mercator quadtree address for each value of y.
  for (std::uint32_t y_next = y_bottom; y_next <= y_top; y_next++) {
    mercator_qtaddresses->push_back(ConvertToQtNode(x, y_next, z));
  }
}

/**
 * Returns the y position on a grid at given depth corresponding
 * to the given latitude.
 * yPos is an integer in the range of [0, 2^z).
 */
 std::uint32_t LatToYPos(double lat, std::uint32_t z, bool is_mercator) {
  double y;
  double min_y;
  double max_y;
  std::uint32_t y_off;
  std::uint32_t max_ypos = 1 << z;

  // y is inverted (montonic downwards)
  lat = -lat;
  if (is_mercator) {
    y = MercatorLatToY(lat);
    min_y = -PI;
    max_y = PI;
    y_off = 0;
  } else {
    y = lat;
    min_y = -90.0;
    max_y = 90.0;
    // For non-Mercator, we are using only half the y space.
    max_ypos >>= 1;
    // Imagery starts a quarter of the way down the grid.
    y_off = max_ypos >> 1;
  }

  // Force legal y pos for illegal inputs beyond extremes.
  if (y >= max_y) {
    return max_ypos - 1 + y_off;
  } else if (y < min_y) {
    return y_off;
  } else {
    return (y - min_y) / (max_y - min_y) * max_ypos + y_off;
  }
}

/**
 * Returns the y position associated with the given latitude.
 * y is a double in the range of (-pi, pi).
 */
double MercatorLatToY(double lat) {
  if (lat >= MAX_MERCATOR_LATITUDE) {
    return PI;
  } else if (lat <= -MAX_MERCATOR_LATITUDE) {
    return -PI;
  }

  return log(tan(PI / 4.0 + lat / 360.0 * PI));
}

/**
 * Returns the latitude associated with the given y location.
 * y is a double in the range of (-pi, pi).
 */
double MercatorYToLat(double y) {
  if (y >= PI) {
    return MAX_MERCATOR_LATITUDE;
  } else if (y <= -PI) {
    return -MAX_MERCATOR_LATITUDE;
  }

  return (atan(exp(y)) - PI / 4.0) * 360.0 / PI;
}

/**
 * Returns the position on a Mercator grid at given depth (z) corresponding
 * to the normalized, linear y value.
 * y is a double in the range of (-pi, pi).
 * yPos is an integer in the range of [0, 2^z).
 */
 std::uint32_t YToYPos(double y, std::uint32_t z) {
  // Use signed to check for underflow.
  std::int32_t max_ypos = 1 << z;
  std::int32_t ypos = max_ypos * ((y + PI) / (2.0 * PI));
  if (ypos < 0) {
    return 0;
  } else if (ypos >= max_ypos) {
    return max_ypos - 1;
  } else {
    return static_cast<std::uint32_t>(ypos);
  }
}

/**
 * Returns the latitude that will appear half way between the two given
 * latitudes on a Mercator map.
 */
double BisectLatitudes(double south, double north, bool is_mercator) {
  if (is_mercator) {
    double y1 = MercatorLatToY(south);
    double y2 = MercatorLatToY(north);
    return MercatorYToLat((y1 + y2) / 2.0);
  } else {
    return (south + north) / 2.0;
  }
}

/**
 * Helper for converting from map space to qtnode address.
 * Return empty string if input is invalid.
 */
std::string ConvertToQtNode(std::uint32_t x, std::uint32_t y, std::uint32_t z) {
  std::string qtnode = "0";
  // Half the width or height of the map coordinates at the target LOD.
  // I.e. the size of the top-level quadrants.
  std::uint32_t  half_ndim = 1 << (z - 1);
  for (std::uint32_t i = 0; i < z; ++i) {
     // Choose quadtree address char based on quadrant that x, y fall into.
    if ((y >= half_ndim) && (x < half_ndim)) {
      qtnode += "0";
      y -= half_ndim;
    } else if ((y >= half_ndim) && (x >= half_ndim)) {
      qtnode += "1";
      y -= half_ndim;
      x -= half_ndim;
    } else if ((y < half_ndim) && (x >= half_ndim)) {
      qtnode += "2";
      x -= half_ndim;
    } else {
      qtnode += "3";
    }

    // Cut in half for the next level of quadrants.
    half_ndim >>= 1;
  }

  // x and y should be cleared by the end.
  if (x || y) {
    notify(NFY_WARN, "Illegal map coordinates: %u %u", x, y);
    return "";
  }

  return qtnode;
}

/**
 * Helper for converting from qtnode address to map space.
 * Expects leading "0". Returns MAX_LEVEL for zoom level if there
 * is an error.
 */
void ConvertFromQtNode(
    const std::string& qtnode, std::uint32_t* x, std::uint32_t* y, std::uint32_t* z) {
  const char* ptr = qtnode.c_str();
  *z = qtnode.size();
  if (*z == 0) {
    notify(NFY_WARN, "Illegal qtnode address: empty string");
    *z = MAX_LEVEL;
    return;
  }
  // LOD is the length of the quadtree address - 1.
  *z -= 1;

  if (*ptr != '0') {
    notify(NFY_WARN, "Illegal qtnode address. Illegal char: %c", *ptr);
    *z = MAX_LEVEL;
    return;
  }

  // For each quadtree address character, add 1 bit of info to x and y
  // location.
  *x = 0;
  *y = 0;
  // Half the width or height of the map coordinates at the target LOD.
  // I.e. the size of the top-level quadrants.
  std::uint32_t half_ndim = 1 << (*z - 1);
  for (std::uint32_t i = 0; i < *z; ++i) {
    switch (*++ptr) {
      case '0':
        *y += half_ndim;
        break;

      case '1':
        *x += half_ndim;
        *y += half_ndim;
        break;

      case '2':
        *x += half_ndim;
        break;

      case '3':
        // No change
        break;

      default:
        notify(NFY_WARN, "Illegal qtnode address. Illegal char: %c", *ptr);
        *z = MAX_LEVEL;
        return;
    }

    // Cut in half for the next level of quadrants.
    half_ndim >>= 1;
  }
}

}  // namespace fusion_portableglobe
