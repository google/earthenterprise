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


// Unit tests for QuadtreePath class

#include <stdio.h>
#include "khEndian.h"
#include "quadtreepath.h"
#include <UnitTest.h>

// ****************************************************************************
// ***  Yanked from kbf.cpp before deleting it there
// ****************************************************************************
void BlistToRowCol(std::uint32_t level, unsigned char* blist, std::uint32_t& row, std::uint32_t& col) {
  static const std::uint32_t rowbits[] = {0x00, 0x00, 0x01, 0x01};
  static const std::uint32_t colbits[] = {0x00, 0x01, 0x01, 0x00};

  assert(level < 32);

  row = 0;
  col = 0;

  for (std::uint32_t j = 0; j < level; ++j) {
    assert(blist[j] < 4);
    row = (row << 1) | (rowbits[blist[j]]);
    col = (col << 1) | (colbits[blist[j]]);
  }
}


class QuadtreePathUnitTest : public UnitTest<QuadtreePathUnitTest> {
 public:
  static const unsigned char blist[QuadtreePath::kMaxLevel];
  static const unsigned char ascii_blist[QuadtreePath::kMaxLevel];
  static const std::uint64_t expected_binary = 0x60D793F16AC10018LL;
  static const std::uint64_t mask48 = 0xFFFFFFFFFFFF0000LL;

  bool TestConstructors() {
    bool result = true;

    // Default

    if (QuadtreePath().path_ != 0) {
      fprintf(stderr, "ERROR: QuadtreePath default constructor failed\n");
      result = false;
    }

    // QuadtreePath(level, blist)

    for (std::uint32_t level = 0; level <= QuadtreePath::kMaxLevel; ++level) {
      QuadtreePath from_blist(level, blist);
      std::uint64_t mask = mask48 << (48 - level*2);
      std::uint64_t expected = (expected_binary & mask) | level;
      if (from_blist.path_ != expected) {
        fprintf(stderr, "ERROR: Quadtree(%u, blist) constructor, "
                "expected 0x%016llX, got 0x%016llX\n",
                level,
                (long long unsigned int)expected,
                (long long unsigned int)from_blist.path_);
        result = false;
      }

      QuadtreePath from_ascii(level, ascii_blist);
      if (from_ascii != from_blist) {
        fprintf(stderr, "ERROR: Quadtree(%u, ascii_blist) constructor, "
                "expected 0x%016llX, got 0x%016llX\n",
                level,
                (long long unsigned int)from_blist.path_,
                (long long unsigned int)from_ascii.path_);
        result = false;
      }
    }

    // QuadtreePath(level, row, column) and GetLevelRowCol

    for (std::uint32_t level = 0; level <= QuadtreePath::kMaxLevel; ++level) {
      std::uint32_t row, col;
      unsigned char branchlist[32];
      memset(branchlist, 0, sizeof(branchlist));
      if (level > 0) memcpy(branchlist, blist, level);
      BlistToRowCol(level, branchlist, row, col);
      QuadtreePath from_lrc(level, row, col);
      std::uint64_t mask = mask48 << (48 - level*2);
      std::uint64_t expected = (expected_binary & mask) | level;
      if (from_lrc.path_ != expected) {
        fprintf(stderr, "ERROR: Quadtree(%u, %u, %u) constructor, "
                "expected 0x%016llX, got 0x%016llX\n",
                level, row, col,
                (long long unsigned int)expected,
                (long long unsigned int)from_lrc.path_);
        result = false;
      }

      std::uint32_t qlevel, qrow, qcol;
      from_lrc.GetLevelRowCol(&qlevel, &qrow, &qcol);
      if (qlevel != level || qrow != row || qcol != col) {
        fprintf(stderr, "ERROR: Quadtree::GetLeveRowCol mismatch, "
                "expected (%u, %u, %u), got (%u, %u, %u)\n",
                level, row, col,
                qlevel, qrow, qcol);
        result = false;
      }
    }

    return result;
  }

  // TestParentChild

