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

#ifndef __MagnifyQuadrant_h
#define __MagnifyQuadrant_h

#include "khRasterTile.h"
#include <khCalcHelper.h>


// ****************************************************************************
// ***  Given four pixels - how do I weight them to make another
// ****************************************************************************
template <unsigned int weightLL, unsigned int weightLR, unsigned int weightUL, unsigned int weightUR>
class PixelWeighting2By2
{
 public:
  static const unsigned int WeightLL = weightLL;
  static const unsigned int WeightLR = weightLR;
  static const unsigned int WeightUL = weightUL;
  static const unsigned int WeightUR = weightUR;
  static const unsigned int TotalWeight = WeightLL + WeightLR + WeightUL + WeightUR;
};

// ****************************************************************************
// ***  WeightedPixelAverager - Helper class
// ***
// ***  This class is a highly specialized template used during the
// ***  magnification of tiles. The purpose of this template is to allow
// ***  efficient code inside the very tight nested loops of the
// ***  magnification.  All of this should be inlined away, resulting in a
// ***  loop as effient as if it had been hand coded.
// ****************************************************************************
template <class T, class PixelWeighting, unsigned int numcomp>
class WeightedPixelAverager { };

// specialized for numcomp == 1
template <class T, class PixelWeighting>
class WeightedPixelAverager<T, PixelWeighting, 1>
{
 public:
  inline static void Average(T *const dest[],
                             std::uint32_t to,
                             const T *const src[],
                             std::uint32_t from1,
                             std::uint32_t from2,
                             std::uint32_t from3,
                             std::uint32_t from4) {
    typedef typename khCalcHelper<T>::AccumType AccumType;
    dest[0][to] =
      (T) ((((AccumType)src[0][from1] * PixelWeighting::WeightLL) +
            ((AccumType)src[0][from2] * PixelWeighting::WeightLR) +
            ((AccumType)src[0][from3] * PixelWeighting::WeightUL) +
            ((AccumType)src[0][from4] * PixelWeighting::WeightUR))
           / (PixelWeighting::TotalWeight));
  }
};

// specialized for numcomp == 3
template <class T, class PixelWeighting>
class WeightedPixelAverager<T, PixelWeighting, 3>
{
 public:
  inline static void Average(T *const dest[],
                             std::uint32_t to,
                             const T *const src[],
                             std::uint32_t from1,
                             std::uint32_t from2,
                             std::uint32_t from3,
                             std::uint32_t from4) {
    typedef typename khCalcHelper<T>::AccumType AccumType;
    dest[0][to] =
      (T) ((((AccumType)src[0][from1] * PixelWeighting::WeightLL) +
            ((AccumType)src[0][from2] * PixelWeighting::WeightLR) +
            ((AccumType)src[0][from3] * PixelWeighting::WeightUL) +
            ((AccumType)src[0][from4] * PixelWeighting::WeightUR))
           / (PixelWeighting::TotalWeight));
    dest[1][to] =
      (T) ((((AccumType)src[1][from1] * PixelWeighting::WeightLL) +
            ((AccumType)src[1][from2] * PixelWeighting::WeightLR) +
            ((AccumType)src[1][from3] * PixelWeighting::WeightUL) +
            ((AccumType)src[1][from4] * PixelWeighting::WeightUR))
           / (PixelWeighting::TotalWeight));
    dest[2][to] =
      (T) ((((AccumType)src[2][from1] * PixelWeighting::WeightLL) +
            ((AccumType)src[2][from2] * PixelWeighting::WeightLR) +
            ((AccumType)src[2][from3] * PixelWeighting::WeightUL) +
            ((AccumType)src[2][from4] * PixelWeighting::WeightUR))
           / (PixelWeighting::TotalWeight));
  }
};



// ****************************************************************************
// ***  What weights do I apply to split one row of pixels into two.
// ***
// ***  Each original pixel ('#' boundary, centered on letters) becomes four
// ***  new pixels ('-' boundary). The value for each new pixel comes from a
// ***  weighted average of neighboring source pixels The weights applied to
// ***  each neighboring pixel (numbers in diagram) favor the closest source
// ***  pixel.
// ***  Notice that it takes 3 rows of source pixels to magnify one row (the
// ***  center one) into two rows.
// ***    ###########################################
// ***    #             #             #             #
// ***    #             #             #             #
// ***    #      a      #      b      #      c      #
// ***    #             #             #             #
// ***    #             #             #             #
// ***    ###########################################
// ***    #             # 1a 3b| 3b 1c#             #
// ***    #             # 3d 9e| 9e 3f#             #
// ***    #      d      #------e------#      f      #
// ***    #             # 3d 9e| 9e 3f#             #
// ***    #             # 1g 3h| 3h 1i#             #
// ***    ###########################################
// ***    #             #             #             #
// ***    #             #             #             #
// ***    #      g      #      h      #      i      #
// ***    #             #             #             #
// ***    #             #             #             #
// ***    ###########################################
// ****************************************************************************
template <class pixelLL, class pixelLR, class pixelUL, class pixelUR>
class PixelCenterMagnifyWeighting
{
 public:
  typedef pixelLL PixelLL;
  typedef pixelLR PixelLR;
  typedef pixelUL PixelUL;
  typedef pixelUR PixelUR;
};
typedef PixelCenterMagnifyWeighting
<PixelWeighting2By2<1, 3, 3, 9>,     // lower left
 PixelWeighting2By2<3, 1, 9, 3>,     // lower right
 PixelWeighting2By2<3, 9, 1, 3>,     // upper left
 PixelWeighting2By2<9, 3, 3, 1> >    // upper right
ImageryMagnifyWeighting;



