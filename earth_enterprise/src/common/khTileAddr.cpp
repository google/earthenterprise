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


#include "common/khTileAddr.h"
#include <cmath>
#include <cassert>

#include "common/khTileAddrConsts.h"
#include "common/notify.h"

// ****************************************************************************
// ***  Standard Fusion Tilespaces
// ****************************************************************************
/*
  --- raster products (formerly known as pyramids) ---
  Tilesize: 1024x1024  (determined heuristicly for fusion performance)
  Level Numbering: level0 is 1 pixel covering world (also know as pixel level)
*/

const khTilespaceBase RasterProductTilespaceBase(
    10,                                          // tileSize:       2^10 = 1024
    0,                                           // pixelsAtLevel0: 2^0  = 1
    StartLowerLeft,                              // orientation
    false);                                      // isVector

const khTilespaceFlat RasterProductTilespaceFlat(RasterProductTilespaceBase);

const khTilespaceMercator RasterProductTilespaceMercator(
    RasterProductTilespaceBase);

/*
  --- client imagery packets -----
  Tilesize: 256x256 (determined by client/server protocol)
  Level Numbering: level0 is one 256x256 tile covering world
*/
const khTilespaceBase ClientImageryTilespaceBase(
    8,                                           // tileSize:       2^8 = 256
    8,                                           // pixelsAtlevel0: 2^8 = 256
    StartUpperLeft,                              // orientation
    false);                                      // isVector

const khTilespaceFlat ClientImageryTilespaceFlat(ClientImageryTilespaceBase);

const khTilespaceMercator ClientImageryTilespaceMercator(
    ClientImageryTilespaceBase);

/*
  --- client tmesh packets -----
  Tilesize: 32x32 (determined by client/server protocol)
  Technically each client packet contains four meshes built from 17x17
  rasters. The four meshes for each packet are processed together from a 33x33
  raster. The 33x33 raster is build from one 32x32 tile with extra pixels
  being fetched from neighboring 32x32 tiles (top, top-right, right).
  But for tilespace purposes, 32x32 is what we use. :-)
  Level Numbering: level0 is one 32x32 tile covering world
*/
const khTilespaceBase ClientTmeshTilespaceBase(
    5,                                           // tileSize:       2^5 = 32
    5,                                           // pixelsAtLevel0: 2^5 = 32
    StartLowerLeft,                              // mesher expects this
    false);                                      // isVector

const khTilespaceFlat ClientTmeshTilespaceFlat(ClientTmeshTilespaceBase);

const khTilespaceMercator ClientTmeshTilespaceMercator(
    ClientTmeshTilespaceBase);

/*
  --- client vector packets -----
  Tilesize: doesn't matter for vector packets (just use the same as imagery)
  Level Numbering: level0 is one tile covering world
*/
const khTilespace ClientVectorTilespace(8,  // tileSize:       2^8 = 256
                                        8,  // pixelsAtlevel0: 2^8 = 256
                                        StartUpperLeft,
                                        true /* isvector */,
                                        khTilespace::FLAT_PROJECTION,
                                        false);


const khTilespace ClientMapFlatTilespace(8,  // tileSize:       2^8 = 256
                                         8,  // pixelsAtlevel0: 2^8 = 256
                                         StartUpperLeft,
                                         false /* isvector */,
                                         khTilespace::FLAT_PROJECTION,
                                         false);

const khTilespace ClientMapMercatorTilespace(
    8,  // tileSize:       2^8 = 256
    8,  // pixelsAtlevel0: 2^8 = 256
    StartUpperLeft,
    false /* isvector */,
    khTilespace::MERCATOR_PROJECTION,
    false);

const khTilespace FusionMapTilespace(
    11,  // tileSize:       2^11 = 2048
    ClientMapFlatTilespace.pixelsAtLevel0Log2,  // use the same levels as client
    StartLowerLeft,
    false /* isvector */,
    khTilespace::FLAT_PROJECTION,
    false);

const khTilespace FusionMapMercatorTilespace(
    FusionMapTilespace.tileSizeLog2,
    FusionMapTilespace.pixelsAtLevel0Log2,
    StartLowerLeft,
    false /* isvector */,
    khTilespace::MERCATOR_PROJECTION,
    true);

// Initially world extent.
khExtents<double>
    khCutExtent::cut_extent(NSEWOrder, 90.0, -90.0, 180.0, -180.0);
const khExtents<double>
    khCutExtent::world_extent(NSEWOrder, 90.0, -90.0, 180.0, -180.0);

