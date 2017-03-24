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

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_KHTILEADDR_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_KHTILEADDR_H_

#include <math.h>
#include <string>
#include "common/khTypes.h"
#include "common/khExtents.h"
#include "common/khGeomUtils.h"
#include "common/khMisc.h"
#include "common/khTileAddrConsts.h"
#include "common/quadtreepath.h"
#include "common/projection.h"


// ****************************************************************************
// ***  Packed 64bit tile address
// ***    - Used in several places as a hash key for tiles
// ****************************************************************************
// Maximum virtual texture size is 2^32
//   we need 24 bits to represent the number
//   of 256x256 tiles on the 2^32 level
//
// Maximum level is 32
//   we need 5 bits, or 2^5 = 32
//
// Sub-tex gets 1 bit.  0 = main texture, 1 = alpha
//
// Support 1024 unique textures with 10 bits
//
// build the tile address as follows:
//   (col:24) + (row:24) + (lev:5) + (sub:1) + (src:10)
//
#define TILEADDR(lev, row, col, sub, src) \
    (  (static_cast<uint64>(src) & 0x0003ff)        | \
      ((static_cast<uint64>(sub) & 0x000001) << 10) | \
      ((static_cast<uint64>(lev) & 0x00001f) << 11) | \
      ((static_cast<uint64>(row) & 0xffffff) << 16) | \
      ((static_cast<uint64>(col) & 0xffffff) << 40) )

#define SRCFROMADDR(addr) static_cast<int>(0x0003ff & (addr))
#define SUBFROMADDR(addr) static_cast<int>(0x000001 & ((addr) >> 10))
#define LEVFROMADDR(addr) static_cast<int>(0x00001f & ((addr) >> 11))
#define ROWFROMADDR(addr) static_cast<int>(0xffffff & ((addr) >> 16))
#define COLFROMADDR(addr) static_cast<int>(0xffffff & ((addr) >> 40))

// Not technically invalid. But in practice it is. We use a src ID of 0
// which means the base texture, but we mark it with sub of 1 which means
// alpha. The base texture never uses alpha, so this addr should never occur
// in the wild.
static const uint64 InvalidTileAddrHash = TILEADDR(0, 0, 0, 1, 0);


// Don't change the values in this enum, they are stored in raster ff indexes
enum TileOrientation {
  StartUpperLeft = 0,
  StartLowerLeft = 1
};

inline std::string TileOrientationName(TileOrientation t) {
  switch (t) {
    case StartUpperLeft:
      return "StartUpperLeft";
    case StartLowerLeft:
      return "StartLowerLeft";
  }
  return std::string();  // unreached but silences warnings
}

class khLevelCoverage;

// ****************************************************************************
// ***  khTilespaceBase - Definition of a tile address space.
// ***
// ***    Tile addresses in Keyhole are comprised of a level, row and column.
// ***  However, there are different tile sizes and different
// ***  interpretations of level numbers. khTilespace holds the information
// ***  necessary to interpret a tile address.
// ***    Most of the code will only deal with one khTilespace. So for the
// ***  sake of efficiency, the tilespace is NOT stored as part of the
// ***  address. The code will implicity know what tilespace it uses. Only
// ***  those piece of code that need to translate from one tilespace to
// ***  another wil need to specify what they have and what they want.
// ***    Some translation routines need the tileSize and pixelsAtLevel0 as
// ***  whole numbers while others need them as log2 of the whole
// ***  number. This class holds both for easy access.
// ***    The standard Keyhole tilespaces are defined below as constants.
// ****************************************************************************
class khTilespaceBase {
 public:
  uint tileSize;  // in pixel unit
  uint tileSizeLog2;
  uint pixelsAtLevel0;
  uint pixelsAtLevel0Log2;
  TileOrientation orientation;
  bool isVector;


  khTilespaceBase(uint tileSizeLog2_, uint pixelsAtLevel0Log2_,
                  TileOrientation orientation_, bool isvec);

  inline bool IsVector(void) const { return isVector; }

  // returns lowest level where one tile covers entire world
  inline uint SingleTileLevel(void) const {
    return tileSizeLog2 - pixelsAtLevel0Log2;
  }

