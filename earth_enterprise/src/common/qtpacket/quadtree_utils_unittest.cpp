// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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

//

#include <stdio.h>
#include <string.h>
#include <string>
#include <quadtreepath.h>
#include "quadtree_utils.h"
#include <UnitTest.h>

#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
   static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

using namespace qtpacket;

class QuadtreeUtilsUnitTest : public UnitTest<QuadtreeUtilsUnitTest>  {
 public:
  QuadtreeUtilsUnitTest()
      : BaseClass("QuadtreeUtilsUnitTest") {
    REGISTER(TestNumberingConversions);
    REGISTER(TestGlobalNodeNumber);
    REGISTER(TestLevelRowColumn);
    REGISTER(TestQuadsetAndSubindex);
    REGISTER(TestIsQuadsetRootLevel);
  }

  bool TestNumberingConversions() {
    QuadtreeNumbering tree1(1, true);

    int level, x, y;
    tree1.SubindexToLevelXY(0, &level, &x, &y);
    CHECK_EQ(level, 0);
    CHECK_EQ(x, 0);
    CHECK_EQ(y, 0);

    struct TestCase {
      int subindex, level, x, y;
    };
    static const TestCase tests2[] = {
      { 0, 0, 0, 0 },
      { 1, 1, 0, 0 },
      { 2, 1, 1, 0 },
      { 3, 1, 1, 1 },
      { 4, 1, 0, 1 },
    };

    QuadtreeNumbering tree2(2, true);
    for (unsigned int i = 0; i < ARRAYSIZE(tests2); ++i) {
      tree2.SubindexToLevelXY(tests2[i].subindex, &level, &x, &y);
      CHECK_EQ(level, tests2[i].level);
      CHECK_EQ(x, tests2[i].x);
      CHECK_EQ(y, tests2[i].y);
      CHECK_EQ(tree2.LevelXYToSubindex(tests2[i].level,
                                       tests2[i].x,
                                       tests2[i].y),
               tests2[i].subindex);
    }

    static const TestCase tests5[] = {
      { 0, 0, 0, 0 },
      { 1, 1, 0, 0 },
      { 2, 2, 0, 0 },
      { 3, 2, 1, 0 },
      { 4, 2, 1, 1 },
      { 5, 2, 0, 1 },
      { 6, 3, 0, 0 },
      { 7, 3, 1, 0 },
      { 8, 3, 1, 1 },
      { 9, 3, 0, 1 },
      { 10, 3, 2, 0 },
      { 11, 3, 3, 0 },
      { 12, 3, 3, 1 },
      { 13, 3, 2, 1 },
      { 86, 1, 1, 0 },
      { 340, 4, 0, 15 },
    };

    QuadtreeNumbering tree5(5, true);
    for (unsigned int i = 0; i < ARRAYSIZE(tests5); ++i) {
      tree5.SubindexToLevelXY(tests5[i].subindex, &level, &x, &y);
      CHECK_EQ(level, tests5[i].level);
      CHECK_EQ(x, tests5[i].x);
      CHECK_EQ(y, tests5[i].y);
      CHECK_EQ(tree5.LevelXYToSubindex(tests5[i].level,
                                       tests5[i].x,
                                       tests5[i].y),
               tests5[i].subindex);
    }
    return true;
  }

  bool TestGlobalNodeNumber() {
    struct PathTest {
      const char *path;
      std::uint32_t node_number;
    };
    static const PathTest tests[] = {
      { "",      0 },
      { "0",     1 },
      { "1",     2 },
      { "2",     3 },
      { "3",     4 },
      { "00",    5 },
      { "000",   21 },
      { "001",   22 },
      { "002",   23 },
      { "003",   24 },
      { "010",   25 },
      { "0000",  85 },
      { "00000", 341 },
    };

    for (unsigned int i = 0; i < ARRAYSIZE(tests); ++i) {
      // Convert path to right format
      std::string str = QuadtreePath(tests[i].path).AsString();
      CHECK_EQ(QuadtreeNumbering::TraversalPathToGlobalNodeNumber(str),
               tests[i].node_number);

      // Test the reverse function
      std::string result = QuadtreeNumbering::GlobalNodeNumberToTraversalPath(
          tests[i].node_number).AsString();

      CHECK_STREQ(tests[i].path, result.c_str());
    }
    return true;
  }

