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

#ifndef __khGeoExtents_h
#define __khGeoExtents_h

#include <cstdint>
#include <khExtents.h>
#include <cmath>
#include "khgdal.h"


// ****************************************************************************
// ***  Class to encapsulate overall khExtents, raster khSize and geoTransform
// ***
// ***  Files read from GDAL will have geoTransform as the native storage
// ***  Files read from .geo, .kip, etc will have khExtents as native We
// ***  want to store them both so there's no conversion back and forth
// ***  (losing precision)
// ****************************************************************************
class khGeoExtents
{
  khExtents<double> extents_;
  double            geoTransform_[6];

 public:

  inline bool topToBottom(void) const {
    return IsTransformTopToBottom(geoTransform_);
  }
  inline bool bottomToTop(void) const { return !topToBottom(); }
  inline bool leftToRight(void) const {
    return IsTransformLeftToRight(geoTransform_);
  }
  inline bool rightToLeft(void) const { return !leftToRight(); }

  inline const khExtents<double> &extents(void) const { return extents_; }
  inline const double* geoTransform(void) const { return geoTransform_; }
  inline khOffset<double> geoTransformOrigin(void) const {
    return khOffset<double>(XYOrder,
                            geoTransform_[0],
                            geoTransform_[3]);
  }


  inline bool empty(void) const { return extents_.empty(); }
  inline operator bool(void) const { return !empty(); }
  inline bool noRotation(void) const {
    return ((geoTransform_[2] == 0.0) &&
            (geoTransform_[4] == 0.0));
  }

  inline double pixelWidth(void) const     { return geoTransform_[1]; }
  inline double pixelHeight(void) const    { return geoTransform_[5]; }
  inline double absPixelWidth(void) const  { return fabs(pixelWidth()); }
  inline double absPixelHeight(void) const { return fabs(pixelHeight()); }


  inline khGeoExtents(void) {
    memset(geoTransform_, 0, sizeof(geoTransform_));
  }
  khGeoExtents(const khExtents<double> &extents__,
               const khSize<std::uint32_t> &rasterSize,
               bool topToBottom = true);
  khGeoExtents(const khExtents<double> &extents__,
               double pixelSize,
               bool topToBottom = true);
  khGeoExtents(const khOffset<double> &origin,
               double pixelSize,
               const khSize<std::uint32_t> &rasterSize,
               bool topToBottom = true);
  khGeoExtents(unsigned int level,
               const khExtents<double> &extents__,
               bool topToBottom,
               bool is_mercator);
  khGeoExtents(double geoTransform__[6], const khSize<std::uint32_t> &rasterSize);
  khGeoExtents(const khExtents<std::int64_t> &pixelExtents,
               double pixelSize,
               bool topToBottom, double offset);
  khSize<std::uint32_t> boundToExtent(const khExtents<double>& extent);
};


#endif /* __khGeoExtents_h */
