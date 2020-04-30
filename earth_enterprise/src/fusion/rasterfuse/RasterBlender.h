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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_RASTERBLENDER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_RASTERBLENDER_H_

#include "fusion/rasterfuse/TileLoader.h"
#include "fusion/khraster/CachedRasterInsetReader.h"
#include "fusion/khraster/khRasterTile.h"
#include "fusion/autoingest/.idl/storage/PacketLevelConfig.h"

namespace fusion {
namespace rasterfuse {

class BlendInset {
 private:
  khDeleteGuard<khRasterProduct> dataRP;
  khDeleteGuard<khRasterProduct> alphaRP;

 public:
  khLevelCoverage targetCoverage;

  const khRasterProductLevel *dataLevel;
  const khRasterProductLevel *alphaLevel;

  BlendInset(unsigned int targetLevel,
             const std::string &dataFile,
             const std::string &alphaFile);
};

template <class CachingDataReader>
class RasterBlender : public TileLoader<typename CachingDataReader::TileType> {
 public:
  typedef typename CachingDataReader::TileType DataTile;

 private:
  bool skip_transparent_;

  // temporary tiles that can be used during magnification
  DataTile         tmp1_data_tile_;
  AlphaProductTile tmp1_alpha_tile_;

  // reverse order from configs - first is on top
  khDeletingVector<BlendInset> insets_;

  // MUST be after insets in class, so created after and destroyed before.
  // insets manages lifetime, insetReader borrows contents
  CachingDataReader inset_data_reader_;
  CachingProductTileReader_Alpha inset_alpha_reader_;
  const bool is_mercator_;

 public:
  RasterBlender(unsigned int level,
                const std::vector<PacketLevelConfig::Inset> &_insets,
                bool _skip_transparent, const bool _is_mercator);

  // implement for my base class - will succeed or throw
  virtual khOpacityMask::OpacityType Load(const khTileAddr &addr,
                                          DataTile *tile,
                                          AlphaProductTile *alpha_tile,
                                          TileBlendStatus *const blend_status);
  bool IsMercator() const { return is_mercator_; }
};

typedef RasterBlender<CachingProductTileReader_Imagery> ImageryRasterBlender;
typedef RasterBlender<
  CachingProductTileReader_Heightmap> HeightmapRasterBlender;

}  // namespace rasterfuse
}  // namespace fusion

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_RASTERBLENDER_H_
