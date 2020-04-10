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


#include "khGeoExtents.h"
#include <khTileAddr.h>

// ****************************************************************************
// ***  Misc Helpers
// ****************************************************************************
static
void
MakeGeoTransform(double startX, double deltaX,
                 double startY, double deltaY,
                 double geoTransform[6])
{
  geoTransform[0] = startX;
  geoTransform[1] = deltaX;
  geoTransform[2] = 0.0;
  geoTransform[3] = startY;
  geoTransform[4] = 0.0;
  geoTransform[5] = deltaY;
}


// ****************************************************************************
// ***  khGeoExtents
// ****************************************************************************
khGeoExtents::khGeoExtents(const khExtents<double> &extents__,
                           const khSize<std::uint32_t> &rasterSize,
                           bool topToBottom) :
    extents_(extents__)
{
  if ((rasterSize.width == 0) || (rasterSize.height == 0)) {
    extents_ = khExtents<double>();
    memset(geoTransform_, 0, sizeof(geoTransform_));
  } else {
    // build a geoTransform_ from the extents_
    double pixelWidth  = extents_.width()  / (double)rasterSize.width;
    double pixelHeight = extents_.height() / (double)rasterSize.height;
    MakeGeoTransform(extents_.west(),
                     pixelWidth,
                     topToBottom ? extents_.north() : extents_.south(),
                     topToBottom ? -pixelHeight    : pixelHeight,
                     geoTransform_);
  }
}

khSize<std::uint32_t> khGeoExtents::boundToExtent(const khExtents<double>& extent) {
  const khExtents<double> new_extent =
      khExtents<double>::Intersection(extents_, extent);
  const double pixel_width = absPixelWidth();
  const double pixel_height = absPixelHeight();
  const khSize<std::uint32_t> raster_size((new_extent.width() / pixel_width + 0.5),
                                   (new_extent.height() / pixel_height + 0.5));

  if (new_extent != extents_) {
    // align to pixel_width and pixel_height (by rounding)
    int n_pxl = static_cast<int>(new_extent.north() / pixel_height +
                                 (new_extent.north() > 0 ? 0.5 : -0.5));
    int s_pxl = n_pxl - raster_size.height;
    int e_pxl = static_cast<int>(new_extent.east() / pixel_width +
                                 (new_extent.east() > 0 ? 0.5 : -0.5));
    int w_pxl = e_pxl - raster_size.width;
    const bool is_top_to_bottom = topToBottom();
    extents_ = khExtents<double>(
        NSEWOrder, n_pxl * pixel_height, s_pxl * pixel_height,
        e_pxl * pixel_width, w_pxl * pixel_width);
    MakeGeoTransform(extents_.west(),
                     pixel_width,
                     is_top_to_bottom ? extents_.north() : extents_.south(),
                     is_top_to_bottom ? -pixel_height : pixel_height,
                     geoTransform_);
  }
  return raster_size;
}

khGeoExtents::khGeoExtents(const khExtents<double> &extents__,
                           double pixelSize,
                           bool topToBottom) :
    extents_(extents__)
{
  // build a geoTransform_ from the extents_
  double pixelWidth  = pixelSize;
  double pixelHeight = pixelSize;
  MakeGeoTransform(extents_.west(),
                   pixelWidth,
                   topToBottom ? extents_.north() : extents_.south(),
                   topToBottom ? -pixelHeight    : pixelHeight,
                   geoTransform_);
}


khGeoExtents::khGeoExtents(const khOffset<double> &origin,
                           double pixelSize,
                           const khSize<std::uint32_t> &rasterSize,
                           bool topToBottom) :
    extents_(origin,
             khSize<double>(rasterSize.width * pixelSize,
                            rasterSize.height * pixelSize))
{
  // build a geoTransform_ from the extents_
  double pixelWidth  = pixelSize;
  double pixelHeight = pixelSize;
  MakeGeoTransform(extents_.west(),
                   pixelWidth,
                   topToBottom ? extents_.north() : extents_.south(),
                   topToBottom ? -pixelHeight    : pixelHeight,
                   geoTransform_);
}


khGeoExtents::khGeoExtents(const khExtents<std::int64_t> &pixelExtents,
                           double pixelSize,
                           bool topToBottom,
                           double offset) :
    extents_(XYOrder,
             (pixelExtents.beginX() * pixelSize) - offset,
             (pixelExtents.endX()   * pixelSize) - offset,
             (pixelExtents.beginY() * pixelSize) - offset,
             (pixelExtents.endY()   * pixelSize) - offset)
{
  // build a geoTransform_ from the extents_
  double pixelWidth  = pixelSize;
  double pixelHeight = pixelSize;
  MakeGeoTransform(extents_.west(),
                   pixelWidth,
                   topToBottom ? extents_.north() : extents_.south(),
                   topToBottom ? -pixelHeight    : pixelHeight,
                   geoTransform_);
}

khGeoExtents::khGeoExtents(unsigned int level,
                           const khExtents<double> &extents__,
                           bool topToBottom, bool is_mercator) :
    extents_(extents__)
{
  double pixelSize = is_mercator ?
      RasterProductTilespaceMercator.AveragePixelSizeInMercatorMeters(level) :
      RasterProductTilespaceFlat.DegPixelSize(level);
  MakeGeoTransform(extents_.west(),
                   pixelSize,
                   topToBottom ? extents_.north() : extents_.south(),
                   topToBottom ? -pixelSize       : pixelSize,
                   geoTransform_);
}


khGeoExtents::khGeoExtents(double geoTransform__[6],
                           const khSize<std::uint32_t> &rasterSize)
{
  if ((rasterSize.width == 0) || (rasterSize.height == 0)) {
    extents_ = khExtents<double>();
    memset(geoTransform_, 0, sizeof(geoTransform_));
  } else {
    memcpy(geoTransform_, geoTransform__, sizeof(geoTransform_));

    // extents_ from the geoTransform_
    double north, south, east, west;
    if (geoTransform_[5] < 0) {
      // top -> bottom image layout
      north = geoTransform_[3];
      south = geoTransform_[3] + geoTransform_[5] * rasterSize.height;
    } else {
      // bottom -> top image layout
      south = geoTransform_[3];
      north = geoTransform_[3] + geoTransform_[5] * rasterSize.height;
    }
    if (geoTransform_[1] < 0) {
      // right -> left image layout
      east  = geoTransform_[0];
      west  = geoTransform_[0] + geoTransform_[1] * rasterSize.width;
    } else {
      // left -> right image layout
      west  = geoTransform_[0];
      east  = geoTransform_[0] + geoTransform_[1] * rasterSize.width;
    }

    extents_ = khExtents<double>(NSEWOrder, north, south, east, west);
  }
}
