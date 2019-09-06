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

const int MAX_LEVEL = 24;

class InsetTilespaceIndex {

protected:
    std::map <QuadtreePath, std::vector<const khExtents <uint32> *>> _mbrExtentsVecMap;
public:
    InsetTilespaceIndex() = default;

    QuadtreePath add(const khExtents <uint32> &extents);

    QuadtreePath getQuadtreeMBR(const khExtents<uint32>& extents, int& level, const int max_level);

    std::vector<const khExtents < uint32>* >

    intersectingExtents(const QuadtreePath tilespaceMBR, uint32 minLevel, uint32 maxLevel);

    std::vector <QuadtreePath>
    intersectingExtentsQuadtreePaths(const QuadtreePath tilespaceMBR, uint32 minLevel, uint32 maxLevel);

protected:

};

#endif 


