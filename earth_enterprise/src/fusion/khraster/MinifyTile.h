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

#ifndef __MinifyTile_h
#define __MinifyTile_h

#include "khRasterTile.h"
#include <khCalcHelper.h>

// ****************************************************************************
// ***  PixelAverager - Helper class
// ***
// ***  This class is a highly specialized template used during the
// ***  minification of tiles. The purpose of this template is to allow
// ***  efficient code inside the very tight nested loops of the minification.
// ****************************************************************************
template <class T, class Averager, unsigned int numcomp>
class PixelAverager { };

// specialized for numcomp == 1
template <class T, class Averager>
class PixelAverager<T, Averager, 1>
{
 public:
  inline static void Average(T *const dest[],
                             std::uint32_t to,
                             const T *const src[],
                             std::uint32_t from1,
                             std::uint32_t from2,
                             std::uint32_t from3,
                             std::uint32_t from4,
                             Averager averager) {
    dest[0][to] = averager(src[0][from1],
                           src[0][from2],
                           src[0][from3],
                           src[0][from4]);
  }
};

// specialized for numcomp == 3
template <class T, class Averager>
class PixelAverager<T, Averager, 3>
{
 public:
  inline static void Average(T *const dest[],
                             std::uint32_t to,
                             const T *const src[],
                             std::uint32_t from1,
                             std::uint32_t from2,
                             std::uint32_t from3,
                             std::uint32_t from4,
                             Averager averager) {
    dest[0][to] = averager(src[0][from1],
                           src[0][from2],
                           src[0][from3],
                           src[0][from4]);
    dest[1][to] = averager(src[1][from1],
                           src[1][from2],
                           src[1][from3],
                           src[1][from4]);
    dest[2][to] = averager(src[2][from1],
                           src[2][from2],
                           src[2][from3],
                           src[2][from4]);
  }
};


// ****************************************************************************
// ***  MinifyTile
// ****************************************************************************
template <class TileType, class Averager>
void
MinifyTile(TileType &destTile, unsigned int quad, const TileType &srcTile,
           Averager averager)
{
  std::uint32_t Qoffset = QuadtreePath::QuadToBufferOffset(quad, TileType::TileWidth,
                                                    TileType::TileHeight);

  for (unsigned int j = 0; j < TileType::TileHeight/2; ++j) {
    std::uint32_t to    = Qoffset + (j * TileType::TileWidth);
    std::uint32_t from1 = (j * 2)     * TileType::TileWidth;
    std::uint32_t from2 = (j * 2 + 1) * TileType::TileWidth;

    for(unsigned int k = 0; k < TileType::TileWidth/2; k++) {
      PixelAverager<
        typename TileType::PixelType,
        Averager,
        TileType::NumComp>::Average(destTile.bufs,
                                    to,
                                    srcTile.bufs,
                                    from1,
                                    from1+1,
                                    from2,
                                    from2+1,
                                    averager);
      ++to;
      from1 += 2;
      from2 += 2;
    }
  }
}

#endif /* __MinifyTile_h */
