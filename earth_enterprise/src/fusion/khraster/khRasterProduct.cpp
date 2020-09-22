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


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <khProgressMeter.h>
#include <khGuard.h>
#include <khException.h>
#include <notify.h>
#include <khFileUtils.h>
#include <khCalcHelper.h>
#include <compressor.h>
#include <khInsetCoverage.h>
#include <autoingest/plugins/RasterProductAsset.h>
#include <autoingest/plugins/MercatorRasterProductAsset.h>

#include "khRasterProduct.h"
#include "pyrio.h"
#include "MinifyTile.h"


/******************************************************************************
 ***  khRasterProduct
 ******************************************************************************/
khRasterProduct*
khRasterProduct::New(RasterType type,
                     khTypes::StorageEnum componentType,
                     const std::string &filename,
                     khTilespace::ProjectionType projectionType,
                     unsigned int minLevel,
                     unsigned int maxLevel,
                     const khExtents<double> &degOrMeterExtents)
{
  assert(minLevel <= maxLevel);

  if (!khTilespace::IsExtentsWithinWorldBoundary(degOrMeterExtents,
                                                 projectionType)) {
    notify(NFY_WARN,
           "Invalid extents for raster product: nsew %f,%f,%f,%f",
           degOrMeterExtents.north(), degOrMeterExtents.south(),
           degOrMeterExtents.east(), degOrMeterExtents.west());
    return 0;
  }

  // try to blow away any existing directory & create a clean new one
  // this will fail if the user has insufficient permissions or if the
  // filename exists but is not a directory
  if (khMakeCleanDir(filename)) {
    // TODO: Once pyramidio is fixed to not make assumtions
    // about the combinations below, modify this routine
    // to let the user specify whatever they want
    Compression compression = JPEG;  /* value to keep compiler from whining */
    unsigned int compressionQuality = 0;
    unsigned int numComponents = 1;
    std::vector<CompressDef> compdefs;
    switch (type) {
      case Imagery:
        compression        = JPEG; // not really used, but fill for header
        compressionQuality = 85;   // not really used, but fill for header
        for (unsigned int lev = minLevel; lev <= maxLevel; ++lev) {
          // bottom 4 levels are JPEG:85, rest are LZ
          if (maxLevel - lev < 4) {
            compdefs.push_back(CompressDef(CompressJPEG, 85));
          } else {
            compdefs.push_back(CompressDef(CompressLZ, 5));
          }
        }
        numComponents      = 3;
        if (componentType != khTypes::UInt8) {
          notify(NFY_WARN, "Unsupported imagery component type: %s",
                 khTypes::StorageName(componentType));
          return 0;
        }
        break;
      case Heightmap:
        compression        = LZ; // not really used, but fill for header
        compressionQuality = 5;  // not really used, but fill for header
        for (unsigned int lev = minLevel; lev <= maxLevel; ++lev) {
          compdefs.push_back(CompressDef(CompressLZ, 5));
        }
        numComponents      = 1;
        if ((componentType != khTypes::Int16) &&
            (componentType != khTypes::Float32)) {
          notify(NFY_WARN, "Unsupported heightmap component type: %s",
                 khTypes::StorageName(componentType));
          return 0;
        }
        break;
      case AlphaMask:
        compression        = LZ; // not really used, but fill for header
        compressionQuality = 5;  // not really used, but fill for header
        for (unsigned int lev = minLevel; lev <= maxLevel; ++lev) {
          compdefs.push_back(CompressDef(CompressLZ, 5));
        }
        numComponents      = 1;
        if (componentType != khTypes::UInt8) {
          notify(NFY_WARN, "Unsupported alpha mask component type: %s",
                 khTypes::StorageName(componentType));
          return 0;
        }
        break;
    }

    const RasterProductStorage::ProjectionType projection =
        projectionType == khTilespace::MERCATOR_PROJECTION ?
        RasterProductStorage::Mercator :
        RasterProductStorage::Flat;
    RasterProductStorage base(type, compression, compressionQuality,
                              numComponents, componentType,
                              RasterProductTileResolution,
                              minLevel, maxLevel,
                              degOrMeterExtents,
                              projection);
    if (!base.Save(HeaderName(filename))) {
      return 0;
    } else {
      try {
        return new khRasterProduct(filename, base,
                                   true /* isnew */,
                                   &compdefs,
                                   true /* noOpacityOK */);
      } catch (const std::exception &e) {
        notify( NFY_WARN, "%s", e.what());
      } catch (...) {
        notify( NFY_WARN, "Unable to create %s: Unknown error",
                filename.c_str());
      }
    }
  }
  return 0;
}

