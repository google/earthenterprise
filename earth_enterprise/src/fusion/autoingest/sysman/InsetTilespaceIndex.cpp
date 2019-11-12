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
#include "autoingest/plugins/MercatorRasterProductAsset.h"
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

khExtents<double> getExtents(const InsetInfo<MercatorRasterProductAssetVersion> &insetInfo){
    return insetInfo.degExtents;
}

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

    quadTree.AddElementAtQuadTreePath(quadtreeMbr, &toAdd);

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

template <class ExtentContainer>
typename InsetTilespaceIndex<ExtentContainer>::ContainerVector
InsetTilespaceIndex<ExtentContainer>::intersectingExtents(const QuadtreePath quadtreeMbr, uint32 minLevel, uint32 maxLevel) {
    InsetTilespaceIndex<ExtentContainer>::ContainerVector vec;

    // We're not worred about maxLevel. We're not trying to filter out absolutely everything thats not needed. Just making
    // a reasonable swag at cutting down the number of insets.
    notify(NFY_WARN, "Calling GetElementsAtQuadTreePath with minLevel of %u", minLevel);
    vec = quadTree.GetElementsAtQuadTreePath(quadtreeMbr, minLevel);
    return vec;
}

// Explicit instantiaions
template class InsetTilespaceIndex<khExtents<double>>;
template class InsetTilespaceIndex<InsetInfo<RasterProductAssetVersion>>;
template class InsetTilespaceIndex<InsetInfo<MercatorRasterProductAssetVersion>>;