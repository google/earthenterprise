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
// std::array<std::string, 9> data {{ "this", "is", "some", "string", "data", 
//                                    "one fish", "two fish" , "red fish", "blue fish"}};
QuadtreePath qtp01("01");
QuadtreePath qtp011("011");
QuadtreePath qtp012("012");
QuadtreePath qtp0123("0123");
QuadtreePath qtp1("1");
QuadtreePath qtp01230("01230");
QuadtreePath qtp01231("01232");
QuadtreePath qtpEmpty("");

// TEST FUNCTIONS
TEST_F(QuadKeyTreeTest, TestNormal) {
    QuadKeyTree<QuadtreePath> tree;
    tree.AddElementAtQuadTreePath(qtpEmpty, &qtpEmpty);
    tree.AddElementAtQuadTreePath(qtp1, &qtp1);
    tree.AddElementAtQuadTreePath(qtp01, &qtp01);
    tree.AddElementAtQuadTreePath(qtp011, &qtp011);
    tree.AddElementAtQuadTreePath(qtp012, &qtp012);
    tree.AddElementAtQuadTreePath(qtp0123, &qtp0123);
    
    std::vector<const QuadtreePath*> vec = tree.GetElementsAtQuadTreePath(qtp012, 3);

    // Should have matched everything except qtp011
    ASSERT(std::find(vec.begin(), vec.end(), &qtpEmpty) != vec.end());
    ASSERT(std::find(vec.begin(), vec.end(), &qtp01)    != vec.end());
    ASSERT(std::find(vec.begin(), vec.end(), &qtp012)   != vec.end());
    ASSERT(std::find(vec.begin(), vec.end(), &qtp0123)  != vec.end());
    ASSERT_EQ(vec.size(), 4);
}

TEST_F(QuadKeyTreeTest, EmptyQuadtreePathMatchAll) {
    QuadKeyTree<QuadtreePath> tree;
    tree.AddElementAtQuadTreePath(qtpEmpty, &qtpEmpty);
    tree.AddElementAtQuadTreePath(qtp1, &qtp1);
    tree.AddElementAtQuadTreePath(qtp01, &qtp01);
    tree.AddElementAtQuadTreePath(qtp011, &qtp011);
    tree.AddElementAtQuadTreePath(qtp0123, &qtp0123);

    std::vector<const QuadtreePath*> vec = tree.GetElementsAtQuadTreePath(qtpEmpty, 0);

    ASSERT(std::find(vec.begin(), vec.end(), &qtpEmpty) != vec.end());
    ASSERT(std::find(vec.begin(), vec.end(), &qtp1) != vec.end());
    ASSERT(std::find(vec.begin(), vec.end(), &qtp01)    != vec.end());
    ASSERT(std::find(vec.begin(), vec.end(), &qtp011)   != vec.end());
    ASSERT(std::find(vec.begin(), vec.end(), &qtp0123)  != vec.end());
    ASSERT_EQ(vec.size(), 5);
}

TEST_F(QuadKeyTreeTest, FindBasedOnLevel) {
    QuadKeyTree<QuadtreePath> tree;
    tree.AddElementAtQuadTreePath(qtpEmpty, &qtpEmpty);
    tree.AddElementAtQuadTreePath(qtp1, &qtp1);
    tree.AddElementAtQuadTreePath(qtp01, &qtp01);
    tree.AddElementAtQuadTreePath(qtp011, &qtp011);
    tree.AddElementAtQuadTreePath(qtp0123, &qtp0123);

    // qtp0123 has a level of 4, but we're going to request all elements that match it
    // starting at level 2. So, this should be the same result as matching qtp01
    // at level 2.
    std::vector<const QuadtreePath*> vec = tree.GetElementsAtQuadTreePath(qtp0123, 2);
    std::vector<const QuadtreePath*> vec2 = tree.GetElementsAtQuadTreePath(qtp01, 2);

    ASSERT(std::find(vec.begin(), vec.end(), &qtpEmpty) != vec.end());
    ASSERT(std::find(vec.begin(), vec.end(), &qtp01)    != vec.end());
    ASSERT(std::find(vec.begin(), vec.end(), &qtp011)   != vec.end());
    ASSERT(std::find(vec.begin(), vec.end(), &qtp0123)  != vec.end());
    ASSERT_EQ(vec.size(), 4);
    ASSERT_EQ(vec, vec2);
}

TEST_F(QuadKeyTreeTest, FindWithLevelGreaterThanPath) {
    QuadKeyTree<QuadtreePath> tree;
    tree.AddElementAtQuadTreePath(qtpEmpty, &qtpEmpty);
    tree.AddElementAtQuadTreePath(qtp1, &qtp1);
    tree.AddElementAtQuadTreePath(qtp01, &qtp01);
    tree.AddElementAtQuadTreePath(qtp011, &qtp011);
    tree.AddElementAtQuadTreePath(qtp0123, &qtp0123);

    std::vector<const QuadtreePath*> vec = tree.GetElementsAtQuadTreePath(qtp01, 5);

    // Should have matched everything except qtp1
    ASSERT(std::find(vec.begin(), vec.end(), &qtpEmpty) != vec.end());
    ASSERT(std::find(vec.begin(), vec.end(), &qtp01)    != vec.end());
    ASSERT(std::find(vec.begin(), vec.end(), &qtp011)   != vec.end());
    ASSERT(std::find(vec.begin(), vec.end(), &qtp0123)  != vec.end());
    ASSERT_EQ(vec.size(), 4);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
