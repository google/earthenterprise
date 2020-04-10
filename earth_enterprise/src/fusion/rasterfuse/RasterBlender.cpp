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


#include "fusion/rasterfuse/RasterBlender.h"

#include <climits>

#include "fusion/khraster/khRasterProduct.h"
#include "fusion/khraster/khRasterTile.h"
#include "fusion/khraster/BlendTile.h"
#include "fusion/khraster/CastTile.h"
#include "fusion/khraster/khOpacityMask.h"
#include "common/khFileUtils.h"
#include "common/notify.h"
#include "common/khException.h"
#include "fusion/khraster/CachedRasterInsetReaderImpl.h"


namespace fusion {
namespace rasterfuse {

// ****************************************************************************
// ***  BlendInset
// ****************************************************************************
BlendInset::BlendInset(unsigned int targetLevel,
                       const std::string &dataFile,
                       const std::string &alphaFile)
    : dataRP(khRasterProduct::Open(dataFile)),
      alphaRP(alphaFile.empty() ? khTransferGuard<khRasterProduct>() :
              khRasterProduct::Open(alphaFile)),
      dataLevel(),
      alphaLevel() {
  if (!dataRP) {
    throw khException(kh::tr("Unable to open %1").arg(dataFile.c_str()));
  }
  if ((dataRP->type() != khRasterProduct::Imagery) &&
      (dataRP->type() != khRasterProduct::Heightmap)) {
    throw khException
      (kh::tr("Data product is neither Imagery nor Heightmap: %1")
       .arg(dataFile.c_str()));
  }
  if (targetLevel < dataRP->minLevel()) {
    throw khException
      (kh::tr("Internal Error: target level (%1) < data min level (%2) %3")
       .arg(targetLevel).arg(dataRP->minLevel()).arg(dataFile.c_str()));
  }

  targetCoverage = dataRP->levelCoverage(targetLevel);
  dataLevel = &dataRP->level(std::min(targetLevel,
                                      dataRP->maxLevel()));

  if (alphaRP) {
    if (alphaRP->type() != khRasterProduct::AlphaMask) {
      throw khException
        (kh::tr("Alpha product is not of type AlphaMask: %1")
         .arg(alphaFile.c_str()));
    }
    if (targetLevel < alphaRP->minLevel()) {
      throw khException
        (kh::tr("Internal Error: target level (%1) < alpha min level (%2) %3")
         .arg(targetLevel).arg(alphaRP->minLevel()).arg(alphaFile.c_str()));
    }
    alphaLevel = &alphaRP->level(std::min(targetLevel,
                                          alphaRP->maxLevel()));
  } else if (alphaFile.size()) {
    throw khException(kh::tr("Unable to open %1").arg(alphaFile.c_str()));
  }
}



// ****************************************************************************
// ***  RasterBlender::RasterBlender
// ****************************************************************************
template <class CachingDataReader>
RasterBlender<CachingDataReader>::RasterBlender(
    unsigned int level,
    const std::vector<PacketLevelConfig::Inset> &_insets,
    bool _skip_transparent, const bool _is_mercator):
    skip_transparent_(_skip_transparent),
    inset_data_reader_(100 /* tileCacheSize */,
                       15  /* readerCacheSize */),
    inset_alpha_reader_(100 /* tileCacheSize */,
                        15  /* readerCacheSize */),
    is_mercator_(_is_mercator) {
  if (_insets.empty()) {
    throw khException(kh::tr("Empty inset stack"));
  }

  insets_.reserve(_insets.size());
  unsigned int i = _insets.size();
  while (i > 0) {
    --i;
    insets_.push_back(new BlendInset(level, _insets[i].dataRP,
                                     _insets[i].alphaRP));
  }
}


// ****************************************************************************
// ***  RasterBlender::Load
// ****************************************************************************
template <class CachingDataReader>
khOpacityMask::OpacityType
RasterBlender<CachingDataReader>::Load(const khTileAddr &target_addr,
                                       DataTile *dst_data_tile,
                                       AlphaProductTile *dst_alpha_tile,
                                       TileBlendStatus *const blend_status) {
  notify(NFY_DEBUG, "RasterBlender::Load (lrc) %u,%u,%u",
         target_addr.level, target_addr.row, target_addr.col);

  *blend_status = NonBlended;

  // NOTE: blend stack is reverse ordered from the other stacks
  // index 0 is the burn item

  // ********************************************************
  // *** Blend each item from stack into the destination tile
  // ********************************************************
  for (unsigned int i = 0; i < insets_.size(); ++i) {
    const BlendInset *inset(insets_[i]);

    std::string debugName = inset->dataLevel->product()->name();

    // see if this inset intersects the target_addr
    if (!inset->targetCoverage.extents.ContainsRowCol(target_addr.row,
                                                      target_addr.col)) {
      if (i == 0) {
        if (skip_transparent_) {
          notify(NFY_VERBOSE, "    (skippable no coverage top) %s",
                 debugName.c_str());
          return khOpacityMask::Transparent;
        } else {
          // others will blend against me, I'd better fill with 0's
          dst_data_tile->FillWithZeros();
          dst_alpha_tile->FillWithZeros();
          notify(NFY_VERBOSE, "    (no coverage top) %s",
                 debugName.c_str());
        }
      } else {
        // An intermediate inset doesn't have coverage for this
        // area. Just skip it.
        notify(NFY_VERBOSE, "    (no coverage other) %s",
               debugName.c_str());
      }

      continue;
    }

    // ***** get this tile's opacity. read alpha tile if necessary *****
    khOpacityMask::OpacityType opacity = khOpacityMask::Opaque;
    if (inset->alphaLevel) {
      const khRasterProduct *kmp = inset->alphaLevel->product();

      opacity = kmp->opacityMask()->GetOpacity(target_addr);

      if (opacity == khOpacityMask::Amalgam) {
        // will succeed or die trying
        inset_alpha_reader_.ReadTile(inset->alphaLevel, target_addr,
                                     tmp1_alpha_tile_);
      } else if (opacity == khOpacityMask::Unknown) {
        // will succeed or die trying
        // It's OK for alpha tiles to be missing (not broken!)
        // Just fill them with zero.
        inset_alpha_reader_.ReadTile(inset->alphaLevel, target_addr,
                                     tmp1_alpha_tile_,
                                     true /* fillMissingWithZero */);

        // Compute opacity since we don't know it
        opacity = ComputeOpacity(tmp1_alpha_tile_);
        assert(opacity != khOpacityMask::Unknown);
      }
    }

    // ***** now use opacity to figure out what to do *****
    switch (opacity) {
      case khOpacityMask::Opaque: {
        // will succeed or die trying
        DataTile& read_data_tile((i == 0) ? *dst_data_tile : tmp1_data_tile_);
        inset_data_reader_.ReadTile(inset->dataLevel, target_addr,
                                    read_data_tile, IsMercator());
        if (i == 0) {
          notify(NFY_VERBOSE, "    (opaque top) %s",
                 debugName.c_str());
          // Fill alpha mask with 1's in case of opaque top.
          dst_alpha_tile->Fill(
              std::numeric_limits<AlphaProductTile::PixelType>::max());
        } else {
          assert(dst_data_tile->bufs[0] != read_data_tile.bufs[0]);
          // Blend tile with alpha-saturation.
          BlendTerminalTile(dst_data_tile,
                            dst_alpha_tile->bufs[0],
                            read_data_tile);
          notify(NFY_VERBOSE, "    (opaque other) %s",
                 debugName.c_str());
          *blend_status = Blended;
        }
        return khOpacityMask::Opaque;
      }
      case khOpacityMask::Transparent:
        if (i == 0) {
          if (skip_transparent_) {
            notify(NFY_VERBOSE, "    (skippable transparent top) %s",
                   debugName.c_str());
            return khOpacityMask::Transparent;
          } else {
            // others will blend against me, I'd better fill with 0's
            dst_data_tile->FillWithZeros();
            dst_alpha_tile->FillWithZeros();
            notify(NFY_VERBOSE, "    (transparent top) %s",
                   debugName.c_str());
          }
        } else {
          notify(NFY_VERBOSE, "    (transparent other) %s",
                 debugName.c_str());
        }
        break;
      case khOpacityMask::Amalgam:
        {
          *blend_status = Blended;
          // will succeed or die trying
          inset_data_reader_.ReadTile(inset->dataLevel, target_addr,
                                      tmp1_data_tile_, IsMercator());
          if (i == 0) {
            ApplyAlpha(dst_data_tile,
                       dst_alpha_tile->bufs[0],
                       tmp1_data_tile_,
                       tmp1_alpha_tile_.bufs[0]);
            notify(NFY_VERBOSE, "    (top w/ alpha) %s",
                   debugName.c_str());
          } else {
            BlendTileAndSaturateAlpha(dst_data_tile,
                                      dst_alpha_tile->bufs[0],
                                      tmp1_data_tile_,
                                      tmp1_alpha_tile_.bufs[0]);
            // check to see if we've completely saturated the alpha
            if (ComputeOpacity(*dst_alpha_tile) == khOpacityMask::Opaque) {
              notify(NFY_VERBOSE,
                     "    (saturated alpha to opaque) %s",
                     debugName.c_str());
              return khOpacityMask::Opaque;
            }
            notify(NFY_VERBOSE, "    (blended other) %s",
                   debugName.c_str());
          }
          break;
        }
      case khOpacityMask::Unknown:
        assert(0);
        break;
    }
  }  // for each inset

  if (*blend_status == Blended) {
    return khOpacityMask::Amalgam;
  } else {
    assert(*blend_status == NonBlended);
    assert(ComputeOpacity(*dst_alpha_tile) == khOpacityMask::Transparent);
    return khOpacityMask::Transparent;
  }
}

}  // namespace rasterfuse
}  // namespace fusion



// ****************************************************************************
// ***  explicit instantiations
// ****************************************************************************
template class fusion::rasterfuse::RasterBlender<
  CachingProductTileReader_Imagery>;
template class fusion::rasterfuse::RasterBlender<
  CachingProductTileReader_Heightmap>;
