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


#ifndef __CachedRasterInsetReaderImpl_h
#define __CachedRasterInsetReaderImpl_h

#include "CachedRasterInsetReader.h"
#include "MagnifyTile.h"
#include <khraster/CastTile.h>


// ****************************************************************************
// ***  ReadTile routines
// ****************************************************************************
template <class TileType_, class MagnifyWeighting>
void
CachingProductTileReader_ImageryAlpha<TileType_, MagnifyWeighting>
::ReadTile(const khRasterProductLevel *prodLev,
           const khTileAddr &targetAddr,
           TileType &dstTile,
           bool fillMissingWithZero)
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

  // figure out which tile to use for the read
  TileType *tiles[2] = {
    &dstTile,
    &tmpTile
  };
  unsigned int tileIndex = (numMagnify % 2) ? 1 : 0;

  ProdTileKey key(prodLev, readAddr);
  khRefGuard<CachedProductTileImpl<TileType> > found;
  // If we need to magnify, maybe we've already cached this source tile
  if (numMagnify && tileCache.Find(key, found)) {
    CopyTile(*tiles[tileIndex], found->tile);
  } else {
    // read tile - will convert to desired type if necessary
    if (fillMissingWithZero &&
        !prodLev->tileExtents().ContainsRowCol(readAddr.row,
                                               readAddr.col)) {
      dstTile.FillWithZeros();
      return;
    } else {
      readerCache.ReadTile(prodLev, readAddr.row, readAddr.col,
                           *tiles[tileIndex]);
    }

    if (numMagnify) {
      // If we have to magnify, remember this src tile, we'll likely
      // need it again
      found = khRefGuardFromNew
              (new CachedProductTileImpl<TileType>());
      CopyTile(found->tile, *tiles[tileIndex]);
      tileCache.Add(key, found);
    }
  }

  // magnify data tile if necessary
  if (numMagnify) {
    // will use tiles for storage, pulling first from tileIndex
    // this will leave the result in dstTile
    MagnifyTile_ImageryAlpha<MagnifyWeighting, TileType>
      (targetAddr, readLevel, tiles, tileIndex);
  }
}

#endif /* __CachedRasterInsetReaderImpl_h */