khTransferGuard<khRasterProduct>
khRasterProduct::Open(const std::string &filename, bool noOpacityOK)
{
  RasterProductStorage base;
  if (khExists(HeaderName(filename)) && base.Load(HeaderName(filename))) {
    try {
      return TransferOwnership(new khRasterProduct(filename, base,
                                                   false /* isnew */,
                                                   0 /* compdefs */,
                                                   noOpacityOK));
    } catch (const std::exception &e) {
      notify( NFY_WARN, "%s", e.what());
    } catch (...) {
      notify( NFY_WARN, "Unable to open %s: Unknown error",
              filename.c_str());
    }
  }
  return khTransferGuard<khRasterProduct>();
}

std::string khRasterProduct::GetAcquisitionDate() const {
  // Need to get the acquisition date from the root imagery asset.
  // This information is not easily passed down, but we can easily recover the
  // imagery resource path from the raster.kip path.
  // filename_ is an imagery asset pathname of the form
  // /gevol/assets/myimagery.kiasset/product.kia/ver001/raster.kip"
  std::string date("");
  std::string asset_name(filename_);
  size_t pos = asset_name.rfind(kImageryAssetSuffix);
  if (pos != asset_name.npos) {
    pos += kImageryAssetSuffix.size();
    asset_name = asset_name.substr(0, pos);
    RasterProductAsset resource(asset_name);
    date = resource->GetAcquisitionDate();
  } else if ((pos = asset_name.rfind(kMercatorImageryAssetSuffix))
             != asset_name.npos) {
    pos += kMercatorImageryAssetSuffix.size();
    asset_name = asset_name.substr(0, pos);
    MercatorRasterProductAsset resource(asset_name);
    date = resource->GetAcquisitionDate();
  }

  return date;
}

khTransferGuard<khRasterProduct>
khRasterProduct::Convert(const std::string &pyrname,
                         bool reminify,
                         const std::string &outdir,
                         bool copy, bool link,
                         bool skipOpacity)
{
  std::string pyrdirname  = khDirname(pyrname);
  std::string pyrdirext   = khExtension(pyrdirname);
  bool isProductPyr = ((pyrdirext == ".kip") ||
                       (pyrdirext == ".ktp") ||
                       (pyrdirext == ".kmp"));

  // split the pyramid name
  std::string basename;
  std::string suffix;
  if (!pyrio::SplitPyramidName(pyrname, basename, suffix)) {
    notify(NFY_WARN, "Poorly formatted pyramid name %s",
           pyrname.c_str());
    return khTransferGuard<khRasterProduct>();
  }

  RasterProductStorage storage;
  if (!pyrio::FindPyramidMinMax(basename, suffix, storage.minLevel_, storage.maxLevel_)) {
    notify(NFY_WARN, "No pyrmid files found that match %s",
           pyrname.c_str());
    return khTransferGuard<khRasterProduct>();
  }
  notify(NFY_DEBUG, "max level = %d", storage.maxLevel_);

  // get some info from the top level
  std::string topfile = pyrio::ComposePyramidName(basename, suffix, storage.maxLevel_);
  unsigned int formatVersion = 0;
  try {
    pyrio::Reader reader(topfile);
    const pyrio::Header &header = reader.header();
    storage.numComponents_ = header.numComponents;
    storage.componentType_ = header.componentType;
    storage.degExtents_ = header.dataExtents;
    formatVersion = header.formatVersion;
    storage.type_ = header.rasterType;

    notify(NFY_DEBUG, "num components = %d",
           storage.numComponents_);
    notify(NFY_DEBUG, "component type = %s",
           khTypes::StorageName(storage.componentType_));
    notify(NFY_DEBUG, "NSEW: %f %f %f %f",
           storage.degExtents_.north(),
           storage.degExtents_.south(),
           storage.degExtents_.east(),
           storage.degExtents_.west());
    notify(NFY_DEBUG, "format version = %u", formatVersion);
  } catch (const std::exception &e) {
    notify( NFY_WARN, "%s", e.what());
    return khTransferGuard<khRasterProduct>();
  } catch (...) {
    notify( NFY_WARN, "Unable to read %s: Unknown error",
            topfile.c_str());
    return khTransferGuard<khRasterProduct>();
  }

  std::string dirext;
  switch (storage.type_) {
    case AlphaMask:
      dirext = ".kmp";
      break;
    case Imagery:
      dirext = ".kip";
      break;
    case Heightmap:
      dirext = ".ktp";
      break;
  }

  std::string filename;
  if (outdir.size()) {
    if (khExtension(outdir) != dirext) {
      notify(NFY_WARN, "%s doesn't end in %s",
             outdir.c_str(), dirext.c_str());
      return khTransferGuard<khRasterProduct>();
    }
    filename = outdir;
  } else {
    if (isProductPyr) {
      notify(NFY_WARN,
             "You must specify 'outdir' to convert product pyramids");
      return khTransferGuard<khRasterProduct>();
    }
    filename = basename + dirext;
  }

  // trim off any extra minification that we don't need
  unsigned int startLevel = 0;
  switch (storage.type_) {
    case Imagery:
    case Heightmap:
      startLevel = DefaultStartLevel(storage.type_);
      break;
    case AlphaMask:
      startLevel = 8;
      break;
  }
  storage.minLevel_ = std::max(storage.minLevel_, startLevel);

  return DoConvert(storage, filename, basename, suffix, startLevel, reminify, copy, link, skipOpacity);
}