  bool TestParentChild() {
    bool result = true;

    for (std::uint32_t level = 0; level < QuadtreePath::kMaxLevel; ++level) {
      QuadtreePath test_path(level, blist);
      for (std::uint32_t i = 0; i < 4; ++i) {
        QuadtreePath child = test_path.Child(i);
        if (child.Level() != level + 1
            || child.Parent() != test_path) {
          fprintf(stderr, "ERROR: TestParentChild, "
                  "invalid child 0x%016llX of parent 0x%016llX\n",
                  (long long unsigned int)child.path_,
                  (long long unsigned int)test_path.path_);
          result = false;
        }
      }
    }

    return result;
  }

  // TestLess - test less than operator (required for sorting)

  bool TestLess() {
    bool result = true;
    static const unsigned char last_blist[QuadtreePath::kMaxLevel] = {
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
      3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };

    // Test that all children follow parent

    for (std::uint32_t level = 0; level < QuadtreePath::kMaxLevel; ++level) {
      QuadtreePath parent(level, blist);
      if (parent < parent) {
        fprintf(stderr, "ERROR: TestLess at level %u, parent < parent\n",
                level);
        result = false;
      }

      for (std::uint32_t i = 0; i < 4; ++i) {
        QuadtreePath child = parent.Child(i);
        if (!(parent < child) || (child < parent)) {
          fprintf(stderr, "ERROR: TestLess parent(0x%016llX) "
                  "not < child %u(0x%016llX) at level %u\n",
                  (long long unsigned int)parent.path_,
                  i,
                  (long long unsigned int)child.path_,
                  level);
          result = false;
        }
        if ((level + 1) < QuadtreePath::kMaxLevel) {
          for (std::uint32_t j = 0; j < 4; ++j) {
            QuadtreePath grandchild = child.Child(j);
            if (!(parent < grandchild) || (grandchild < parent)) {
              fprintf(stderr, "ERROR: TestLess parent(0x%016llX) "
                      "not < grandchild %u,%u(0x%016llX) at level %u\n",
                      (long long unsigned int)parent.path_,
                      i, j,
                      (long long unsigned int)grandchild.path_,
                      level);
              result = false;
            }
          }
        }
      }

      // Test ordering of children within level

      if (level < QuadtreePath::kMaxLevel) {
        QuadtreePath child[4];
        for (std::uint32_t i = 0; i < 4; ++i) {
          child[i] = parent.Child(i);
          for (std::uint32_t j = 0; j < i; ++j) {
            if (!(child[j] < child[i]) || child[i] < child[j]) {
              fprintf(stderr, "ERROR: TestLess order level %u, "
                      "child[%u] (%016llX) child[%u] (%016llX)\n",
                      level, i, (long long unsigned int)child[i].path_,
                      j, (long long unsigned int)child[j].path_);
              result = false;
            }
          }
        }
      }

      // Test ordering of different paths within level

      if (level > 0) {
        QuadtreePath last(level, last_blist);
        if (!(parent < last) || last < parent) {
          fprintf(stderr, "ERROR: TestLess order level %u, "
                  "parent(%016llX) vs. last(%016llX)\n",
                  level, (long long unsigned int)parent.path_,
                  (long long unsigned int)last.path_);
          result = false;
        }
      }
    }

    return result;
  }

