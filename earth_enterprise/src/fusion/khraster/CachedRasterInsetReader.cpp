// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include "CachedRasterInsetReader.h"
#include <khraster/CastTile.h>


// ****************************************************************************
// ***  NOTE:
// ***
// ***  Some of the routines you would expect to find here are in
// ***  CachedRasterInsetReaderImpl.h
// ****************************************************************************


void
CachingProductTileReader_Heightmap::FetchExtraPixels
(const khRasterProductLevel *prodLev,
 const khTileAddr &addr,
 ExpandedTileType &dstTile,
 const khOffset<std::uint32_t> &dstOffset,
 const khExtents<std::uint32_t> &srcExtents,
 const khExtents<std::uint32_t> &fallbackExtents)
{
  if (prodLev->tileExtents().ContainsRowCol(addr.row, addr.col)) {
    // we have the desired tile, fetch in and copy the extra pixels
    CachedTile found = FetchAndCacheProductTile(prodLev, addr);
    CopySubtile(dstTile, dstOffset, found->tile, srcExtents);
  } else {
    // we don't have the tile, copy the fallback pixels
    CopySubtile(dstTile, dstOffset, dstTile, fallbackExtents);
  }
}



CachingProductTileReader_Heightmap::CachedTile
CachingProductTileReader_Heightmap::FetchAndCacheProductTile
(const khRasterProductLevel *prodLev,
 const khTileAddr &addr)
{
  ProdTileKey key(prodLev, addr);
  CachedTile found;

  if (!tileCache.Find(key, found)) {
    found = khRefGuardFromNew
            (new CachedProductTileImpl<TileType>());

    // read tile - will convert to desired type if necessary
    readerCache.ReadTile(prodLev, addr.row, addr.col, found->tile);

    tileCache.Add(key, found);
  }
  return found;
}


