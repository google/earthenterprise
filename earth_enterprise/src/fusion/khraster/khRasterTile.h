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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_KHRASTER_KHRASTERTILE_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_KHRASTER_KHRASTERTILE_H_

#include <string.h>

#include <common/base/macros.h>
#include <string>
#include <memory>
//#include "common/khTypes.h"
#include <cstdint>
#include "common/khTileAddr.h"
#include "common/khGuard.h"


// ****************************************************************************
// *** khRasterTile -  Encapsulation of storage of raster tile
// ***
// *** This is similar to ImageObj<T>, but differs in the following ways:
// *** 1) components always stored separately and in LowerLeft order
// ***    - all tiles used internal to fusion use this layout. Only the final
// ***      image tiles that go to the client differ.
// *** 2) templated on tile width/height (instead of a contruction parameter)
// ***    There are so few different tile sizes in the system that the code
// ***    bloat from the extra instatiations is negligible. Having it
// ***    templated on width/height doesn't add much to the speed, but it does
// ***    mean that tiles of different sizes can be distinguished by the
// ***    compiler. So you won't be ably to pass a 256x256 tile into a routine
// ***    expecting a 1024x1024 tile.
// *** 3) templated on numcomp. This removes the inner (numcomp) loop from all
// ***    the work routines. This results in _significant_ speedups of tight
// ***    copy/convert/blend/etc. loops.
// ***
// *** Because of C++'s current limitation of not being able to partially
// *** specialize template functions, this class doesn't have any members to
// *** manipulate the tile. Rather there are other template objects that are
// *** partially specialized that have the routines to manipulate these
// *** tiles. See some of the other headers in this directory ...
// ****************************************************************************
template <class T, unsigned int tileWidth, unsigned int tileHeight, unsigned int numcomp>
class khRasterTile {
 public:
  typedef T PixelType;
  static const unsigned int TileWidth  = tileWidth;
  static const unsigned int TileHeight = tileHeight;
  static const unsigned int NumComp    = numcomp;
  static const khTypes::StorageEnum Storage
  = khTypes::Helper<PixelType>::Storage;
  static const unsigned int BandPixelCount  = tileWidth * tileHeight;
  static const unsigned int TotalPixelCount = BandPixelCount * numcomp;
  static const unsigned int BandBufSize     = BandPixelCount * sizeof(PixelType);
  static const unsigned int TotalBufSize    = TotalPixelCount * sizeof(PixelType);

 public:
  // actual storage
  khDeleteGuard<PixelType, ArrayDeleter > storage;

  // pointers to each band - no memory ownership
  PixelType* bufs[numcomp];
  unsigned char* ucharBufs[numcomp];

  khRasterTile(void)
    : storage(TransferOwnership(new PixelType[TotalPixelCount])),
      is_monochromatic_(false) {
    // now fill in the pointers to each band
    for (unsigned int i = 0; i < numcomp; ++i) {
      bufs[i] = &storage[i * BandPixelCount];
      ucharBufs[i] = reinterpret_cast<unsigned char*>(bufs[i]);
    }
  }

  explicit khRasterTile(const bool is_monochromatic)
    : storage(TransferOwnership(new PixelType[TotalPixelCount])),
      is_monochromatic_(is_monochromatic) {
    // now fill in the pointers to each band
    for (unsigned int i = 0; i < numcomp; ++i) {
      bufs[i] = &storage[i * BandPixelCount];
      ucharBufs[i] = reinterpret_cast<unsigned char*>(bufs[i]);
    }
  }


  bool IsMonoChromatic() const {
    return is_monochromatic_;
  }

  void FillWithZeros(void) {
    for (unsigned int i = 0; i < numcomp; ++i) {
      memset(bufs[i], 0, BandBufSize);
    }
  }
  void Fill(T t) {
    for (unsigned int b = 0; b < numcomp; ++b) {
      for (unsigned int i = 0; i < BandPixelCount; ++i) {
        bufs[b][i] = t;
      }
    }
  }
  void FillQuadWithZeros(unsigned int quad) {
    std::uint32_t Qoffset = QuadtreePath::QuadToBufferOffset(quad, tileWidth,
                                                      tileHeight);
    for (unsigned int i = 0; i < numcomp; ++i) {
      for (unsigned int j = 0; j < tileHeight/2; ++j) {
        std::uint32_t quadLineOffset = Qoffset + j * tileWidth;
        memset(bufs[i] + quadLineOffset,
               0, (tileWidth/2)*sizeof(PixelType));
      }
    }
  }
  void FillQuad(unsigned int quad, T value) {
    std::uint32_t Qoffset = QuadtreePath::QuadToBufferOffset(quad, tileWidth,
                                                      tileHeight);
    for (unsigned int b = 0; b < numcomp; ++b) {
      for (unsigned int j = 0; j < tileHeight/2; ++j) {
        std::uint32_t quadLineOffset = Qoffset + j * tileWidth;
        for (unsigned int i = quadLineOffset; i < quadLineOffset + tileWidth/2; ++i)
          bufs[b][i] = value;
      }
    }
  }