khTransferGuard<khRasterProduct>
khRasterProduct::DoConvert(const RasterProductStorage& storage,
                           const std::string &filename,
                           const std::string &basename,
                           const std::string &suffix,
                           unsigned int startLevel, bool reminify,
                           bool copy, bool link, bool skipOpacity)
{
  int maxLevel = (int)storage.maxLevel_;
  int minLevel = reminify ? maxLevel : (int)storage.minLevel_;

  try {
    // try to make the new rasterproduct
    const khTilespace::ProjectionType projection =
        storage.projectionType_ == RasterProductStorage::Mercator ?
        khTilespace::MERCATOR_PROJECTION :
        khTilespace::FLAT_PROJECTION;
    khRasterProduct *tmp = New(storage.type_, storage.componentType_,
                               filename,
                               projection,
                               startLevel, storage.maxLevel_,
                               storage.degExtents_);
    khDeleteGuard<khRasterProduct> deleter(TransferOwnership(tmp));
    if (tmp) {
      std::string newbasename = filename + "/level";
      std::string newsuffix  = ".pyr";
      int lev;
      // rename the old pyr files for the levels we want to keep
      for (lev = maxLevel; lev >= minLevel; --lev) {
        std::string oldpyr =
          pyrio::ComposePyramidName(basename, suffix, lev);
        std::string newpyr =
          pyrio::ComposePyramidName(newbasename, newsuffix, lev);
        if (copy) {
          notify(NFY_NOTICE, "Copying %s ...", oldpyr.c_str());
          if (!khLinkOrCopyFile(oldpyr.c_str(), newpyr.c_str())) {
            if (lev < maxLevel)
              notify(NFY_WARN, "Some files already renamed!");
            return khTransferGuard<khRasterProduct>();
          }
        } else if (link) {
          notify(NFY_NOTICE, "Linking %s ...", oldpyr.c_str());
          if (!khLink(oldpyr.c_str(), newpyr.c_str())) {
            if (lev < maxLevel)
              notify(NFY_WARN, "Some files already renamed!");
            return khTransferGuard<khRasterProduct>();
          }
        } else {
          if (!khRename(oldpyr.c_str(), newpyr.c_str())) {
            if (lev < maxLevel)
              notify(NFY_WARN, "Some files already renamed!");
            return khTransferGuard<khRasterProduct>();
          }
        }
        if ((storage.type_ == AlphaMask) && !skipOpacity) {
          notify(NFY_NOTICE, "Computing opacity %s ...",
                 newpyr.c_str());
          tmp->ReadLevelAndGenerateOpacityMask(tmp->level(lev));
        }
      }

      // make any new levels that we need to
      if (lev >= (int)startLevel) {
        notify(NFY_NOTICE, "Re-minifying ...");
        switch (tmp->type()) {
          case khRasterProduct::Imagery:
            tmp->MinifyRemainingLevels<ImageryProductTile>(lev);
            break;
          case khRasterProduct::Heightmap:
            switch (tmp->componentType()) {
              case khTypes::Int16:
                tmp->MinifyRemainingLevels<HeightmapInt16ProductTile>
                  (lev);
                break;
              case khTypes::Float32:
                tmp->MinifyRemainingLevels<HeightmapFloat32ProductTile>
                  (lev);
                break;
              default:
                throw khException
                  (kh::tr
                   ("Unsupported heightmap product pixel type: %1")
                   .arg(khTypes::StorageName(tmp->componentType())));
            }
            break;
          case khRasterProduct::AlphaMask:
            tmp->MinifyRemainingLevels<AlphaProductTile>(lev);
            break;
        }
      }
    }
    return TransferOwnership(deleter);
  } catch (const std::exception &e) {
    notify(NFY_WARN, "%s", e.what());
  } catch (...) {
    notify(NFY_WARN, "Unknown error converting pyramids");
  }
  return khTransferGuard<khRasterProduct>();
}


