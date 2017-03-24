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

//

#include <stdio.h>
#include <string.h>
#include "tree_utils.h"
#include <UnitTest.h>

#define ARRAYSIZE(a) \
  ((sizeof(a) / sizeof(*(a))) / \
   static_cast<size_t>(!(sizeof(a) % sizeof(*(a)))))

using namespace qtpacket;

class TreeUtilsUnitTest : public UnitTest<TreeUtilsUnitTest> {
 public:
  TreeUtilsUnitTest()
      : BaseClass("TreeUtilsUnitTest") {
    REGISTER(TestNodeCount);
    REGISTER(TestNumberingConversions);
    REGISTER(TestTraversalPaths);
    REGISTER(TestChildren);
    REGISTER(TestLevel);
    REGISTER(TestParent);
  }

  bool TestNodeCount() {
    int count00 = TreeNumbering(0, 0, true).num_nodes();
    int count11 = TreeNumbering(1, 1, true).num_nodes();
    int count21 = TreeNumbering(2, 1, true).num_nodes();
    int count22 = TreeNumbering(2, 2, true).num_nodes();
    int count23 = TreeNumbering(2, 3, true).num_nodes();
    int count24 = TreeNumbering(2, 4, true).num_nodes();
    int count41 = TreeNumbering(4, 1, true).num_nodes();
    int count42 = TreeNumbering(4, 2, true).num_nodes();
    int count43 = TreeNumbering(4, 3, true).num_nodes();
    int count44 = TreeNumbering(4, 4, true).num_nodes();
    int count45 = TreeNumbering(4, 5, true).num_nodes();
    CHECK_EQ(count00, 0);
    CHECK_EQ(count11, 1);
    CHECK_EQ(count21, 1);
    CHECK_EQ(count22, 3);
    CHECK_EQ(count23, 7);
    CHECK_EQ(count24, 15);
    CHECK_EQ(count41, 1);
    CHECK_EQ(count42, 5);
    CHECK_EQ(count43, 21);
    CHECK_EQ(count44, 85);
    CHECK_EQ(count45, 341);
    return true;
  }

  bool TestNumberingConversions() {
    //
    // Child nodes (mangled second row)
    //
    TreeNumbering tree24(2, 4, true);
    static const int tree24_tests[] = {
      0, 1, 2, 5, 3, 4, 6, 7, 8, 9, 12, 10, 11, 13, 14
    };
    for (int i = 0; i < static_cast<int>(ARRAYSIZE(tree24_tests)); ++i) {
      CHECK_EQ(tree24.SubindexToInorder(i),  tree24_tests[i]);
      CHECK_EQ(tree24.InorderToSubindex(tree24_tests[i]), i);
    }

    TreeNumbering tree45(4, 5, true);
    static const int tree45_tests[] = {
      0, 1, 2, 23, 44, 65, 3, 8, 13, 18, 24
    };
    for (int i = 0; i < static_cast<int>(ARRAYSIZE(tree45_tests)); ++i) {
      CHECK_EQ(tree45.SubindexToInorder(i), tree45_tests[i]);
      CHECK_EQ(tree45.InorderToSubindex(tree45_tests[i]), i);
    }

    //
    // Child nodes (normal second row)
    //
    TreeNumbering tree242(2, 4, false);
    static const int tree242_tests[] = {
      0, 1, 8, 2, 5, 9, 12, 3, 4, 6, 7, 10, 11, 13, 14
    };
    for (int i = 0; i < static_cast<int>(ARRAYSIZE(tree242_tests)); ++i) {
      CHECK_EQ(tree242.SubindexToInorder(i),  tree242_tests[i]);
      CHECK_EQ(tree242.InorderToSubindex(tree242_tests[i]), i);
    }

    TreeNumbering tree452(4, 5, false);
    static const int tree452_tests[] = {
      0, 1, 86, 171, 256, 2, 23, 44, 65, 87, 108
    };
    for (int i = 0; i < static_cast<int>(ARRAYSIZE(tree452_tests)); ++i) {
      CHECK_EQ(tree452.SubindexToInorder(i), tree452_tests[i]);
      CHECK_EQ(tree452.InorderToSubindex(tree452_tests[i]), i);
    }
    return true;
  }

  bool TestTraversalPaths() {
    TreeNumbering tree45(4, 5, true);

    struct PathTest {
      const char *path;
      int inorder;
      int subindex;
    };
    static const PathTest tests45[] = {
      { "",     0,   0 },
      { "0",    1,   1 },
      { "1",    86,  86 },
      { "2",    171, 171 },
      { "3",    256, 256 },
      { "01",   23,  3 },
      { "02",   44,  4 },
      { "0333", 85,  85 },
      { "1333", 170, 170 },
      { "00",   2,   2 },
      { "000",  3,   6 },
      { "0000", 4,   22 },
      { "3333", 340, 340 },
    };

    for (int i = 0; i < static_cast<int>(ARRAYSIZE(tests45)); ++i) {
      // Convert path to right format
      std::string str = MakeTraversalPath(tests45[i].path);
      CHECK_EQ(tree45.TraversalPathToInorder(str), tests45[i].inorder);
      CHECK_EQ(tree45.TraversalPathToSubindex(str), tests45[i].subindex);

      std::string path =
        tree45.InorderToTraversalPath(tests45[i].inorder).AsString();
      CHECK_STREQ(path.c_str(), tests45[i].path);

      path = tree45.SubindexToTraversalPath(tests45[i].subindex).AsString();
      CHECK_STREQ(path.c_str(), tests45[i].path);
    }
    return true;
  }

