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


/******************************************************************************
File:        InsetInfo.cpp
Description:

Changes:
Description: Support for terrain "overlay" projects.

******************************************************************************/

#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_EXTENTINDEXES_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_EXTENTINDEXES_H_

#include <string>
#include <functional>
#include "common/khTileAddr.h"
#include "common/quadtreepath.h"
#include "common/khExtents.h"
#include "common/khInsetCoverage.h"
#include "fusion/autoingest/sysman/InsetInfo.h"
#include "fusion/autoingest/sysman/InsetTilespaceIndex.h"

// // TODO - pack this struct to fit in a tidy 64 bit "size_t" for optimal performance
// struct  TilespaceMBR { 
//     byte projectionType,  // TODO - Flat, Mercator, etc.  Reuse Define  a definition other headers?
//     byte maxDepth,
//     uint32 quadTreeBits
// };


class InsetTilespaceIndex {

public:

    QuadtreePath add(const khExtents <uint32> &extents);

    QuadtreePath getQuadtreeMBR(const khExtents <uint32> extents);

    std::vector <khExtents<uint32>*> intersectingExtents(const QuadtreePath tilespaceMBR, uint32 minLevel, uint32 maxLevel);

    std::vector <uint64> intersectingExtentsHashes(const QuadtreePath tilespaceMBR, uint32 minLevel, uint32 maxLevel);


protected:
    // qtHashes  internal to this class are for implemented efficiency. In QuadtreePath and in TileAddr, The path and
    // the levels are represented as MSB and LSB bits, respectively.  In qtHash, the MSB bits represent the level,
    // so they can be used to sort the Quadtree references as uint64 words.
    // This make it more efficient to sort through all quadtreepaths
    // based on zoom levels, which in turn would let us start with leaf-node
    // insets (i.e., highest zoom) first.

    inline uint64 qtHash(const khExtents <uint32> e) {
        return qtHash(getQuadtreeMBR(e));
    }


    inline uint64 qtHash(const QuadtreePath qtp) {

        /* The qtHash, used only internally within this class to
        perform bitwise operations to partition, sort and rapidly identify
        Extents by their Quadtree minimum Bounding Rectangle.
        */
      return qtp.internalPath();
    }

    inline QuadtreePath hashToQuadtreePath(uint64 qtHash) {

        QuadtreePath qtp(qtHash);
        return qtp;
    }
};

#endif 