uint
khRasterProduct::DefaultStartLevel(RasterType type)
{
  switch (type) {
    case Imagery:
      return ImageryToProductLevel(StartImageryLevel);
    case Heightmap:
      return TmeshToProductLevel(StartTmeshLevel);
    default:
      assert(0);
  }
  return 0;
}



template <class TileType, class Averager>
void
khRasterProduct::MakeTileFromLevel(std::uint32_t destRow, std::uint32_t destCol,
                                   khRasterProduct::Level &srcLevel,
                                   TileType &tmpTile,
                                   TileType &destTile,
                                   Averager averager)
{
  static_assert(TileType::TileWidth == RasterProductTileResolution,
                      "Invalid Tile Width");
  static_assert(TileType::TileHeight == RasterProductTileResolution,
                      "Invalid Tile Height");
  assert(TileType::NumComp == numComponents());
  assert(TileType::Storage == componentType());

  // for each of the four source tiles / quadrants
  for (unsigned int quad = 0; quad < 4; ++quad) {
    // magnify the dest row/col/quad to get a src row/col
    std::uint32_t srcRow = 0;
    std::uint32_t srcCol = 0;
    QuadtreePath::MagnifyQuadAddr(destRow, destCol, quad, srcRow, srcCol);

    // read high res tile to temporary buffers
    if (srcLevel.tileExtents().ContainsRowCol(srcRow, srcCol)) {
      if (!srcLevel.ReadTile(srcRow, srcCol, tmpTile)) {
        throw
          khException(kh::tr
                      ("Unable to read tile lev %1, row %2, col %3")
                      .arg(srcLevel.levelnum()).arg(srcRow)
                      .arg(srcCol));
      }

      MinifyTile(destTile, quad, tmpTile, averager);
    } else {
      // no source tile, fill this quad with 0 in all bands
      // Imagery:   0 -> black
      // Heightmap: 0 -> sea level
      // AlphaMask: 0 -> transparent
      destTile.FillQuadWithZeros(quad);
    }
  }
}


