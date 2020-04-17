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


#ifndef __CastTile_h
#define __CastTile_h

#include "khRasterTile.h"
#include <khCalcHelper.h>

// ****************************************************************************
// ***  PixelCaster - Helper class
// ***
// ***    This class is a specialized template used during the
// ***  type casting of tiles. The purpose of this template is to allow
// ***  efficient code inside the tight loop of the cast.
// ***    These calls will be completely inlined.
// ****************************************************************************
template <class T, class U, unsigned int numcomp>
class PixelCaster { };
// specialized for numcomp == 1
template <class T, class U>
class PixelCaster<T, U, 1>
{
 public:
  inline static void CastPixel(T *const dest[], std::uint32_t destPos,
                               const U *const src[], std::uint32_t srcPos) {
    dest[0][destPos] = ClampRange<T>(src[0][srcPos]);
  }
};
// specialized for numcomp == 3
template <class T, class U>
class PixelCaster<T, U, 3>
{
 public:
  inline static void CastPixel(T *const dest[], std::uint32_t destPos,
                               const U *const src[], std::uint32_t srcPos) {
    dest[0][destPos] = ClampRange<T>(src[0][srcPos]);
    dest[1][destPos] = ClampRange<T>(src[1][srcPos]);
    dest[2][destPos] = ClampRange<T>(src[2][srcPos]);
  }
};


// ****************************************************************************
// ***  CastTile
// ***
// ***  Cast a tile from one type to another (typically std::int16_t <-> float for
// ***  heightmaps). Calls to this function should be completely inlined.
// ****************************************************************************
template <class DestTile, class SrcTile>
inline void
CastTile(DestTile &dest, const SrcTile &src)
{
  static_assert(DestTile::TileWidth == SrcTile::TileWidth,
                      "Incompatible Tile Width");
  static_assert(DestTile::TileHeight == SrcTile::TileHeight,
                      "Incompatible Tile Height");

  for (std::uint32_t i = 0; i < DestTile::BandPixelCount; ++i) {
    PixelCaster<
      typename DestTile::PixelType,
      typename SrcTile::PixelType,
      DestTile::NumComp>::CastPixel(dest.bufs, i, src.bufs, i);
  }
}

// ****************************************************************************
// ***  CastTile
// ***
// ***  Cast a raw bufs to a tile
// ***  Calls to this function should be completely inlined.
// ****************************************************************************
template <class DestTile, class SrcType>
inline void
CastTile(DestTile &dest, const SrcType *const *srcBufs)
{
  for (std::uint32_t i = 0; i < DestTile::BandPixelCount; ++i) {
    PixelCaster<typename DestTile::PixelType, SrcType, DestTile::NumComp>
      ::CastPixel(dest.bufs, i, srcBufs, i);
  }
}

template <class DestTile>
inline void
CastTile(DestTile &dest, const void *const srcBufs,
         khTypes::StorageEnum srcType)
{
  switch (srcType) {
    case khTypes::UInt8:
      CastTile(dest, (const std::uint8_t *const *)srcBufs);
      break;
    case khTypes::Int8:
      CastTile(dest, (const std::int8_t *const *)srcBufs);
      break;
    case khTypes::UInt16:
      CastTile(dest, (const std::uint16_t *const *)srcBufs);
      break;
    case khTypes::Int16:
      CastTile(dest, (const std::int16_t *const *)srcBufs);
      break;
    case khTypes::UInt32:
      CastTile(dest, (const std::uint32_t *const *)srcBufs);
      break;
    case khTypes::Int32:
      CastTile(dest, (const std::int32_t *const *)srcBufs);
      break;
    case khTypes::UInt64:
      CastTile(dest, (const std::uint64_t *const *)srcBufs);
      break;
    case khTypes::Int64:
      CastTile(dest, (const std::int64_t *const *)srcBufs);
      break;
    case khTypes::Float32:
      CastTile(dest, (const float32 *const *)srcBufs);
      break;
    case khTypes::Float64:
      CastTile(dest, (const float64 *const *)srcBufs);
      break;
  }
}


// ****************************************************************************
// ***  CastBufs
// ****************************************************************************
template <class T, class U>
inline void
CastBuf(T * const dest, const U * const src, unsigned int num)
{
  for (unsigned int pos = 0; pos < num; ++pos) {
    dest[pos] = ClampRange<T>(src[pos]);
  }
}