  // Maximum number of tiles on a given level. This number is valid for
  // both X & Y, but does NOT take into account empty tiles near the
  // poles.
  inline uint32 MaxNumTiles(uint level) const {
    return ((level <= SingleTileLevel())
            ? 1U
            : (0x1U << (level - SingleTileLevel())));
  }

  inline int SinglePixelLevel(void) const {
    return -pixelsAtLevel0Log2;
  }

  inline int64 MaxNumPixels(uint level) const {
    return ((static_cast<int>(level) <= SinglePixelLevel())
            ? 1LL
            : (0x1LL << (static_cast<int>(level) - SinglePixelLevel())));
  }

  // Return the normalized size of each pixel at the specified level
  // By normalized we mean the whole map is [0, 1.0] * [0, 1.0], i.e max
  // dimension in both x and y coordinates are 1.0 and min dimension is 0.0
  inline double NormPixelSize(uint level) const {
    uint pixels_at_this_thile_space_level = pixelsAtLevel0 * (0x1U << level);
    return 1.0 / pixels_at_this_thile_space_level;
  }

  // Return the normalized size of each tile at the specified level
  inline double NormTileSize(uint level) const {
    if (pixelsAtLevel0 == tileSize) {
      // avoid any potential rounding errors - hand simplify
      return 1.0 / (0x1U << level);
    } else {
      return NormPixelSize(level) * tileSize;
    }
  }

  // Normalize degrees in range [-180, 180] to [0, 1]
  static double Normalize(double deg) { return ((deg + 180.0) / 360.0); }

  // Transform from degree extents to corresponding normalized extents
  static khExtents<double> DegToNormExtents(
      const khExtents<double> &degExtents) {
    return khExtents<double>(NSEWOrder,
                             Normalize(degExtents.north()),
                             Normalize(degExtents.south()),
                             Normalize(degExtents.east()),
                             Normalize(degExtents.west()));
  }

  // Given a x or y coordinate in mercator meter units get its corresponding
  // normalized coordinate. The mercator meter coordinate may be in
  // [-khEarthCircumference/2, +khEarthCircumference/2]. So we need to both
  // scale and translate (+0.5) to get in normalized unit in range [0,1].
  // nsew is used to represent north/south/east/west.
  static double NormalizeMeter(double meter_coordinate) {
    return (meter_coordinate / khGeomUtilsMercator::khEarthCircumference + 0.5);
  }

  // Given a khExtents in mercator meter units get its corresponding
  // normalized khExtents.
  static khExtents<double> MeterToNormExtents(
      const khExtents<double> &degExtents) {
    return khExtents<double>(NSEWOrder,
                             NormalizeMeter(degExtents.north()),
                             NormalizeMeter(degExtents.south()),
                             NormalizeMeter(degExtents.east()),
                             NormalizeMeter(degExtents.west()));
  }

  // Transform from range [0, 1] to [-180, 180]
  static double Denormalize(double norm) { return ((norm * 360.0) - 180.0); }

  // The fusion standard is to store geographic extents in WGS84 degrees.
  // If somebody has norm extents, it's because they already converted
  // from degrees. To convert back again is just asking for data loss. Any code
  // that wants this routine should use the original deg extents instead.
  static khExtents<double> NormToDegExtents(
      const khExtents<double> &normExtents) {
    return khExtents<double>(NSEWOrder,
                             Denormalize(normExtents.north()),
                             Denormalize(normExtents.south()),
                             Denormalize(normExtents.east()),
                             Denormalize(normExtents.west()));
  }

  // Convert from normalized unit to mercator meter unit
  static double DeNormalizeMeter(double norm) {
    return ((norm - 0.5) * khGeomUtilsMercator::khEarthCircumference);
  }

  // Given a khExtents in normalized units get khExtents in its corresponding
  // mercator meter units.
  static khExtents<double> NormToMeterExtents(
      const khExtents<double> &normExtents) {
    return khExtents<double>(NSEWOrder,
                             DeNormalizeMeter(normExtents.north()),
                             DeNormalizeMeter(normExtents.south()),
                             DeNormalizeMeter(normExtents.east()),
                             DeNormalizeMeter(normExtents.west()));
  }

 protected:
  khTilespaceBase(const khTilespaceBase& other);

