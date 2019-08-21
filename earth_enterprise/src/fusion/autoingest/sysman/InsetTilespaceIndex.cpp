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
Description: Support for terrain "overlay" projects.

******************************************************************************/
#include "fusion/autoingest/sysman/InsetTilespaceIndex.h"
#include "fusion/autoingest/sysman/InsetInfo.h"
#include "common/khException.h"
#include <unordered_map>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <assert.h>

const defautTilespace = khTilespaceMercator;

class InsetTileSpaceIndex { 
    protected:
    class IndexStorage<khTileSpace> {
        /* 
         * Stores an ordered map of TilespaceMBR to lists of RasterProduct IDs 
         * (TODO TBD: Shared Stirngs?  share_ptrs?)
         */
    private:
    std::map<khTileSpace><map<QuadtreePath><InsetInfo>> spaceIndexMap();
    
    public: 
    
    QuadtreePath index( InsetInfo inset ) {
        return index ( defaultTilespace );
    }

    QuadtreePath index( khTilespace space, InsetInfo inset ) {
        std::map index;
        // TOOD - cleanup 
        tilespacemap = spaceIndexMap.get( space );
        if ( !map ) {
            map = std::map<space><map()>  
            spaceIndexMap.put( )
        }
        tsIndex = getTilespaceMBR( inset );
        map.put( index, inset);
        return tsIndex;
    }

    QuadtreePath getTileSpaceMBR(  InsetInfo inset ) {
        QuadtreePath qtp; 
        mbr = new QuadtreePath();


for (size_t i = 0; i < num_insets; ++i) {
    const khVirtualRaster::InputTile& input_tile = virtraster.inputTiles[i];
    khOffset<double> offset = input_tile.origin;
    double degX = (input_tile.rastersize.width) * dX;
    double degY = (input_tile.rastersize.height) * dY;
    // Here khExtents expects lower left origin:
    const khExtents<double> deg_extent(XYOrder,
                                       std::min(offset.x(), offset.x() + degX),
                                       std::max(offset.x(), offset.x() + degX),
                                       std::min(offset.y(), offset.y() + degY),
                                       std::max(offset.y(), offset.y() + degY));
    const khLevelCoverage
        cov_toplevel(tilespace,
                     is_mercator ?
                     khCutExtent::ConvertFromFlatToMercator(deg_extent) :
                     deg_extent,
                     toplevel, toplevel);
    const khExtents<uint32>& extent = cov_toplevel.extents;
    mosaic_extent.grow(extent);
    sum_inset_area += extent.width() * extent.height();
  }


        return qtp;
    }


    public vector getAllInsets(  QuadtreePath tilespaceMBR ) {
        return getAllInsets( defautTilespace, tilespaceMBR);
    }


    public vector getAllInsets( khTileSpace tilespace, QuadtreePath tilespaceMBR) {
        std::map<>  tsIndex spaceIndexMap.get( tilespace );
        tsIndex.getKeys();
        // TODO sort them!
        // TODO - deal with sort order, where QuatTreePath has the MSBits as 
        // the path and the LSBits represet the level. Reversing this order
        // would mak eit MUCH more efficient to sort through all quadtreepaths
        // based on zoom levels, which in turn would let us start with leaf-node
        // insets (i.e., highest zoom) first. 
        // Collect all insets under keys below a certain level....

        // NOTE - can also leverage qtp.isParent( qtp2 ) 
        // and qtp.isChild( qtp2 )

    }

    public InsetTileSpaceIndex( ) {


    }   

    
}