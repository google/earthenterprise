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

// Unit tests for khQtPreindex.

#include "common/khStringUtils.h"
#include "common/quadtreepath.h"
#include "fusion/autoingest/sysman/InsetTilespaceIndex.h"
#include "fusion/autoingest/sysman/InsetInfo.h"
#include "common/khException.h"
#include "common/khExtents.h"
#include "common/khInsetCoverage.h"
#include "common/khException.h"
#include "autoingest/plugins/RasterProductAsset.h"
#include "autoingest/plugins/MercatorRasterProductAsset.h"
#include <UnitTest.h>
#include <gtest/gtest.h>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <assert.h>
#include <khTypes.h>
#include <string>

// TODO _ HIGH LEVEL TODOS:
// TODO : Look at src/common/tests/qtpathgen.cpp to see if we don't already have
// what we need for the QT MBR datatype.
//
// Pseudocode for Test:
//
// Generate a bunch of InsetInfos (resources)
//      TBD - where to store initial coords?  Just an array?
//
// for each inset_info i:
//   call FindNeededImageryInsets
//   call new code
//   compare results

//   convert neededIndexes to vector of InsetInfos
//   for each level
//     Call FindNeededImageryInsets for that level
//     call new code
//     compare results

// Lowest level of common code  - excerpt from InsetInfo's Overlap calculator
// InsetInfos->FindNeededImageryInsets

// khExtents<uint32> genExtents(gencov.levelExtents(level));
// if (needAlign) {
//   genExtents.alignBy(alignSize);
// }

// for (uint i = 0; i < numInsets; ++i) {
//   // Aligning here would be redundant, so we save ourselves the effort.
//   const khExtents<uint32> &iExtents
//     (insets[i]->coverage.levelExtents(level));

//   if (iExtents.intersects(genExtents)) {
//     neededIndexes.push_back(i);
//   }

class InsetTilespaceIndex : public UnitTest<InsetTilespaceIndex> {
  public:
    static const uchar blist[QuadtreePath::kMaxLevel];
    static const uchar ascii_blist[QuadtreePath::kMaxLevel];
    static const bool is_mercator = true; 
    static const uint64 expected_binary = 0x60D793F16AC10018LL;
    static const uint64 mask48 = 0xFFFFFFFFFFFF0000LL;

    std::vector<InsetInfo<RasterProductAssetVersion>*> getTestInsetsGroup1 ()  {
      std::vector<InsetInfo<RasterProductAssetVersion>> insets;
      int numInsets = 10;
      // std::vector<SimpleInsetInfo<RasterProductAssetVersion> > imageryInsets;
      //     imagery_project->LoadSimpleInsetInfo(ClientTmeshTilespaceFlat,
      //                                         imageryInsets,
      //                                         maxClientImageryLevel);

      auto newRPInset =
          [](double x1, double y1, double x2, double y2, uint beginCovLev=0, uint endCovLev=19) {
        // TODO - Establish proper Config parameters
        // const uint effectiveMaxProdLev = 24;

        RasterProductConfig config();
        //RasterProductAssetVersion assetVersion =
        //   RasterProductAssetVersion(input);

        std::vector<InsetInfo<RasterProductAssetVersion>> rpAssetVersions();
        //   for (uint i = 0; i < proj->inputs.size(); ++i) {
        //       RasterProductAssetVersion resource(proj->inputs[i]);

        //       InsetInfo<RasterProductAssetVersion> newInst =
        //         InsetInfo(RasterProductTilespace,  &resource, &assetVersion, effectiveMaxProdLev,
        //                 beginCovLev, endCovLev);
        //         rpAssetVersions.push_back( newInset) ;
        //   }
        // }
        return rpAssetVersions;
      }; // end Lambda

      while (numInsets) {
        //   inset = newRPInset(
        //       0, 0, 0, 0, 0); // TODO - populate with values from test data structs
        //   insets.push_back(inset);
        numInsets--;
      }
        

    return insets;
  }

  const QuadtreePath getInsetMBR( InsetInfo<RasterProductAssetVersion> inset ) {
    // TODO 
    return  QuadtreePath();
  }
  
   // TODO - set gencov
    // Build an inset coverage from NORM extents 
    // IMPORTANT: (Note the different c'tor's paramter order between DEGREES and NORM )
    //
    // khInsetCoverage(const khTilespace &tilespace,
    //                 const khExtents<double> &Extents,
    //                 uint fullresTileLevel,
    //                 uint beginCoverageLevel,
    //                 uint endCoverageLevel);
    
    std::vector<InsetInfo<RasterProductAssetVersion>*> findInsetsAlgo1(std::vector<InsetInfo<RasterProductAssetVersion>> inputInsets)
    {
      std::vector<InsetInfo<RasterProductAssetVersion>> neededInsets;
      //uint32 lat = 45; //TODO
      //uint32 lon = 45; //TODO
      uint rastersize=28000;
      khExtents<uint32> extents; 
      // TODO 
      // extents = (
      //   khOffset<uint32>(XYOrder, lat,lon),  rastersize );
      khInsetCoverage gencov();
      uint beginMinifyLevel = 1;
      uint endMinifyLevel = 18;
      std::vector<uint>   neededIndexes(); //This is our return value... 
      FindNeededImageryInsets(gencov,
                                  inputInsets,
                                  inputInsets.size(),
                                  neededIndexes,
                                  beginMinifyLevel,
                                  endMinifyLevel);
      return neededInsets;                                 
    }


  const bool TestAlgo1Algo2OutputMatch( )  {
    std::vector<const InsetInfo<RasterProductAssetVersion>*> inputInsets = getTestInsetsGroup1();
    std::vector<const InsetInfo<RasterProductAssetVersion>*> neededInsets1 = findInsetsAlgo1( inputInsets );
    std::vector<const InsetInfo<RasterProductAssetVersion>*> neededInsets2 = findInsetsAlgo2( inputInsets );
   
   // TODO Determine if we need to sort the vectors first
    bool listsMatch = (neededInsets1 == neededInsets2);
     
    return listsMatch;
  }


// CLUE - TODO - possibly use this to compute the QuatTree space from raw TIFF files?
// void
// khGDALDatasetImpl::ComputeReprojectSnapupExtents(GDALDataset *srcDS,
//                                                  const std::string &srcSRS,
//                                                  const std::string &dstSRS,
//                                                  khSize<uint32> &rasterSize,
//                                                  khGeoExtents   &geoExtents,
//                                                  uint           &level,
//                                                  const bool is_mercator)



// CLUE - TODO - 
//
// std::vector<InsetInfo<RasterProductAssetVersion> > insets;
// project->LoadInsetInfo(ClientTmeshTilespaceFlat,
//                               beginSSLevel, endSSLevel,
//                               insets);
// const uint beginMinifyLevel  = StartTmeshLevel;
// const uint endMinifyLevel    = terrainInsets[0].effectiveMaxLevel;

};


int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

