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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_TILEFETCHER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_TILEFETCHER_H_

#include <assert.h>

#include "common/khTileAddr.h"
#include "common/khGuard.h"
#include "fusion/rasterfuse/TileLoader.h"
#include "fusion/khraster/khRasterTile.h"
#include "fusion/khraster/ffioRasterWriter.h"

namespace fusion {
namespace rasterfuse {

template <class TileType>
class TileFetcher {
 public:
  typedef TileLoader<TileType> Loader;

  ffio::raster::Writer<TileType> *cached_blend_writer_;
  ffio::raster::Writer<AlphaProductTile> *cached_blend_alpha_writer_;

 private:
  khDeleteGuard<Loader> loader_;

 public:
  inline TileFetcher(
      const khTransferGuard<Loader> &_loader,
      ffio::raster::Writer<TileType> *_cached_blend_writer,
      ffio::raster::Writer<AlphaProductTile> *_cached_blend_alpha_writer)
      : cached_blend_writer_(_cached_blend_writer),
        cached_blend_alpha_writer_(_cached_blend_alpha_writer),
        loader_(_loader) {
  }

  // will succeed or throw
  inline khOpacityMask::OpacityType Fetch(const khTileAddr &addr,
                                          TileType *tile,
                                          AlphaProductTile *alpha_tile) {
    TileBlendStatus blend_status = NonBlended;
    return Fetch(addr, tile, alpha_tile, &blend_status);
  }

  // will succeed or throw
  khOpacityMask::OpacityType Fetch(const khTileAddr &addr,
                                   TileType *tile,
                                   AlphaProductTile *alpha_tile,
                                   TileBlendStatus *const blend_status) {
    *blend_status = NonBlended;

    // will succeed or throw
    khOpacityMask::OpacityType opacity =
        loader_->Load(addr, tile, alpha_tile, blend_status);

    // Write to the blend cache if we need to.
    if (*blend_status &&
        cached_blend_writer_ && !cached_blend_writer_->GetPresence(addr)) {
      // Blended tile can't have 'Transparent'-opacity. Its opacity is
      // 'Amalgam' or 'Opaque'.
      assert(khOpacityMask::Amalgam == opacity ||
             khOpacityMask::Opaque == opacity);
      khInsetCoverage coverage;
      cached_blend_writer_->GetPresenceMask().PopulateCoverage(coverage);

      if (coverage.hasTile(addr)) {
        // write data tile to the blend cache.
        cached_blend_writer_->WriteTile(addr, *tile);

        // write alpha tile to the blend cache.
        if ((khOpacityMask::Amalgam == opacity) && cached_blend_alpha_writer_) {
          assert(!cached_blend_alpha_writer_->GetPresence(addr));
          cached_blend_alpha_writer_->WriteTile(addr, *alpha_tile);
        }
      } else {
        notify(NFY_WARN, "Attempt to cache tile outside boundaries");
      }
    }
    return opacity;
  }
};

}  // namespace rasterfuse
}  // namespace fusion

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_TILEFETCHER_H_