 private:
  khTilespaceBase();
  const khTilespaceBase& operator =(const khTilespaceBase& other);
};


// Adds projection specific methods to  khTilespaceBase
class khTilespace : public khTilespaceBase {
 public:
  // projection support
  enum ProjectionType { FLAT_PROJECTION, MERCATOR_PROJECTION };
  const ProjectionType projection_type;
  Projection* const projection;

  khTilespace(uint tileSizeLog2_, uint pixelsAtLevel0Log2_,
              TileOrientation orientation_, bool isvec,
              ProjectionType proj_type,
              bool need_stretching_for_mercator);

  bool NeedStretchingForMercator() const { return projection != NULL; }

  // Return true if projection_type of this khTilespace is mercator false
  // otherwise.
  bool IsMercator() const { return (projection_type == MERCATOR_PROJECTION); }

  // Returns the number of rows that will be empty (both on top and on
  // bottom). This is because latitudes only span from -90 -> +90 while
  // longitudes span from -180 -> +180.
  inline uint32 NumEmptyRows(uint level) const {
    if (projection_type == FLAT_PROJECTION) {
      return ((level < SingleTileLevel() + 2)
              ? 0U :
              (0x1U << (level - SingleTileLevel() - 2)));
    } else {
      return 0;
    }
  }

  // Return the tile extents that give full world coverage at the
  // specified level. Empty tiles due to latitude range restrictions (see
  // NumEmptyRows above) are not included.
  khExtents<uint32> WorldExtents(uint level) const;

  // Helper method to create khLevelCoverage from khExtents
  khLevelCoverage FromNormExtents(const khExtents<double>& degExtents,
                                  uint fullresLevel,
                                  uint targetLevel) const;

  // Helper method: create khLevelCoverage from khExtents with oversize factor.
  khLevelCoverage FromNormExtentsWithOversizeFactor(
      const khExtents<double>& degExtents, uint fullresLevel, uint targetLevel,
      double oversizeFactor) const;

  // In case projection_type is MERCATOR_PROJECTION the extents are expected in
  // GdalMercatorMeter unit else (if projection_type is Wjs84) the extents are
  // expected in degree unit.
  static bool IsExtentsWithinWorldBoundary(
      const khExtents<double>& extents,
      khTilespace::ProjectionType projection_type);

 protected:
  khTilespace(const khTilespaceBase& other, ProjectionType proj_type,
              bool need_stretching_for_mercator);

 private:
  khTilespace();
  khTilespace(const khTilespace& other);
  const khTilespace& operator =(const khTilespace& other);
};

// Tile space config. Higher-level abstraction mixing data/alpha tile types
// and product/target tile spaces.
template<class DataTile, class AlphaTile>
class TilespaceConfig {
 public:
  TilespaceConfig(const khTilespace &_product_tilespace,
                  const khTilespaceBase &_target_tilespace)
      : product_tilespace(_product_tilespace),
        target_tilespace(_target_tilespace) {
    assert(DataTile::TileWidth == product_tilespace.tileSize);
    assert(DataTile::TileHeight == product_tilespace.tileSize);
    assert(AlphaTile::TileWidth == product_tilespace.tileSize);
    assert(AlphaTile::TileHeight == product_tilespace.tileSize);
    assert(sizeof(typename DataTile::PixelType) ==
           sizeof(typename AlphaTile::PixelType));
  }

  inline uint32 RasterProductTileSize(void) const {
    return product_tilespace.tileSize;
  }

  inline uint32 TargetTileSize(void) const {
    return target_tilespace.tileSize;
  }

  inline uint32 TargetBufSize(void) const {
    return (target_tilespace.tileSize *
            target_tilespace.tileSize *
            DataTile::NumComp *
            sizeof(typename DataTile::PixelType));
  }

  inline uint32 TargetAlphaBufSize(void) const {
    // if is_mercator return 4 channel buffer size.
    return (target_tilespace.tileSize *
            target_tilespace.tileSize *
            (product_tilespace.IsMercator() ?
             (DataTile::NumComp + AlphaTile::NumComp) : AlphaTile::NumComp) *
            sizeof(typename AlphaTile::PixelType));
  }

  inline uint32 DataNumComp(void) const {
    return DataTile::NumComp;
  }