template <class T>
inline void
CastBuf(T * const dest, void* src, khTypes::StorageEnum srcType, unsigned int num)
{
  switch (srcType) {
    case khTypes::UInt8:
      CastBuf(dest, (const std::uint8_t *const)src, num);
      break;
    case khTypes::Int8:
      CastBuf(dest, (const std::int8_t *const)src, num);
      break;
    case khTypes::UInt16:
      CastBuf(dest, (const std::uint16_t *const)src, num);
      break;
    case khTypes::Int16:
      CastBuf(dest, (const std::int16_t *const)src, num);
      break;
    case khTypes::UInt32:
      CastBuf(dest, (const std::uint32_t *const)src, num);
      break;
    case khTypes::Int32:
      CastBuf(dest, (const std::int32_t *const)src, num);
      break;
    case khTypes::UInt64:
      CastBuf(dest, (const std::uint64_t *const)src, num);
      break;
    case khTypes::Int64:
      CastBuf(dest, (const std::int64_t *const)src, num);
      break;
    case khTypes::Float32:
      CastBuf(dest, (const float32 *const)src, num);
      break;
    case khTypes::Float64:
      CastBuf(dest, (const float64 *const)src, num);
      break;
  }
}

inline void
CastBuf(void* dest, khTypes::StorageEnum destType,
        void* src, khTypes::StorageEnum srcType,
        unsigned int num)
{
  switch (destType) {
    case khTypes::UInt8:
      CastBuf((std::uint8_t *const)dest, src, srcType, num);
      break;
    case khTypes::Int8:
      CastBuf((std::int8_t *const)dest, src, srcType, num);
      break;
    case khTypes::UInt16:
      CastBuf((std::uint16_t *const)dest, src, srcType, num);
      break;
    case khTypes::Int16:
      CastBuf((std::int16_t *const)dest, src, srcType, num);
      break;
    case khTypes::UInt32:
      CastBuf((std::uint32_t *const)dest, src, srcType, num);
      break;
    case khTypes::Int32:
      CastBuf((std::int32_t *const)dest, src, srcType, num);
      break;
    case khTypes::UInt64:
      CastBuf((std::uint64_t *const)dest, src, srcType, num);
      break;
    case khTypes::Int64:
      CastBuf((std::int64_t *const)dest, src, srcType, num);
      break;
    case khTypes::Float32:
      CastBuf((float32 *const)dest, src, srcType, num);
      break;
    case khTypes::Float64:
      CastBuf((float64 *const)dest, src, srcType, num);
      break;
  }
}



// ****************************************************************************
// ***  CopyTile
// ***
// ****************************************************************************
template <class TileType>
inline void
CopyTile(TileType &dest, const TileType &src)
{
  for (std::uint32_t i = 0; i < TileType::BandPixelCount; ++i) {
    PixelCaster<
      typename TileType::PixelType,
      typename TileType::PixelType,
      TileType::NumComp>::CastPixel(dest.bufs, i, src.bufs, i);
  }
}


// ****************************************************************************
// ***  CopySubtile
// ***
// ****************************************************************************
template <class DestTile, class SrcTile>
inline void
CopySubtile(DestTile &dest, const khOffset<std::uint32_t> &destOffset,
            const SrcTile &src, const khExtents<std::uint32_t> &srcExtents)
{
  static_assert(DestTile::NumComp == SrcTile::NumComp,
                      "Incompatible Num Bands");

  // get write extents from destOffset and src size
  // intersect it with the valid extents of the dest tile
  khExtents<std::uint32_t> writeExtents
    (khExtents<std::uint32_t>::Intersection
     (khExtents<std::uint32_t>(RowColOrder,
                        destOffset.row(),
                        destOffset.row() + srcExtents.height(),
                        destOffset.col(),
                        destOffset.col() + srcExtents.width()),
      khExtents<std::uint32_t>(khOffset<std::uint32_t>(RowColOrder, 0, 0),
                        khSize<std::uint32_t>(DestTile::TileWidth,
                                       DestTile::TileHeight))));

  // get read extents from src offset and new write size
  khExtents<std::uint32_t> readExtents(srcExtents.origin(),
                                writeExtents.size());

  for (std::uint32_t y = 0; y < readExtents.height(); ++y) {
    std::uint32_t readPos = (readExtents.beginRow() + y) * SrcTile::TileWidth +
                     readExtents.beginCol();
    std::uint32_t writePos = (writeExtents.beginRow() + y) * DestTile::TileWidth +
                      writeExtents.beginCol();
    std::uint32_t width = readExtents.width();
    while (width > 0) {
      PixelCaster<
        typename DestTile::PixelType,
        typename SrcTile::PixelType,
        SrcTile::NumComp>::CastPixel(dest.bufs, writePos,
                                     src.bufs, readPos);
      --width;
      ++readPos;
      ++writePos;
    }
  }
}


#endif /* __CastTile_h */
