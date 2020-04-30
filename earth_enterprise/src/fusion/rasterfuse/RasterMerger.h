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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_RASTERMERGER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_RASTERMERGER_H_

#include "fusion/rasterfuse/TileLoader.h"
#include "fusion/rasterfuse/ReaderCache.h"
#include "fusion/khraster/khRasterTile.h"
#include "fusion/khraster/CachedRasterInsetReader.h"
#include "fusion/autoingest/.idl/storage/PacketLevelConfig.h"


namespace fusion {
namespace rasterfuse {

template <class DataTile>
class MergeInset {
  // Note: MergeInset is only used by RasterMerger, and it might be internal
  // class of RasterMerger, but let's make everything private and declare
  // all the instantiation of RasterMerger as friends.
  template <class CachingDataReader> friend class RasterMerger;

  MergeInset(unsigned int magnify_level,
             const std::string &dataRPFile,
             const std::string &alphaRPFile,
             const std::string &cached_blend_file,
             const std::string &cached_blend_alpha_file);

  bool IsMercator() const {
    return ((dataRP && dataRP->IsMercator()) ||
            (alphaRP && alphaRP->IsMercator()));
  }

  void InitRasterProducts(unsigned int magnify_level,
                          const std::string &dataRPFile,
                          const std::string &alphaRPFile);

  void InitCachedBlendReaders(unsigned int magnify_level,
                              const std::string &data_rp_file,
                              const std::string &cached_blend_file,
                              const std::string &cached_blend_alpha_file);

  khDeleteGuard<khRasterProduct> dataRP;
  khLevelCoverage magnifyCoverage;
  khDeleteGuard<khRasterProduct> alphaRP;

  // if exists, guaranteed to match magnifyLevel - we use the tiles
  // directly w/o scaling
  const khRasterProductLevel *dataLevel;

  // if exists, it may be lower resolution - we only need this if the
  // opacity mask says Unknown
  const khRasterProductLevel *alphaLevel;

  khDeleteGuard<ffio::raster::Reader<DataTile> > cached_blend_reader;
  khDeleteGuard<
    ffio::raster::Reader<AlphaProductTile> > cached_blend_alpha_reader;
};


template <class CachingDataReader>
class RasterMerger : public TileLoader<typename CachingDataReader::TileType> {
 public:
  typedef typename CachingDataReader::TileType DataTile;
  bool IsMercator() const { return is_mercator_; }

 private:
  bool is_mercator_;
  DataTile readDataTile;
  AlphaProductTile readAlphaTile;  // for reading alpha tiles to compute

  khDeletingVector<MergeInset<DataTile> > insets;

  khDeleteGuard<khRasterProduct> burnDataRP;
  khDeleteGuard<khRasterProduct> burnAlphaRP;
  khRasterProductLevel *burnDataLevel;
  khRasterProductLevel *burnAlphaLevel;

  CachingDataReader inset_data_reader_;
  CachingProductTileReader_Alpha inset_alpha_reader_;
  FFIORasterReaderCache<DataTile> ffio_reader_cache_;
  FFIORasterReaderCache<AlphaProductTile> ffio_alpha_reader_cache_;

  khOpacityMask::OpacityType
  GetMaskOpacity(const khTileAddr &addr,
                 const khRasterProductLevel *alphaProdLevel);

  bool GetInsetTile(const khTileAddr &addr,
                    MergeInset<DataTile> *inset,
                    DataTile *dst_tile,
                    AlphaProductTile *dst_alpha_tile);

 public:
  RasterMerger(unsigned int targetLevel,
               const std::vector<PacketLevelConfig::Inset> &insets_,
               const std::string &burnDataRPFile,
               const std::string &burnAlphaRPFile);

  // implement for my base class - will succeed or throw
  virtual khOpacityMask::OpacityType Load(const khTileAddr &addr,
                                          DataTile *tile,
                                          AlphaProductTile *alpha_tile,
                                          TileBlendStatus *const blend_status);
};


typedef RasterMerger<CachingProductTileReader_Imagery> ImageryRasterMerger;
typedef RasterMerger<CachingProductTileReader_Heightmap> HeightmapRasterMerger;

}  // namespace rasterfuse
}  // namespace fusion

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_RASTERFUSE_RASTERMERGER_H_