  inline uint32 DataPixelTypeSize(void) const {
    return sizeof(typename DataTile::PixelType);
  }

  inline uint32 AlphaNumComp(void) const {
    return AlphaTile::NumComp;
  }

  inline uint32 AlphaPixelTypeSize(void) const {
    return sizeof(typename AlphaTile::PixelType);
  }

  const khTilespace& product_tilespace;
  const khTilespaceBase& target_tilespace;
};

// Adds projection specific methods to  khTilespaceBase
class khTilespaceFlat : public khTilespace {
 public:
  khTilespaceFlat(uint tileSizeLog2_, uint pixelsAtLevel0Log2_,
                  TileOrientation orientation_, bool isvec);

  explicit khTilespaceFlat(const khTilespaceBase& other);

  inline int64 NumEmptyPixels(uint level) const {
    return ((static_cast<int>(level) < SinglePixelLevel() + 2)
            ? 0LL :
            (0x1LL << (static_cast<int>(level) - SinglePixelLevel() - 2)));
  }

  // Return the tile extents that give full world coverage at the
  // specified level. Empty tiles due to latitude range restrictions (see
  // NumEmptyRows above) are not included.
  khExtents<int64> WorldPixelExtents(uint level) const;

  // Return the degree size of each pixel at the specified level
  inline double DegPixelSize(uint level) const {
    uint pixels_at_this_thile_space_level = pixelsAtLevel0 * (0x1U << level);
    return 360.0 / pixels_at_this_thile_space_level;
  }

  // Return the level where this pixel size belongs. It will always "snapup"
  // the level.
  inline uint LevelFromDegPixelSize(double degPixelSize) const {
    uint level = 0;
    for (; level < NumFusionLevels; ++level) {
      if (degPixelSize >= DegPixelSize(level))
        break;
    }
    return level;
  }

 private:
  khTilespaceFlat();
  khTilespaceFlat(const khTilespaceFlat& other);
  const khTilespaceFlat& operator =(const khTilespaceFlat& other);
  bool NeedStretchingForMercator() const;
  bool IsMercator() const;
};


// Adds projection specific methods to  khTilespaceBase
class khTilespaceMercator : public khTilespace {
 public:
  khTilespaceMercator(uint tileSizeLog2_, uint pixelsAtLevel0Log2_,
                      TileOrientation orientation_, bool isvec);

  explicit khTilespaceMercator(const khTilespaceBase& other);

  // Return the tile extents that give full world coverage at the specified
  // zoom level for mercator projection. Note that unlike Wjs84 map there is no
  // NumEmptyRows for mercator map as we need it square.
  khExtents<int64> WorldPixelExtentsMercator(uint level) const;

  // For Mercator projection GdalWarp gives world map in a virtual meter unit
  // of 40075016.6855784 * 40075016.6855784 (considering latitude range of
  // (-85.051128779806575, 85.051128779806575)
  // This method tries to find out pixel size in those virtual meter units when
  // zoom level is provided. Note: The meter unit is virtual because world is
  // by no means that big in y coordinate. Its just Mercator stretching.
  inline double AveragePixelSizeInMercatorMeters(uint level) const {
    uint pixels_at_this_thile_space_level = pixelsAtLevel0 * (0x1U << level);
    return (khGeomUtilsMercator::khEarthCircumference /
            pixels_at_this_thile_space_level);
  }

  // Return the level where this pixel size belongs. It will always "snapup"
  // the level.
  inline uint LevelFromPixelSizeInMeters(double pixelSizeInMeters) const {
    uint level = 0;
    for (; level < NumFusionLevels; ++level) {
      if (pixelSizeInMeters >= AveragePixelSizeInMercatorMeters(level))
        break;
    }
    return level;
  }

 private:
  khTilespaceMercator();
  khTilespaceMercator(const khTilespaceMercator& other);
  const khTilespaceMercator& operator =(const khTilespaceMercator& other);
  bool NeedStretchingForMercator() const;
  bool IsMercator() const;
};


