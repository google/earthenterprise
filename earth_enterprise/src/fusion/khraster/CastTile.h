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


#ifndef __CastTile_h
#define __CastTile_h

#include "khRasterTile.h"
#include <khAssert.h>
#include <khCalcHelper.h>

// ****************************************************************************
// ***  PixelCaster - Helper class
// ***
// ***    This class is a specialized template used during the
// ***  type casting of tiles. The purpose of this template is to allow
// ***  efficient code inside the tight loop of the cast.
// ***    These calls will be completely inlined.
// ****************************************************************************
template <class T, class U, uint numcomp>
class PixelCaster { };
// specialized for numcomp == 1
template <class T, class U>
class PixelCaster<T, U, 1>
{
 public:
  inline static void CastPixel(T *const dest[], uint32 destPos,
                               const U *const src[], uint32 srcPos) {
    dest[0][destPos] = ClampRange<T>(src[0][srcPos]);
  }
};
// specialized for numcomp == 3
template <class T, class U>
class PixelCaster<T, U, 3>
{
 public:
  inline static void CastPixel(T *const dest[], uint32 destPos,
                               const U *const src[], uint32 srcPos) {
    dest[0][destPos] = ClampRange<T>(src[0][srcPos]);
    dest[1][destPos] = ClampRange<T>(src[1][srcPos]);
    dest[2][destPos] = ClampRange<T>(src[2][srcPos]);
  }
};


// ****************************************************************************
// ***  CastTile
// ***
// ***  Cast a tile from one type to another (typically int16 <-> float for
// ***  heightmaps). Calls to this function should be completely inlined.
// ****************************************************************************
template <class DestTile, class SrcTile>
inline void
CastTile(DestTile &dest, const SrcTile &src)
{
  COMPILE_TIME_ASSERT(DestTile::TileWidth == SrcTile::TileWidth,
                      IncompatibleTileWidth);
  COMPILE_TIME_ASSERT(DestTile::TileHeight == SrcTile::TileHeight,
                      IncompatibleTileHeight);

  for (uint32 i = 0; i < DestTile::BandPixelCount; ++i) {
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
  for (uint32 i = 0; i < DestTile::BandPixelCount; ++i) {
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
      CastTile(dest, (const uint8 *const *)srcBufs);
      break;
    case khTypes::Int8:
      CastTile(dest, (const int8 *const *)srcBufs);
      break;
    case khTypes::UInt16:
      CastTile(dest, (const uint16 *const *)srcBufs);
      break;
    case khTypes::Int16:
      CastTile(dest, (const int16 *const *)srcBufs);
      break;
    case khTypes::UInt32:
      CastTile(dest, (const uint32 *const *)srcBufs);
      break;
    case khTypes::Int32:
      CastTile(dest, (const int32 *const *)srcBufs);
      break;
    case khTypes::UInt64:
      CastTile(dest, (const uint64 *const *)srcBufs);
      break;
    case khTypes::Int64:
      CastTile(dest, (const int64 *const *)srcBufs);
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
CastBuf(T * const dest, const U * const src, uint num)
{
  for (uint pos = 0; pos < num; ++pos) {
    dest[pos] = ClampRange<T>(src[pos]);
  }
}

template <class T>
inline void
CastBuf(T * const dest, void* src, khTypes::StorageEnum srcType, uint num)
{
  switch (srcType) {
    case khTypes::UInt8:
      CastBuf(dest, (const uint8 *const)src, num);
      break;
    case khTypes::Int8:
      CastBuf(dest, (const int8 *const)src, num);
      break;
    case khTypes::UInt16:
      CastBuf(dest, (const uint16 *const)src, num);
      break;
    case khTypes::Int16:
      CastBuf(dest, (const int16 *const)src, num);
      break;
    case khTypes::UInt32:
      CastBuf(dest, (const uint32 *const)src, num);
      break;
    case khTypes::Int32:
      CastBuf(dest, (const int32 *const)src, num);
      break;
    case khTypes::UInt64:
      CastBuf(dest, (const uint64 *const)src, num);
      break;
    case khTypes::Int64:
      CastBuf(dest, (const int64 *const)src, num);
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
        uint num)
{
  switch (destType) {
    case khTypes::UInt8:
      CastBuf((uint8 *const)dest, src, srcType, num);
      break;
    case khTypes::Int8:
      CastBuf((int8 *const)dest, src, srcType, num);
      break;
    case khTypes::UInt16:
      CastBuf((uint16 *const)dest, src, srcType, num);
      break;
    case khTypes::Int16:
      CastBuf((int16 *const)dest, src, srcType, num);
      break;
    case khTypes::UInt32:
      CastBuf((uint32 *const)dest, src, srcType, num);
      break;
    case khTypes::Int32:
      CastBuf((int32 *const)dest, src, srcType, num);
      break;
    case khTypes::UInt64:
      CastBuf((uint64 *const)dest, src, srcType, num);
      break;
    case khTypes::Int64:
      CastBuf((int64 *const)dest, src, srcType, num);
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
  for (uint32 i = 0; i < TileType::BandPixelCount; ++i) {
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
CopySubtile(DestTile &dest, const khOffset<uint32> &destOffset,
            const SrcTile &src, const khExtents<uint32> &srcExtents)
{
  COMPILE_TIME_ASSERT(DestTile::NumComp == SrcTile::NumComp,
                      IncompatibleNumBands);

  // get write extents from destOffset and src size
  // intersect it with the valid extents of the dest tile
  khExtents<uint32> writeExtents
    (khExtents<uint32>::Intersection
     (khExtents<uint32>(RowColOrder,
                        destOffset.row(),
                        destOffset.row() + srcExtents.height(),
                        destOffset.col(),
                        destOffset.col() + srcExtents.width()),
      khExtents<uint32>(khOffset<uint32>(RowColOrder, 0, 0),
                        khSize<uint32>(DestTile::TileWidth,
                                       DestTile::TileHeight))));

  // get read extents from src offset and new write size
  khExtents<uint32> readExtents(srcExtents.origin(),
                                writeExtents.size());

  for (uint32 y = 0; y < readExtents.height(); ++y) {
    uint32 readPos = (readExtents.beginRow() + y) * SrcTile::TileWidth +
                     readExtents.beginCol();
    uint32 writePos = (writeExtents.beginRow() + y) * DestTile::TileWidth +
                      writeExtents.beginCol();
    uint32 width = readExtents.width();
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