  // We will create 3 levels of QuadtreePaths and store the parent before
  // children. Then we will convert each such path to corresponding generation
  // sequence. Next we will check whether the generation sequence is sorted in
  // increasing order of generation sequence.
  bool TestGenerationSequence() {
    bool result = true;
    // num_paths at level l = 4 ** l;
    const int num_paths = 1 + 4 + 4 * 4 + 4 * 4 * 4;
    QuadtreePath paths[num_paths];
    int cnt = 0;
    std::uint32_t row0 = 0;
    std::uint32_t col0 = 0;
    const std::uint32_t level0 = 0;  // Level 0
    // Create the parents before the children.
    paths[cnt] = QuadtreePath(level0, row0, col0);
    cnt++;
    // Create 3 levels of QuadtreePaths
    for (unsigned int quad0 = 0; quad0 < 4; ++quad0) {
      std::uint32_t row1 = 0;
      std::uint32_t col1 = 0;
      const std::uint32_t level1 = 1;  // Level 1
      QuadtreePath::MagnifyQuadAddr(row0, col0, quad0, row1, col1);
      paths[cnt] = QuadtreePath(level1, row1, col1);
      cnt++;
      for (unsigned int quad1 = 0; quad1 < 4; ++quad1) {
        std::uint32_t row2 = 0;
        std::uint32_t col2 = 0;
        const std::uint32_t level2 = 2;  // Level 2
        QuadtreePath::MagnifyQuadAddr(row1, col1, quad1, row2, col2);
        paths[cnt] = QuadtreePath(level2, row2, col2);
        cnt++;
        for (unsigned int quad2 = 0; quad2 < 4; ++quad2) {
          std::uint32_t row3 = 0;
          std::uint32_t col3 = 0;
          const std::uint32_t level3 = 3;  // Level 3
          QuadtreePath::MagnifyQuadAddr(row2, col2, quad2, row3, col3);
          paths[cnt] = QuadtreePath(level3, row3, col3);
          cnt++;
        }
      }
    }
    assert(cnt == num_paths);
    result = (cnt == num_paths);
    std::uint64_t gen_sequences[num_paths];
    // Get generation sequences for paths
    for (cnt = 0; cnt < num_paths; ++cnt) {
      gen_sequences[cnt] = paths[cnt].GetGenerationSequence();
    }
    // Test correctness of generation sequences
    for (cnt = 0; cnt < num_paths; ++cnt) {
      for (int j = cnt+1; j < num_paths; ++j) {
        if (!(gen_sequences[cnt] < gen_sequences[j])) {
          fprintf(stderr, "%s:%d -> %s \n", __FILE__, __LINE__,
                  "Failed 'gen_sequences[cnt] < gen_sequences[j]'");
          result = false;
        }
      }
    }
    return result;
  }

  // In this test we take a set of test cells and get all 4 quad buffer offsets
  // to be used for its parent cells pixel buffer and compare those with golden
  // values. The pixel buffer is ordered from left to right and then from
  // bottom to top
  // ***  +----+----+
  // ***  | 2  | 3  |
  // ***  +----+----+
  // ***  | 0  | 1  |
  // ***  +----+----+
  bool TestQuadToBufferOffset() {
    bool result = true;
    struct {
      std::uint32_t tile_width;
      std::uint32_t tile_height;
      std::uint32_t golden_offsets[4];

    } test_cases[] = { { 10, 30, {0, 5, 150, 155} },
                       { 30, 10, {0, 15, 150, 165} },
    };
    const int num_input = sizeof(test_cases) / sizeof(test_cases[0]);
    for (int i = 0; i < num_input; ++i) {
      fprintf(stderr, "---------------- testing case %d ----------------\n", i);
      for (unsigned int quad = 0; quad < 4; ++quad) {
        std::uint32_t offset = QuadtreePath::QuadToBufferOffset(quad,
            test_cases[i].tile_width, test_cases[i].tile_height);
        if (!(test_cases[i].golden_offsets[quad] == offset)) {
          fprintf(stderr, "%s:%d -> %s, golden %u bad %u\n", __FILE__, __LINE__,
                  "Failed 'test_cases[i].golden_offsets[quad] == offset'",
                  test_cases[i].golden_offsets[quad], offset);
          result = false;
        }
      }
    }

    return result;
  }