template <class TileType>
void
khRasterProduct::MinifyLevel(unsigned int newLevel, khProgressMeter &progress)
{

  typedef typename TileType::PixelType PixelType;

  Level &srcLevel(level(newLevel+1));
  if (!srcLevel.OpenReader()) {
    throw khException(kh::tr("Unable to open level %1 for reading")
                      .arg(newLevel+1));
  }
  khMemFunCallGuard<Level> srcCloser(&srcLevel, &Level::CloseReader);

  Level &destLevel(level(newLevel));
  khMemFunCallGuard<Level> destCloser(&destLevel, &Level::CloseWriter);


  notify(NFY_NOTICE, "Reducing level %d -> %d", newLevel+1, newLevel);


  // Pre-allocate tiles so I don't have do a bunch of mallocs/frees
  // inside the loop. Even though there are 4 src tiles for each dest tile,
  // they are only processed one at a time, so we only need one srcTile
  TileType srcTile(srcLevel.IsMonoChromatic());
  TileType destTile(srcLevel.IsMonoChromatic());

  khExtents<std::uint32_t> destExtents(destLevel.tileExtents());
  notify(NFY_DEBUG, "About to minify %u x %u tiles",
         destExtents.numRows(), destExtents.numCols());
  for (std::uint32_t row = destExtents.beginRow();
       row < destExtents.endRow(); row++) {
    for (std::uint32_t col = destExtents.beginCol();
         col < destExtents.endCol(); col++) {
      if (type() == AlphaMask) {
          // ALPHA ERODING
          // We use the ZeroOrAverage function to erode the alpha
          // mask. If any of the source pixels is 0 (transparent) then
          // the result will be 0 (transparent).  Eroding the alpha
          // like this will make sure that when we get around to
          // blending, we won't be including any source data pixels
          // that could have been damaged during their own
          // minification.  This has to happen in conjuntion with
          // "bleading out" during the blend phase or you'd get
          // transparent gaps between sibling assets of an MSA.
          MakeTileFromLevel(row, col,
                            srcLevel,
                            srcTile, destTile,
                            &khCalcHelper<PixelType>::ZeroOrAverage);
      } else {
        // Do normal/simple minification
        MakeTileFromLevel(row, col,
                          srcLevel,
                          srcTile, destTile,
                          &khCalcHelper<PixelType>::AverageOf4);
      }
      if (!destLevel.WriteTile(row, col, destTile)) {
        throw
          khException(kh::tr
                      ("Unable to write tile lev %1, row %2, col %3")
                      .arg(newLevel).arg(row).arg(col));
      }

      progress.increment();
    }
  }
}

template <class TileType>
void
khRasterProduct::MinifyRemainingLevels(unsigned int firstMinifyLevel,
                                       khProgressMeter *progress)
{
  khDeleteGuard<khProgressMeter> deleter;
  if (!progress) {
    deleter = TransferOwnership(
        progress =
        new khProgressMeter(CalcNumTiles(minLevel(), firstMinifyLevel+1)));
  }
  for (int lev = firstMinifyLevel; lev >= (int)minLevel(); --lev) {
    MinifyLevel<TileType>(lev, *progress);
  }
}

// ***** explicit instantiations since this is in the cpp file *****
template void
khRasterProduct::MinifyRemainingLevels<AlphaProductTile>
(unsigned int firstMinifyLevel, khProgressMeter *progress);
template void
khRasterProduct::MinifyRemainingLevels<ImageryProductTile>
(unsigned int firstMinifyLevel, khProgressMeter *progress);
template void
khRasterProduct::MinifyRemainingLevels<HeightmapInt16ProductTile>
(unsigned int firstMinifyLevel, khProgressMeter *progress);
template void
khRasterProduct::MinifyRemainingLevels<HeightmapFloat32ProductTile>
(unsigned int firstMinifyLevel, khProgressMeter *progress);


std::string
khRasterProduct::LevelName(unsigned int levelNum) const
{
  char buf[256];
  snprintf(buf, 256, "%2.2d.pyr", levelNum);
  return filename_ + "/level" + buf;
}


std::string
khRasterProduct::HeaderName(const std::string &filename)
{
  return filename + "/" + kHeaderXmlFile;
}

std::string
khRasterProduct::OpacityMaskName(void) const
{
  return filename_ + "/opacity";
}


khRasterProduct::khRasterProduct(const std::string &filename,
                               const RasterProductStorage &base,
                               bool isnew,
                               const std::vector<CompressDef> *compdefs,
                               bool noOpacityOK)
