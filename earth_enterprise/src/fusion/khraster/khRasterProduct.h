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


#ifndef _khRasterProduct_h_
#define _khRasterProduct_h_

#include <vector>
#include <math.h>
#include <khExtents.h>
#include <khTileAddr.h>
#include "khRasterTile.h"
#include <khraster/.idl/RasterProductStorage.h>
#include "khRasterProductLevel.h"
#include "khOpacityMask.h"

class khProgressMeter;

/******************************************************************************
 ***  Encapsulation around a collection of pyramid files
 ***
 ***  This replaces the old pyramidio API. Please see detailed comment blocks
 ***  before the khRasterProduct and khRasterProduct::Level definitions later
 ***  in this file.
 ***
 ***  Note: all input georefs (n,s,e,w) are in degrees (-180->180, -90->90)
 ***        output functions are prefixed with 'deg' or 'norm'
 ******************************************************************************/

/******************************************************************************
 ***  khRasterProduct - The public interface to Keyhole Raster Product Files
 ***
 ***  The disk memory of this object is an XML record named
 ***  <filename>/header.xml. All references to the raster product are via the
 ***  containing directory name. This is usually named .kip, .ktp, or .kmp
 ***  (for imagery, terrain and alphamask raster products).
 ***
 ***  The constructors are all private. This forces you to create
 ***  khRasterProduct objects via the static functions khRasterProduct::Open
 ***  and khRasterProduct::New. This was done since the creation of the
 ***  object requires file IO (either reading or writing), which can fail. If
 ***  the IO were done in the constructor, a failure would still require me
 ***  to return a valid c++ object. But that c++ object would be in an
 ***  invalid state. My preference would have been to throw an exception, but
 ***  the rest of the system isn't ready for exceptions just yet.
 ***
 ***  Usage:
 ***    khRasterProduct *rptr = khRasterProduct::Open(filename);
 ***    if (rptr) {
 ***      khRasterProduct &rp(*rptr);
 ***      notify(NFY_DEBUG, "minLevel = %d, maxLevel = %d",
 ***             rp.minLevel(), rp.maxLevel());
 ***      for (unsigned int i = rp.minLevel(); i <= rp.maxLevel(); ++i) {
 ***        notify(NFY_DEBUG, "Level %d has %d x %d tiles",
 ***               i, rp[i].xNumTiles(), rp[i].yNumTiles());
 ***      }
 ***      delete rptr;
 ***    }
 ***
 ***    - or -
 ***
 ***    ... TODO: Move to raster product writer ...;
 ***    khRasterProduct *rp =
 ***       khRasterProduct::Open(filename, khRasterType::Imagery, 8, 25,
 ***                             khExtents<double>(NSEWOrder, north, souuth, east, west));
 ***    if (rp) {
 ***      for (unsigned int i = rp->minLevel(); i <= rp->maxLevel(); ++i) {
 ***        khRasterProduct::Level &level(rp->level(i));
 ***        level.OpenReader();
 ***        for (...) {
 ***          level.WriteTile(row, col, bufs);
 ***        }
 ***        level.CloseReader();
 ***      }
 ***      delete rp;
 ***    }
 ***
 ******************************************************************************/
class khRasterProduct : private RasterProductStorage
{
  friend class khRasterProductLevel;
 public:
  // for convenience and backwards compatibility
  typedef khRasterProductLevel Level;

  // change the protection on these symbols from my baseclass
  // enum RasterType { Imagery, Heightmap, AlphaMask };
  using RasterProductStorage::RasterType;
  using RasterProductStorage::Imagery;
  using RasterProductStorage::Heightmap;
  using RasterProductStorage::AlphaMask;

  // ***** Factory Functions *****
  static khRasterProduct* New(RasterType type,
                              khTypes::StorageEnum componentType,
                              const std::string &filename,
                              khTilespace::ProjectionType projectionType,
                              unsigned int minLevel,
                              unsigned int maxLevel,
                              const khExtents<double> &degOrMeterExtents);

  static khTransferGuard<khRasterProduct>Open(const std::string &filename,
                                              bool noOpacityOK = false);

  // if outdir is not specified, it is built from pyrname
  // copy takes precedence over link
  static khTransferGuard<khRasterProduct> Convert(const std::string &pyrname,
                                                  bool reminify,
                                                  const std::string &outdir,
                                                  bool copy, bool link,
                                                  bool skipOpacity);

  // Low level conversion routine. Used by upgrade tool.
  static khTransferGuard<khRasterProduct> DoConvert(
      const RasterProductStorage &storage,
      const std::string &filename,
      const std::string &basename,
      const std::string &suffix,
      unsigned int startLevel, bool reminify,
      bool copy, bool link,
      bool skipOpacity);

  // Base class accessor
  const RasterProductStorage& GetRasterProductStorage() const {
    return (const RasterProductStorage&)*this;
  }