  void FillExtent(const khExtents<std::uint32_t> &extents, T value) {
    for (unsigned int b = 0; b < numcomp; ++b) {
      PixelType* row_start = bufs[b] + extents.beginRow() * tileWidth;
      for (unsigned int j = extents.beginRow();
           j < extents.endRow(); ++j, row_start += tileWidth) {
        for (unsigned int i = extents.beginCol(); i < extents.endCol(); ++i) {
          *(row_start + i) = value;
        }
      }
    }
  }

  void FillExtentWithZeros(const khExtents<std::uint32_t> &extents) {
    for (std::uint32_t b = 0; b < numcomp; ++b) {
      PixelType* row_start = bufs[b] + extents.beginRow() * tileWidth +
          extents.beginCol();
      for (std::uint32_t j = extents.beginRow();
           j < extents.endRow(); ++j, row_start += tileWidth) {
        memset(row_start, 0, extents.width()*sizeof(PixelType));
      }
    }
  }

 protected:
  enum Skip { SkipAlloc };
  // used by derived classes to skip allocation
  khRasterTile(Skip, bool is_monochromatic)
      : is_monochromatic_(is_monochromatic) {
    for (unsigned int i = 0; i < numcomp; ++i) {
      bufs[i] = 0;
      ucharBufs[i] = 0;
    }
  }

 private:
  const bool is_monochromatic_;
  DISALLOW_COPY_AND_ASSIGN(khRasterTile);
};

// explicit specialization for uchar-PixelType instantiations.
template <>
inline void khRasterTile<unsigned char, RasterProductTileResolution,
                         RasterProductTileResolution, 1>::Fill(unsigned char t) {
  memset(bufs[0], t, BandPixelCount);
}

template <>
inline void khRasterTile<unsigned char, RasterProductTileResolution,
                         RasterProductTileResolution, 3>::Fill(unsigned char t) {
  for (unsigned int b = 0; b < NumComp; ++b) {
    memset(bufs[b], t, BandPixelCount);
  }
}

template <>
inline void khRasterTile<unsigned char, ImageryQuadnodeResolution,
                         ImageryQuadnodeResolution, 3>::Fill(unsigned char t) {
  for (unsigned int b = 0; b < NumComp; ++b) {
    memset(bufs[b], t, BandPixelCount);
  }
}

// Specialization for one band, 8 bit per pixel raster.
template <>
inline void khRasterTile<unsigned char, RasterProductTileResolution,
                         RasterProductTileResolution, 1>::FillExtent(
                             const khExtents<std::uint32_t> &extents, unsigned char value) {
  PixelType* row_start = bufs[0] + extents.beginRow() * TileWidth +
      extents.beginCol();
  for (std::uint32_t j = extents.beginRow();
       j < extents.endRow(); ++j, row_start += TileWidth) {
    memset(row_start, value, extents.width());
  }
}

// Specialization for one band, 8 bit per pixel raster.
template <>
inline void khRasterTile<unsigned char, RasterProductTileResolution,
                         RasterProductTileResolution, 1>::FillExtentWithZeros(
                             const khExtents<std::uint32_t> &extents) {
  FillExtent(extents, static_cast<unsigned char>(0));
}

// ****************************************************************************
// ***  ConvenienceNames
// ****************************************************************************
typedef khRasterTile<unsigned char, RasterProductTileResolution,
                     RasterProductTileResolution, 1> AlphaProductTile;
typedef khRasterTile<unsigned char, RasterProductTileResolution,
                     RasterProductTileResolution, 3> ImageryProductTile;
typedef khRasterTile<std::int16_t, RasterProductTileResolution,
                     RasterProductTileResolution, 1> HeightmapInt16ProductTile;
typedef khRasterTile<float32, RasterProductTileResolution,
                     RasterProductTileResolution, 1>
HeightmapFloat32ProductTile;

typedef khRasterTile<unsigned char, ImageryQuadnodeResolution,
                     ImageryQuadnodeResolution, 3> ImageryFFTile;

// special tiles used for "one extra" tmesh samples
typedef khRasterTile<float32, RasterProductTileResolution+1,
                     RasterProductTileResolution+1, 1> ExpandedHeightmapTile;
typedef khRasterTile<float32, RasterProductTileResolution, 1,
                     1> ExtraHeightmapRow;
typedef khRasterTile<float32, 1, RasterProductTileResolution,
                     1> ExtraHeightmapCol;


// ****************************************************************************
// ***  Alternate public API, for when you already have the buffers
// ***
// ***  This API is for transition purposes only. All new code should use
// ***  khRasterTile to manage the ownership of the buffers.
// ****************************************************************************
template <class TileType>
class khRasterTileBorrowBufs : public TileType {
 public:
  typedef typename TileType::PixelType PixelType;

  khRasterTileBorrowBufs(PixelType *const borrow[], bool is_monochromatic)
      // tell my base to skipalloc
      : TileType(TileType::SkipAlloc, is_monochromatic) {
    // assign the buf pointer from my args
    for (unsigned int i = 0; i < TileType::NumComp; ++i) {
      this->bufs[i] = borrow[i];
      this->ucharBufs[i] = reinterpret_cast<unsigned char*>(borrow[i]);
    }
  }
};



#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_KHRASTER_KHRASTERTILE_H_
