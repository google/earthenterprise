// Copyright 2017 Google Inc.
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
 const khOffset<uint32> &dstOffset,
 const khExtents<uint32> &srcExtents,
 const khExtents<uint32> &fallbackExtents)
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
                          uint quad)
{

  typedef khCalcHelper<ExpandedHeightmapTile::PixelType> Helper;
  static const uint TileWidth  = ExpandedHeightmapTile::TileWidth;
  //    static const uint TileHeight = ExpandedHeightmapTile::TileHeight;
  static const uint ProdTileWidth  = ExpandedHeightmapTile::TileWidth - 1;
  static const uint ProdTileHeight = ExpandedHeightmapTile::TileHeight - 1;

  // use 'quad' to determine the offsets into the source tile
  // quads are numbered 0 : miny, minx
  //                    1 : miny, maxx
  //                    2 : maxy, minx
  //                    3 : maxy, maxx
  const uint beginSrcX = (quad & 0x1) ? ProdTileWidth/2 : 0;
  const uint endSrcX   = beginSrcX + ProdTileWidth/2;
  const uint beginSrcY = (quad & 0x2) ? ProdTileHeight/2 : 0;
  const uint endSrcY   = beginSrcY + ProdTileHeight/2;

  // offset variables that represent where we write the results
  uint32 to0 = 0;
  uint32 to1 = TileWidth;

  // traverse each row in the source quadrant
  uint srcY;
  for (srcY = beginSrcY; srcY < endSrcY; ++srcY) {
    // calculate the buffer offset for the 2 source rows we'll use
    uint32 thisOffY = srcY*TileWidth;
    uint32 nextOffY = thisOffY+TileWidth;

    // traverse each column in the source quadrant
    uint srcX;
    for (srcX = beginSrcX; srcX < endSrcX; ++srcX) {
      // calculate the buffer offset for the 3 source columns we'll use
      uint32 thisOffX = srcX;
      uint32 nextOffX = thisOffX+1;

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
      uint32 thisOffX = srcX;

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
    uint32 thisOffY = srcY*TileWidth;

    // traverse each column in the source quadrant
    uint srcX;
    for (srcX = beginSrcX; srcX < endSrcX; ++srcX) {
      // calculate the buffer offset for the 3 source columns we'll use
      uint32 thisOffX = srcX;
      uint32 nextOffX = thisOffX+1;

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
      uint32 thisOffX = srcX;

      //lower left
      destTile.bufs[0][to0++] = srcTile.bufs[0][thisOffY+thisOffX];
    }
  }
}


static void
MagnifyTile_Heightmap(const khTileAddr &targetAddr,
                      uint srcLevel,
                      ExpandedHeightmapTile* tiles[],
                      uint startIndex)
{
  while (srcLevel < targetAddr.level) {
    // Determine the magnification quad by mapping the
    // target row/col to the level just above the src_level
    khTileAddr tmpAddr(targetAddr.MinifiedToLevel(srcLevel+1));

    // Then using even/odd addresses to determine
    // the quadrant:
    // SW:0  SE:1  NW:2  NE:3
    uint quad = (tmpAddr.row & 0x1)*2 + (tmpAddr.col & 0x1);

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

  uint readLevel = prodLev->levelnum();
  assert(readLevel <= targetAddr.level);
  uint numMagnify = targetAddr.level - readLevel;

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
    uint tileIndex = (numMagnify % 2) ? 1 : 0;

    // fetch primary tile and copy it into the oversized heightmap buf
    CachedTile found = FetchAndCacheProductTile(prodLev, readAddr);
    CopySubtile(*tiles[tileIndex],
                khOffset<uint32>(RowColOrder, 0, 0),
                found->tile,
                khExtents<uint32>(RowColOrder,
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
                       khOffset<uint32>(RowColOrder,
                                        TileType::TileHeight, 0),
                       // srcExtents
                       khExtents<uint32>(RowColOrder,
                                         0, 1,
                                         0, TileType::TileWidth),
                       // fallbackExtents
                       khExtents<uint32>(RowColOrder,
                                         TileType::TileHeight-1,
                                         TileType::TileHeight,
                                         0, TileType::TileWidth));
    }

    if (needExtraPixelsOnRight) {
      FetchExtraPixels(prodLev,
                       readAddr.RightAddr(tile_space),
                       *tiles[tileIndex],
                       // dstOffset
                       khOffset<uint32>(RowColOrder,
                                        0, TileType::TileWidth),
                       // srcExtents
                       khExtents<uint32>(RowColOrder,
                                         0, TileType::TileHeight,
                                         0, 1),
                       // fallbackExtents
                       khExtents<uint32>(RowColOrder,
                                         0, TileType::TileHeight,
                                         TileType::TileWidth-1,
                                         TileType::TileWidth));
    }

    if (needExtraPixelsOnTop && needExtraPixelsOnRight) {
      FetchExtraPixels(prodLev,
                       readAddr.UpperRightAddr(tile_space),
                       *tiles[tileIndex],
                       // dstOffset
                       khOffset<uint32>(RowColOrder,
                                        TileType::TileHeight,
                                        TileType::TileWidth),
                       // srcExtents
                       khExtents<uint32>(RowColOrder,
                                         0, 1, 0, 1),
                       // fallbackExtents
                       khExtents<uint32>(RowColOrder,
                                         TileType::TileHeight-1,
                                         TileType::TileHeight,
                                         TileType::TileWidth-1,
                                         TileType::TileWidth));
    }

    MagnifyTile_Heightmap(targetAddr, readLevel, tiles, tileIndex);

    CopySubtile(dstTile,
                khOffset<uint32>(RowColOrder, 0, 0),
                tmpTile1,
                khExtents<uint32>(RowColOrder,
                                  0, TileType::TileHeight,
                                  0, TileType::TileWidth));
  }
}
