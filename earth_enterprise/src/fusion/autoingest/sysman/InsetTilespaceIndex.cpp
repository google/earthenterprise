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
File:        InsetTilespaceIndex.cpp
Description:

Changes:


******************************************************************************/
#include "fusion/autoingest/sysman/InsetTilespaceIndex.h"
#include "common/khException.h"
#include <boost/range/sub_range.hpp>
#include <boost/range/as_literal.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/any_range.hpp>
#include <map>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <assert.h>
/*
QuadtreePath InsetTilespaceIndex::add(const khExtents <uint32> &extents) {
    int level;
    QuadtreePath quadtreeMbr = getQuadtreeMBR(extents, level, MAX_LEVEL);

    //std::vector<const khExtents<uint32>*> mbrExtentsVec = _mbrExtentsVecMap.find( mbrHash );
    std::vector<const khExtents <uint32> *> *mbrExtentsVec;
    std::map<QuadtreePath, std::vector<const khExtents <uint32> *>>::iterator it;
    it = _mbrExtentsVecMap.find(quadtreeMbr);

    if (it == _mbrExtentsVecMap.end()) {
        mbrExtentsVec = new std::vector<const khExtents <uint32> *>();
        // _mbrExtentsVecMap.insert(  <uint64, std::vector<const khExtents <uint32>*>>::value_type(  mbrHash, *mbrExtentsVec );
        _mbrExtentsVecMap.insert({quadtreeMbr, *mbrExtentsVec});
    } else {
        mbrExtentsVec = &(it->second);
    }
    mbrExtentsVec->push_back(&extents);
    return quadtreeMbr;
}
*/
QuadtreePath InsetTilespaceIndex::getQuadtreeMBR(const khExtents<double>& extents, int& level, const int max_level) {
    double north = 180;
    double south = -180;
    double west = -180;
    std::string base_path = "";
    char next_qt_node;
    for (level = 0; level < max_level; level += 1) {
      double level_dim_size = pow(2, level);
      double qt_node_size = 180.0 / level_dim_size;
      double north_south_midpoint = (south + north) / 2.0;
      // Get which sub-node the SW vertex is in.
      if (extents.south() <= north_south_midpoint) {
        if (extents.west() <= west + qt_node_size) {
          next_qt_node = '0';
        } else {
          next_qt_node = '1';
        }
      } else {
        if (extents.west() <= west + qt_node_size) {
          next_qt_node = '3';
        } else {
          next_qt_node = '2';
        }
      }
      // Check if NE vertex is in the same sub-node. If
      // not, then break at the node we are at.
      if (extents.north() <= north_south_midpoint) {
        if (extents.east() <= west + qt_node_size) {
          if (next_qt_node != '0') {
            break;
          }
        } else {
          if (next_qt_node != '1') {
            break;
          }
          west += qt_node_size;
        }
        north = north_south_midpoint;
      } else {
        if (extents.east() <= west + qt_node_size) {
          if (next_qt_node != '3') {
            break;
          }
        } else {
          if (next_qt_node != '2') {
            break;
          }
          west += qt_node_size;
        }
        south = north_south_midpoint;
      }
      // If still contained, descend to the next level of the tree.
      (base_path) += next_qt_node;
    }

    return QuadtreePath(base_path);

}

std::vector <QuadtreePath>
InsetTilespaceIndex::intersectingExtentsQuadtreePaths(QuadtreePath quadtreeMbr, uint32 minLevel, uint32 maxLevel) {
    //uint64 mbrHash = qtpath.internalPath();
    //std::vector <QuadtreePath>  mbrHashVec = _mbrExtentsVecMap.getKeys();
    std::vector <QuadtreePath> mbrHashVec;
    boost::copy(_mbrExtentsVecMap | boost::adaptors::map_keys,
                std::back_inserter(mbrHashVec));
    std::vector <QuadtreePath> intersectingMbrHashes;
    // TODO - redo this section to use bitwise filtering and partitioning using the QuadtreePath's internal path_
    // bits, as this will be most expeditions.  However, this requires  access to private constructors and data.
    for (QuadtreePath &otherMbr : mbrHashVec) {
        if (otherMbr.Level() >= minLevel && otherMbr.Level() <= maxLevel) {
            if (otherMbr.IsAncestorOf(quadtreeMbr) || quadtreeMbr.IsAncestorOf(otherMbr)) {
                intersectingMbrHashes.push_back(otherMbr);
            }
        }
    }
    return intersectingMbrHashes;
}


std::vector<const khExtents < uint32>*>

InsetTilespaceIndex::intersectingExtents(const QuadtreePath quadtreeMbr, uint32 minLevel, uint32 maxLevel) {
    std::vector <QuadtreePath> intersectingQuadtreeMbrs = intersectingExtentsQuadtreePaths(quadtreeMbr, minLevel,
                                                                                           maxLevel);
    std::vector < const khExtents < uint32 > * > intersectingExtentsVec;
    for (auto otherMbr : intersectingQuadtreeMbrs) {
        std::vector<const khExtents <uint32> *> extentsVec = _mbrExtentsVecMap[otherMbr];
        intersectingExtentsVec.insert(intersectingExtentsVec.end(), extentsVec.begin(), extentsVec.end());
    };
    return intersectingExtentsVec;
}





