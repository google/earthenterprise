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
// TODO - 1) refactor InsetInfos:: FindNeededImageryInsets(
// const khInsetCoverage        &gencov,
// const std::vector<const InsetInfo<RasterProductAssetVersion> *> &insets,
// uint                          numInsets,
// std::vector<uint>            &neededIndexes,
// uint beginMinifyLevel,
// uint endMinifyLevel);
// with an overload to accept khExtents instead of insets ^^^
// 
//
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
//   call ExperimentalFunc
//   compare results

// ExperimentalFunc( coverage, all insets,  &neededInsets)
// ......returns &neededInsets
//  1) Prep:
//      a)build QuadTreeMBR for each coverageInset, 
//        add to index
//        Sort by Level then by QTP
//      b) build QuadTreeMBR for each coverageInset
//        add to index
//        Sort by Level then by QTP
//
//  2) Loop through Coverage extents' MBRs 
//      start with bottom coverage inset (i.e., highest level )
//      a) Find overlapping Children  (i.e., higher level)
//             -- compare QT path bits - any child with the first bits that binary 'ands' with the coverage inset is a match. 
//            add matching children to output list. 
//      b) Find overlapping parents  (i.e., lower level)
//             -- compare QT path bits - any coverage with the first bits that binary 'ands' with the parent's inset MBR bits at it's level is a match. 
//            add matching PARENTS to output list. 




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

//  Bread crumb trail of production algorithm code path.
//  RasterProject.src -> RasterProjectAssetVersionImplD::BuildIndex(

// INSETINFOS
//          -> PreprocessForInset->
//           ->CalculateOverlap(neededIndexes, gencov);
//             -> FindNeededImageryInsets
//                -> **extracts khInset
//                const khExtents<uint32> &iExtents
//                        (insets[i]->coverage.levelExtents(level));
//


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
// std::vector<InsetInfo<RasterProductAssetVersion> *> insets;
// project->LoadInsetInfo(ClientTmeshTilespaceFlat,
//                               beginSSLevel, endSSLevel,
//                               insets);
// const uint beginMinifyLevel  = StartTmeshLevel;
// const uint endMinifyLevel    = terrainInsets[0].effectiveMaxLevel;

class InsetTilespaceIndex_test : public UnitTest<InsetTilespaceIndex> {
public:
    static const uchar blist[QuadtreePath::kMaxLevel];
    static const uchar ascii_blist[QuadtreePath::kMaxLevel];
    static const bool is_mercator = true;
    static const uint64 expected_binary = 0x60D793F16AC10018LL;
    static const uint64 mask48 = 0xFFFFFFFFFFFF0000LL;

    std::vector<const khExtents <uint32> *> getTestInsetsGroup1(uint numExtents) const {
        std::vector<const khExtents <uint32> *> newExtents;
        // std::vector<SimpleInsetInfo<RasterProductAssetVersion> *> imageryInsets;
        //     imagery_project->LoadSimpleInsetInfo(ClientTmeshTilespaceFlat,
        //                                         imageryInsets,
        //                                         maxClientImageryLevel);
        // TODO -
        double x1 = 0, x2 = 1, y1 = 0, y2 = 0;

        while (numExtents) {

            //       0, 0, 0, 0, 0); // TODO - populate with values from test data structs

            const khExtents <uint32> newExtent(XYOrder, x1, y1, x2, y2);
            newExtents.push_back(&newExtent);
            numExtents--;
        }
        return newExtents;
    }


    const QuadtreePath getExtentQuadTreeMBR(const khExtents <uint32> *&extents) {
        return QuadtreePath();
    }

    // TODO - set gencov
    // Build an inset coverage from NORM extents 
    // IMPORTANT: (Note the different c'tor's paramter order between DEGREES and NORM )
    //
    // khInsetCoverage(const khTilespace &tilespace,
    //                 const khExtents<uint32> &Extents,
    //                 uint fullresTileLevel,
    //                 uint beginCoverageLevel,
    //                 uint endCoverageLevel);

    std::vector<const khExtents <uint32> *>
    findInsetsProductionAlgo(const khInsetCoverage &coverage,
                             const std::vector<const khExtents <uint32> *> &inputExtents) {
        uint beginMinifyLevel = 1;
        uint endMinifyLevel = 19;
        std::vector <uint32> neededIndexes; //This is our return value...
        std::vector  <const khExtents<uint32>*> matchingExtents;

        FindNeededImageryInsets(coverage,
                                inputExtents,
                                (uint) inputExtents.size(),
                                neededIndexes,
                                (uint) beginMinifyLevel,
                                (uint) endMinifyLevel);
        for ( uint index : neededIndexes ) {
            const khExtents<uint32>* ex = inputExtents[index];
            matchingExtents.push_back(ex);
        };
        return matchingExtents;
    }

    std::vector <const khExtents<uint32>*>
    findInsetsExperimentalAlgo(const khInsetCoverage &coverage, std::vector<const khExtents <uint32> *> &inputExtents) {
        std::vector <const khExtents<uint32>*> matchingExtentsVec;
        std::vector <khExtents<uint32>> requiredExtents = coverage.RawExtentsVec();
        InsetTilespaceIndex qtIndex;

        for (auto extents : inputExtents) {
            qtIndex.add(*extents);
        }


        for (const khExtents <uint32> *extents : inputExtents) {
            auto extentsMBR = getExtentQuadTreeMBR(extents);
            std::vector < khExtents < uint32 > * > foundExtentsVec = qtIndex.intersectingExtents(
                    extentsMBR.internalPath(),
                    coverage.beginLevel(),
                    coverage.endLevel());
            foundExtentsVec.insert(foundExtentsVec.end(), foundExtentsVec.begin(), foundExtentsVec.end());
        };
        return matchingExtentsVec;
    }


    const bool TestAlgo1Algo2OutputMatch() {
        const uint numExtents = 400;
        //TODO - populate this coverage object with test data
        std::vector <khExtents<uint>> testExtents;

        for (uint i = 0; i < numExtents; i++) {
            auto newInset = khExtents<uint>(XYOrder, 0, 0, 0, 0);
            testExtents.push_back(newInset);
        }

        khInsetCoverage coverage(4, 18, testExtents);
        std::vector<const khExtents <uint32> *> inputExtents = getTestInsetsGroup1(numExtents);

        std::vector<const khExtents <uint32> *> requiredExtentsProd = findInsetsProductionAlgo(coverage, inputExtents);
        std::vector<const khExtents <uint32> *> requiredExtentsExp = findInsetsExperimentalAlgo(coverage, inputExtents);
        //std::vector <uint32> requiredExtentsProd = findInsetsProductionAlgo(coverage, inputExtents);
        //std::vector <uint32> requiredExtentsExp = findInsetsExperimentalAlgo(coverage, inputExtents);
        // TODO Determine if we need to sort the vectors first
        bool listsMatch = (requiredExtentsProd == requiredExtentsExp);
        return listsMatch;
    }


};


int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

