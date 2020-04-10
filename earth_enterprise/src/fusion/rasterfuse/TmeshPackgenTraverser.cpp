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


#include "fusion/rasterfuse/TmeshPackgenTraverser.h"

#include <functional>
#include "common/khTileTraversal.h"
#include "common/khstl.h"
#include "fusion/khraster/CastTile.h"
#include "fusion/khraster/AttributionByExtents.h"

namespace fusion {
namespace rasterfuse {

// ****************************************************************************
// ***  ExtraPixels
// ****************************************************************************
ExtraPixelsImpl::ExtraPixelsImpl(
    const HeightmapFloat32ProductTile& productTile) {
  CopySubtile(leftPixels,
              khOffset<std::uint32_t>(RowColOrder, 0, 0),
              productTile,
              khExtents<std::uint32_t>(RowColOrder,
                                0,
                                HeightmapFloat32ProductTile::TileHeight,
                                0, 1));
  CopySubtile(bottomPixels,
              khOffset<std::uint32_t>(RowColOrder, 0, 0),
              productTile,
              khExtents<std::uint32_t>(RowColOrder,
                                0, 1,
                                0,
                                HeightmapFloat32ProductTile::TileWidth));
}


// ****************************************************************************
// ***  TmeshWorkItem
// ****************************************************************************
TmeshWorkItem::TmeshWorkItem(PacketFileWriter &_writer,
                             const AttributionByExtents &_attributions,
                             const khTilespaceFlat& _sampleTilespace,
                             bool decimate,
                             double decimation_threshold):
    writer(_writer),
    attributions(_attributions),
    sampleTilespace(_sampleTilespace),
    numPiecesUsed(0),
    generator(sampleTilespace),
    decimate_(decimate),
    decimation_threshold_(decimation_threshold) {

  unsigned int ratio = RasterProductTilespaceBase.tileSize / sampleTilespace.tileSize;
  unsigned int totalPieces = ratio * ratio;
  pieces.reserve(totalPieces);
  for (unsigned int i = 0; i < totalPieces; ++i) {
    pieces.push_back(new Piece());
  }
}


void TmeshWorkItem::DoWork(TmeshPrepItem *prep) {
  numPiecesUsed = 0;

  for (std::uint32_t row = prep->workCoverage.extents.beginRow();
       row < prep->workCoverage.extents.endRow(); ++row) {
    for (std::uint32_t col = prep->workCoverage.extents.beginCol();
         col < prep->workCoverage.extents.endCol(); ++col) {
      Piece *piece = pieces[numPiecesUsed++];
      piece->targetAddr = khTileAddr(prep->workCoverage.level,
                                     row, col);
      piece->compressed.reset();

      // extract my piece of the bigger tile
      std::uint32_t subrow = row - prep->heightmapTileOrigin.row();
      std::uint32_t subcol = col - prep->heightmapTileOrigin.col();
      generator.Generate
        (prep->heightmapTile.bufs[0],
         khSize<std::uint32_t>(ExpandedHeightmapTile::TileWidth,
                        ExpandedHeightmapTile::TileHeight),
         khOffset<std::uint32_t>(RowColOrder,
                          subrow * sampleTilespace.tileSize,
                          subcol * sampleTilespace.tileSize),
         piece->targetAddr,
         piece->compressed,
         kCRC32Size            /* reserve size */,
         decimate_,            /* do we decimate? */
         decimation_threshold_ /* decimation error threshold */ 
        );
    }
  }
}

void TmeshWorkItem::DoWrite(khMTProgressMeter *progress) {
  for (unsigned int i = 0; i < numPiecesUsed; ++i) {
    Piece* piece = pieces[i];
    writer.WriteAppendCRC(piece->targetAddr,
                          reinterpret_cast<char*>(piece->compressed.peek()),
                          piece->compressed.length + kCRC32Size,
                          attributions.GetInsetId(piece->targetAddr));
  }

  progress->incrementDone(numPiecesUsed);
}



// ****************************************************************************
// ***  TmeshPackgenTraverser
// ****************************************************************************
namespace {

static const khTilespace& BuildSampleTilespace(unsigned int tileSize,
                                               khTilespace* space) {
  unsigned int tileSizeLog2 = log2(tileSize);
  if (tileSizeLog2 == 0) {
    throw khException
      (kh::tr
       ("Internal Error: Tmesh sample size '%1' is not power of 2")
       .arg(tileSize));
  }
  new (space) khTilespace(tileSizeLog2, tileSizeLog2, StartLowerLeft,  // NOLINT
                        false /* isvector */, khTilespace::FLAT_PROJECTION,
                        false);
  return *space;
}

}  // namespace


TmeshPackgenTraverser::TmeshPackgenTraverser(
    const PacketLevelConfig &cfg, geFilePool &file_pool_,
    const std::string &output, double decimation_threshold)
    : BaseClass(
        cfg, output,
        TranslateLevelCoverage(BuildSampleTilespace(cfg.sampleSize,
                                                    &sample_tilespace_),
                               cfg.coverage,
                               RasterProductTilespaceBase),
        ffio::raster::HeightmapCached),
      sample_tilespace_(log2(cfg.sampleSize),
                        log2(cfg.sampleSize),
                        StartLowerLeft,
                        false /* isvector */),
      writer_(file_pool_, output, true /* overwrite */),
      extra_cache_(20000 /* apx 160M */, "tmesh traverser"),
      not_covered_tiles_exist_(false) {

  if((decimation_threshold <= 0.0) || (config.decimate == false)) {
    decimate_ = false;
  } else {
    decimation_threshold_ = decimation_threshold;
    decimate_ = true;
  }
  notify(NFY_NOTICE, "Generating %u Tmeshes (4 @ %ux%u) for level %u",
         cfg.coverage.extents.width() * cfg.coverage.extents.height(),
         sample_tilespace_.tileSize / 2 + 1,
         sample_tilespace_.tileSize / 2 + 1,
         config.coverage.level);
  notify(NFY_NOTICE, "    from %u heightmap tile(s)",
         productCoverage.extents.width() * productCoverage.extents.height());
  notify(NFY_NOTICE, "    %s",
         decimate_ ? "with decimation" : "no decimation");
}

void TmeshPackgenTraverser::Traverse(unsigned int numcpus) {
  SetNotCoveredTilesExist(false);
  int numWorkThreads = numcpus;
  DoTraverse(numWorkThreads);

  writer_.Close();

  // Report about not covered tiles once per traverse (per level) after
  // processing all tiles.
  if (GetNotCoveredTilesExist()) {
    notify(NFY_WARN,
           "One or more tiles not covered (partially or completely)"
           " by an inset of the project (transparent tmesh tile).");
    notify(NFY_WARN,
           "Corresponding areas were filled with 0 value of elevation.");
  }
}


TmeshPrepItem* TmeshPackgenTraverser::NewPrepItem(void) {
  return new TmeshPrepItem();
}


TmeshWorkItem* TmeshPackgenTraverser::NewWorkItem(void) {
  return new TmeshWorkItem(writer_, attributions, sample_tilespace_,
                           decimate_, decimation_threshold_);
}


void TmeshPackgenTraverser::ReadProductTile(const khTileAddr &product_addr,
                                            ProductTile *product_tile,
                                            AlphaProductTile *alpha_tile) {
  TileBlendStatus blend_status = NoDataBlended;
  const khOpacityMask::OpacityType opacity =
      fetcher->Fetch(product_addr, product_tile, alpha_tile, &blend_status);
  if (opacity == khOpacityMask::Transparent ||
      blend_status == NoDataBlended) {
    // GEE 4.4.1 threw an exception here, but actually Fetcher couldn't
    // report 'Transparent'- opacity. 'Amalgam'-opacity was always reported
    // instead of 'Transparent'.
    // GEE 5.0.0 threw an exception here and Fetcher could return 'Transparent'
    // opacity.
    // GEE 5.0.1: we detect whether the product tile was built based on
    // the tile(s) that is(are) not covered by any inset of project,
    // Also, we switched to report Warning.
//     throw khException(kh::tr("Internal Error: Transparent tmesh tile"));

    // GEE 5.1.1: set flag to report warning after traversing, since,
    // otherwise, there could be a lot of messages in log for high-resolution
    // levels.
    SetNotCoveredTilesExist(true);
  }
}


ExtraPixels TmeshPackgenTraverser::CacheExtraPixels(
    const khTileAddr &productAddr,
    const ProductTile &productTile) {
  // make a new cache entry
  ExtraPixels entry(khRefGuardFromNew(new ExtraPixels::RefType
                                      (productTile)));
  extra_cache_.Add(productAddr.Id(), entry);
  return entry;
}

ExtraPixels TmeshPackgenTraverser::FindMakeExtraPixels(
    const khTileAddr &productAddr) {
  std::uint64_t tileid = productAddr.Id();
  ExtraPixels found;

  if (extra_cache_.Find(tileid, found)) {
    return found;
  } else {
    ReadProductTile(productAddr, &read_product_tile_, &read_alpha_tile_);
    // add the new one to the cache
    found = CacheExtraPixels(productAddr, read_product_tile_);
  }
  return found;
}


void TmeshPackgenTraverser::FillExtraUpperPixels(
    const khTileAddr &productAddr,
    ExpandedHeightmapTile &heightmapTile,
    bool releaseCacheEntry) {
  const khTilespace& tile_space = RasterProductTilespace(is_mercator_);
  khTileAddr upperAddr(productAddr.UpperAddr(tile_space));
  if (upperAddr.row == 0) {
    // we wrapped at the top of the world - just fill extra top
    // pixels with 0
    std::uint32_t pos = ExpandedHeightmapTile::BandPixelCount -
                 ExpandedHeightmapTile::TileWidth;
    memset(&heightmapTile.bufs[0][pos], 0,
           ExpandedHeightmapTile::TileWidth *
           sizeof(ExpandedHeightmapTile::PixelType));
  } else {
    // We have valid upper addr. Pull the cache record that will
    // have the extra pixels. This will succeed or die trying;
    ExtraPixels extra(FindMakeExtraPixels(upperAddr));

    // copy the extra pixels into the PrepItem's buffer
    CopySubtile(heightmapTile,
                khOffset<std::uint32_t>(RowColOrder, tile_space.tileSize, 0),
                extra->bottomPixels,
                khExtents<std::uint32_t>(RowColOrder, 0, 1, 0, tile_space.tileSize));

    if (releaseCacheEntry) {
      // Nobody else will need this cache entry. Go ahead and drop it
      // from the cache.
      extra_cache_.Remove(upperAddr.Id());
    }
  }
}

void TmeshPackgenTraverser::FillExtraRightPixels(
    const khTileAddr &productAddr,
    ExpandedHeightmapTile &heightmapTile,
    bool releaseCacheEntry) {
  const khTilespace& tile_space = RasterProductTilespace(is_mercator_);
  khTileAddr rightAddr(productAddr.RightAddr(tile_space));

  // Pull the cache record that will have the extra pixels.
  // This will succeed or die trying;
  ExtraPixels extra(FindMakeExtraPixels(rightAddr));

  // copy the extra pixels into the PrepItem's buffer
  CopySubtile(heightmapTile,
              khOffset<std::uint32_t>(RowColOrder, 0, tile_space.tileSize),
              extra->leftPixels,
              khExtents<std::uint32_t>(RowColOrder, 0, tile_space.tileSize, 0, 1));

  if (releaseCacheEntry) {
    // Nobody else will need this cache entry. Go ahead and drop it
    // from the cache.
    extra_cache_.Remove(rightAddr.Id());
  }
}

void TmeshPackgenTraverser::FillExtraUpperRightPixel(
    const khTileAddr &productAddr,
    ExpandedHeightmapTile &heightmapTile) {
  const khTilespace& tile_space = RasterProductTilespace(is_mercator_);
  khTileAddr upperRightAddr(productAddr.UpperRightAddr(tile_space));
  ExpandedHeightmapTile::PixelType extraPixel;

  if (upperRightAddr.row == 0) {
    // we wrapped at the top of the world. Just fill the pixel with 0
    extraPixel = ExpandedHeightmapTile::PixelType();
  } else {
    // We have valid upper right addr. Pull the cache record that will
    // have the extra pixels. This will succeed or die trying;
    ExtraPixels extra(FindMakeExtraPixels(upperRightAddr));

    extraPixel = extra->leftPixels.bufs[0][0];

    // we know that nobody will ever need this tile again
    extra_cache_.Remove(upperRightAddr.Id());
  }

  std::uint32_t pos = ExpandedHeightmapTile::BandPixelCount - 1;
  heightmapTile.bufs[0][pos] = extraPixel;
}


void TmeshPackgenTraverser::PrepAddr(const khTileAddr &productAddr) {
  bool isBottomProductTile = (productAddr.row ==
                              productCoverage.extents.beginRow());
  bool isLeftProductTile = (productAddr.col ==
                            productCoverage.extents.beginCol());


  const khTilespace& tile_space = RasterProductTilespace(is_mercator_);
  // Translate this productAddr into targetTilespace coverage
  khLevelCoverage thisTileCoverage
    (TranslateLevelCoverage(tile_space,
                            khLevelCoverage(productAddr),
                            sample_tilespace_));

  // Intersect it with what we really want
  khExtents<std::uint32_t> workExtents
    (khExtents<std::uint32_t>::Intersection(thisTileCoverage.extents,
                                     config.coverage.extents));
  bool needExtraPixelsOnTop   = (thisTileCoverage.extents.endRow() ==
                                 workExtents.endRow());
  bool needExtraPixelsOnRight = (thisTileCoverage.extents.endCol() ==
                                 workExtents.endCol());


  // Fetch the primary product tile
  ReadProductTile(productAddr, &read_product_tile_, &read_alpha_tile_);

  // Note: At level 4 ClientTmeshTilespaceFlat has a world extent of
  // 4-12(SN), 0-16(WE) i.e (128-384 pxl, 0-512 pxl). When the workExtent is
  // WorldExtent and levels are 4 do the wrap-over around Antimeridian.
  //
  // For level 4, out of 1024 * 1024 read_product_tile we only populate data in
  //   col 0 to 511 (both inclusive) and row 128 to 383 (both inclusive)
  // We use col 512 (in right) for TMesh generation. So let's copy col 0 to col
  // 512 (wrap over around the world boundary). For other levels the same wrap
  // over problem is solved by CacheExtraPixels. This fixes the seam-stitching
  // problems for level 4 terrain tiles at Antimeridian (http://b/1683608)
  //
  // We have noticed this short coming in only terrain tiles from terrain data.
  // In terrain tiles from imagery we have
  // sample_tilespace_.tileSize < ClientTmeshTilespaceBase.tileSize
  //
  // Also when needExtraPixelsOnRight is true, the rest of the code takes care
  // of it. Only for level 4, for terrain tiles it is false and we do need to
  // get the extra pixels, (not at 1024 col of 0-1024 tile, but at 512th col).
  if ((config.coverage.level == 4) && !needExtraPixelsOnRight &&
      (sample_tilespace_.tileSize == ClientTmeshTilespaceBase.tileSize)) {
    const khExtents<std::uint32_t> world_extent = ClientTmeshTilespaceFlat.
        WorldExtents(config.coverage.level);
    // We can only do wrapping around antimeridian if we have world extents
    if (world_extent.beginX() == workExtents.beginX() &&
        world_extent.endX() == workExtents.endX()) {
      const std::uint32_t min_row = sample_tilespace_.tileSize * workExtents.beginY();
      const std::uint32_t max_row = sample_tilespace_.tileSize * workExtents.endY();
      const std::uint32_t min_col = sample_tilespace_.tileSize * workExtents.beginX();
      const std::uint32_t max_col = sample_tilespace_.tileSize * workExtents.endX();
      // Wrap 1st col pixels to 512th col
      CopySubtile(
          read_product_tile_, khOffset<std::uint32_t>(RowColOrder, min_row, max_col),
          read_product_tile_, khExtents<std::uint32_t>(RowColOrder, min_row, max_row,
                                             min_col, min_col + 1));
    }
  }

  // Save my left and bottom rows for others to use
  CacheExtraPixels(productAddr, read_product_tile_);


  // Wait for another PrepItem to store the results.
  // This throttles the reads.
  TmeshPrepItem *prep = prepEmpty.pop();
  prep->heightmapTileOrigin = thisTileCoverage.extents.origin();
  prep->workCoverage = khLevelCoverage(config.coverage.level,
                                       workExtents);

  // copy it into the PrepItem's oversized heightmap buf
  CopySubtile(prep->heightmapTile,
              khOffset<std::uint32_t>(RowColOrder, 0, 0),
              read_product_tile_,
              khExtents<std::uint32_t>(RowColOrder,
                                0, tile_space.tileSize,
                                0, tile_space.tileSize));
  // If needed fetch an extra row of pixels for the top
  if (needExtraPixelsOnTop) {
    FillExtraUpperPixels(productAddr, prep->heightmapTile,
                         isLeftProductTile /* releaseCacheEntry */);
  }

  // If needed fetch an extra col of pixels for the right
  if (needExtraPixelsOnRight) {
    FillExtraRightPixels(productAddr, prep->heightmapTile,
                         isBottomProductTile /* releaseCacheEntry */);
  }

  // If needed fetch an extra pixel for the top - right
  if (needExtraPixelsOnTop && needExtraPixelsOnRight) {
    FillExtraUpperRightPixel(productAddr, prep->heightmapTile);
  }

  // Add it to the work queue.
  prepFull.push(prep);
}


void TmeshPackgenTraverser::PrepLoop(void) {
  try {
    TopRightToBottomLeftTraversal
      (productCoverage,
       unary_obj_closure(this, &TmeshPackgenTraverser::PrepAddr));
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "PrepLoop: %s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "PrepLoop: Unknown error");
  }
}

}  // namespace rasterfuse
}  // namespace fusion



// ****************************************************************************
// ***  explicit instantiaion of base class
// ****************************************************************************
#include "fusion/rasterfuse/PackgenTraverserImpl.h"
template class fusion::rasterfuse::PackgenTraverser<
  fusion::rasterfuse::TmeshPackgenBaseConfig>;