// ****************************************************************************
// ***  Like MagnifyQuadrant_ImageryAlpha
// ***  Except it works with an ExpandedHeightmapTile 1025x1025 and has
// ***  Specialized averaging logic to match heighmaps
// ***
// ***  ... More later ...
// ***
// ****************************************************************************
void
MagnifyQuadrant_Heightmap(ExpandedHeightmapTile &destTile,
                          const ExpandedHeightmapTile &srcTile,
                          unsigned int quad)
{

  typedef khCalcHelper<ExpandedHeightmapTile::PixelType> Helper;
  static const unsigned int TileWidth  = ExpandedHeightmapTile::TileWidth;
  //    static const unsigned int TileHeight = ExpandedHeightmapTile::TileHeight;
  static const unsigned int ProdTileWidth  = ExpandedHeightmapTile::TileWidth - 1;
  static const unsigned int ProdTileHeight = ExpandedHeightmapTile::TileHeight - 1;

  // use 'quad' to determine the offsets into the source tile
  // quads are numbered 0 : miny, minx
  //                    1 : miny, maxx
  //                    2 : maxy, minx
  //                    3 : maxy, maxx
  const unsigned int beginSrcX = (quad & 0x1) ? ProdTileWidth/2 : 0;
  const unsigned int endSrcX   = beginSrcX + ProdTileWidth/2;
  const unsigned int beginSrcY = (quad & 0x2) ? ProdTileHeight/2 : 0;
  const unsigned int endSrcY   = beginSrcY + ProdTileHeight/2;

  // offset variables that represent where we write the results
  std::uint32_t to0 = 0;
  std::uint32_t to1 = TileWidth;

  // traverse each row in the source quadrant
  unsigned int srcY;
  for (srcY = beginSrcY; srcY < endSrcY; ++srcY) {
    // calculate the buffer offset for the 2 source rows we'll use
    std::uint32_t thisOffY = srcY*TileWidth;
    std::uint32_t nextOffY = thisOffY+TileWidth;

    // traverse each column in the source quadrant
    unsigned int srcX;
    for (srcX = beginSrcX; srcX < endSrcX; ++srcX) {
      // calculate the buffer offset for the 3 source columns we'll use
      std::uint32_t thisOffX = srcX;
      std::uint32_t nextOffX = thisOffX+1;

      // make the four result pixels by combining the various source
      //lower left
      destTile.bufs[0][to0++] = srcTile.bufs[0][thisOffY+thisOffX];

      // lower right
      destTile.bufs[0][to0++] =
        Helper::AverageOf2(srcTile.bufs[0][thisOffY+thisOffX],
                           srcTile.bufs[0][thisOffY+nextOffX]);

      // upper left
      destTile.bufs[0][to1++] =
        Helper::AverageOf2(srcTile.bufs[0][thisOffY+thisOffX],
                           srcTile.bufs[0][nextOffY+thisOffX]);

      // upper right
      destTile.bufs[0][to1++] =
        Helper::AverageOf2(srcTile.bufs[0][thisOffY+thisOffX],
                           srcTile.bufs[0][nextOffY+nextOffX]);
    }

    // bring over the extra pixles to complete the ExpandedHeightmap row
    {
      std::uint32_t thisOffX = srcX;

      //lower left
      destTile.bufs[0][to0++] = srcTile.bufs[0][thisOffY+thisOffX];

      // upper left
      destTile.bufs[0][to1++] =
        Helper::AverageOf2(srcTile.bufs[0][thisOffY+thisOffX],
                           srcTile.bufs[0][nextOffY+thisOffX]);
    }

    to0 += TileWidth;
    to1 += TileWidth;
  }

  // bring over the extra pixels to fill the last ExpandedHeightmapRow
  {
    // calculate the buffer offset for the 2 source rows we'll use
    std::uint32_t thisOffY = srcY*TileWidth;

    // traverse each column in the source quadrant
    unsigned int srcX;
    for (srcX = beginSrcX; srcX < endSrcX; ++srcX) {
      // calculate the buffer offset for the 3 source columns we'll use
      std::uint32_t thisOffX = srcX;
      std::uint32_t nextOffX = thisOffX+1;

      // make the two result pixels by combining the various source
      //lower left
      destTile.bufs[0][to0++] = srcTile.bufs[0][thisOffY+thisOffX];

      // lower right
      destTile.bufs[0][to0++] =
        Helper::AverageOf2(srcTile.bufs[0][thisOffY+thisOffX],
                           srcTile.bufs[0][thisOffY+nextOffX]);
    }

    // bring over the extra pixel to complete the upper right corner
    {
      std::uint32_t thisOffX = srcX;

      //lower left
      destTile.bufs[0][to0++] = srcTile.bufs[0][thisOffY+thisOffX];
    }
  }
}


static void
MagnifyTile_Heightmap(const khTileAddr &targetAddr,
                      unsigned int srcLevel,
                      ExpandedHeightmapTile* tiles[],
                      unsigned int startIndex)
{
  while (srcLevel < targetAddr.level) {
    // Determine the magnification quad by mapping the
    // target row/col to the level just above the src_level
    khTileAddr tmpAddr(targetAddr.MinifiedToLevel(srcLevel+1));

    // Then using even/odd addresses to determine
    // the quadrant:
    // SW:0  SE:1  NW:2  NE:3
    unsigned int quad = (tmpAddr.row & 0x1)*2 + (tmpAddr.col & 0x1);

    MagnifyQuadrant_Heightmap(*tiles[(startIndex+1)%2],
                              *tiles[startIndex], quad);

    // switch which buffer is my source and which is my destination
    startIndex = (startIndex+1) %2;

    srcLevel++;
  }
}