// ****************************************************************************
// ***  MagnifyQuadrant_ImageryAlpha
// ***
// ***  Magnify the specified quadrant of the srcTile to fill destTile.
// ***
// ***  Use a PixelCenterMagnifyWeighting (defined above) to magnify one
// ***  quadrant of a source tile into another tile. We don't have neighbor
// ***  tiles to get the extra edge pixels, just clamp this tile at the edges.
// ***                                 Source buffer
// ***                     +---+---+---+---+---+---+---+---+---+---+
// ***                     |   |   |   |   |   |   |   |   |   |   |
// ***                     +---+---+---+---+---+---+---+---+---+---+
// ***                     |   |   |   |   |   |   |   |   |   |   |
// ***                     +---+---+---+---+---+---+---+---+---+---+
// ***                     |   |   |   |   |   |   |   |   |   |   |
// ***                     +---+---+---+---+---+---+---+---+---+---+
// ***                     |   |   |   |   |   |   |   |   |   |   |
// ***                     +---+---+---+---+---+---+---+---+---+---+
// ***       nextOffY ---> |   |   |   |   |   |   |   |   |   |   |
// ***                     +---+---+---+---+---+---+---+---+---+---+
// ***(srcY) thisOffY ---> |   |   |   | @ |   |   |   |   |   |   |
// ***                     +---+---+---+---+---+---+---+---+---+---+
// ***       prevOffY ---> |   |   |   |   |   |   |   |   |   |   |
// ***                     +---+---+---+---+---+---+---+---+---+---+
// ***                               ^   ^   ^
// ***                               |   |    \ nextOffX
// ***                               |    \ thisOffX (srcX)
// ***                                \ prevOffX
// ***
// ***   srcY and srcX are walked through the source quadrant and
// ***   the this/next/prev X/Y offsets are adjusted to match. If there is no
// ***   next or prev, it will just point to 'this' (clamping).
// ***   Each iteration through the nexted loops generates all four new pixels
// ***   at the intersection of srcY and srcX.
// ****************************************************************************
template <class MagnifyWeighting, class TileType>
void
MagnifyQuadrant_ImageryAlpha(TileType &destTile,
                             const TileType &srcTile, unsigned int quad)
{

  // use 'quad' to determine the offsets into the source tile
  // quads are numbered 0 : miny, minx
  //                    1 : miny, maxx
  //                    2 : maxy, minx
  //                    3 : maxy, maxx
  const unsigned int beginSrcX = (quad & 0x1) ? TileType::TileWidth/2 : 0;
  const unsigned int endSrcX   = beginSrcX + TileType::TileWidth/2;
  const unsigned int beginSrcY = (quad & 0x2) ? TileType::TileHeight/2 : 0;
  const unsigned int endSrcY   = beginSrcY + TileType::TileHeight/2;

  // offset variables that represent where we write the results
  std::uint32_t to0 = 0;
  std::uint32_t to1 = TileType::TileWidth;

  // traverse each row in the source quadrant
  for (unsigned int srcY = beginSrcY; srcY < endSrcY; ++srcY) {
    // calculate the buffer offset for the 3 source rows we'll use
    std::uint32_t thisOffY = srcY*TileType::TileWidth;
    std::uint32_t prevOffY = (srcY==0)
                      ? thisOffY  // clamp bottom
                      : thisOffY-TileType::TileWidth;
    std::uint32_t nextOffY = (srcY==TileType::TileHeight-1)
                      ? thisOffY  // clamp top
                      : thisOffY+TileType::TileWidth;

    // traverse each column in the source quadrant
    for (unsigned int srcX = beginSrcX; srcX < endSrcX; ++srcX) {
      // calculate the buffer offset for the 3 source columns we'll use
      std::uint32_t thisOffX   = srcX;
      std::uint32_t prevOffX = (srcX==0)
                        ? thisOffX  // clamp left
                        : thisOffX-1;
      std::uint32_t nextOffX = (srcX==TileType::TileWidth-1)
                        ? thisOffX  // clamp right
                        : thisOffX+1;

      // make the four result pixels by combining the various source
      // pixels with the appropriate weights
      //lower left
      WeightedPixelAverager<
        typename TileType::PixelType,
        typename MagnifyWeighting::PixelLL,
        TileType::NumComp>::Average
        (destTile.bufs,
         to0++,
         srcTile.bufs,
         prevOffY+prevOffX,
         prevOffY+thisOffX,
         thisOffY+prevOffX,
         thisOffY+thisOffX);
      // lower right
      WeightedPixelAverager<
        typename TileType::PixelType,
        typename MagnifyWeighting::PixelLR,
        TileType::NumComp>::Average
        (destTile.bufs,
         to0++,
         srcTile.bufs,
         prevOffY+thisOffX,
         prevOffY+nextOffX,
         thisOffY+thisOffX,
         thisOffY+nextOffX);
      // upper left
      WeightedPixelAverager<
        typename TileType::PixelType,
        typename MagnifyWeighting::PixelUL,
        TileType::NumComp>::Average
        (destTile.bufs,
         to1++,
         srcTile.bufs,
         thisOffY+prevOffX,
         thisOffY+thisOffX,
         nextOffY+prevOffX,
         nextOffY+thisOffX);
      // upper right
      WeightedPixelAverager<
        typename TileType::PixelType,
        typename MagnifyWeighting::PixelUR,
        TileType::NumComp>::Average
        (destTile.bufs,
         to1++,
         srcTile.bufs,
         thisOffY+thisOffX,
         thisOffY+nextOffX,
         nextOffY+thisOffX,
         nextOffY+nextOffX);

    }
    to0 += TileType::TileWidth;
    to1 += TileType::TileWidth;
  }
}

#endif /* __MagnifyQuadrant_h */