  // MagnifyQuadAddr magnifies x,y th cell to 4 children cells.
  // For example 0,0 maps to the following children cells in the next level.
  // ***  +----+----+
  // ***  | 2  | 3  |
  // ***  +----+----+
  // ***  | 0  | 1  |
  // ***  +----+----+
  // In this test we take a set of test cells and get all 4 quad children cells
  // of those and compare those with golden values.
  bool TestMagnifyQuadAddr() {
    bool result = true;
    struct {
      std::uint32_t row;
      std::uint32_t col;
      std::uint32_t golden_row_col[4][2];

    } test_cases[] = { { 0, 0, {{0, 0}, {0, 1}, {1, 0}, {1, 1}} },
                       { 1, 1, {{2, 2}, {2, 3}, {3, 2}, {3, 3}} },
                       { 3, 4, {{6, 8}, {6, 9}, {7, 8}, {7, 9}} },
    };
    const int num_input = sizeof(test_cases) / sizeof(test_cases[0]);
    for (int i = 0; i < num_input; ++i) {
      fprintf(stderr, "---------------- testing case %d ----------------\n", i);
      for (unsigned int quad = 0; quad < 4; ++quad) {
        std::uint32_t out_row = 0;
        std::uint32_t out_col = 0;
        QuadtreePath::MagnifyQuadAddr(test_cases[i].row, test_cases[i].col,
                                      quad, out_row, out_col);
        if (!(test_cases[i].golden_row_col[quad][0] == out_row)) {
          fprintf(stderr, "%s:%d -> %s, golden %u bad %u\n", __FILE__, __LINE__,
                  "Failed 'test_cases[i].golden_row_col[quad][0] == out_row'",
                  test_cases[i].golden_row_col[quad][0], out_row);
          result = false;
        }
        if (!(test_cases[i].golden_row_col[quad][1] == out_col)) {
          fprintf(stderr, "%s:%d -> %s, golden %u bad %u\n", __FILE__, __LINE__,
                  "Failed 'test_cases[i].golden_row_col[quad][1] == out_col'",
                  test_cases[i].golden_row_col[quad][1], out_col);
          result = false;
        }
      }
    }
    return result;
  }

  // TestEndian - test endian convesion

  bool TestEndian() {
    bool result = true;

    for (std::uint32_t level = 0; level <= QuadtreePath::kMaxLevel; ++level) {
      LittleEndianWriteBuffer wbuffer(QuadtreePath::kStoreSize);

      QuadtreePath test_path(level, blist);
      wbuffer << test_path;
      std::uint64_t raw_path = HostToLittleEndian(test_path.path_);

      if (memcmp(&raw_path, wbuffer.data(), QuadtreePath::kStoreSize) != 0) {
        fprintf(stderr, "ERROR: StoreToLittleEndianBuffer error at level %u\n",
                level);
        result = false;
        continue;
      }

      QuadtreePath test_path2;
      LittleEndianReadBuffer rBuffer(wbuffer.data(), wbuffer.size());
      rBuffer >> test_path2;
      if (test_path.path_
          != test_path2.path_) {
        fprintf(stderr, "ERROR: LoadFromLittleEndianBuffer error at level %u, "
                "expected %016llX, got %016llX\n",
                level,
                (long long unsigned int)test_path.path_,
                (long long unsigned int)test_path2.path_);
        result = false;
      }
    }

    return result;
  }

  bool TestFromString() {
    bool result = true;
    std::string test1 = "";
    std::string test2 = "120013031";
    if (QuadtreePath(test1) !=
        QuadtreePath(test1.size(),
                     (const unsigned char*)(test1.data()))) {
      fprintf(stderr,
              "ERROR: TestFromString, empty failed\n");
      result = false;
    }
    if (QuadtreePath(test2) !=
        QuadtreePath(test2.size(),
                     (const unsigned char*)(test2.data()))) {
      fprintf(stderr,
              "ERROR: TestFromString, non-empty failed\n");
      result = false;
    }
    return result;
  }


  bool TestSubAddr() {
    bool result = true;
    QuadtreePath parent0("");
    QuadtreePath parent1("0231");
    QuadtreePath parent2("02312032");
    QuadtreePath   child("02312032132");
    QuadtreePath   other("321");

    if ((QuadtreePath(child,   8) != parent2) ||
        (QuadtreePath(parent2, 4) != parent1) ||
        (QuadtreePath(parent1, 0) != parent0)) {
      fprintf(stderr,
              "ERROR: TestSubAddr, construct with level limit failed\n");
      result = false;
    }

    if (!parent0.IsAncestorOf(child) ||
        !parent1.IsAncestorOf(child) ||
        !child.IsAncestorOf(child)) {
      fprintf(stderr,
              "ERROR: TestSubAddr: IsAncestorOf (postive case) failed\n");
      result = false;
    }

    if (child.IsAncestorOf(parent1) ||
        child.IsAncestorOf(parent0) ||
        other.IsAncestorOf(child)) {
      fprintf(stderr,
              "ERROR: TestSubAddr: IsAncestorOf (negative case) failed\n");
      result = false;
    }

    return result;
  }

