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


#ifndef KHSRC_COMMON_PROJECTION_H__
#define KHSRC_COMMON_PROJECTION_H__

#include <cstdint>
#include <khTileAddrConsts.h>

// Utility to calculates the integral zoom level for Google Maps that most
// closely matches the altitude for the Earth Client Viewer.
// altitude: altitude of camera in meters
// returns: zoom level between 1 and 32
// This assumes a fixed GE Field of View (FOV) of 30 degrees.
// Note: this mapping is not perfect because of perceptual distortion
// of the sphere at low zoom levels (e.g., 0..3), but it is reasonably close
// for higher zoom levels where the mapping becomes nearly linear.
int AltitudeToZoomLevel(double altitude);

class Projection {
 public:
  // POD for 2D latitude/longitude pair in degrees
  class LatLng {
   public:
    LatLng(double lat, double lng) : lat_(lat), lng_(lng) {}
    LatLng(const LatLng& that) : lat_(that.Lat()), lng_(that.Lng()) {}

    bool operator==(const LatLng& that) const {
      return lat_ == that.Lat() && lng_ == that.Lng();
    }
    bool operator!=(const LatLng& that) const {
      return lat_ != that.Lat() || lng_ != that.Lng();
    }

    double Lat() const { return lat_; }
    double Lng() const { return lng_; }

   private:
    LatLng& operator=(const LatLng&);

    double lat_;
    double lng_;
  };

  // POD for 2D x/y pair
  class Point {
   public:
    Point(std::uint64_t x, std::uint64_t y) : x_(x), y_(y) {}
    Point(const Point& that) : x_(that.X()), y_(that.Y()) {}

    bool operator==(const Point& that) const {
      return x_ == that.X() && y_ == that.Y();
    }
    bool operator!=(const Point& that) const {
      return x_ != that.X() || y_ != that.Y();
    }

    std::uint64_t X() const { return x_; }
    std::uint64_t Y() const { return y_; }

   private:
    Point& operator=(const Point&);

    std::uint64_t x_;
    std::uint64_t y_;
  };

  Projection(unsigned int tilesize) : tile_size_(tilesize) {}
  virtual ~Projection() {}

  unsigned int TileSize() const { return tile_size_; }
  Point FromNormLatLngToPixel(const LatLng& latLng, int zoom) const;

  virtual Point FromLatLngToPixel(const LatLng& latLng, int zoom) const = 0;
  virtual LatLng FromPixelToLatLng(const Point& pixel, int zoom) const = 0;

 private:
  // private and unimplemented
  Projection(const Projection&);
  Projection& operator=(const Projection&);

  unsigned int tile_size_;
};

class MercatorProjection : public Projection {
 public:
  MercatorProjection(int pixels_at_level_0);

  virtual Point FromLatLngToPixel(const LatLng& latLng, int zoom) const;
  virtual LatLng FromPixelToLatLng(const Point& pixel, int zoom) const;
  // Converts normalized latitude to where it will be positioned in a normalized
  // mercator map. Normalized latitude means (latitude_in_deg + 180) / 360.
  static double FromNormalizedLatitudeToMercatorNormalizedLatitude(
      double latitude);
  // Does the inverse transform of above function.
  static double FromMercatorNormalizedLatitudeToNormalizedLatitude(
      double latitude);
  // Assumption, it is not outside mercator range
  static double FromFlatDegLatitudeToMercatorMeterLatitude(double latitude);
  // Assumption, it is not outside mercator domain
  static double FromMercatorMeterLatitudeToFlatDegLatitude(double latitude);
  // Assumption: original north south are in Normalized Flat Projection.
  // Note: East West normalized coordinates are same in both projection
  static void NormalizeToMercatorFromFlat(double* north, double* south);
  // Assumption: original north south are in Normalized Mercator Projection.
  static void NormalizeToFlatFromMercator(double* north, double* south);

 private:
  // private and unimplemented
  MercatorProjection(const MercatorProjection&);
  MercatorProjection& operator=(const MercatorProjection&);

  double pixels_per_lon_degree_[Max2DClientLevel + 1];
  double pixels_per_lon_radian_[Max2DClientLevel + 1];
  std::uint64_t pixel_origin_[Max2DClientLevel + 1];        // center of pixels space
  std::uint64_t pixels_range_[Max2DClientLevel + 1];        // size of pixel space
};

#endif  // !KHSRC_COMMON_PROJECTION_H__