khExtents<double> khCutExtent::ConvertFromFlatToMercator(
    const khExtents<double>& deg_extent) {
  return khExtents<double>(
      NSEWOrder,
      MercatorProjection::FromFlatDegLatitudeToMercatorMeterLatitude(
          deg_extent.north()),
      MercatorProjection::FromFlatDegLatitudeToMercatorMeterLatitude(
          deg_extent.south()),
      deg_extent.east() / 360.0 * khGeomUtilsMercator::khEarthCircumference,
      deg_extent.west() / 360.0 * khGeomUtilsMercator::khEarthCircumference);
}

// ****************************************************************************
// ***  khTilespace
// ****************************************************************************
khTilespaceBase::khTilespaceBase(unsigned int tileSizeLog2_,
                                 unsigned int pixelsAtLevel0Log2_,
                                 TileOrientation orientation_,
                                 bool isvec) :
    tileSize(uint(pow(2, tileSizeLog2_))),
    tileSizeLog2(tileSizeLog2_),
    pixelsAtLevel0(uint(pow(2, pixelsAtLevel0Log2_))),
    pixelsAtLevel0Log2(pixelsAtLevel0Log2_),
    orientation(orientation_),
    isVector(isvec) {
  assert(tileSize >= pixelsAtLevel0);
}

khTilespaceBase::khTilespaceBase(const khTilespaceBase& other)
  : tileSize(other.tileSize),
    tileSizeLog2(other.tileSizeLog2),
    pixelsAtLevel0(other.pixelsAtLevel0),
    pixelsAtLevel0Log2(other.pixelsAtLevel0Log2),
    orientation(other.orientation),
    isVector(other.isVector) {
}

// ****************************************************************************
// ***  khTilespace
// ****************************************************************************
khTilespace::khTilespace(unsigned int tileSizeLog2_,
                         unsigned int pixelsAtLevel0Log2_,
                         TileOrientation orientation_,
                         bool isvec,
                         ProjectionType proj_type,
                         bool need_stretching_for_mercator) :
    khTilespaceBase(tileSizeLog2_, pixelsAtLevel0Log2_, orientation_, isvec),
    projection_type(proj_type),
    projection(need_stretching_for_mercator ?
               new MercatorProjection(pixelsAtLevel0) : NULL) {
}

khTilespace::khTilespace(const khTilespaceBase& other, ProjectionType proj_type,
                         bool need_stretching_for_mercator) :
    khTilespaceBase(other),
    projection_type(proj_type),
    projection(need_stretching_for_mercator ?
               new MercatorProjection(pixelsAtLevel0) : NULL) {
}

khTilespaceFlat::khTilespaceFlat(unsigned int tileSizeLog2_,
                                 unsigned int pixelsAtLevel0Log2_,
                                 TileOrientation orientation_,
                                 bool isvec) :
    khTilespace(tileSizeLog2_, pixelsAtLevel0Log2_, orientation_, isvec,
                FLAT_PROJECTION, false) {
}

khTilespaceFlat::khTilespaceFlat(const khTilespaceBase& other)
  : khTilespace(other, FLAT_PROJECTION, false) {
}

khTilespaceMercator::khTilespaceMercator(unsigned int tileSizeLog2_,
                                         unsigned int pixelsAtLevel0Log2_,
                                         TileOrientation orientation_,
                                         bool isvec)
  : khTilespace(tileSizeLog2_, pixelsAtLevel0Log2_, orientation_, isvec,
                MERCATOR_PROJECTION, false) {
}

khTilespaceMercator::khTilespaceMercator(const khTilespaceBase& other)
  : khTilespace(other, MERCATOR_PROJECTION, false) {
}



khExtents<std::uint32_t> khTilespace::WorldExtents(unsigned int level) const {
  // One beyond the last valid tile at this level;
  std::uint32_t endTile = MaxNumTiles(level);

  // Number of blank rows at this level
  std::uint32_t emptyRows = NumEmptyRows(level);

  return khExtents<std::uint32_t>(RowColOrder,
                           0       + emptyRows /* beginRow */,
                           endTile - emptyRows /* endRow */,
                           0                   /* beginCol */,
                           endTile             /* endCol */);
}


khExtents<std::int64_t> khTilespaceFlat::WorldPixelExtents(unsigned int level) const {
  // One beyond the last valid tile at this level;
  std::int64_t endPixel = MaxNumPixels(level);

  // Number of blank rows at this level
  std::int64_t emptyRows = NumEmptyPixels(level);

  return khExtents<std::int64_t>(RowColOrder,
                          0       + emptyRows  /* beginRow */,
                          endPixel - emptyRows /* endRow */,
                          0                    /* beginCol */,
                          endPixel             /* endCol */);
}

