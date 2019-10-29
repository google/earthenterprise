/*
 * Copyright 2019 The Open GEE Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <autoingest/sysman/InsetTilespaceIndex.h>
#include "common/quadtreepath.h"

using namespace std;

class QuadKeyTreeTest : public testing::Test {
 public:
  QuadKeyTreeTest()
  {
  }
};

// TEST DATA


// TEST FUNCTIONS
TEST_F(QuadKeyTreeTest, DoStuff) {
    std::array<std::string, 6> data {{ "01", "012", "0123", "1", "01230", "01231" }};
    const uchar path01[2] = {0, 1};
    const uchar path012[3] = {0, 1, 2};
    const uchar path0123[4] = {0, 1, 2, 3};
    const uchar path1[1] = {1};
    const uchar path01230[5] = {0, 1, 2, 3, 0};
    const uchar path01231[5] = {0, 1, 2, 3, 1};
    QuadtreePath qtp01(2, path01);
    QuadtreePath qtp012(3, path012);
    QuadtreePath qtp0123(4, path0123);
    QuadtreePath qtp1(1, path1);
    QuadtreePath qtp01230(5, path01230);
    QuadtreePath qtp01231(5, path01231);
    QuadKeyTree<std::string> tree;
    tree.AddElementAtQuadTreePath(qtp01, &data[0]);
    tree.AddElementAtQuadTreePath(qtp012, &data[1]);
    tree.AddElementAtQuadTreePath(qtp0123, &data[2]);
    tree.AddElementAtQuadTreePath(qtp1, &data[3]);
    tree.AddElementAtQuadTreePath(qtp01230, &data[4]);
    tree.AddElementAtQuadTreePath(qtp01231, &data[5]);
    std::vector<const std::string*> vec = tree.GetElementsAtQuadTreePath(qtp0123);
    std::vector<const std::string*> vec2 = tree.GetElementsAtQuadTreePath(qtp0123, false);
    std::cout << "vec\n";
    for(auto & str : vec)
      std::cout << *str << "\n";
    std::cout << "vec2\n";
    for(auto & str : vec2)
      std::cout << *str << "\n";
    ASSERT_EQ(vec.size(), 5);
    ASSERT_EQ(vec2.size(), 1);
    // ASSERT_EQ(qtp01.AsString(), "01");
    // ASSERT_EQ(qtp012.AsString(), "012");
    // ASSERT_EQ(qtp0123.AsString(), "0123");
    // ASSERT_EQ(qtp1.AsString(), "1");
    //QuadKeyTree
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
