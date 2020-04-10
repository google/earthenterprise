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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_KHRASTER_KHOPACITYMASK_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_KHRASTER_KHOPACITYMASK_H_

#include <string>

#include "common/khInsetCoverage.h"
#include "common/khGuard.h"
#include "common/khTileAddr.h"
#include "fusion/khraster/khRasterTile.h"


class khOpacityMask {
 public:
  // don't change these enum values, they are stored in binary form in the
  // opacity file. 'Unknown' will never be stored in a file or returned
  // from ComputeOpacity. It only results from asking for too high of a level
  // and we have to guess. It's safe to handle 'Unknown' the same as 'Amalgam'
  // since that's probaly what it is anyway.
  enum OpacityType { Transparent = 0, Opaque = 1,
                     Amalgam = 2, Unknown = 3 };
  static inline std::string toString(OpacityType opacity) {
    switch (opacity) {
      case Transparent:
        return "Transparent";
        break;
      case Opaque:
        return "Opaque";
        break;
      case Amalgam:
        return "Amalgam";
        break;
      case Unknown:
        return "Unknown";
        break;
    }
    return std::string();  // unreached but silences warnings
  }


  class Level : public khLevelCoverage {
    friend class khOpacityMask;

    // private and unimplemented - illegal to copy
    Level(const Level &);
    Level& operator=(const Level&);

    // used to build a level from a stored buffer
    Level(unsigned int lev, const khExtents<std::uint32_t> &extents,
          unsigned char *src);

    // used to build an empty level (all transparent) in preparation
    // to fill it in with real values
    Level(unsigned int lev, const khExtents<std::uint32_t> &extents);

   public:
    khDeleteGuard<unsigned char, ArrayDeleter> buf;

    // Will return 'Unknown' if you ask for somthing outside it's known
    // range.
    OpacityType GetOpacity(std::uint32_t row, std::uint32_t col) const;
    void SetOpacity(std::uint32_t row, std::uint32_t col, OpacityType opacity);

    static std::uint32_t BufferSize(std::uint32_t numRows, std::uint32_t numCols) {
      // We can store four OpacityType's per byte (2 bits each)
      // so calc the number of tiles, and divide by 4 (rounding up)
      return ((numRows * numCols + 3) / 4);
    }

    inline std::uint32_t BufferSize(void) const {
      return BufferSize(extents.height(), extents.width());
    }
  };


  // for empty masks that will be filled & saved later
  explicit khOpacityMask(const khInsetCoverage &coverage);

  // load mask from file - will throw exception
  explicit khOpacityMask(const std::string &filename);

  // will throw exception
  void Save(const std::string &filename);

  // May return 'Unknown' if you ask for somthing outside it's known range.
  // But it may be able to figure it out from what it has. :-)
  OpacityType GetOpacity(const khTileAddr &addr) const;

  inline void SetOpacity(const khTileAddr &addr, OpacityType opacity) {
    assert(addr.level < NumFusionLevels);
    assert(levels[addr.level]);
    assert(opacity != Unknown);
    levels[addr.level]->SetOpacity(addr.row, addr.col, opacity);
  }

  unsigned int numLevels;
  unsigned int beginLevel;
  unsigned int endLevel;
  khDeleteGuard<Level> levels[NumFusionLevels];

 private:
  // private and unimplemented
  khOpacityMask(const khOpacityMask &);
  khOpacityMask& operator=(const khOpacityMask &);
};


// should never get called but is needed for compilation
template <class TileType>
khOpacityMask::OpacityType
ComputeOpacity(const TileType &tile) {
  assert(0);
  return khOpacityMask::Unknown;
}

// will never return 'khOpacitymask::Unknown'
extern khOpacityMask::OpacityType
ComputeOpacity(const AlphaProductTile &tile);
khOpacityMask::OpacityType
ComputeOpacity(const AlphaProductTile &tile, const khExtents<std::uint32_t>& extents);




#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_KHRASTER_KHOPACITYMASK_H_