khExtents<std::int64_t>
khTilespaceMercator::WorldPixelExtentsMercator(unsigned int level) const {
  // One beyond the last valid tile at this level;
  std::int64_t endPixel = MaxNumPixels(level);
  return khExtents<std::int64_t>(RowColOrder,
                          0        /* beginRow */,
                          endPixel /* endRow */,
                          0        /* beginCol */,
                          endPixel /* endCol */);
}


khLevelCoverage khTilespace::FromNormExtents(
    const khExtents<double>& degExtents, unsigned int fullresLevel,
    unsigned int targetLevel) const {
  return khLevelCoverage::FromNormExtents(
      *this, IsMercator() ? khTilespace::MeterToNormExtents(degExtents)
                          : khTilespace::DegToNormExtents(degExtents),
      fullresLevel, targetLevel);
}


khLevelCoverage khTilespace::FromNormExtentsWithOversizeFactor(
      const khExtents<double>& degExtents, unsigned int fullresLevel, unsigned int targetLevel,
      double oversizeFactor) const {
  return khLevelCoverage::FromNormExtentsWithOversizeFactor(
      *this, IsMercator() ? khTilespace::MeterToNormExtents(degExtents)
                          : khTilespace::DegToNormExtents(degExtents),
      fullresLevel, targetLevel, oversizeFactor);
}


bool khTilespace::IsExtentsWithinWorldBoundary(
    const khExtents<double>& extents,
    khTilespace::ProjectionType projection_type) {
  const double x_max = (projection_type == khTilespace::MERCATOR_PROJECTION) ?
    (khGeomUtilsMercator::khEarthCircumference / 2.0) : (360.0 /2.0);
  const double y_max = (projection_type == khTilespace::MERCATOR_PROJECTION) ?
      x_max : x_max / 2.0;

  return ((extents.north() <=  y_max) && (extents.south() >= -y_max) &&
          (extents.east()  <=  x_max) && (extents.west()  >= -x_max));
}


// ****************************************************************************
// ***  khLevelCoverage
// ****************************************************************************
khLevelCoverage::khLevelCoverage(const khTilespace &tilespace,
                                 const khExtents<double> &degExtents,
                                 unsigned int fullresLevel,
                                 unsigned int targetLevel) {
  khLevelCoverage tmp = tilespace.FromNormExtents(degExtents,
                                                  fullresLevel, targetLevel);
  level = tmp.level;
  extents = tmp.extents;
}

khLevelCoverage::khLevelCoverage(const khTilespace &tilespace,
                                 const khExtents<double> &degExtents,
                                 unsigned int fullresLevel,
                                 unsigned int targetLevel,
                                 double oversizeFactor) {
  khLevelCoverage tmp = tilespace.FromNormExtentsWithOversizeFactor(
      degExtents, fullresLevel, targetLevel, oversizeFactor);
  level = tmp.level;
  extents = tmp.extents;
}

khLevelCoverage::khLevelCoverage(const khTileAddr &addr)
    : level(addr.level),
      extents(RowColOrder,
              addr.row, addr.row+1,
              addr.col, addr.col+1) {
}

