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


class TestData {
    // TODO - doc structure.
    std::vector <khExtents<double>> extentsVec;

public:
    TestData(std::string fname) {
        std::ifstream input(fname);

        // First line and all even lines have 4 floats, for decimal degree representations of the input insets.
        // Every odd line is a QtMBR.  Let's just read them in pairs.
        for (std::string line; std::getline(input, line);)   //read stream line by line
        {
            std::istringstream in(line);
            double x1, x2, y1, y2;
            in >> x1 >> x2 >> y1 >> y2;
            khExtents<double> extents(XYOrder, x1, x2, y1, y2);
            std::getline(input, line);
            // Note -  discarding the MBR for now, we'll recalculate it.
            std::string qtp_txt, qtp_level;
            in >> qtp_txt >> qtp_level;
            // QuadtreePath qtpMbr(qtp_txt);
            extentsVec.push_back(extents);
        }
    }

    std::vector <khExtents<double>>

    getData() { return extentsVec; }
};

class InsetTilespaceIndexTest : public testing::Test {
protected:
    InsetTilespaceIndex insetTilespaceIndex;
public:

    // TODO - set gencov
    // Build an inset coverage from NORM extents
    // IMPORTANT: (Note the different c'tor's paramter order between DEGREES and NORM )
    //
    // khInsetCoverage(const khTilespace &tilespace,
    //                 const khExtents<uint32> &Extents,
    //                 uint fullresTileLevel,
    //                 uint beginCoverageLevel,
    //                 uint endCoverageLevel);

    std::vector <khExtents<double>>
    findInsetsControlAlgo(const khInsetCoverage &coverage,
                          const std::vector <khExtents<double>> &inputExtents) {
        uint beginMinifyLevel = 1;
        uint endMinifyLevel = 19;
        std::vector <uint32> neededIndexes; //This is our return value...
        std::vector <khExtents<double>> matchingExtents;
        std::vector <khExtents<uint32>> tileExtentsVec;


        for (auto degExtents : inputExtents) {
            khExtents <uint32> tileExtents = DegExtentsToTileExtents(degExtents, coverage.beginLevel());
            tileExtentsVec.push_back(tileExtents);
        }

        FindNeededImageryInsets(coverage,
                                tileExtentsVec,
                                (uint) tileExtentsVec.size(),
                                neededIndexes,
                                (uint) beginMinifyLevel,
                                (uint) endMinifyLevel);
        for (uint index : neededIndexes) {
            khExtents<double> ex = inputExtents[index];
            matchingExtents.push_back(ex);
        };
        return matchingExtents;
    }

    std::vector <khExtents<double>>
    findInsetsExperimentalAlgo(const khInsetCoverage &queryCoverage,
                               const std::vector <khExtents<double>> &inputExtents) {
        std::vector <khExtents<double>> matchingExtentsVec;
        InsetTilespaceIndex qtIndex;

        for (auto extents : inputExtents) {
            qtIndex.add(extents);
        }
        int level = queryCoverage.beginLevel();
        auto queryMBR = qtIndex.getQuadtreeMBR(queryCoverage.degreeExtents(), level, queryCoverage.endLevel());

        matchingExtentsVec = qtIndex.intersectingExtents(
                queryMBR,
                queryCoverage.beginLevel(),
                queryCoverage.endLevel());


        return matchingExtentsVec;
    }


    const bool compareAlgorithmOutputs() {

        // TODO - reference file in the correct testdata path.
        TestData dataset("TestDataQTPs.txt");
        std::vector <khExtents<double>> testExtents = dataset.getData();

        khExtents<double> insetExtents(XYOrder, 114.032, 114.167, 19.1851, 19.3137);
        khInsetCoverage coverage(RasterProductTilespace(false), insetExtents, 19, 7, 21);

        std::vector <khExtents<double>> requiredExtentsProd = findInsetsControlAlgo(coverage, testExtents);
        std::vector <khExtents<double>> requiredExtentsExp = findInsetsExperimentalAlgo(coverage, testExtents);
        bool listsMatch = (requiredExtentsProd == requiredExtentsExp);
        return listsMatch;
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

TEST_F(InsetTilespaceIndexTest, compareAlgorithmOutputs
) {
EXPECT_EQ(compareAlgorithmOutputs(),

true);
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

TEST_F(InsetTilespaceIndexTest, SampleDataTest
) {
std::ifstream in("/home/jkent/coveragefile.txt", std::ifstream::in);
//std::ofstream out("/home/jkent/test.txt", std::ifstream::out);
std::string line;
while(
getline(in, line
)) {
//out << line;
//out << "\n";
std::istringstream iss(line);
std::vector<double> coords;
std::string coord;
while(
getline(iss, coord,
' ')) {
coords.
push_back(std::stod(coord)
);  }
//    const khExtents<double> extent = khExtents<double>(XYOrder, coords.at(0), coords.at(1), coords.at(2), coords.at(3));
//    int level;
//QuadtreePath qtp = insetTilespaceIndex. (extent, level, 24);
//out << "\t";
//out << qtp.AsString();
//out << "\t";
//out << level;
//out << "\n";
}
//out.close();
in.

close();

}

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

