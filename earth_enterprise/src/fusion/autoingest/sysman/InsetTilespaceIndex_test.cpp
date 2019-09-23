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
//#include "fusion/autoingest/sysman/InsetInfo.h"
#include "common/khException.h"
#include "common/khExtents.h"
#include "common/khInsetCoverage.h"
#include "common/khTileAddr.h"
#include "autoingest/plugins/RasterProductAsset.h"
#include "autoingest/plugins/MercatorRasterProductAsset.h"
#include <UnitTest.h>
#include <gtest/gtest.h>
#include <boost/range/sub_range.hpp>
#include <boost/range/as_literal.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/any_range.hpp>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <assert.h>
#include <khTypes.h>
#include <sstream>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

class InsetTilespaceIndexTest : public testing::Test {
protected:
    InsetTilespaceIndex insetTilespaceIndex;
public:
    struct TestData {
        uint beginLevel;
        uint endLevel;
        uint numLevels;
        khExtents<double> degExtents;
        std::vector<khExtents<uint32>> levelExtentsVec;
    };

    std::vector<TestData> getTestData() {
        std::vector<TestData> testDataVec;
        std::ifstream dataFile("/home/jkent/VSCode/earthenterprise/earth_enterprise/src/fusion/testdata/TestDataForQTPs.txt");
        int datasets = 1;
        //TODO: fix condition to stop at last data set correctly.
        while (datasets < 5) {
            TestData testData;
            std::string s;
            dataFile >> testData.beginLevel;
            dataFile >> testData.endLevel;
            dataFile >> s;
            std::stringstream degSS(s);
            std::vector<double> degs;
            for (double deg; degSS >> deg;) {
                degs.push_back(deg);
                if (degSS.peek() == ',') {
                    degSS.ignore();
                }
            }
            testData.degExtents = khExtents<double>(XYOrder, degs.at(0), degs.at(1), degs.at(2), degs.at(3));
            dataFile >> testData.numLevels;
            for (int i = 0; i < (int)testData.numLevels; i++) {
                std::string s2;
                dataFile >> s2;
                std::stringstream levelSS(s2);
                std::vector<uint32> levels;
                for (uint32 level; levelSS >> level;) {
                    levels.push_back(level);
                    if (levelSS.peek() == ',') {
                        levelSS.ignore();
                    }
                }
                testData.levelExtentsVec.push_back(khExtents<uint32>(XYOrder, levels.at(0), levels.at(1), levels.at(2), levels.at(3)));
            }
            testDataVec.push_back(testData);
            datasets++;
        }
        return testDataVec;
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

    std::vector <khExtents<uint32>>
    findInsetsControlAlgo(const khInsetCoverage &coverage,
                          const std::vector<khExtents<uint32>> &testLevelExtentsVec,
                          const uint &level) {
        uint beginMinifyLevel = 0;
        uint endMinifyLevel = 21;
        std::vector <uint32> neededIndexes; //This is our return value...
        std::vector <khExtents<uint32>> matchingExtents;
        
        FindNeededImageryInsets(coverage,
                                testLevelExtentsVec,
                                testLevelExtentsVec.size(),
                                neededIndexes,
                                beginMinifyLevel,
                                endMinifyLevel,
                                level);
        for (uint index : neededIndexes) {
            khExtents<uint32> ex = testLevelExtentsVec[index];
            matchingExtents.push_back(ex);
        };
        
        return matchingExtents;
    }

    std::vector <khExtents<double>>
    findInsetsExperimentalAlgo(const khExtents<double> &queryDegreeExtents,
                                const khInsetCoverage &queryCoverage,
                                const std::vector <khExtents<double>> &testDegreeExtents) {
        std::vector <khExtents<double>> matchingExtentsVec;
        InsetTilespaceIndex qtIndex;
        notify(NFY_WARN, "oriExtents: %f, %f, %f, %f", queryDegreeExtents.beginX(), queryDegreeExtents.endX(), 
                                                                    queryDegreeExtents.beginY(), queryDegreeExtents.endY());
        for (auto extents : testDegreeExtents) {
            qtIndex.add(extents);
        }
        int level = queryCoverage.beginLevel();
        auto queryMBR = qtIndex.getQuadtreeMBR(queryDegreeExtents, level, queryCoverage.endLevel());
        notify(NFY_WARN, "queryMBR: %s", queryMBR.AsString().c_str());

        matchingExtentsVec = qtIndex.intersectingExtents(
                queryMBR,
                queryCoverage.beginLevel(),
                queryCoverage.endLevel());

        return matchingExtentsVec;
    }


    const bool compareAlgorithmOutputs() {
        //TODO: Rewrite test to compare based on level.
        TestData ori;
        ori.beginLevel = 0;
        ori.endLevel = 14;
        ori.numLevels = 14;
        ori.degExtents = khExtents<double>(XYOrder, -123.531, -120.713, 36.4544, 38.4647);
        ori.levelExtentsVec.push_back(khExtents<uint32>(XYOrder, 0, 1, 0, 1));
        ori.levelExtentsVec.push_back(khExtents<uint32>(XYOrder, 0, 1, 1, 2));
        ori.levelExtentsVec.push_back(khExtents<uint32>(XYOrder, 0, 1, 2, 3));
        ori.levelExtentsVec.push_back(khExtents<uint32>(XYOrder, 1, 2, 4, 5));
        ori.levelExtentsVec.push_back(khExtents<uint32>(XYOrder, 2, 3, 9, 10));
        ori.levelExtentsVec.push_back(khExtents<uint32>(XYOrder, 5, 6, 19, 20));
        ori.levelExtentsVec.push_back(khExtents<uint32>(XYOrder, 10, 11, 38, 39));
        ori.levelExtentsVec.push_back(khExtents<uint32>(XYOrder, 20, 22, 76, 78));
        ori.levelExtentsVec.push_back(khExtents<uint32>(XYOrder, 40, 43, 153, 156));
        ori.levelExtentsVec.push_back(khExtents<uint32>(XYOrder, 80, 85, 307, 311));
        ori.levelExtentsVec.push_back(khExtents<uint32>(XYOrder, 160, 169, 615, 622));
        ori.levelExtentsVec.push_back(khExtents<uint32>(XYOrder, 321, 338, 1231, 1243));
        ori.levelExtentsVec.push_back(khExtents<uint32>(XYOrder, 642, 675, 2462, 2486));
        ori.levelExtentsVec.push_back(khExtents<uint32>(XYOrder, 1284, 1350, 4925, 4972));
        //khInsetCoverage oriCov = khInsetCoverage(ori.beginLevel, ori.endLevel, ori.levelExtentsVec);
        std::vector<TestData> testDataVec = getTestData();
        //std::vector<uint> requiredExtentsProdIndexes;
        //std::vector<khExtents<double>> requiredExtentsProd;
        //std::vector<khExtents<double>> requiredExtentsExp;
        //std::vector<khExtents<double>> testDegreeExtents;
        //std::vector<khExtents<uint32>> testLevelExtents;
        std::vector<int> levelsCoveredProd(testDataVec.size());
        std::vector<int> stopVec(testDataVec.size());
        std::vector<int> levelsCoveredExp(testDataVec.size());
        std::vector<khExtents<double>> requiredExtentsExp;
        //bool listsMatch;
        std::vector<khExtents<double>> testDegreeExtents;
        khInsetCoverage oriCov = khInsetCoverage(ori.beginLevel, ori.endLevel, ori.levelExtentsVec);
        for (auto testData : testDataVec) {
            testDegreeExtents.push_back(testData.degExtents);
        }
        for (uint i = ori.beginLevel; i < ori.endLevel; i++) {
            std::vector<khExtents<uint32>> requiredExtentsProd;
            std::vector<khExtents<uint32>> testLevelExtentsVec;
            for (auto testData : testDataVec) {
                testLevelExtentsVec.push_back(testData.levelExtentsVec[i]);
            }
            requiredExtentsProd = findInsetsControlAlgo(oriCov, testLevelExtentsVec, i);
            int k = 0;
            for (auto levelExtents : testLevelExtentsVec) {
                std::vector<khExtents<uint32>>::iterator it = std::find(requiredExtentsProd.begin(), requiredExtentsProd.end(), levelExtents);
                if (it == requiredExtentsProd.end() || testDataVec[k].levelExtentsVec.size() - 1 == i || i == (ori.endLevel - 1)) {
                    notify(NFY_WARN, "\tlevel: %u\tlevelExtents: %d, %d, %d, %d", i, levelExtents.beginX(), levelExtents.endX(), levelExtents.beginY(), levelExtents.endY());
                    if (stopVec[k] == 0) {
                        levelsCoveredProd[k] = i;
                        stopVec[k] = 1;
                    }
                }
                k++;
            }
        }
        notify(NFY_WARN, "Control Algorithm Completed");
        for (auto levels : levelsCoveredProd) {
            notify(NFY_WARN, "\tlevel: %d", levels);
        }
        requiredExtentsExp = findInsetsExperimentalAlgo( ori.degExtents, oriCov, testDegreeExtents);
        

        /*
        for (auto testData : testDataVec) {
            testLevelExtents.push_back(testData.levelExtentsVec);
            testDegreeExtents.push_back(testData.degExtents);
        }
        requiredExtentsProdIndexes = findInsetsControlAlgo(ori, testLevelExtents);
        for (auto index : requiredExtentsProdIndexes) {
            requiredExtentsProd.push_back(testDataVec[index].degExtents);
        }
        notify(NFY_WARN, "Old Algo Done, %lu", requiredExtentsProd.size());
        requiredExtentsExp = findInsetsExperimentalAlgo( ori.degExtents, oriCov, testDegreeExtents);
        notify(NFY_WARN, "New Algo Done %lu", requiredExtentsExp.size());
        listsMatch = (requiredExtentsProd == requiredExtentsExp);*/
        //return listsMatch;
        return true;
    }

    InsetTilespaceIndexTest() : insetTilespaceIndex() {};
};

//This test should result in the getExtentMBR method breaking after level 0.

TEST_F(InsetTilespaceIndexTest, BreakBeforeMaxLevelReached
) {
khExtents<double> extent = khExtents<double>(XYOrder, 180, 180, 90, 180);
int level;
insetTilespaceIndex.
getQuadtreeMBR(extent, level, MAX_LEVEL
);
EXPECT_EQ(1, level);
}

//This test should result in the getExtentMBR method looping until the max_level is reached.
TEST_F(InsetTilespaceIndexTest, DecendToMaxLevel
) {
khExtents<double> extent = khExtents<double>(XYOrder, 90, 90, 90, 90);
int level;
insetTilespaceIndex.
getQuadtreeMBR(extent, level, MAX_LEVEL
);
EXPECT_EQ(MAX_LEVEL, level
);
}

TEST_F(InsetTilespaceIndexTest, compareAlgorithmOutputs) {
    EXPECT_EQ(compareAlgorithmOutputs(), true);
}

//This test should result in the quadtree path 202 being returned.
TEST_F(InsetTilespaceIndexTest, ReturnSpecificQTP
) {
khExtents<double> extent = khExtents<double>(XYOrder, 90, 90, 90, 90);
int level;
QuadtreePath qtp = insetTilespaceIndex.getQuadtreeMBR(extent, level, 3);
EXPECT_EQ("202", qtp.

AsString()

);
}

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