khLevelCoverage
khLevelCoverage::FromNormExtentsWithOversizeFactor(
    const khTilespace &tilespace,
    khExtents<double> normExtents,
    unsigned int fullresLevel,
    unsigned int targetLevel,
    double oversizeFactor) {
  // when converting from geo-space to tile space, we MUST
  // clamp to avoid negative tile addresses (which wrap when stored in std::uint32_t)
  khExtents<double> clampExtents(RowColOrder, 0.0, 1.0, 0.0, 1.0);

  khExtents<double> boundedExtents
    (khExtents<double>::Intersection(normExtents, clampExtents));

  std::uint32_t north, south, east, west;

  const std::uint32_t numTiles = tilespace.MaxNumTiles(fullresLevel);

  if (tilespace.NeedStretchingForMercator()) {
    // calculate pixel boundaries
    Projection::Point upper_left =
        tilespace.projection->FromNormLatLngToPixel(
            Projection::LatLng(boundedExtents.north(),
                               boundedExtents.west()),
            fullresLevel);
    Projection::Point lower_right =
        tilespace.projection->FromNormLatLngToPixel(
            Projection::LatLng(boundedExtents.south(),
                               boundedExtents.east()),
            fullresLevel);

    // create an extents object with our pixel boundaries to facilitate adding
    // the oversize factor
    khExtents<std::uint64_t> pixel_space(XYOrder, upper_left.X(), lower_right.X(),
                                  lower_right.Y(), upper_left.Y());
    pixel_space.alignBy(tilespace.tileSize);
    north = static_cast<std::uint32_t>(pixel_space.north() / tilespace.tileSize);
    south = static_cast<std::uint32_t>(pixel_space.south() / tilespace.tileSize);
    east = static_cast<std::uint32_t>(pixel_space.east() / tilespace.tileSize);
    west = static_cast<std::uint32_t>(pixel_space.west() / tilespace.tileSize);
  } else {
    // initialize with the fullres level & extents
    north = static_cast<std::uint32_t>(ceil(boundedExtents.north() * numTiles)),
    south = static_cast<std::uint32_t>(boundedExtents.south() * numTiles),
    east = static_cast<std::uint32_t>(ceil(boundedExtents.east()  * numTiles)),
    west = static_cast<std::uint32_t>(boundedExtents.west()  * numTiles);
  }
  // Refer gstGeoIndex.cpp gstGeoIndexImpl::Finalize; We may have already gone
  // to 3 levels down. Now if we don't add any extra tile, going to 3 level up
  // will not add even one 256 * 256 tile and thatway there will be no
  // oversize even when oversizeFactor != 0. Note that oversize factor is being
  // applied on feature bounding box and not on tile boundary.
  if (oversizeFactor != 0.0) {
    // Stretch in width, delta_width = width * oversizeFactor
    // Stretch in width in each dir = delta_width / 2.0
    const double more_in_each_dir = oversizeFactor / 2.0;
    std::uint32_t more;
    // TODO: temporary solution. It is required conceptual solution
    // because the bounding box of point-feature is degenerate
    // (width and height equal 0).
    std::uint32_t delta = north - south;
    if (0 == delta)
      ++delta;

    more = static_cast<std::uint32_t>(ceil(more_in_each_dir * delta));
    north += more;
    // Since we use std::uint32_t for south and west need to take care not to go
    // below 0 while oversizing.
    south = south < more ? 0 : south - more;

    delta = east - west;
    if (0 == delta)
      ++delta;

    more = static_cast<std::uint32_t>(ceil(more_in_each_dir * delta));
    east += more;
    west = west < more ? 0 : west - more;
  }
  // vector is special - if the end lands exactly on the boundary
  // we need an extra tile to contain it. For raster the last pixel ends
  // at the boundary so we don't need (or want) the extra tile.
  if (tilespace.IsVector()) {
    north++;
    east++;
    // only add extra tile left & down if it's absolutely necessary
    if (south > 0 &&
        (boundedExtents.south() * numTiles) - static_cast<double>(south) == 0.0)
      south--;
    if (west > 0 &&
        (boundedExtents.west() * numTiles) - static_cast<double>(west) == 0.0)
      west--;
  }

  khLevelCoverage cov(fullresLevel,
                      khExtents<std::uint32_t>(NSEWOrder, north, south, east, west));

  // scale it to the target level
  if (targetLevel != fullresLevel) {
    cov.scaleToLevel(targetLevel);
  }
  return cov;
}

khLevelCoverage
khLevelCoverage::FromNormExtentsWithCrop(const khTilespace &tilespace,
                                         const khExtents<double> &normExtents,
                                         unsigned int fullresTileLevel,
                                         unsigned int targetLevel) {
  khLevelCoverage cov = FromNormExtents(tilespace, normExtents,
                                        fullresTileLevel, targetLevel);
  cov.cropToWorld(tilespace);
  return cov;
}


khLevelCoverage
khLevelCoverage::GetSubset(unsigned int subsetThis, unsigned int subsetTotal) const {
  // By virtue of the range of unsigned int, this check also ensures that
  // subsetTotal != 0
  if (subsetThis >= subsetTotal) {
    notify(NFY_WARN, "Internal Error: Invalid subset specification");
    return khLevelCoverage(level, khExtents<std::uint32_t>());
  }

  // We make a subset by breaking up the columns
  // TODO: try to break it into roughly equal sized squares
  if (subsetTotal == 1) {
    return *this;
  } else {
    assert(subsetTotal != 0);
    std::uint32_t numCols = extents.width();
    std::uint32_t colsPerSubset = (numCols + subsetTotal - 1) / subsetTotal;
    std::uint32_t beginCol = subsetThis * colsPerSubset;
    std::uint32_t endCol   = std::min((subsetThis+1) * colsPerSubset,
                               extents.endCol());
    if (beginCol >= endCol) {
      // There's nothing left for this part to do. Just return an
      // empty coverage
      return khLevelCoverage(level, khExtents<std::uint32_t>());
    } else {
      return khLevelCoverage(level,
                             khExtents<std::uint32_t>(RowColOrder,
                                               extents.beginRow(),
                                               extents.endRow(),
                                               beginCol, endCol));
    }
  }
}

