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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_KHRASTER_FFIORASTER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_KHRASTER_FFIORASTER_H_

#include "common/khConstants.h"
//#include "common/khTypes.h"
#include <cstdint>
#include "common/compressor.h"
#include "common/khEndian.h"
#include "fusion/khraster/khRasterTile.h"


namespace ffio {
namespace raster {
// Don't change the values in this enum, they are stored in raster
// ff indexes
// How to interpret the ff records
enum Subtype {
  Imagery   = 0,        // servable imagery 256x256 tiles
  // Heightmap = 1,
  ImageryCached = 2,    // blended only imagery stored as 1024x1024
  HeightmapCached = 3,  // blended only heightmaps stored as 1024x1024
  AlphaCached = 4       // blended only alpha stored as 1024x1024
};

inline std::string SubtypeName(Subtype t) {
  switch (t) {
    case Imagery:
      return kImageryType;
    case ImageryCached:
      return kImageryCached;
    case HeightmapCached:
      return kHeightMapCached;
    case AlphaCached:
      return kAlphaCached;
  }
  return std::string();  // unreached but silences warnings
}

inline const khTilespaceBase& TilespaceFromSubtype(Subtype stype) {
  switch (stype) {
    case Imagery:
      return ClientImageryTilespaceBase;
    case ImageryCached:
    case HeightmapCached:
    case AlphaCached:
      return RasterProductTilespaceBase;
  }
  return RasterProductTilespaceBase;  // unreached - but silences warnings
}

// TODO: these functions are not used.
# if 0

inline unsigned int NumCompFromSubtype(Subtype stype) {
  switch (stype) {
    case Imagery:
    case ImageryCached:
      return 3;
    case HeightmapCached:
      return 1;
  }
  return 0;  // unreached - but silences warnings
}

inline TileOrientation TileOrientationFromSubtype(Subtype stype) {
  switch (stype) {
    case Imagery:
      return StartUpperLeft;
    case ImageryCached:
    case HeightmapCached:
      return StartLowerLeft;
  }
  return StartLowerLeft;  // unreached - but silences warnings
}

#endif


// Don't re-arrange this class. It's carefully sized and packed
// to be exactly 16 bytes long.
class IndexTypeData {
 public:
  std::uint16_t tilesize;
  std::uint8_t  numcomp;
  std::uint8_t  datatype;    /* khTypes::Storage enum */
  std::uint8_t  orientation; /* TileOrientation enum */
  std::uint8_t  compression; /* CompressionMode enum */
  std::uint8_t  subtype;     /* ffio::Raster::Subtype enum */
  std::uint8_t  unused1;
  std::uint64_t unused2;

  IndexTypeData(ffio::raster::Subtype  subtype_,
                std::uint16_t                 tilesize_,
                std::uint8_t                  numcomp_,
                khTypes::StorageEnum   datatype_,
                TileOrientation        orientation_,
                CompressMode           compression_) :
      tilesize(tilesize_),
      numcomp(numcomp_),
      datatype((std::uint8_t)datatype_),
      orientation((std::uint8_t)orientation_),
      compression((std::uint8_t)compression_),
      subtype((std::uint8_t)subtype_),
      unused1(0),
      unused2(0) { }
  void LittleEndianToHost(void) {
    tilesize    = ::LittleEndianToHost(tilesize);
    numcomp     = ::LittleEndianToHost(numcomp);
    datatype    = ::LittleEndianToHost(datatype);
    orientation = ::LittleEndianToHost(orientation);
    compression = ::LittleEndianToHost(compression);
    subtype     = ::LittleEndianToHost(subtype);
    // no need to swap unused1, 0 is always 0
    // no need to swap unused2, 0 is always 0
  }
  // just another name for code clarity
  inline void HostToLittleEndian(void) { LittleEndianToHost(); }

  // used only by Reader
  IndexTypeData(void) { }
};
static_assert(sizeof(IndexTypeData) == 16, "Bad Index Type Data Size");


}  // namespace ffio::Raster
}  // namespace ffio

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_KHRASTER_FFIORASTER_H_