// ****************************************************************************
// ***  Standard Fusion Tilespaces
// ***  See .cpp file for detailed descriptions
// ****************************************************************************
extern const khTilespaceBase RasterProductTilespaceBase;
extern const khTilespaceFlat RasterProductTilespaceFlat;
extern const khTilespaceMercator RasterProductTilespaceMercator;
extern const khTilespaceBase ClientImageryTilespaceBase;
extern const khTilespaceFlat ClientImageryTilespaceFlat;
extern const khTilespaceMercator ClientImageryTilespaceMercator;
extern const khTilespaceBase ClientTmeshTilespaceBase;
extern const khTilespaceFlat ClientTmeshTilespaceFlat;
extern const khTilespaceMercator ClientTmeshTilespaceMercator;

// pixels sizes & tile orientation don't matter for vectors, but a lot of
// coverage routines want a tilespace so thay can know which level has a
// full tile. This definition gives us something named nice to pass to those
// routines
extern const khTilespace ClientVectorTilespace;

extern const khTilespace ClientMapFlatTilespace;
extern const khTilespace ClientMapMercatorTilespace;
extern const khTilespace FusionMapTilespace;
extern const khTilespace FusionMapMercatorTilespace;

// ****************************************************************************
// ***  LevelTranslation routines
// ****************************************************************************

// Warning! - Not all levels can be translated from one tilespace to another
// For example, product level 3 is imagery level -5 which is invalid.  This
// routine (like its predecessors) makes no attempt to check for invalid
// conversions.
inline
uint
TranslateTileLevel(
    const khTilespaceBase& from, uint level, const khTilespaceBase& to) {
  return level + (static_cast<int>(from.pixelsAtLevel0Log2) -
                  static_cast<int>(to.pixelsAtLevel0Log2));
}

// Shorthand, convenience routines
inline uint ProductToImageryLevel(uint prodLevel) {
  return TranslateTileLevel(RasterProductTilespaceBase, prodLevel,
                            ClientImageryTilespaceBase);
}
inline uint ImageryToProductLevel(uint imageryLevel) {
  return TranslateTileLevel(ClientImageryTilespaceBase, imageryLevel,
                            RasterProductTilespaceBase);
}
inline uint ProductToTmeshLevel(uint prodLevel) {
  return TranslateTileLevel(RasterProductTilespaceBase, prodLevel,
                            ClientTmeshTilespaceBase);
}
inline uint TmeshToProductLevel(uint tmeshLevel) {
  return TranslateTileLevel(ClientTmeshTilespaceBase, tmeshLevel,
                            RasterProductTilespaceBase);
}

class khTileAddr;

// ****************************************************************************
// ***  khLevelCoverage - Tile coverage over a single level
// ****************************************************************************
class khLevelCoverage {
 public:
  uint level;
  khExtents<uint32> extents;

  inline bool empty(void) const { return extents.empty(); }
  inline uint64 NumTiles(void) const {
    return static_cast<uint64>(extents.numRows()) *
           static_cast<uint64>(extents.numCols());
  }

  inline bool operator==(const khLevelCoverage& o) const {
    return ((level == o.level) && (extents == o.extents));
  }
  inline bool operator!=(const khLevelCoverage &o) const {
    return !operator==(o);
  }


  inline khLevelCoverage(void) : level(0), extents() { }


  // build a coverage from a single tile
  explicit khLevelCoverage(const khTileAddr &addr);

  // build a coverage from low level pieces
  inline khLevelCoverage(uint lev, const khExtents<uint32> &extents_) :
      level(lev), extents(extents_) { }


  // ** Note ***
  // Don't make fullresLevel or targetlevel optional. It would then be too
  // easy for the caller to forget and just map directly from degExtents
  // to targetLevel.
  khLevelCoverage(const khTilespace& tilespace,
                  const khExtents<double>& degExtents,
                  uint fullresTileLevel,
                  uint targetLevel);

  khLevelCoverage(const khTilespace& tilespace,
                  const khExtents<double>& degExtents,
                  uint fullresTileLevel,
                  uint targetLevel,
                  double oversizeFactor);

  static inline khLevelCoverage FromNormExtents(
      const khTilespace& tilespace,
      const khExtents<double> &normExtents,
      uint fullresTileLevel,
      uint targetLevel) {
    return  FromNormExtentsWithOversizeFactor(tilespace, normExtents,
                                              fullresTileLevel, targetLevel,
                                              0.0 /* oversizeFactor */);
  }

