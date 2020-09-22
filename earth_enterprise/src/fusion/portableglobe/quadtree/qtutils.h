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


// Helper functions for doing calculations on quadtree addresses.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_QUADTREE_QTUTILS_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_QUADTREE_QTUTILS_H_

#include <string>
#include <vector>
#include "common/khTypes.h"
#include <cstdint>

namespace fusion_portableglobe {

const double PI = 3.14159265358979;
const std::uint32_t MAX_LEVEL = 24;
const double MAX_MERCATOR_LATITUDE = 85.051128779806589;

/**
 * Fills in all of the mercator addresses that correspond to nodes that touch
 * the lat/lng bounding box of the node specifed by the given
 * plate carree address.
 */
void ConvertFlatToMercatorQtAddresses(
    std::string flat_qtaddress,
    std::vector<std::string>* mercator_qtaddresses);

/**
 * Returns the y position on a grid at given depth corresponding
 * to the given latitude.
 */
 std::uint32_t LatToYPos(double lat, std::uint32_t z, bool is_mercator);

/**
 * Returns the normalized, linear y associated with the given latitude.
 * Y is in the range of (-pi, pi) for latitudes in the
 * range of (-MAX_MERCATOR_LATITUDE, MAX_MERCATOR_LATITUDE).
 */
double MercatorLatToY(double lat);

/**
 * Returns the latitude associated with the given normalized, linear y.
 * Y is in the range of (-pi, pi) for latitudes in the
 * range of (-MAX_MERCATOR_LATITUDE, MAX_MERCATOR_LATITUDE).
 */
double MercatorYToLat(double y);

/**
 * Returns the position on a Mercator grid at given depth (z) corresponding
 * to the normalized, linear y value.
 * Y is in the range (-pi, pi) and return value is in
 * the range [0, 2^z-1].
 */
 std::uint32_t YToYPos(double y, std::uint32_t z);

/**
 * Returns the latitude that will appear half way between the two given
 * latitudes on a Mercator map.
 */
double BisectLatitudes(double south, double north, bool is_mercator);

/**
 * Helper for converting from map space to qtnode address.
 * @param x Requested column for map tile.
 * @param y Requested row for map tile.
 * @param z Requested zoom level for map.
 */
std::string ConvertToQtNode(std::uint32_t x, std::uint32_t y, std::uint32_t z);

/**
 * Helper for converting to map space from a qtnode address.
 * @param qtnode Quadtree address to be converted to map coordinates.
 * @param x Column for map tile.
 * @param y Row for map tile.
 * @param z Zoom level for map.
 */
void ConvertFromQtNode(const std::string& qtnode,
                       std::uint32_t* x, std::uint32_t* y, std::uint32_t* z);

}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_QUADTREE_QTUTILS_H_