  bool TestAsString(void) {
    bool result = true;
    std::string test1 = "";
    std::string test2 = "1";
    std::string test3 = "123012031203";

    if ((QuadtreePath(test1).AsString() != test1) ||
        (QuadtreePath(test2).AsString() != test2) ||
        (QuadtreePath(test3).AsString() != test3)) {
      result = false;
    }
    return result;
  }

  bool TestRelativePath() {
    bool result = true;
    QuadtreePath test0("");
    QuadtreePath test2("02");
    QuadtreePath test8("02312032");

    if ((QuadtreePath::RelativePath(test0, test0) != QuadtreePath()) ||
        (QuadtreePath::RelativePath(QuadtreePath(), test2) != test2) ||
        (QuadtreePath::RelativePath(test2, test2) != QuadtreePath()) ||
        (QuadtreePath::RelativePath(test2, test8) != QuadtreePath("312032"))) {
      result = false;
    }

    return result;
  }

  bool TestSubscript() {
    bool result = true;
    QuadtreePath test("230123232");

    if ((test[0] != 2) ||
        (test[3] != 1) ||
        (test[8] != 2)) {
      result = false;
    }

    return result;
  }

  bool TestAsIndex() {
    return ((QuadtreePath("").AsIndex(0) == 0) &&
            (QuadtreePath("0").AsIndex(1) == 0) &&
            (QuadtreePath("1").AsIndex(1) == 1) &&
            (QuadtreePath("2").AsIndex(1) == 2) &&
            (QuadtreePath("3").AsIndex(1) == 3) &&
            (QuadtreePath("00").AsIndex(2) == 0) &&
            (QuadtreePath("10").AsIndex(2) == 4) &&
            (QuadtreePath("32").AsIndex(2) == 14) &&
            (QuadtreePath("0000").AsIndex(4) == 0) &&
            (QuadtreePath("0132").AsIndex(4) == 30) &&
            (QuadtreePath("2013").AsIndex(4) == 135) &&
            (QuadtreePath("3333").AsIndex(4) == 255) &&
            (QuadtreePath("20130321").AsIndex(4) == 135) &&
            (QuadtreePath("33331234").AsIndex(4) == 255));
  }

  bool TestAdvanceInLevel() {
    const std::uint32_t kTestLevel = 4;
    const std::uint32_t kNodeCount = 256;
    const std::string kStartNode("0000");
    QuadtreePath qt_path(kStartNode);
    TestAssert(kTestLevel == qt_path.Level());

    for (std::uint32_t i = 0; i < kNodeCount-1; ++i) {
      QuadtreePath qt_last_path = qt_path;
      TestAssert(qt_path.AdvanceInLevel());
      TestAssert(qt_last_path < qt_path);
    }

    TestAssert(!qt_path.AdvanceInLevel());
    return true;
  }

  bool TestAdvance() {
    // Enumerate all paths of depth 5 or less
    QuadtreePath qt_path;
    const QuadtreePath final_qt_path("33333");
    std::uint32_t count = 0;

    while (qt_path != final_qt_path) {
      QuadtreePath prev_qt_path = qt_path;
      TestAssert(qt_path.Advance(final_qt_path.Level()));
      TestAssert(prev_qt_path < qt_path);
      ++count;
    }

    TestAssert(!qt_path.Advance(final_qt_path.Level()));
    TestAssert(count == 4 + 4*4 + 4*4*4 + 4*4*4*4 + 4*4*4*4*4);
    return true;
  }

