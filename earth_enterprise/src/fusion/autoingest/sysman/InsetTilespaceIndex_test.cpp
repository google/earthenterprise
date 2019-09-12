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
    std::map<khExtents < double>*, QuadtreePath>
    extentQtMBRMap;

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
            std::string qtp_txt, qtp_level;
            in >> qtp_txt >> qtp_level;
            QuadtreePath qtpMbr(qtp_txt);
            extentQtMBRMap.insert({&extents, qtpMbr});
        }
    }

    std::map<khExtents < double>*, QuadtreePath>

    getData() { return extentQtMBRMap; }
};

class InsetTilespaceIndexTest : public testing::Test {
protected:
    InsetTilespaceIndex insetTilespaceIndex;
public:
    static const uchar blist[QuadtreePath::kMaxLevel];
    static const uchar ascii_blist[QuadtreePath::kMaxLevel];
    static const bool is_mercator = true;
    static const uint64 expected_binary = 0x60D793F16AC10018LL;
    static const uint64 mask48 = 0xFFFFFFFFFFFF0000LL;


    const QuadtreePath getExtentMBR(const khExtents<double> &extent, int &level, const int max_level) {

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
            if (extent.south() <= north_south_midpoint) {
                if (extent.west() <= west + qt_node_size) {
                    next_qt_node = '0';
                } else {
                    next_qt_node = '1';
                }
            } else {
                if (extent.west() <= west + qt_node_size) {
                    next_qt_node = '3';
                } else {
                    next_qt_node = '2';
                }
            }
            // Check if NE vertex is in the same sub-node. If
            // not, then break at the node we are at.
            if (extent.north() <= north_south_midpoint) {
                if (extent.east() <= west + qt_node_size) {
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
                if (extent.east() <= west + qt_node_size) {
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

    // TODO - set gencov
    // Build an inset coverage from NORM extents 
    // IMPORTANT: (Note the different c'tor's paramter order between DEGREES and NORM )
    //
    // khInsetCoverage(const khTilespace &tilespace,
    //                 const khExtents<uint32> &Extents,
    //                 uint fullresTileLevel,
    //                 uint beginCoverageLevel,
    //                 uint endCoverageLevel);

    std::vector<const khExtents<double> *>
    findInsetsProductionAlgo(const khInsetCoverage &coverage,
                             const std::vector<const khExtents<double> *> &inputExtents) {
        uint beginMinifyLevel = 1;
        uint endMinifyLevel = 19;
        std::vector <uint32> neededIndexes; //This is our return value...
        std::vector<const khExtents<double> *> matchingExtents;


        std::vector<const khExtents <uint32> *> normExtentsVec;
        for (auto degExtents : inputExtents) {
            const khExtents <uint32> normExtents = DegExtentsToTileExtents(*degExtents, coverage.beginLevel());
            normExtentsVec.push_back(&normExtents);
        }

        // TODO - inputExtents in InsetInfos is a khExtents<uint32> - so we must covert the khExtents<double>
        FindNeededImageryInsets(coverage,
                                normExtentsVec,
                                (uint) normExtentsVec.size(),
                                neededIndexes,
                                (uint) beginMinifyLevel,
                                (uint) endMinifyLevel);
        for (uint index : neededIndexes) {
            const khExtents<double> *ex = inputExtents[index];
            matchingExtents.push_back(ex);
        };
        return matchingExtents;
    }

    std::vector<const khExtents<double> *>
    findInsetsExperimentalAlgo(const khInsetCoverage &coverage,
                               const std::vector<const khExtents<double> *> &inputExtents) {
        std::vector<const khExtents<double> *> matchingExtentsVec;
        InsetTilespaceIndex qtIndex;

        for (auto extents : inputExtents) {
            qtIndex.add(*extents);
        }

        for (const khExtents<double> *extents : inputExtents) {
            int level;
            auto extentsMBR = qtIndex.getQuadtreeMBR(*extents, level, MAX_LEVEL);
            std::vector<const khExtents<double> *> foundExtentsVec = qtIndex.intersectingExtents(
                    extentsMBR,
                    coverage.beginLevel(),
                    coverage.endLevel());
            foundExtentsVec.insert(foundExtentsVec.end(), foundExtentsVec.begin(), foundExtentsVec.end());
        };
        return matchingExtentsVec;
    }


    const bool TestAlgo1Algo2OutputMatch() {
        //TODO - populate this coverage object with test data
        TestData dataset("TestDataQTPs.txt");
        std::vector<const khExtents<double> *> testExtents;
        std::vector <QuadtreePath> mbrHashVec;
        boost::copy(dataset.getData() | boost::adaptors::map_keys,
                    std::back_inserter(testExtents));
// 114.032 114.167 19.1851 19.3137
//
//        khInsetCoverage(const khTilespace &tilespace,
//        const khExtents<double> &degExtents,
//        uint fullresTileLevel,
//        uint beginCoverageLevel,
//        uint endCoverageLevel);

        const khExtents<double> insetExtents(XYOrder, 114.032, 114.167, 19.1851, 19.3137);
        khInsetCoverage coverage(RasterProductTilespace(false), insetExtents, 19, 7, 21);

        std::vector<const khExtents<double> *> requiredExtentsProd = findInsetsProductionAlgo(coverage, testExtents);
        std::vector<const khExtents<double> *> requiredExtentsExp = findInsetsExperimentalAlgo(coverage, testExtents);
        bool listsMatch = (requiredExtentsProd == requiredExtentsExp);
        return listsMatch;
    }

    InsetTilespaceIndexTest() : insetTilespaceIndex() {};
};

//This test should result in the getExtentMBR method breaking after level 0.

TEST_F(InsetTilespaceIndexTest, BreakBeforeMaxLevelReached) {
    const khExtents<double> extent = khExtents<double>(XYOrder, 180, 180, 90, 180);
    int level;
    insetTilespaceIndex.
    getQuadtreeMBR(extent, level, MAX_LEVEL);
    EXPECT_EQ(1, level);
}

//This test should result in the getExtentMBR method looping until the max_level is reached.
TEST_F(InsetTilespaceIndexTest, DecendToMaxLevel
) {
    const khExtents<double> extent = khExtents<double>(XYOrder, 90, 90, 90, 90);
    int level;
    insetTilespaceIndex.
    getQuadtreeMBR(extent, level, MAX_LEVEL);
    EXPECT_EQ(MAX_LEVEL, level);
}

//This test should result in the quadtree path 202 being returned.
TEST_F(InsetTilespaceIndexTest, ReturnSpecificQTP) {
    const khExtents<double> extent = khExtents<double>(XYOrder, 90, 90, 90, 90);
    int level;
    QuadtreePath qtp = insetTilespaceIndex.getQuadtreeMBR(extent, level, 3);
    EXPECT_EQ("202", qtp.AsString());
}

TEST_F(InsetTilespaceIndexTest, SampleDataTest) {
    std::ifstream in("/home/jkent/coveragefile.txt", std::ifstream::in);
    //std::ofstream out("/home/jkent/test.txt", std::ifstream::out);
    std::string line;
    while(getline(in, line)) {
        //out << line;
        //out << "\n";
        std::istringstream iss(line);
        std::vector<double> coords;
        std::string coord;
        while( getline(iss, coord,' ')) { coords.push_back(std::stod(coord));  }
        //    const khExtents<double> extent = khExtents<double>(XYOrder, coords.at(0), coords.at(1), coords.at(2), coords.at(3));
        //    int level;
        //QuadtreePath qtp = insetTilespaceIndex.getQuadtreeMBR(extent, level, 24);
        //out << "\t";
        //out << qtp.AsString();
        //out << "\t";
        //out << level;
        //out << "\n";
    }
    //out.close();
    in.close();

}

int main(int argc, char *argv[]) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

