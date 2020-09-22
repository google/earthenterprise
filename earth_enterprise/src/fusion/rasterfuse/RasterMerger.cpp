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


/******************************************************************************
File:        RasterMerger.cpp
Description:

******************************************************************************/
#include "fusion/rasterfuse/RasterMerger.h"

#include <climits>

#include "common/khProgressMeter.h"
#include "common/notify.h"
#include "common/khException.h"
#include "common/khFileUtils.h"
#include "fusion/khraster/khRasterProduct.h"
#include "fusion/khraster/khRasterTile.h"
#include "fusion/khraster/ffioRasterWriter.h"
#include "fusion/khraster/MinifyTile.h"
#include "fusion/khraster/khOpacityMask.h"
#include "fusion/khraster/CachedRasterInsetReaderImpl.h"


namespace fusion {
namespace rasterfuse {

// ****************************************************************************
// ***  MergeInset
// ****************************************************************************
template <class DataTile>
MergeInset<DataTile>::MergeInset(unsigned int magnify_level,
                                 const std::string &dataRPFile,
                                 const std::string &alphaRPFile,
                                 const std::string &cached_blend_file,
                                 const std::string &cached_blend_alpha_file)
    : dataRP(),
      magnifyCoverage(),
      alphaRP(),
      dataLevel(0),
      alphaLevel(0),
      cached_blend_reader(),
      cached_blend_alpha_reader() {

  InitRasterProducts(magnify_level, dataRPFile, alphaRPFile);
  InitCachedBlendReaders(magnify_level, dataRPFile, cached_blend_file,
                         cached_blend_alpha_file);
}


template<class DataTile>
void MergeInset<DataTile>::InitRasterProducts(unsigned int magnify_level,
                                             const std::string &dataRPFile,
                                             const std::string &alphaRPFile) {
  if (dataRPFile.size()) {
    dataRP = khRasterProduct::Open(dataRPFile);
    if (!dataRP) {
      throw khException(kh::tr("Unable to open %1").arg(dataRPFile.c_str()));
    }
    if ((dataRP->type() != khRasterProduct::Imagery) &&
        (dataRP->type() != khRasterProduct::Heightmap)) {
      throw khException
        (kh::tr("Data product is neither Imagery nor Heightmap: %1")
         .arg(dataRPFile.c_str()));
    }
    // Terrain RPs that are upgraded from Fusion <3.1 might have RPs that are
    // generated only to level 8. Fusion 3.1 and later will upgrade these by
    // merging levels 4-7 from level 8. In that case, treat them as if the RP
    // file wasn't present.
    if (!dataRP->validLevel(magnify_level)) {
      if (dataRP->type() == khRasterProduct::Heightmap &&
          magnify_level <= dataRP->maxLevel()) {
        notify(NFY_NOTICE, "Merging upgraded inset level %d", magnify_level);
        dataRP.clear();
      } else {
        throw khException(kh::tr("Internal Error: No level %1 in %2")
                          .arg(magnify_level).arg(dataRPFile.c_str()));
      }
    } else {
      magnifyCoverage = dataRP->levelCoverage(magnify_level);
      dataLevel = &dataRP->level(magnify_level);

      if (alphaRPFile.size()) {
        alphaRP = khRasterProduct::Open(alphaRPFile);
        if (!alphaRP) {
          throw khException(kh::tr("Unable to open %1")
                            .arg(alphaRPFile.c_str()));
        }
        if (alphaRP->type() != khRasterProduct::AlphaMask) {
          throw khException
              (kh::tr("Alpha product is not AlphaMask: %1")
               .arg(alphaRPFile.c_str()));
        }
        alphaLevel = &alphaRP->level(std::min(magnify_level,
                                              alphaRP->maxLevel()));
      }
    }
  }
}

template<class DataTile>
void MergeInset<DataTile>::InitCachedBlendReaders(
    unsigned int magnify_level,
    const std::string &data_rp_file,
    const std::string &cached_blend_file,
    const std::string &cached_blend_alpha_file) {
  if (cached_blend_file.size()) {
    cached_blend_reader = TransferOwnership(
        new ffio::raster::Reader<DataTile>(cached_blend_file));
    if (!cached_blend_reader->ValidLevel(magnify_level)) {
      throw khException(kh::tr("level %1 not valid for %2")
                        .arg(magnify_level).arg(cached_blend_file.c_str()));
    }

    // Note: The check is narrowed only to Imagery-resource
    // because we added "Overlay"-mode for Terrain. So for Terrain in
    // "normal"-mode the cached blend can be larger due to terrain "step out",
    // and in "Overlay"-mode it can be smaller due to calculating/specifying
    // coverage for "Filling"-insets based on extents of Terrain "Overlay"-
    // insets.
    if (dataRP && dataRP->type() == khRasterProduct::Imagery) {
      khExtents<std::uint32_t> cachedExtents
        (cached_blend_reader->levelCoverage(magnify_level).extents);
      khExtents<std::uint32_t> prodExtents(magnifyCoverage.extents);

      if (!cachedExtents.contains(prodExtents)) {
        notify(NFY_WARN,
               "Product Level %u Coverage: (xywh) %u,%u,%u,%u",
               magnify_level,
               prodExtents.beginX(),
               prodExtents.beginY(),
               prodExtents.width(),
               prodExtents.height());
        notify(NFY_WARN,
               "Cached Blend Level %u Coverage: (xywh) %u,%u,%u,%u",
               magnify_level,
               cachedExtents.beginX(),
               cachedExtents.beginY(),
               cachedExtents.width(),
               cachedExtents.height());
        throw khException
          (kh::tr("Specified data product (%1) and cached blend (%2) have"
                  " different coverage").arg(data_rp_file.c_str(), cached_blend_file.c_str()));
      }
    }

    magnifyCoverage = cached_blend_reader->levelCoverage(magnify_level);

    // Initialize cached alpha reader.
    if (cached_blend_alpha_file.size()) {
      // Note: we check if the cached alpha blend directory exists for the
      // asset since in GEE-5.x we need to pick up the GEE-4.x Imagery/Terrain
      // Projects (the PacketLevel assets) that have no alpha blend cached.
      if (khExists(cached_blend_alpha_file)) {
        cached_blend_alpha_reader = TransferOwnership(
            new ffio::raster::Reader<AlphaProductTile>(cached_blend_alpha_file));
      }
      else {
        notify(NFY_INFO, "Cached blend alpha file %s does not exist.", cached_blend_alpha_file.c_str());
      }
    }
  }
}

template class MergeInset<ImageryProductTile>;
template class MergeInset<HeightmapFloat32ProductTile>;


template <class CachingDataReader>
khOpacityMask::OpacityType RasterMerger<CachingDataReader>::GetMaskOpacity(
    const khTileAddr &addr,
    const khRasterProductLevel *alphaProdLevel) {
  khOpacityMask::OpacityType opacity = khOpacityMask::Opaque;

  if (alphaProdLevel) {
    opacity = alphaProdLevel->product()->opacityMask()->GetOpacity(addr);
    if (opacity == khOpacityMask::Unknown) {
      // will succeed or die trying
      // It's OK for alpha tiles to be missing (not broken!)
      // Just fill them with zero.
      inset_alpha_reader_.ReadTile(alphaProdLevel, addr,
                                readAlphaTile,
                                true /* fillMissingWithZero */);
      opacity = ComputeOpacity(readAlphaTile);
      assert(opacity != khOpacityMask::Unknown);
    }
  }

  return opacity;
}


// ****************************************************************************
// ***  RasterMerger::GetInsetTile
// ****************************************************************************
template <class CachingDataReader>
bool RasterMerger<CachingDataReader>::GetInsetTile(
    const khTileAddr &addr,
    MergeInset<DataTile> *inset,
    DataTile *dst_tile,
    AlphaProductTile *dst_alpha_tile) {
  if (inset->cached_blend_reader &&
      inset->cached_blend_reader->presence().GetPresence(addr)) {
    // Read data tile.
    ffio_reader_cache_.ReadTile(inset->cached_blend_reader, addr, *dst_tile);

    // Read alpha tile.
    if (inset->cached_blend_alpha_reader &&
        inset->cached_blend_alpha_reader->presence().GetPresence(addr)) {
      ffio_alpha_reader_cache_.ReadTile(inset->cached_blend_alpha_reader, addr,
                                        *dst_alpha_tile);
    } else {  // Tile is opaque (alpha is not cached for opaque tiles).
      // Fill alpha mask with 1's.
      dst_alpha_tile->Fill(
          std::numeric_limits<AlphaProductTile::PixelType>::max());
    }

    return true;
  }

  if (inset->dataLevel &&
      inset->dataLevel->tileExtents().ContainsRowCol(addr.row, addr.col)) {
    khOpacityMask::OpacityType opacity = GetMaskOpacity(addr,
                                                        inset->alphaLevel);
    if (opacity == khOpacityMask::Opaque) {
      inset_data_reader_.ReadTile(
          inset->dataLevel, addr, *dst_tile, IsMercator());
      // Fill alpha mask with 1's.
      dst_alpha_tile->Fill(
          std::numeric_limits<AlphaProductTile::PixelType>::max());
      return true;
    }
  }

  return false;
}


// ****************************************************************************
// ***  RasterMerger::RasterMerger
// ****************************************************************************
template <class CachingDataReader>
RasterMerger<CachingDataReader>::RasterMerger(
    unsigned int targetLevel,
    const std::vector<PacketLevelConfig::Inset> &insets_,
    const std::string &burnDataRPFile,
    const std::string &burnAlphaRPFile):
    is_mercator_(false),
    burnDataRP(),
    burnAlphaRP(),
    burnDataLevel(0),
    burnAlphaLevel(0),
    inset_data_reader_(0  /* tileCacheSize */,
                    15 /* readerCacheSize */),
    inset_alpha_reader_(100 /* tileCacheSize */,
                     15  /* readerCacheSize */),
    ffio_reader_cache_(15),
    ffio_alpha_reader_cache_(15) {
  // If the user does not have worldwide terrain coverage, but at the highest
  // level 4 the tiles of an imagery and terrain assets intersect, then the
  // Database will request supersampled terrain under the imagery.  If the two
  // assets do not intersect at higher resolutions, then there might be no
  // insets here to supersample.  This is a rare and discouraged case.  Allowing
  // zero insets is the easiest way to fix it, and usually only results in
  // seconds of processing. The resulting terrain is all generated/supersampled
  // by the code marked "Fill any remaining quads with 0" below.
  if (insets_.empty()) {
    notify(NFY_NOTICE, "No insets to process");
  }

  insets.reserve(insets_.size());
  unsigned int i = insets_.size();
  // Note: The input insets may or may not have dataRP entries
  // Also there may or may not be burnDataRPFile. For imagery dataRP is must.
  // So to know whether mercator or not we try to find mercator anywhere
  // possible. For imagery it is bound to get atleast one. There cannot be a
  // mix of mercator and non-mercator because the way it is generated.
  while (i > 0) {
    --i;
    insets.push_back(new MergeInset<DataTile>(
        targetLevel+1,
        insets_[i].dataRP,
        insets_[i].alphaRP,
        insets_[i].packetLevel,
        insets_[i].packetAlphaLevel));
    is_mercator_ = is_mercator_ || insets.back()->IsMercator();
  }

  if (burnDataRPFile.size()) {
    burnDataRP = khRasterProduct::Open(burnDataRPFile);
    if (!burnDataRP) {
      throw khException(kh::tr("Unable to open %1").arg(burnDataRPFile.c_str()));
    }
    if ((burnDataRP->type() != khRasterProduct::Imagery) &&
        (burnDataRP->type() != khRasterProduct::Heightmap)) {
      throw khException
        (kh::tr("Burn data product is neither Imagery nor Heightmap: %1")
         .arg(burnDataRPFile.c_str()));
    }
    is_mercator_ = is_mercator_ || burnDataRP->IsMercator();
    // Starting in Fusion 3.1, terrain is generated to level 4, from 8
    // previously. If the burnDataRP was generated to level 8, then we minify
    // from the level 8 data to generate lower levels. This is signified by
    // burnDataLevel == NULL.
    if (!burnDataRP->validLevel(targetLevel)) {
      if (burnDataRP->type() == khRasterProduct::Heightmap &&
          targetLevel <= burnDataRP->maxLevel()) {
        notify(NFY_NOTICE, "Merging upgraded level %d", targetLevel);
        burnDataRP.clear();
      } else {
        throw khException(kh::tr("Internal Error: No level %1 in %2")
                          .arg(targetLevel).arg(burnDataRPFile.c_str()));
      }
    } else {
      burnDataLevel = &burnDataRP->level(targetLevel);

      if (burnAlphaRPFile.size()) {
        burnAlphaRP = khRasterProduct::Open(burnAlphaRPFile);
        if (!burnAlphaRP) {
          throw khException(kh::tr("Unable to open %1")
                            .arg(burnAlphaRPFile.c_str()));
        }
        if (burnAlphaRP->type() != khRasterProduct::AlphaMask) {
          throw khException
              (kh::tr("Burn alpha product is not AlphaMask: %1")
               .arg(burnAlphaRPFile.c_str()));
        }
        burnAlphaLevel = &burnAlphaRP->level
            (std::min(targetLevel, burnAlphaRP->maxLevel()));
      }
    }
  }
}


// ****************************************************************************
// ***  RasterMerger::Load
// ****************************************************************************
template <class CachingDataReader>
khOpacityMask::OpacityType RasterMerger<CachingDataReader>::Load(
    const khTileAddr &targetAddr,
    DataTile *dst_tile,
    AlphaProductTile *dst_alpha_tile,
    TileBlendStatus *const blend_status) {
  notify(NFY_DEBUG, "RasterMerger::Load (lrc) %u,%u,%u",
         targetAddr.level, targetAddr.row, targetAddr.col);

  *blend_status = NonBlended;

  // See if we can short circuit - the merge with opaque top level tiles.
  if (burnDataLevel &&
      burnDataLevel->tileExtents().ContainsRowCol(targetAddr.row,
                                                  targetAddr.col)) {
    khOpacityMask::OpacityType opacity = GetMaskOpacity(targetAddr,
                                                        burnAlphaLevel);

    notify(NFY_DEBUG,
           "RasterMerger::Load (short circuit) opacity: %d", opacity);
    if (opacity == khOpacityMask::Opaque) {
      inset_data_reader_.ReadTile(burnDataLevel, targetAddr, *dst_tile,
                                  IsMercator());
      // Fill alpha mask with 1's.
      dst_alpha_tile->Fill(
          std::numeric_limits<AlphaProductTile::PixelType>::max());
      return khOpacityMask::Opaque;
    }
  }

  khLevelCoverage srcCoverage(targetAddr.MagnifiedBy(1));

  class Quad {
   public:
    bool done;
    std::uint32_t srcRow;
    std::uint32_t srcCol;

    Quad(std::uint32_t row, std::uint32_t col, unsigned int q)
        : done(false) {
      QuadtreePath::MagnifyQuadAddr(row, col, q, srcRow, srcCol);
    }
  } quads[4] = {
    Quad(targetAddr.row, targetAddr.col, 0),
    Quad(targetAddr.row, targetAddr.col, 1),
    Quad(targetAddr.row, targetAddr.col, 2),
    Quad(targetAddr.row, targetAddr.col, 3)
  };
  unsigned int numQuadsDone = 0;

  // NOTE: index 0 of mergestack is the top of the stack
  for (unsigned int i = 0; i < insets.size(); ++i) {
    MergeInset<DataTile> *inset(insets[i]);

    // make our debug number reflect the reverse order
    unsigned int debugNum = insets.size() - i - 1;

    // see if this inset intersects the srcCoverage
    if (!inset->magnifyCoverage.extents.intersects(srcCoverage.extents)) {
      notify(NFY_DEBUG, "    (no coverage) inset %d", debugNum);
      continue;
    }

    bool contributed = false;
    for (unsigned int q = 0; q < 4; ++q) {
      if (!quads[q].done) {
        khTileAddr srcAddr(srcCoverage.level,
                           quads[q].srcRow, quads[q].srcCol);

        // this read tile will return false if it's not supposed to
        // have the requested tile. If it is supposed to have it, it
        // will succeeed to read it or die trying
        if (GetInsetTile(srcAddr, inset, &readDataTile, &readAlphaTile)) {
          // minify readTile into quad 'q' of dst_tile.
          MinifyTile(*dst_tile, q, readDataTile,
                     khCalcHelper<typename DataTile::PixelType>::AverageOf4);

          // TODO: Do optimize alpha_tile-minify in case
          // readAlphaTile is opaque. And below do not call ComputeOpacity()
          // if all quad-tiles are opaque.

          // minify readAlphaTile into quad 'q' of alpha_tile.
          MinifyTile(
              *dst_alpha_tile, q, readAlphaTile,
              khCalcHelper<AlphaProductTile::PixelType>::ZeroOrAverage);
          quads[q].done = true;
          ++numQuadsDone;

          notify(NFY_DEBUG, "    (quad %d [%u,%u,%u]) inset %d",
                 q,
                 srcAddr.level, srcAddr.row, srcAddr.col,
                 debugNum);
          contributed = true;

          if (numQuadsDone == 4) {
            // we've done all the quads, it's time to bail.
            *blend_status = Blended;
            return ComputeOpacity(*dst_alpha_tile);
          }
        } else {
          // normally, if we pass the srcCoverage intersection
          // test above, we'll never get here. However, if an
          // inset only contains a blendCache, it may no have full
          // coverage.

          // Do nothing
        }
      }
    }
    assert(numQuadsDone < 4);

    if (!contributed) {
      notify(NFY_DEBUG, "    (no contrib) inset %d", debugNum);
    }
  }  // for each inset

  // Fill any remaining quads with 0. This should only happen where there is no
  // coverage at all (> 90.0 North, < -90.0 South, ocean terrain tiles, or if
  // there is no terrain resource with worldwide coverage).
  if (0 == numQuadsDone) {
    // Fill data and alpha tile with 0's.
    dst_tile->FillWithZeros();
    dst_alpha_tile->FillWithZeros();
    *blend_status = NonBlended;
    return khOpacityMask::Transparent;
  }

  // 0 < numQuadsDone < 4
  *blend_status = NoDataBlended;
  for (unsigned int q = 0; q < 4; ++q) {
    if (!quads[q].done) {
      khTileAddr srcAddr(srcCoverage.level,
                         quads[q].srcRow, quads[q].srcCol);
      // Fill data quad with 0's.
      dst_tile->FillQuadWithZeros(q);
      // Fill alpha quad with 0's.
      dst_alpha_tile->FillQuadWithZeros(q);
      notify(NFY_DEBUG, "    (quad %d [%u,%u,%u]) empty",
             q, srcAddr.level, srcAddr.row, srcAddr.col);
    }
  }

  return khOpacityMask::Amalgam;
}

}  // namespace rasterfuse
}  // namespace fusion



// ****************************************************************************
// ***  explicit instantiations
// ****************************************************************************
template class fusion::rasterfuse::RasterMerger<
  CachingProductTileReader_Imagery>;
template class fusion::rasterfuse::RasterMerger<
  CachingProductTileReader_Heightmap>;