  bool TestConcatenate() {
    for (size_t tot_size = 0; tot_size <= QuadtreePath::kMaxLevel; ++tot_size) {
      QuadtreePath test_path(tot_size, blist);

      for (size_t breakpt = 0; breakpt <= tot_size; ++breakpt) {
        QuadtreePath prefix = QuadtreePath(test_path, breakpt);
        QuadtreePath suffix = QuadtreePath::RelativePath(prefix, test_path);
        TestAssert(prefix.Concatenate(suffix) == test_path);
        TestAssert((prefix + suffix) == test_path);
      }
    }
    return true;
  }

  bool TestPostorder() {
    const QuadtreePath kPathRoot(  "");
    const QuadtreePath kPath0(     "0");
    const QuadtreePath kPath01332( "01332");
    const QuadtreePath kPath013320("013320");
    const QuadtreePath kPath013321("013321");
    const QuadtreePath kPath023321("023321");

    return QuadtreePath::IsPostorder(kPath0, kPathRoot)
      && !QuadtreePath::IsPostorder(kPathRoot, kPath0)

      && QuadtreePath::IsPostorder(kPath01332, kPathRoot)
      && !QuadtreePath::IsPostorder(kPathRoot, kPath01332)

      && QuadtreePath::IsPostorder(kPath013320, kPath01332)
      && !QuadtreePath::IsPostorder(kPath01332, kPath013320)

      && QuadtreePath::IsPostorder(kPath013320, kPath013321)
      && !QuadtreePath::IsPostorder(kPath013321, kPath013320)

      && QuadtreePath::IsPostorder(kPath013321, kPath023321)
      && !QuadtreePath::IsPostorder(kPath023321, kPath013321);
  }


  struct ChildCoordinateData {
    int tile_width;
    std::string qpath_string;
    std::string child_step;
    int row;
    int column;
    int width;
  };

  // Utility to check that ChildTileCoordinates results are as expected.
  // parent_qpath: the parent quadtree path
  // child_qpath: the quadtree path of the child quad node for which we're
  //              computing the tile coordinates.
  // tile_width: the width and height of the tile for the parent quadtree path.
  // row: the row index of the upper left part of the tile coordinates
  //      within the current tile.
  // column: the column index of the upper left part of the tile coordinates
  //         within the current tile.
  // width: the width and height of the tile coordinates within the current
  //        tile.
  bool CheckChildTileCoordinates(const ChildCoordinateData& test_data) {
    int row;
    int column;
    int width;
    QuadtreePath qpath(test_data.qpath_string);
    QuadtreePath child_qpath(test_data.qpath_string + test_data.child_step);
    EXPECT_TRUE(qpath.ChildTileCoordinates(test_data.tile_width,child_qpath,
                                           &row, &column, &width));
    if (test_data.row != row || test_data.column != column ||
        test_data.width != width) {
      fprintf(stderr, "Failed test of %s, %s\n", test_data.qpath_string.c_str(),
              test_data.child_step.c_str());
      qpath.ChildTileCoordinates(test_data.tile_width,child_qpath,
                                                 &row, &column, &width);
      EXPECT_EQ(test_data.row, row);
      EXPECT_EQ(test_data.column, column);
      EXPECT_EQ(test_data.width, width);
    }
    return true;
  }