  static khLevelCoverage FromNormExtentsWithOversizeFactor(
      const khTilespace& tilespace,
      khExtents<double> normExtents,
      uint fullresTileLevel,
      uint targetLevel,
      double oversizeFactor);

  static khLevelCoverage FromNormExtentsWithCrop(
      const khTilespace& tilespace, const khExtents<double> &noExtents,
      uint fullresTileLevel, uint targetLevel);

  inline khLevelCoverage& minifyBy(uint numLevels) {
    level -= numLevels;
    uint pad = (0x1U << numLevels) - 1;
    extents = khExtents<uint32>(XYOrder,
                                extents.beginX()     >> numLevels,
                                (extents.endX()+pad) >> numLevels,
                                extents.beginY()     >> numLevels,
                                (extents.endY()+pad) >> numLevels);
    return *this;
  }

  inline khLevelCoverage MinifiedBy(uint numLevels) const {
    uint pad = (0x1U << numLevels) - 1;
    return khLevelCoverage
      (level - numLevels,
       khExtents<uint32>(XYOrder,
                         extents.beginX()     >> numLevels,
                         (extents.endX()+pad) >> numLevels,
                         extents.beginY()     >> numLevels,
                         (extents.endY()+pad) >> numLevels));
  }

  inline khLevelCoverage MinifiedToLevel(uint targetLevel) const {
    assert(targetLevel <= level);
    return MinifiedBy(level - targetLevel);
  }
  inline khLevelCoverage& magnifyBy(uint numLevels) {
    level += numLevels;
    extents = khExtents<uint32>(XYOrder,
                                extents.beginX() << numLevels,
                                extents.endX()   << numLevels,
                                extents.beginY() << numLevels,
                                extents.endY()   << numLevels);
    return *this;
  }

  inline khLevelCoverage MagnifiedBy(uint numLevels) {
    return khLevelCoverage
      (level + numLevels,
        khExtents<uint32>(XYOrder,
                          extents.beginX() << numLevels,
                          extents.endX()   << numLevels,
                          extents.beginY() << numLevels,
                          extents.endY()   << numLevels));
  }

  inline khLevelCoverage& scaleToLevel(uint targetLevel) {
    if (level < targetLevel) {
      magnifyBy(targetLevel - level);
    } else if (level > targetLevel) {
      minifyBy(level - targetLevel);
    }
    return *this;
  }

  inline khLevelCoverage& expandBy(uint32 num) {
    extents.expandBy(num);
    return *this;
  }

  inline khLevelCoverage& cropTo(const khExtents<uint32> &cropExtents) {
    extents = khExtents<uint32>::Intersection(extents, cropExtents);
    return *this;
  }

  inline khLevelCoverage& cropToWorld(const khTilespace &tilespace) {
    return cropTo(tilespace.WorldExtents(level));
  }

  inline khExtents<double> normExtents(const khTilespaceBase& tilespace) const {
    const double normTileSize = tilespace.NormTileSize(level);
    const double north = extents.north() * normTileSize;
    const double south = extents.south() * normTileSize;
    const double east  = extents.east() * normTileSize;
    const double west  = extents.west() * normTileSize;
    return khExtents<double>(NSEWOrder, north, south, east, west);
  }

  inline khExtents<double> degExtents(const khTilespaceBase& tilespace) const {
    return khTilespaceBase::NormToDegExtents(normExtents(tilespace));
  }

  inline khExtents<double> meterExtents(
      const khTilespaceBase& tilespace) const {
    return khTilespaceBase::NormToMeterExtents(normExtents(tilespace));
  }

  khLevelCoverage GetSubset(uint subsetThis, uint subsetTotal) const;
  khLevelCoverage UpperCoverage(const khTilespace &tilespace) const;
  khLevelCoverage RightCoverage(const khTilespace &tilespace) const;
  khLevelCoverage UpperRightCoverage(const khTilespace &tilespace) const;
};


