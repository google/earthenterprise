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
#include "common/khException.h"
#include "autoingest/plugins/RasterProductAsset.h"
#include "autoingest/plugins/RasterProjectAsset.h"
#include "fusion/autoingest/sysman/InsetTilespaceIndex.h"
#include <boost/range/sub_range.hpp>
#include <boost/range/as_literal.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/any_range.hpp>
#include <map>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <assert.h>

// getExtents functions - allow us to get khExtents<double> from the different types
//   we want to work with
khExtents<double> getExtents(const khExtents<double> &extents){
    return extents;
}

khExtents<double> getExtents(const InsetInfo<RasterProductAssetVersion> &insetInfo){
    return insetInfo.degExtents;
}

// khExtents<double> getExtents(const InsetInfoAndIndex &info){
//     return info.ptrInsetInfo->degExtents;
// }

// InsetTilespaceIndex methods
template <class ExtentContainer>
QuadtreePath InsetTilespaceIndex<ExtentContainer>::add(const ExtentContainer &toAdd){
    int level;
    khExtents<double> tempExtents = ::getExtents(toAdd);
    notify(NFY_WARN, "Entered InsetTilespaceIndex<ExtentContainer>::add");
    // Temporary assert while debugging. It's possible that this could be triggered
    // by valid data, but unlikely enough that I think it's still helpful for
    // debugging.
    assert(!(tempExtents.beginX() == 0.0 || tempExtents.endX() == 0.0 ||
             tempExtents.beginY() == 0.0 || tempExtents.endY() == 0.0 ));
    QuadtreePath quadtreeMbr = getQuadtreeMBR(tempExtents, level, MAX_LEVEL);

    //std::vector<const khExtents<uint32>*> mbrExtentsVec = _mbrExtentsVecMap.find( mbrHash );
    //ContainerVector *mbrExtentsVec;
    quadTree.AddElementAtQuadTreePath(quadtreeMbr, &toAdd);
    // typename QuadTreeMap::iterator it;
    // it = _mbrExtentsVecMap.find(quadtreeMbr);

    // if (it == _mbrExtentsVecMap.end()) {
    //     mbrExtentsVec = new ContainerVector();//std::vector<ExtentContainer*>();
    //     _mbrExtentsVecMap.insert({quadtreeMbr, *mbrExtentsVec});
    // } else {
    //     mbrExtentsVec = &(it->second);
    // }
    // mbrExtentsVec->push_back(&toAdd);
    return quadtreeMbr;
}

template <class ExtentContainer>
QuadtreePath InsetTilespaceIndex<ExtentContainer>::getQuadtreeMBR(const khExtents<double> &extents, int &level, const int max_level) {
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

// template <class ExtentContainer>
// std::vector <QuadtreePath>
// InsetTilespaceIndex<ExtentContainer>::intersectingExtentsQuadtreePaths(QuadtreePath quadtreeMbr, uint32 minLevel, uint32 maxLevel) {
//     //uint64 mbrHash = qtpath.internalPath();
//     //std::vector <QuadtreePath>  mbrHashVec = _mbrExtentsVecMap.getKeys();
//     std::vector <QuadtreePath> mbrHashVec;
//     boost::copy(_mbrExtentsVecMap | boost::adaptors::map_keys,
//                 std::back_inserter(mbrHashVec));
//     std::vector <QuadtreePath> intersectingQuadtrees;

//     // TODO - redo this section to use bitwise filtering and partitioning using the QuadtreePath's internal path_
//     // bits, as this will be most expeditions.  However, this requires  access to private constructors and data.
//     // BTree lookups in the mbrHashVec could also bring the time complexity to O(log n)
//     notify(NFY_WARN, "Looping from level %u to %u", minLevel, maxLevel);
//     for (uint32 level = minLevel; level <= maxLevel; level++) {
//         notify(NFY_WARN, "On level %u", level);
//         for (QuadtreePath &otherMbr : mbrHashVec) {
//             //if (otherMbr.Level() >= minLevel && otherMbr.Level() <= maxLevel) {
//                 notify(NFY_WARN, "Comparing %s with %s", quadtreeMbr.AsString().c_str(), otherMbr.AsString().c_str());
//                 if (QuadtreePath::OverlapsAtLevel(quadtreeMbr, otherMbr, level)) {
//                     notify(NFY_WARN, "It's a match!");        
//                     intersectingQuadtrees.push_back(otherMbr);
//                     break;
//                 }
//             //}
//         }
//     }
//     return intersectingQuadtrees;
// }


template <class ExtentContainer>
typename InsetTilespaceIndex<ExtentContainer>::ContainerVector
InsetTilespaceIndex<ExtentContainer>::intersectingExtents(const QuadtreePath quadtreeMbr, uint32 minLevel, uint32 maxLevel) {
    InsetTilespaceIndex<ExtentContainer>::ContainerVector vec;
    vec = quadTree.GetElementsAtQuadTreePath(quadtreeMbr);
    return vec;
    // notify(NFY_WARN, "In InsetTilespaceIndex<ExtentContainer>::intersectingExtents, about to call intersectingExtentsQuadtreePaths");
    // std::vector <QuadtreePath> intersectingQuadtreeMbrs = intersectingExtentsQuadtreePaths(quadtreeMbr, minLevel,
    //                                                                                        maxLevel);
    // notify(NFY_WARN, "In InsetTilespaceIndex<ExtentContainer>::intersectingExtents, got a vector of %zu elements", intersectingQuadtreeMbrs.size());
    // ContainerVector intersectingExtentsVec;
    // for (auto otherMbr : intersectingQuadtreeMbrs) {
    //     ContainerVector extentsVec = _mbrExtentsVecMap[otherMbr];
    //     intersectingExtentsVec.insert(intersectingExtentsVec.end(), extentsVec.begin(), extentsVec.end());
    // }

    // notify(NFY_WARN, "In InsetTilespaceIndex<ExtentContainer> finished work, about to return");
    // return intersectingExtentsVec;
}


template class InsetTilespaceIndex<khExtents<double>>;
template class InsetTilespaceIndex<InsetInfo<RasterProductAssetVersion>>;
//template class InsetTilespaceIndex<InsetInfoAndIndex>;