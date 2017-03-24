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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_TILELOADER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_TILELOADER_H_

#include "common/khTileAddr.h"
#include "fusion/khraster/khOpacityMask.h"
#include "fusion/khraster/khRasterTile.h"

namespace fusion {
namespace rasterfuse {

// Status of Tile loading by RasterBlender/RasterMerger.
enum TileBlendStatus {
  NonBlended = 0,    // There was no blending/merging.
  Blended = 1,       // There was blending/merging, but all the tiles are
                     // covered with one of the insets of the project.
  NoDataBlended = 2  // There was merging and at least one, but not all of the
                     // source tiles is not covered by any source inset of the
                     // project.
};

template <class TileType>
class TileLoader {
 public:
  virtual ~TileLoader(void) { }

  // Fetches product and alpha tile.
  // @note will succeed or throw.
  // @param[in] addr tile address.
  // @param[out] data_tile fetched product tile.
  // @param[out] alpha_tile fetched alpha tile.
  // @param[out] blend_status status of tile loading.
  // @return tile opacity value.
  virtual khOpacityMask::OpacityType Load(
      const khTileAddr &addr,
      TileType *data_tile,
      AlphaProductTile *alpha_tile,
      TileBlendStatus *const blend_status) = 0;
};

}  // namespace rasterfuse
}  // namespace fusion

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_TILELOADER_H_