inline
khLevelCoverage
TranslateLevelCoverage(const khTilespaceBase& from, const khLevelCoverage& cov,
                       const khTilespaceBase& to) {
  int tileSizeLog2Diff = static_cast<int>(to.tileSizeLog2) -
      static_cast<int>(from.tileSizeLog2);

  // make a temporary that we can mess with
  khLevelCoverage tmp(cov);

  // minify the extents - tmp.level will be totally irrelevant
  if (tileSizeLog2Diff > 0) {
    // small -> large tile size
    tmp.minifyBy(tileSizeLog2Diff);
  } else if (tileSizeLog2Diff < 0) {
    // large -> small tile size
    tmp.magnifyBy(-tileSizeLog2Diff);
  } else {
    // same tile size
    // NoOp
  }

  return khLevelCoverage(TranslateTileLevel(from, cov.level, to),
                         tmp.extents);
}


// ****************************************************************************
// ***  khTileAddr - single tile addresses (no sub or src texture)
// ****************************************************************************
class khTileAddr {
 public:
  uint32 level;
  uint32 row;
  uint32 col;

 public:
  inline operator QuadtreePath(void) {
    return QuadtreePath(level, row, col);
  }

  inline bool operator==(const khTileAddr &o) const {
    return ((level == o.level) &&
            (row == o.row) &&
            (col == o.col));
  }

  inline explicit khTileAddr(uint64 id) :
      level(LEVFROMADDR(id)), row(ROWFROMADDR(id)), col(COLFROMADDR(id)) { }

  inline khTileAddr(uint32 level_, uint32 row_, uint32 col_) :
      level(level_), row(row_), col(col_) { }

  inline explicit khTileAddr(const QuadtreePath &qt_path) {
    qt_path.GetLevelRowCol(&level, &row, &col);
  }

  // row will be 0 if wrapped at world boundary
  khTileAddr UpperAddr(const khTilespace &tilespace) const;

  // col will be 0 if wrapped at world boundary
  khTileAddr RightAddr(const khTilespace &tilespace) const;

  // row and/or col will be 0 if wrapped at world boundary
  khTileAddr UpperRightAddr(const khTilespace &tilespace) const;

  inline uint64 Id(uint sub = 0, uint src = 0) const {
    return TILEADDR(level, row, col, sub, src);
  }

  inline void minifyBy(uint numLevels) {
    level -= numLevels;
    row >>= numLevels;
    col >>= numLevels;
  }

  inline khTileAddr MinifiedBy(uint numLevels) const {
    return khTileAddr(level - numLevels,
                      row >> numLevels,
                      col >> numLevels);
  }
  inline khTileAddr MinifiedToLevel(uint targetLevel) const {
    assert(targetLevel <= level);
    return MinifiedBy(level - targetLevel);
  }
  inline khLevelCoverage MagnifiedBy(uint numLevels) const {
    return khLevelCoverage(level + numLevels,
                           khExtents<uint32>(RowColOrder,
                                             row     << numLevels,
                                             (row+1) << numLevels,
                                             col     << numLevels,
                                             (col+1) << numLevels));
  }
  inline khLevelCoverage MagnifiedToLevel(uint targetLevel) const {
    assert(targetLevel >= level);
    return MagnifiedBy(targetLevel - level);
  }

  inline khExtents<double> normExtents(const khTilespaceBase& tilespace) const {
    const double normTileSize = tilespace.NormTileSize(level);
    const double row_begin = static_cast<double>(row) * normTileSize;
    const double row_end = static_cast<double>((row + 1)) * normTileSize;
    const double col_begin = static_cast<double>(col)  * normTileSize;
    const double col_end = static_cast<double>((col + 1)) * normTileSize;
    return khExtents<double>(RowColOrder,
                             row_begin, row_end, col_begin, col_end);
  }

  inline khExtents<double> degExtents(const khTilespaceBase& tilespace) const {
    return khTilespace::NormToDegExtents(normExtents(tilespace));
  }

  // children are numbered as follows:
  // 0=lower-left, 1=lower-right, 2=upper-left, 3=upper-right
  inline khTileAddr QuadChild(uint child) const {
    uint32 out_row = 0;
    uint32 out_col = 0;
    QuadtreePath::MagnifyQuadAddr(row, col, child, out_row, out_col);
    return khTileAddr(level + 1, out_row, out_col);
  }
};