  bool TestLevelRowColumn() {
    struct {
      std::uint32_t level;
      std::uint32_t row;
      std::uint32_t column;
      const char *path;
      const char *maps;
    } tests[] = {
      { 0, 0, 0, "", "t" },
      { 1, 0, 0, "3", "tq" },
      { 1, 0, 1, "2", "tr" },
      { 1, 1, 0, "0", "tt" },
      { 1, 1, 1, "1", "ts" },
      { 2, 0, 0, "33", "tqq" },
      { 2, 0, 1, "32", "tqr" },
      { 2, 1, 0, "30", "tqt" },
      { 2, 1, 1, "31", "tqs" },
      { 2, 2, 0, "03", "ttq" },
      { 2, 2, 1, "02", "ttr" },
      { 2, 3, 0, "00", "ttt" },
      { 2, 3, 1, "01", "tts" },
      { 2, 0, 2, "23", "trq" },
      { 2, 0, 3, "22", "trr" },
      { 2, 1, 2, "20", "trt" },
      { 2, 1, 3, "21", "trs" },
      { 2, 2, 2, "13", "tsq" },
      { 2, 2, 3, "12", "tsr" },
      { 2, 3, 2, "10", "tst" },
      { 2, 3, 3, "11", "tss" },
      { 23,
        /*0B10011001001000100101001*/ 0x4c9129,
        /*0B11101010000110010110011*/ 0x750cb3,
           "12201320330223023120321",
          "tsrrtsqrtqqtrrqtrqsrtqrs",
      },
    };

    // Convert row numbers from Magrathean convention to Fusion
    // convention (row numbering runs in opposite directions)
    for (unsigned int i = 0; i < ARRAYSIZE(tests); ++i) {
      tests[i].row = (1 << tests[i].level) - tests[i].row - 1;
    }

    // Test conversion from level, row, column to quadtree path & maps path.
    for (unsigned int i = 0; i < ARRAYSIZE(tests); ++i) {
      std::string result =
        QuadtreePath(tests[i].level, tests[i].row, tests[i].column).AsString();

      CHECK_STREQ(tests[i].path, result.c_str());

      std::string maps = QuadtreeNumbering::LevelRowColumnToMapsTraversalPath(
          tests[i].level, tests[i].row, tests[i].column);

      CHECK_STREQ(tests[i].maps, maps.c_str());
    }

    // Test conversion from quadtree path & maps path to level, row, column.
    for (unsigned int i = 0; i < ARRAYSIZE(tests); ++i) {
      QuadtreePath path2(tests[i].path);

      unsigned int level, row, col;
      path2.GetLevelRowCol(&level, &row, &col);
      CHECK_EQ(tests[i].level, level);
      CHECK_EQ(tests[i].row, row);
      CHECK_EQ(tests[i].column, col);

      int ilevel, irow, icol;
      QuadtreeNumbering::MapsTraversalPathToLevelRowColumn(tests[i].maps,
                                                           &ilevel,
                                                           &irow, &icol);
      level = ilevel;
      row = irow;
      col = icol;
      CHECK_EQ(tests[i].level, level);
      CHECK_EQ(tests[i].row, row);
      CHECK_EQ(tests[i].column, col);
    }
    return true;
  }

  bool TestQuadsetAndSubindex() {
    struct {
      const char *path;
      std::uint64_t quadset;
      int subindex;
    } tests[] = {
      { "",    0, 0, },
      { "0",   0, 1, },
      { "002", 0, 23 },
      { "333",  0, 84 },
      { "0000",  21, 1 },
      { "00000", 21, 2 },
      { "000000", 21, 6 },
      { "0000000", 21, 22 },
      { "00000000", 0x1555, 1 },
      { "00000001", 0x1555, 86 },
      { "000000000", 0x1555, 2 },
      { "000000001", 0x1555, 3 },
    };

    for (unsigned int i = 0; i < ARRAYSIZE(tests); ++i) {
      QuadtreePath expected_path(tests[i].path);
      QuadtreePath path = QuadtreeNumbering::QuadsetAndSubindexToTraversalPath(
          tests[i].quadset, tests[i].subindex);
      CHECK(expected_path == path);

      std::uint64_t quadset;
      int subindex;
      QuadtreeNumbering::TraversalPathToQuadsetAndSubindex(expected_path,
                                                           &quadset,
                                                           &subindex);
      CHECK_EQ(quadset, tests[i].quadset);
      CHECK_EQ(subindex, tests[i].subindex);
    }
    return true;
  }

  bool TestIsQuadsetRootLevel() {
    bool is_quadset_level[QuadtreePath::kMaxLevel+1] =
      { true, false, false,
        true, false, false, false,
        true, false, false, false,
        true, false, false, false,
        true, false, false, false,
        true, false, false, false,
        true, false };
    for (std::uint32_t level = 0; level <= QuadtreePath::kMaxLevel; ++level) {
      if (QuadtreeNumbering::IsQuadsetRootLevel(level)
          != is_quadset_level[level])
        return false;
    }
    return true;
  }
};

int main(int argc, char **argv) {
  QuadtreeUtilsUnitTest f;
  return f.Run();
}