  // ChildTileCoordinates finds the relative pixel coordinates within a
  // parent tile that corresponds to a child quad node.
  bool TestChildTileCoordinates() {
    std::string path_string("01332");
    int tile_width = 256;  // This is all we use practically.
    int tw_2 = tile_width / 2;
    int tw_4 = tile_width / 4;
    int tw_8 = tile_width / 8;
    int tw_2_4 = tw_2 + tw_4;
    std::string parent_path = "30121";
    std::string parent_path_empty = "";
    ChildCoordinateData test_data[] = {
      {tile_width, parent_path, "",  0, 0, tile_width},

      {tile_width, parent_path, "0",  tw_2, 0,    tw_2},
      {tile_width, parent_path, "1",  tw_2, tw_2, tw_2},
      {tile_width, parent_path, "2",  0,    tw_2, tw_2},
      {tile_width, parent_path, "3",  0,    0,    tw_2},

      {tile_width, parent_path, "00", tw_2_4,  0,    tw_4},
      {tile_width, parent_path, "01", tw_2_4,  tw_4, tw_4},
      {tile_width, parent_path, "02", tw_2,    tw_4, tw_4},
      {tile_width, parent_path, "03", tw_2,    0,    tw_4},

      {tile_width, parent_path, "10", tw_2_4,  tw_2,   tw_4},
      {tile_width, parent_path, "11", tw_2_4,  tw_2_4, tw_4},
      {tile_width, parent_path, "12", tw_2,    tw_2_4, tw_4},
      {tile_width, parent_path, "13", tw_2,    tw_2,   tw_4},

      {tile_width, parent_path, "20", tw_4, tw_2,   tw_4},
      {tile_width, parent_path, "21", tw_4, tw_2_4, tw_4},
      {tile_width, parent_path, "22", 0,    tw_2_4, tw_4},
      {tile_width, parent_path, "23", 0,    tw_2,   tw_4},

      {tile_width, parent_path, "30", tw_4, 0,    tw_4},
      {tile_width, parent_path, "31", tw_4, tw_4, tw_4},
      {tile_width, parent_path, "32", 0,    tw_4, tw_4},
      {tile_width, parent_path, "33", 0,    0,    tw_4},

      {tile_width, parent_path, "330", tw_8, 0,    tw_8},
      {tile_width, parent_path, "331", tw_8, tw_8, tw_8},
      {tile_width, parent_path, "332", 0,    tw_8, tw_8},
      {tile_width, parent_path, "333", 0,    0,    tw_8},

      // Must test past 8 levels between child and parent to be safe.
      // Once the quadnode is a pixel in size that's as far as we go.
      {tile_width, parent_path_empty, "1333333",     tw_2, tw_2, 2 },
      {tile_width, parent_path_empty, "13333333",    tw_2, tw_2, 1 },
      {tile_width, parent_path_empty, "1333333333",  tw_2, tw_2, 1 },

    };
    int count = static_cast<int>(sizeof(test_data)/sizeof(ChildCoordinateData));
    for(int i = 0; i < count; ++i) {
      CheckChildTileCoordinates(test_data[i]);
    }

    // Check that non-child does the right thing, which is returning false.
    QuadtreePath qpath("11");
    QuadtreePath not_child_qpath("101");
    int row;
    int column;
    int width;
    EXPECT_FALSE(qpath.ChildTileCoordinates(tile_width, not_child_qpath,
                                            &row, &column, &width));
    return true;
  }

  QuadtreePathUnitTest(void) : BaseClass("QuadtreePathUnitTest") {
    REGISTER(TestConstructors);
    REGISTER(TestParentChild);
    REGISTER(TestLess);
    REGISTER(TestGenerationSequence);
    REGISTER(TestQuadToBufferOffset);
    REGISTER(TestMagnifyQuadAddr);
    REGISTER(TestEndian);
    REGISTER(TestFromString);
    REGISTER(TestSubAddr);
    REGISTER(TestAsString);
    REGISTER(TestRelativePath);
    REGISTER(TestSubscript);
    REGISTER(TestAsIndex);
    REGISTER(TestAdvanceInLevel);
    REGISTER(TestAdvance);
    REGISTER(TestConcatenate);
    REGISTER(TestPostorder);
    REGISTER(TestChildTileCoordinates);
  }

};

const unsigned char QuadtreePathUnitTest::blist[QuadtreePath::kMaxLevel] = {
  1, 2, 0, 0, 3, 1, 1, 3, 2, 1, 0, 3,
  3, 3, 0, 1, 1, 2, 2, 2, 3, 0, 0, 1 };
const unsigned char QuadtreePathUnitTest::ascii_blist[QuadtreePath::kMaxLevel] = {
  '1', '2', '0', '0', '3', '1', '1', '3', '2', '1', '0', '3',
  '3', '3', '0', '1', '1', '2', '2', '2', '3', '0', '0', '1' };



int main(int argc, char *argv[]) {
  QuadtreePathUnitTest tests;
  return tests.Run();
}