  bool TestChildren() {
    int children[4];

    //
    // Inorder
    //
    TreeNumbering tree41(4, 1, true);
    CHECK_EQ(false, tree41.GetChildrenInorder(0, children));

    TreeNumbering tree45(4, 5, true);
    CHECK_EQ(true, tree45.GetChildrenInorder(0, children));
    CHECK_EQ(1, children[0]);
    CHECK_EQ(86, children[1]);
    CHECK_EQ(171, children[2]);
    CHECK_EQ(256, children[3]);

    CHECK_EQ(true, tree45.GetChildrenInorder(1, children));
    CHECK_EQ(2, children[0]);
    CHECK_EQ(23, children[1]);
    CHECK_EQ(44, children[2]);
    CHECK_EQ(65, children[3]);

    // Leaf node
    CHECK_EQ(false, tree45.GetChildrenInorder(tree45.num_nodes() - 1,
                                              children));

    //
    // Subindex
    //
    CHECK_EQ(false, tree41.GetChildrenSubindex(0, children));

    CHECK_EQ(true, tree45.GetChildrenSubindex(0, children));
    CHECK_EQ(1, children[0]);
    CHECK_EQ(86, children[1]);
    CHECK_EQ(171, children[2]);
    CHECK_EQ(256, children[3]);

    CHECK_EQ(true, tree45.GetChildrenSubindex(1, children));
    CHECK_EQ(2, children[0]);
    CHECK_EQ(3, children[1]);
    CHECK_EQ(4, children[2]);
    CHECK_EQ(5, children[3]);

    // Leaf node
    CHECK_EQ(false, tree45.GetChildrenSubindex(tree45.num_nodes() - 1,
                                               children));
    return true;
  }

  bool TestLevel() {
    //
    // Subindex
    //
    TreeNumbering tree41(4, 1, true);
    CHECK_EQ(0, tree41.GetLevelSubindex(0));

    TreeNumbering tree45(4, 5, true);
    CHECK_EQ(0, tree45.GetLevelSubindex(0));
    CHECK_EQ(1, tree45.GetLevelSubindex(1));
    CHECK_EQ(2, tree45.GetLevelSubindex(2));
    CHECK_EQ(2, tree45.GetLevelSubindex(3));
    CHECK_EQ(1, tree45.GetLevelSubindex(86));
    CHECK_EQ(2, tree45.GetLevelSubindex(87));
    CHECK_EQ(1, tree45.GetLevelSubindex(256));
    CHECK_EQ(2, tree45.GetLevelSubindex(257));
    CHECK_EQ(4, tree45.GetLevelSubindex(340));

    //
    // Inorder
    //
    CHECK_EQ(0, tree41.GetLevelInorder(0));

    CHECK_EQ(0, tree45.GetLevelInorder(0));
    CHECK_EQ(1, tree45.GetLevelInorder(1));
    CHECK_EQ(2, tree45.GetLevelInorder(2));
    CHECK_EQ(3, tree45.GetLevelInorder(3));
    CHECK_EQ(3, tree45.GetLevelInorder(8));
    CHECK_EQ(4, tree45.GetLevelInorder(4));
    CHECK_EQ(1, tree45.GetLevelInorder(86));
    CHECK_EQ(2, tree45.GetLevelInorder(87));
    CHECK_EQ(1, tree45.GetLevelInorder(256));
    CHECK_EQ(2, tree45.GetLevelInorder(257));
    CHECK_EQ(3, tree45.GetLevelInorder(258));
    CHECK_EQ(4, tree45.GetLevelInorder(340));
    return true;
  }


  bool TestParent() {
    struct {
      int inorder;
      int parent;
    } tests45[] = {
      { 0,  -1 },
      { 1,   0 },
      { 86,  0 },
      { 171, 0 },
      { 256, 0 },
      { 257, 256 },
      { 258, 257 },
      { 2,   1 },
      { 23,  1 },
      { 44,  1 },
      { 65,  1 },
      { 3,   2 },
      { 8,   2 },
      { 13,  2 },
      { 18,  2 },
    };

    TreeNumbering tree45(4, 5, true);
    for (int i = 0; i < static_cast<int>(ARRAYSIZE(tests45)); ++i) {
      CHECK_EQ(tree45.GetParentInorder(tests45[i].inorder),
               tests45[i].parent);

      int subindex = tree45.InorderToSubindex(tests45[i].inorder);
      CHECK_EQ(tree45.GetParentSubindex(subindex),
               tests45[i].parent == -1 ? -1 :
               tree45.InorderToSubindex(tests45[i].parent));
    }
    return true;
  }

 private:

  std::string MakeTraversalPath(const char *str) {
    std::string path;
    while (*str) {
      path.push_back(*str - '0');
      ++str;
    }
    return path;
  }
};

int main(int argc, char **argv) {
  TreeUtilsUnitTest f;
  return f.Run();
}