khLevelCoverage
khLevelCoverage::UpperCoverage(const khTilespace &tilespace) const {
  if (extents.empty())
    return *this;

  // Pick one of the tiles in my upper row and use khTileAddr's UpperAddr
  // routine to figure out any wrapping
  khTileAddr addr(level, extents.endRow()-1, extents.beginCol());
  khTileAddr upper(addr.UpperAddr(tilespace));

  // build a new level coverage around the answer
  return khLevelCoverage(level,
                         khExtents<std::uint32_t>(RowColOrder,
                                           upper.row,
                                           upper.row+1,
                                           extents.beginCol(),
                                           extents.endCol()));
}

khLevelCoverage
khLevelCoverage::RightCoverage(const khTilespace &tilespace) const {
  if (extents.empty())
    return *this;

  // Pick one of the tiles in my right col and use khTileAddr's RightAddr
  // routine to figure out any wrapping
  khTileAddr addr(level, extents.beginRow(), extents.endCol()-1);
  khTileAddr right(addr.RightAddr(tilespace));

  // build a new level coverage around the answer
  return khLevelCoverage(level,
                         khExtents<std::uint32_t>(RowColOrder,
                                           extents.beginRow(),
                                           extents.endRow(),
                                           right.col,
                                           right.col+1));
}

khLevelCoverage
khLevelCoverage::UpperRightCoverage(const khTilespace &tilespace) const {
  if (extents.empty())
    return *this;

  // Pick my upper right tile and use khTileAddr's UpperRightAddr
  // routine to figure out any wrapping
  khTileAddr addr(level, extents.endRow()-1, extents.endCol()-1);
  khTileAddr upperright(addr.UpperRightAddr(tilespace));

  // build a new level coverage around the answer
  return khLevelCoverage(level,
                         khExtents<std::uint32_t>(RowColOrder,
                                           upperright.row,
                                           upperright.row+1,
                                           upperright.col,
                                           upperright.col+1));
}



// ****************************************************************************
// ***  khTileAddr
// ****************************************************************************
khTileAddr
khTileAddr::UpperAddr(const khTilespace &tilespace) const {
  std::uint32_t newRow;
  if (tilespace.WorldExtents(level).endRow()-1 > row) {
    newRow = row+1;
  } else {
    newRow = 0;
  }
  return khTileAddr(level, newRow, col);
}

khTileAddr
khTileAddr::RightAddr(const khTilespace &tilespace) const {
  std::uint32_t newCol;
  if (tilespace.WorldExtents(level).endCol()-1 > col) {
    newCol = col+1;
  } else {
    newCol = 0;
  }
  return khTileAddr(level, row, newCol);
}

khTileAddr
khTileAddr::UpperRightAddr(const khTilespace &tilespace) const {
  std::uint32_t newRow;
  if (tilespace.WorldExtents(level).endRow()-1 > row) {
    newRow = row+1;
  } else {
    newRow = 0;
  }
  std::uint32_t newCol;
  if (tilespace.WorldExtents(level).endCol()-1 > col) {
    newCol = col+1;
  } else {
    newCol = 0;
  }
  return khTileAddr(level, newRow, newCol);
}


int EfficientLOD(const double feature_diameter,
                 const khTilespace &tilespace,
                 const double diameter_at_lod) {
  assert(0 != tilespace.pixelsAtLevel0);
  if (.0 == feature_diameter) {
    return 0;
  }

  const double kLog2 = std::log(2);
  // halfway of next level in logarithmic scale.
  const double m = std::log(3/2) / kLog2;

  double efficient_lod_f =
      std::log(diameter_at_lod / (feature_diameter *
                      tilespace.pixelsAtLevel0)) / kLog2;
  int efficient_lod = (efficient_lod_f < .0) ? 0 :
                       static_cast<int>(efficient_lod_f + (1.0 - m));

  return efficient_lod;
}