: RasterProductStorage(base),
filename_(filename),
wroteSomeTiles(false)
{
  if (!khTilespace::IsExtentsWithinWorldBoundary(degOrMeterExtents(),
                                                 projectionType())) {
    throw khException
      (kh::tr("%1 has invalid extents: (nsew) %2,%3,%4,%5")
       .arg(filename.c_str())
       .arg(degOrMeterExtents().north())
       .arg(degOrMeterExtents().south())
       .arg(degOrMeterExtents().east())
       .arg(degOrMeterExtents().west()));
  }

  khInsetCoverage coverage(RasterProductTilespace(IsMercator()),
                           degOrMeterExtents(),
                           realMaxLevel()   /* fullresLevel */,
                           realMinLevel()   /* beginLevel */,
                           realMaxLevel()+1 /* endLevel */);


  unsigned int numLevels = realMaxLevel() - realMinLevel() + 1;
  levels__.reserve(numLevels);
  for (unsigned int i = 0; i < numLevels; ++i) {
    levels__.push_back(new Level(this,
                            coverage.levelCoverage(i + realMinLevel()),
                            compdefs ? (*compdefs)[i] : CompressDef()));
  }

  if (type() == AlphaMask) {
    if (!isnew) {
      if (khExists(OpacityMaskName())) {
        try {
          opacityMask_ = TransferOwnership(
              new khOpacityMask(OpacityMaskName()));
        } catch (const std::exception &e) {
          notify(NFY_WARN, "%s", e.what());
        } catch (...) {
          notify(NFY_WARN, "Unknown error loading %s",
                 OpacityMaskName().c_str());
        }
        if (!opacityMask_ && !noOpacityOK) {
          throw khException
            (kh::tr("%1 has a broken opacity mask.\n"
                    "Rebuild resouce from scratch to repair.")
             .arg(filename.c_str()));
        }
      } else if (!noOpacityOK) {
        throw khException
          (kh::tr("%1 is missing an opacity mask.\n"
                  "Rebuild resource from scratch to repair.")
           .arg(filename.c_str()));
      }
    }
    if (!opacityMask_) {
      // We made it here w/o one, let's make it now
      opacityMask_ = TransferOwnership(new khOpacityMask(coverage));
    }
  }
}

khRasterProduct::~khRasterProduct(void)
{
  if (wroteSomeTiles) {
    if (type() == AlphaMask) {
      try {
        opacityMask_->Save(OpacityMaskName());
      } catch (const std::exception &e) {
        notify(NFY_WARN, "%s", e.what());
      } catch (...) {
        notify(NFY_WARN, "Unknown error saving %s",
               OpacityMaskName().c_str());
      }
    } else {
      // other types don't need to do anything here
    }
  }
}

uint
khRasterProduct::effectiveMinLevel(void) const
{
  switch (type_) {
    case Imagery:
    case Heightmap:
      return std::max(realMinLevel(),DefaultStartLevel(type_));
    case AlphaMask:
      return realMinLevel();
      break;
  }
  return realMinLevel();      // unreached but silences warnings
}

std::uint64_t
khRasterProduct::CalcNumTiles(unsigned int beginLevel, unsigned int endLevel) const
{
  std::uint64_t numTiles = 0;
  for (unsigned int lev = beginLevel; lev < endLevel; ++lev) {
    numTiles += level(lev).tileExtents().numRows() *
                level(lev).tileExtents().numCols();
  }
  return numTiles;
}


void
khRasterProduct::ReadLevelAndGenerateOpacityMask(khRasterProductLevel &level,
                                                 khProgressMeter *progress)
{
  assert(type() == AlphaMask);
  const khExtents<std::uint32_t> &extents(level.tileExtents());
  for (unsigned int row = extents.beginRow(); row < extents.endRow(); ++row) {
    for (unsigned int col = extents.beginCol(); col < extents.endCol(); ++col) {
      AlphaProductTile tile(level.IsMonoChromatic());
      if (!level.ReadTile(row, col, tile)) {
        notify(NFY_FATAL, "Unable to read tile");
      }
      opacityMask_->SetOpacity(khTileAddr(level.levelnum(), row, col),
                               ComputeOpacity(tile));
      if (progress) progress->increment();
      wroteSomeTiles = true;
    }
  }
}


void
khRasterProduct::GenerateOpacityMask(void)
{
  assert(type() == AlphaMask);
  khInsetCoverage coverage(RasterProductTilespace(IsMercator()),
                           degOrMeterExtents(),
                           realMaxLevel()   /* fullresLevel */,
                           realMinLevel()   /* beginLevel */,
                           realMaxLevel()+1 /* endLevel */);
  opacityMask_ = TransferOwnership(new khOpacityMask(coverage));

  khProgressMeter progress(CalcNumTiles());

  for (khDeletingVector<Level>::const_iterator level = levels__.begin();
       level != levels__.end(); ++level) {
    ReadLevelAndGenerateOpacityMask(*(*level), &progress);
  }
  try {
    opacityMask_->Save(OpacityMaskName());
  } catch (const std::exception &e) {
    notify(NFY_WARN, "%s", e.what());
  } catch (...) {
    notify(NFY_WARN, "Unknown error saving %s",
           OpacityMaskName().c_str());
  }
}
