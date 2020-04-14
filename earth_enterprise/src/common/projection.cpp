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


#include <math.h>
#include <algorithm>
#include "khMisc.h"
#include "khTileAddr.h"
#include "projection.h"
#include <common/khConstants.h>

static inline double DegreesToRadians(double degrees) {
  return degrees * (M_PI / 180.0);
}

static inline double RadiansToDegrees(double radians) {
  return radians * (180.0 / M_PI);
}

Projection::Point Projection::FromNormLatLngToPixel(const LatLng& norm_latLng,
                                                    int zoom) const {
  return FromLatLngToPixel(LatLng(khTilespace::Denormalize(norm_latLng.Lat()),
                                  khTilespace::Denormalize(norm_latLng.Lng())),
                           zoom);
}

// -----------------------------------------------------------------------------
MercatorProjection::MercatorProjection(int pixels_at_level_0)
  : Projection(pixels_at_level_0) {
  // First zoom level is a single tile
  std::uint64_t pixels = pixels_at_level_0;
  for (unsigned int level = 0; level <= Max2DClientLevel; ++level) {
    pixels_per_lon_degree_[level] = static_cast<double>(pixels) / 360.0;
    pixels_per_lon_radian_[level] = static_cast<double>(pixels) / (2 * M_PI);

    pixel_origin_[level] = pixels >> 1;
    pixels_range_[level] = pixels;

    pixels <<= 1;
  }
}

Projection::Point MercatorProjection::FromLatLngToPixel(const LatLng& latLng, int zoom) const {
  const double latitude_in_deg = Clamp(latLng.Lat(),
      -khGeomUtilsMercator::khMaxLatitude, khGeomUtilsMercator::khMaxLatitude);
  std::uint64_t origin = pixel_origin_[zoom];
  std::uint64_t x = static_cast<std::uint64_t>(round(static_cast<double>(origin) + latLng.Lng() *
       static_cast<double>(pixels_per_lon_degree_[zoom])));
  double siny = Clamp(sin(DegreesToRadians(latitude_in_deg)), -0.9999, 0.9999);
  std::uint64_t y = static_cast<std::uint64_t>(round(static_cast<double>(origin) + 0.5 * log((1 + siny) /
       (1 - siny)) * -static_cast<double>(pixels_per_lon_radian_[zoom])));
  return Point(x, pixels_range_[zoom] - y);  // lower-left origin
}

Projection::LatLng MercatorProjection::FromPixelToLatLng(const Point& pixel, int zoom) const {
  std::uint64_t origin = pixel_origin_[zoom];
  double lng = (static_cast<double>(pixel.X()) - static_cast<double>(origin)) /
     pixels_per_lon_degree_[zoom];
  double lat_radians = (static_cast<double>(pixel.Y()) - origin) /
     static_cast<double>(pixels_per_lon_radian_[zoom]);
  double lat = RadiansToDegrees(2 * atan(exp(lat_radians)) - M_PI / 2.0);
  return LatLng(lat, lng);
}

int AltitudeToZoomLevel(double altitude) {
  // The Client Field Of View is assumed to be 30 degrees.
  double kGEClientFovRadians = DegreesToRadians(30);

  // We compute it at a close to linear level...say level 10.
  double earthCircumference = khEarthMeanRadius * 2 * M_PI;

  // altitude = earth_circ/(2^(level+1)) / tan(FOV/2)
  // 2^(level+1) = earth_circ/(altitude * tan(FOV/2)
  // level + 1 = log(earth_circ/(altitude * tan(FOV/2))
  double level = log2(earthCircumference /
                      (altitude * tan(kGEClientFovRadians/2.0))) - 1;
  int zoomLevel = static_cast<int>(floor(level));
  zoomLevel = std::min(32, zoomLevel);
  zoomLevel = std::max(1, zoomLevel);
  return zoomLevel;
}

double MercatorProjection::FromFlatDegLatitudeToMercatorMeterLatitude(
    double latitude) {
  static const double kTwicePi = M_PI * 2.0;
  const bool is_negeative = latitude < 0;
  if (is_negeative) {
    latitude = -latitude;
  }
  const double radian_latitude_flat = latitude * kTwicePi / 360.0;
  const double sin_latitude = sin(radian_latitude_flat);
  const double radian_latitude_mercator =
      0.5 * log((1.0 + sin_latitude) / (1.0 - sin_latitude));
  const double meter_latitude_mercator = radian_latitude_mercator *
      khGeomUtilsMercator::khEarthCircumference / kTwicePi;
  return is_negeative ? -meter_latitude_mercator : meter_latitude_mercator;
}

double MercatorProjection::FromMercatorMeterLatitudeToFlatDegLatitude(
    double latitude) {
  static const double kTwicePi = M_PI * 2.0;
  const bool is_negeative = latitude < 0;
  if (is_negeative) {
    latitude = -latitude;
  }
  const double radian_latitude_mercator =
      latitude * kTwicePi / khGeomUtilsMercator::khEarthCircumference;
  const double radian_latitude = 2 * atan(exp(radian_latitude_mercator)) -
                                 M_PI_2;
  const double deg_latitude = radian_latitude * 180.0 / M_PI;
  return is_negeative ? -deg_latitude : deg_latitude;
}

double MercatorProjection::FromNormalizedLatitudeToMercatorNormalizedLatitude(
    const double normalized_latitude) {
  static const double min_normal =
      (-khGeomUtilsMercator::khMaxLatitude + 180.0) / 360.0;
  static const double max_normal =
      (khGeomUtilsMercator::khMaxLatitude + 180.0) / 360.0;
  static const double kTwicePi = M_PI * 2.0;
  const double clamped_normalized_latitude =
      Clamp(normalized_latitude, min_normal, max_normal);
  const double clamped_radian_latitude =
      kTwicePi * (clamped_normalized_latitude - 0.5);
  const double sin_latitude = sin(clamped_radian_latitude);
  const double mercator_latitude =
      0.5 * log((1.0 + sin_latitude) / (1.0 - sin_latitude));
  const double normalized_mercator_latitude =
      (mercator_latitude + M_PI) / kTwicePi;
  return normalized_mercator_latitude;
}

double MercatorProjection::FromMercatorNormalizedLatitudeToNormalizedLatitude(
    const double normalized_mercator_latitude) {
  static const double kTwicePi = M_PI * 2.0;
  const double mercator_latitude = normalized_mercator_latitude * kTwicePi
                                   - M_PI;
  const double radian_latitude = 2 * atan(exp(mercator_latitude)) - M_PI_2;
  const double normalized_latitude = (radian_latitude + M_PI) / kTwicePi;
  return normalized_latitude;
}

void MercatorProjection::NormalizeToMercatorFromFlat(
    double* north, double* south) {
  *north = FromNormalizedLatitudeToMercatorNormalizedLatitude(*north);
  *south = FromNormalizedLatitudeToMercatorNormalizedLatitude(*south);
}

void MercatorProjection::NormalizeToFlatFromMercator(
    double* north, double* south) {
  *north = FromMercatorNormalizedLatitudeToNormalizedLatitude(*north);
  *south = FromMercatorNormalizedLatitudeToNormalizedLatitude(*south);
}
