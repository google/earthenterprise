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


#ifndef KHSRC_FUSION_KHRASTER_MAGNIFYTILE_H__
#define KHSRC_FUSION_KHRASTER_MAGNIFYTILE_H__

#include <notify.h>
#include <khTileAddr.h>
#include <khraster/MagnifyQuadrant.h>


// ****************************************************************************
// ***  MagnifyTile
// ****************************************************************************
template <class MagnifyWeighting, class TileType>
static void
MagnifyTile_ImageryAlpha(const khTileAddr &targetAddr,
                         unsigned int srcLevel,
                         TileType* tiles[],
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

    MagnifyQuadrant_ImageryAlpha<MagnifyWeighting>
      (*tiles[(startIndex+1)%2], *tiles[startIndex], quad);

    // switch which buffer is my source and which is my destination
    startIndex = (startIndex+1) %2;

    srcLevel++;
  }
}

#endif // !KHSRC_FUSION_KHRASTER_MAGNIFYTILE_H__
