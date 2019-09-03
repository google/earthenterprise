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
#include "common/khException.h"
#include <boost/range/sub_range.hpp>
#include <boost/range/as_literal.hpp>
#include <map>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <assert.h>

const defautTilespace = khTilespaceMercator;

    private std::map<uint64, khExtents<uint64> _mbrExtentsMap;
    
    public uint64 InsetTilespaceIndex::add(  khExtents<T> extents& ) {
        std::map index;
        _mbrExtentsMap.get( space );
        if ( !tilespacemap ) {
            tilespacemap = std::map<space><map>;
            _mbrExtentsMap.put( extents);
        }
        tsIndex = getTilespaceMBR( extents );
        tilespacemap.put( tsIndex, extents );
        return tsIndex;
    }

    public uint64 InsetTilespaceIndex::getQuadTreeMBR(  khExtents<uint32> extents ) {
        return qtHash( khTileAddr.getQuadtreeMBR( extents ) );
    }

    //     for (size_t i = 0; i < num_insets; ++i) {
    //         const khVirtualRaster::InputTile& input_tile = virtraster.inputTiles[i];
    //         khOffset<double> offset = input_tile.origin;
    //         double degX = (input_tile.rastersize.width) * dX;
    //         double degY = (input_tile.rastersize.height) * dY;
    //         // Here khExtents expects lower left origin:
    //         // This snippet lifted from RasterClusterAnalyzer
    //         const khExtents<double> deg_extent(XYOrder,
    //                                         std::min(offset.x(), offset.x() + degX),
    //                                         std::max(offset.x(), offset.x() + degX),
    //                                         std::min(offset.y(), offset.y() + degY),
    //                                         std::max(offset.y(), offset.y() + degY));
    //         const khLevelCoverage
    //             cov_toplevel(tilespace,
    //                         is_mercator ?
    //                         khCutExtent::ConvertFromFlatToMercator(deg_extent) :
    //                         deg_extent,
    //                         toplevel, toplevel);
    //         const khExtents<uint32>& extent = cov_toplevel.extents;
    //         mosaic_extent.grow(extent);
    //         sum_inset_area += extent.width() * extent.height();
    //     }


    //     return qtp;
    // }



    public std::vector<uint64> InsetTilespaceIndex::intersectingExtentsHashes( const QuadtreePath tilespaceMBR, uint32 minLevel, uint32 maxLevel) {
        uint64 tilespaceMBR = qtHash(tilespaceMBR);
        vector<uint64> qtHashKeys = _mbrExtentsMap.getKeys();
        minIndex = 0;
        maxQH = qtHash( tilespaceMBR );
        maxIndex = std::find(qtHashKeys.begin(), qtHashKeys.end(), maxQH);
        // TODO - use a range
        // TODO sort them!
        // TODO - deal with sort order, where QuadtreePath has the MSBits as

        std::vector<int>  intersectingQtHashes = sub(&qtHashKeys[minIndex],&data[maxIndex]);
        return intersectingHashes;
    }

    public std::vector<khExtents<uint32*>> InsetTilespaceIndex::intersectingExtents( const  QuadtreePath tilespaceMBR, uint32 minLevel, uint32 maxLevel) {
        std::vector<int>  intersectingQtHashes = this.intersectingExtentHashes( tilespace, tilespaceMBR);
        std::vector<khExtents<uint32>> intersectingExtents;
        for ( auto qthash : intersectingQtHashes ) {
            std::vector<khExtents<uint32>  extentsVec = _mbrExtentsMap[qthash];
            intersectingExtents.insert(intersectingExtents.end(), extentsVec.begin(), extentsVec.end());
        };
        return    intersectingExtents();
    }




    protected QuadTreePath InsetTilespaceIndex::getQuadTreeMBR( const khExtents<uint32> extents) {
        // TODO
        return new QuadTreePath();
    }

    public InsetTilespaceIndex::InsetTileSpaceIndex() {};

    protected uint64 InsetTilespaceIndex::findParentQTIndex( QuadtreePath tilespaceMBR ) {
        // TODO
        return 0;
    }

    protected uint64 InsetTilespaceIndex::findFirstChildQTIndex( QuadtreePath tilespaceMBR ) {
        // TODO
        return 0;
    }

    protected uint64 InsetTilespaceIndex::findLastQTIndex( QuadtreePath tilespaceMBR ) {
        // TODO
        return 0;
    }

    protected std::vector<uint64> InsetTilespaceIndex::getQtMBRRange( uint64 minQtMBR, uint64 maxQtMBR) {
        std::vector<uint64> range;
        return range;
    }

}