  static unsigned int DefaultStartLevel(RasterType type);

  template <class TileType>
  void MinifyRemainingLevels(unsigned int firstMinifyLevel,
                             khProgressMeter *progress = 0);
  template <class TileType>
  void MinifyLevel(unsigned int newLevel, khProgressMeter &progress);

  template <class TileType, class Averager>
  void MakeTileFromLevel(std::uint32_t destRow, std::uint32_t destCol,
                         Level &srcLevel,
                         TileType &srcTile,
                         TileType &destTile,
                         Averager averager);

  // Run on an existing AlphaMask product to generate opacity masks
  void GenerateOpacityMask(void);
  void ReadLevelAndGenerateOpacityMask(khRasterProductLevel &level,
                                       khProgressMeter *progress = 0);


  ~khRasterProduct(void);


  // ***** Accessor Functions *****
  inline const std::string &name(void) const { return filename_; }
  inline RasterType type(void) const { return type_; }
  inline unsigned int numComponents(void) const { return numComponents_; }
  inline khTypes::StorageEnum componentType(void) const { return componentType_; }
  inline khTilespace::ProjectionType projectionType(void) const {
    if (projectionType_ == RasterProductStorage::Flat) {
      return khTilespace::FLAT_PROJECTION;
    } else {
      assert(projectionType_ == RasterProductStorage::Mercator);
    }
    return khTilespace::MERCATOR_PROJECTION;
  }
  bool IsMercator() const {
    return projectionType_ == RasterProductStorage::Mercator;
  }
  inline unsigned int componentSize(void) const { return khTypes::StorageSize(componentType_); }
  inline unsigned int realMinLevel(void) const { return minLevel_; }
  inline unsigned int realMaxLevel(void) const { return maxLevel_; }

  // old heightmap products had too many levels of minification
  // these routines were added to ease the burdon on the rest of
  // the code
  unsigned int effectiveMinLevel(void) const;
  inline unsigned int effectiveMaxLevel(void) const { return realMaxLevel(); }
  inline unsigned int minLevel(void) const { return effectiveMinLevel(); }
  inline unsigned int maxLevel(void) const { return effectiveMaxLevel(); }
  inline double degOrMeterNorth(void) const
      { return degExtents_.north(); }
  inline double degOrMeterSouth(void) const
      { return degExtents_.south(); }
  inline double degOrMeterEast(void)  const
      { return degExtents_.east(); }
  inline double degOrMeterWest(void)  const
      { return degExtents_.west(); }
  inline const khExtents<double>& degOrMeterExtents(void) const
      {return degExtents_;}

  inline khLevelCoverage levelCoverage(unsigned int level) const {
    return khLevelCoverage(RasterProductTilespace(IsMercator()),
        degOrMeterExtents(), realMaxLevel(), level /* targetlevel */);
  }

  std::uint64_t CalcNumTiles(unsigned int beginLevel, unsigned int endLevel) const;
  inline std::uint64_t CalcNumTiles(void) const {
    return CalcNumTiles(minLevel(), maxLevel()+1);
  }


  // No runtime range checking!
  inline const Level& level(unsigned int lev) const {
    assert(validLevel(lev));
    return *levels__[lev-minLevel_];
  }
  inline Level& level(unsigned int lev) {
    assert(validLevel(lev));
    return *levels__[lev-minLevel_];
  }
  inline const Level& operator[](unsigned int lev) const { return level(lev);}
  inline Level& operator[](unsigned int lev) { return level(lev);}

  inline bool validLevel(unsigned int lev) const {
    return ((lev >= minLevel()) && (lev <= maxLevel()));
  }

  // Will only be non-0 for AlphaMask raster products
  inline const khOpacityMask* opacityMask(void) const {
    return &*opacityMask_;
  }
  // Return the Acquisition date of the inset if any.
  // Return empty string if not found.
  std::string GetAcquisitionDate() const;

 private:
  std::string filename_;
  khDeletingVector<Level> levels__;
  khDeleteGuard<khOpacityMask> opacityMask_;
  bool wroteSomeTiles;

  // contsructor private, must be built from static members Open &
  // New. Those routines return false if they are unable to create or
  // sucessfully open the header file.  If we were to try to do the
  // open/create from the constructor, a failure would result in a valid
  // c++ object that has an undefined internal state ... not good.
  khRasterProduct(const std::string &filename,
                  const RasterProductStorage &o,
                  bool isnew,
                  const std::vector<CompressDef> *compdefs,
                  bool noOpacityOK);

  // disable copy construction & assignment
  // it simplifies ownership logic
  khRasterProduct(const khRasterProduct &);
  khRasterProduct &operator=(const khRasterProduct &);

  std::string LevelName(unsigned int levelNum) const;
  static std::string HeaderName(const std::string &filename);
  std::string OpacityMaskName(void) const;
};


#endif // !_khRasterProduct_h_
