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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_TMESHPACKGENTRAVERSER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_TMESHPACKGENTRAVERSER_H_

#include "common/khTileAddr.h"
#include "common/khGuard.h"
#include "common/khCache.h"
#include "common/geFilePool.h"
#include "common/packetfile/packetfilewriter.h"
#include "vipm/array.h"
#include "fusion/rasterfuse/PackgenTraverser.h"
#include "fusion/rasterfuse/TmeshGenerator.h"
#include "fusion/khraster/MagnifyQuadrant.h"
#include "fusion/khraster/khRasterTile.h"
#include "fusion/rasterfuse/RasterBlender.h"
#include "fusion/rasterfuse/RasterMerger.h"


class AttributionByExtents;

namespace fusion {
namespace rasterfuse {

// To cache pixles we've already seen. So we can use them as the extras for
// other tiles.
class ExtraPixelsImpl : public khRefCounter {
 public:
  ExtraHeightmapCol leftPixels;
  ExtraHeightmapRow bottomPixels;

  explicit ExtraPixelsImpl(const HeightmapFloat32ProductTile& productTile);
};
typedef khRefGuard<ExtraPixelsImpl> ExtraPixels;



class TmeshPrepItem {
 public:
  ExpandedHeightmapTile heightmapTile;
  khOffset<uint32>      heightmapTileOrigin;  // in targetTilespace
  khLevelCoverage       workCoverage;         // in targetTilespace

  inline TmeshPrepItem(void) { }
};


class TmeshWorkItem {
 public:
  PacketFileWriter &writer;
  const AttributionByExtents &attributions;

  class Piece {
   public:
    khTileAddr targetAddr;
    etArray<uchar> compressed;

    Piece(void) : targetAddr(0, 0, 0), compressed() { }
  };
  const khTilespaceFlat &sampleTilespace;
  khDeletingVector<Piece> pieces;
  uint numPiecesUsed;
  TmeshGenerator generator;
  bool decimate_;
  double decimation_threshold_;

  TmeshWorkItem(PacketFileWriter &_writer,
                const AttributionByExtents &_attributions,
                const khTilespaceFlat &_sampleTilespace,
                bool decimate,
                double decimation_threshold);

  void DoWork(TmeshPrepItem *prep);
  void DoWrite(khMTProgressMeter *progress);

};


class TmeshPackgenBaseConfig {
 public:
  typedef HeightmapFloat32ProductTile DataTile;
  typedef TmeshPrepItem               PrepItem;
  typedef TmeshWorkItem               WorkItem;
  typedef HeightmapRasterBlender Blender;
  typedef HeightmapRasterMerger Merger;
};


class TmeshPackgenTraverser : public PackgenTraverser<TmeshPackgenBaseConfig> {
  typedef khCache<uint64, ExtraPixels> ExtraPixelCache;

 public:
  TmeshPackgenTraverser(const PacketLevelConfig &config,
                        geFilePool &file_pool_,
                        const std::string &output,
                        double decimation_threshold
                       );

  virtual TmeshPrepItem* NewPrepItem(void);
  virtual TmeshWorkItem* NewWorkItem(void);

  void Traverse(uint numcpus);

 protected:
  typedef PackgenTraverser<TmeshPackgenBaseConfig> BaseClass;
  typedef HeightmapFloat32ProductTile ProductTile;

  ProductTile             read_product_tile_;
  AlphaProductTile        read_alpha_tile_;
  khTilespaceFlat         sample_tilespace_;
  PacketFileWriter        writer_;
  ExtraPixelCache         extra_cache_;
  bool                    decimate_;
  float                   decimation_threshold_;

  void PrepAddr(const khTileAddr &productAddr);

  void FillExtraUpperPixels(const khTileAddr &productAddr,
                            ExpandedHeightmapTile &heightmapTile,
                            bool releaseCacheEntry);
  void FillExtraRightPixels(const khTileAddr &productAddr,
                            ExpandedHeightmapTile &heightmapTile,
                            bool releaseCacheEntry);
  void FillExtraUpperRightPixel(const khTileAddr &productAddr,
                                ExpandedHeightmapTile &heightmapTile);

  ExtraPixels CacheExtraPixels(const khTileAddr &productAddr,
                               const ProductTile &productTile);
  ExtraPixels FindMakeExtraPixels(const khTileAddr &productAddr);

  virtual void PrepLoop(void);

 private:
  // Read tile at product_addr and populate those pixels in product_tile
  // Assert if the tile is transparent.
  void ReadProductTile(const khTileAddr &product_addr,
                       HeightmapFloat32ProductTile *product_tile,
                       AlphaProductTile *alpha_tile);

  inline bool GetNotCoveredTilesExist() const {
    return not_covered_tiles_exist_;
  }

  inline void SetNotCoveredTilesExist(bool val) {
    not_covered_tiles_exist_ = val;
  }

  // Whether any tiles were not covered (completely or partially) by an inset of
  // a project.
  bool not_covered_tiles_exist_;
};

}  // namespace rasterfuse
}  // namespace fusion

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_TMESHPACKGENTRAVERSER_H_