// ****************************************************************************
// ***  PixelExtents conversion routines (extents in pixels, NOT tiles)
// ****************************************************************************
// Translate Pixel extents (number of _pixels_ in width & height) to tile
// extents (number of _tiles_ in width and height)
inline khExtents<uint32>
PixelExtentsToTileExtents(const khExtents<int64> &pixelExtents,
                          uint tileResolution) {
  return khExtents<uint32>(RowColOrder,
                           pixelExtents.beginRow() / tileResolution,
                           (pixelExtents.endRow() + tileResolution - 1)
                           / tileResolution,
                           pixelExtents.beginCol() / tileResolution,
                           (pixelExtents.endCol() + tileResolution - 1)
                           / tileResolution);
}

inline khExtents<int64>
TileExtentsToPixelExtents(const khExtents<uint32> &tileExtents,
                          uint tileResolution) {
  return khExtents<int64>(
      RowColOrder,
      static_cast<int64>(tileExtents.beginRow()) * tileResolution,
      static_cast<int64>(tileExtents.endRow()) * tileResolution,
      static_cast<int64>(tileExtents.beginCol()) * tileResolution,
      static_cast<int64>(tileExtents.endCol()) * tileResolution);
}

// Translate Degree extents to a pixel raster size (number of _pixels_ in
// width and height). This is NOT the same as making a khLevelCoverage from
// these extents. That would give you _tile_ extents.
inline khSize<uint64>
DegExtentsToPixelLevelRasterSize(const khExtents<double> &degExtents,
                                 uint lev) {
  double pixelsize = RasterProductTilespaceFlat.DegPixelSize(lev);
  return khSize<uint64>(
      static_cast<uint64>(degExtents.width()/pixelsize + 0.5),
      static_cast<uint64>(degExtents.height()/pixelsize + 0.5));
}

// Translate Degree extents to a pixel raster size (number of _pixels_ in
// width and height). This is NOT the same as making a khLevelCoverage from
// these extents. That would give you _tile_ extents.
inline khSize<uint64>
MeterExtentsToPixelLevelRasterSize(const khExtents<double> &meterExtents,
                                   uint lev) {
  double pixelsize =
      RasterProductTilespaceMercator.AveragePixelSizeInMercatorMeters(lev);
  return khSize<uint64>(
      static_cast<uint64>(meterExtents.width()/pixelsize + 0.5),
      static_cast<uint64>(meterExtents.height()/pixelsize + 0.5));
}

inline const khTilespace& RasterProductTilespace(const bool is_mercator) {
  if (is_mercator) {
    return RasterProductTilespaceMercator;
  }
  return RasterProductTilespaceFlat;
}

// Calculates Efficient Level Of Display (LOD) based on specified feature
// diameter, tilespace and desirable size (diameter_at_lod) in pixel
// at Efficient LOD.
// The formula assumes that
// - the resolution doubles with each additional level.
// So, efficient_lod is
// log2(desirable_size_at_lod_in_pixels/average_feature_diameter_in_pixels).
//
// @param feature_diameter diameter of feature in product coordinates.
// @param tilespace tilespace definitions.
// @param diameter_at_lod - desirable diameter in pixel at Efficient LOD.
int EfficientLOD(const double feature_diameter,
                 const khTilespace &tilespace,
                 const double diameter_at_lod = 1.0);


// ****************************************************************************
// ***  Older APIs
// ****************************************************************************
// Tilesize constants used in various places. These can't use the tilespace
// members because they need to be "translation unit compile time constants" so
// they can be used in template specializations.
// TODO: find a way to unify these definitions with those of the
// khTilespace constants. They'll probably never change, but having them in
// two places is just wrong!
static const uint ImageryQuadnodeResolution   = 256;
static const uint TmeshQuadnodeResolution     = 32;
static const uint HeightmapTileSize           = 1024;
static const uint RasterProductTileResolution = 1024;
// The best resolution Fusion can handle is 0.0186 m (18.6 mm).
static const float kWidthAtMaxResolution       = 0.0186;


// Really old (not quite ancient) style macros - define them now to be the
// constants above. This results in the same values as before, but now
// they'll have a type.
#define NUM_LEVELS NumFusionLevels
#define MAX_CLIENT_LEVEL MaxClientLevel

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_KHTILEADDR_H_