void
CachingProductTileReader_Heightmap
::ReadTile(const khRasterProductLevel *prodLev,
           const khTileAddr &targetAddr,
           TileType &dstTile, const bool is_mercator)
{
  // TODO: - ??? make this cache all intermediate magnify tiles not just
  // the lowest level? This probably doesn't even matter yet.
  // The later processing costs (blending, merging, compressing) likely
  // dwarf the costs of the magnify.

  unsigned int readLevel = prodLev->levelnum();
  assert(readLevel <= targetAddr.level);
  unsigned int numMagnify = targetAddr.level - readLevel;

  // translate coords from targetLevel to readLevel
  khTileAddr readAddr(targetAddr.MinifiedBy(numMagnify));

  if (!numMagnify) {
    // read tile - will convert to desired type if necessary
    readerCache.ReadTile(prodLev, readAddr.row, readAddr.col, dstTile);
  } else {
    // figure out which tile to use for the read
    ExpandedTileType *tiles[2] = {
      &tmpTile1,
      &tmpTile2
    };
    unsigned int tileIndex = (numMagnify % 2) ? 1 : 0;

    // fetch primary tile and copy it into the oversized heightmap buf
    CachedTile found = FetchAndCacheProductTile(prodLev, readAddr);
    CopySubtile(*tiles[tileIndex],
                khOffset<std::uint32_t>(RowColOrder, 0, 0),
                found->tile,
                khExtents<std::uint32_t>(RowColOrder,
                                  0, TileType::TileHeight,
                                  0, TileType::TileWidth));

    // see if we need to load any extra pixels
    khLevelCoverage targetCov(targetAddr);
    khLevelCoverage readCov(readAddr);
    readCov.magnifyBy(numMagnify);
    bool needExtraPixelsOnTop   =
      (targetCov.extents.endRow() == readCov.extents.endRow());
    bool needExtraPixelsOnRight =
      (targetCov.extents.endCol() == readCov.extents.endCol());

    const khTilespace& tile_space = RasterProductTilespace(is_mercator);
    // load the extra pixels we need
    if (needExtraPixelsOnTop) {
      FetchExtraPixels(prodLev,
                       readAddr.UpperAddr(tile_space),
                       *tiles[tileIndex],
                       // dstOffset
                       khOffset<std::uint32_t>(RowColOrder,
                                        TileType::TileHeight, 0),
                       // srcExtents
                       khExtents<std::uint32_t>(RowColOrder,
                                         0, 1,
                                         0, TileType::TileWidth),
                       // fallbackExtents
                       khExtents<std::uint32_t>(RowColOrder,
                                         TileType::TileHeight-1,
                                         TileType::TileHeight,
                                         0, TileType::TileWidth));
    }

    if (needExtraPixelsOnRight) {
      FetchExtraPixels(prodLev,
                       readAddr.RightAddr(tile_space),
                       *tiles[tileIndex],
                       // dstOffset
                       khOffset<std::uint32_t>(RowColOrder,
                                        0, TileType::TileWidth),
                       // srcExtents
                       khExtents<std::uint32_t>(RowColOrder,
                                         0, TileType::TileHeight,
                                         0, 1),
                       // fallbackExtents
                       khExtents<std::uint32_t>(RowColOrder,
                                         0, TileType::TileHeight,
                                         TileType::TileWidth-1,
                                         TileType::TileWidth));
    }

    if (needExtraPixelsOnTop && needExtraPixelsOnRight) {
      FetchExtraPixels(prodLev,
                       readAddr.UpperRightAddr(tile_space),
                       *tiles[tileIndex],
                       // dstOffset
                       khOffset<std::uint32_t>(RowColOrder,
                                        TileType::TileHeight,
                                        TileType::TileWidth),
                       // srcExtents
                       khExtents<std::uint32_t>(RowColOrder,
                                         0, 1, 0, 1),
                       // fallbackExtents
                       khExtents<std::uint32_t>(RowColOrder,
                                         TileType::TileHeight-1,
                                         TileType::TileHeight,
                                         TileType::TileWidth-1,
                                         TileType::TileWidth));
    }

    MagnifyTile_Heightmap(targetAddr, readLevel, tiles, tileIndex);

    CopySubtile(dstTile,
                khOffset<std::uint32_t>(RowColOrder, 0, 0),
                tmpTile1,
                khExtents<std::uint32_t>(RowColOrder,
                                  0, TileType::TileHeight,
                                  0, TileType::TileWidth));
  }
}
