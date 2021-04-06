/*
 * Copyright 2021 The Open GEE Contributors
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

#include "vector.h"

//////////////////////////////////////////////////////////////////////////////
// etVec3

TEST(etVec3, Construct) {
  etVec3d v1;
  EXPECT_EQ(0, v1.elem[0]);
  EXPECT_EQ(0, v1.elem[1]);
  EXPECT_EQ(0, v1.elem[2]);

  etVec3d v2(3, 4, 5);
  EXPECT_EQ(3, v2.elem[0]);
  EXPECT_EQ(4, v2.elem[1]);
  EXPECT_EQ(5, v2.elem[2]);

  etVec3d v3(8);
  EXPECT_EQ(8, v3.elem[0]);
  EXPECT_EQ(8, v3.elem[1]);
  EXPECT_EQ(8, v3.elem[2]);
}

TEST(etVec3, Length) {
  etVec3d v1(3, 4, 5);
  EXPECT_FLOAT_EQ(7.0710678, v1.length());
  EXPECT_EQ(50, v1.length2());

  etVec3d v2;
  EXPECT_EQ(0, v2.length());
  EXPECT_EQ(0, v2.length2());
}

TEST(etVec3, Normalize) {
  etVec3d v1(8, 9, 10);
  v1.normalize();
  EXPECT_FLOAT_EQ(0.51110125, v1.elem[0]);
  EXPECT_FLOAT_EQ(0.57498891, v1.elem[1]);
  EXPECT_FLOAT_EQ(0.63887656, v1.elem[2]);

  etVec3d v2;
  v2.normalize();
  EXPECT_FLOAT_EQ(0, v2.elem[0]);
  EXPECT_FLOAT_EQ(0, v2.elem[1]);
  EXPECT_FLOAT_EQ(0, v2.elem[2]);
}

TEST(etVec3, CrossProd) {
  etVec3d v1(1, 2, 3);
  etVec3d v2(4, 9, 3);
  etVec3d v3;
  v3.cross(v1, v2);
  EXPECT_EQ(-21, v3.elem[0]);
  EXPECT_EQ(9, v3.elem[1]);
  EXPECT_EQ(1, v3.elem[2]);
}

TEST(etVec3, DotProd) {
  etVec3d v1(9, 8, 7);
  etVec3d v2(3, 7, 9);
  double dot = v1.dot(v2);
  EXPECT_EQ(146, dot);
}

TEST(etVec3, SubscriptOperators) {
  etVec3d v1(2, 3, 4);
  EXPECT_EQ(3, v1[1]);
  const etVec3d v2(v1);
  EXPECT_EQ(4, v2[2]);
}

//////////////////////////////////////////////////////////////////////////////
// etFace3

TEST(etFace3, Construct) {
  etFace3i f1;
  EXPECT_EQ(0, f1[0]);
  EXPECT_EQ(0, f1[1]);
  EXPECT_EQ(0, f1[2]);

  etFace3i f2(8, 7, 5);
  EXPECT_EQ(8, f2[0]);
  EXPECT_EQ(7, f2[1]);
  EXPECT_EQ(5, f2[2]);

  etFace3i f3(12);
  EXPECT_EQ(12, f3[0]);
  EXPECT_EQ(12, f3[1]);
  EXPECT_EQ(12, f3[2]);
}

TEST(etFace3, Set) {
  etFace3i f1;
  f1.set(5);
  EXPECT_EQ(5, f1[0]);
  EXPECT_EQ(5, f1[1]);
  EXPECT_EQ(5, f1[2]);

  f1.set(4, 3, 2);
  EXPECT_EQ(4, f1[0]);
  EXPECT_EQ(3, f1[1]);
  EXPECT_EQ(2, f1[2]);

  etFace3i f2(7, 8, 9);
  f1.set(f2);
  EXPECT_EQ(7, f1[0]);
  EXPECT_EQ(8, f1[1]);
  EXPECT_EQ(9, f1[2]);
}

TEST(etFace3, Get) {
  etFace3i f1(7, 2, 4);
  etFace3i f2;
  f1.get(f2);
  EXPECT_EQ(7, f2[0]);
  EXPECT_EQ(2, f2[1]);
  EXPECT_EQ(4, f2[2]);
}

TEST(etFace3, SubscriptOperators) {
  etFace3i f1(2, 3, 4);
  EXPECT_EQ(3, f1[1]);
  const etFace3i f2(f1);
  EXPECT_EQ(4, f2[2]);
}

//////////////////////////////////////////////////////////////////////////////
// etBoundingBox3

TEST(etBoundingBox3, Construct) {
  etBoundingBox3<double> b1;
  EXPECT_LT(0, b1.min[0]);
  EXPECT_LT(0, b1.min[1]);
  EXPECT_LT(0, b1.min[2]);
  EXPECT_GT(0, b1.max[0]);
  EXPECT_GT(0, b1.max[1]);
  EXPECT_GT(0, b1.max[2]);
}

TEST(etBoundingBox3, Add) {
  etBoundingBox3<double> b1;
  b1.add(1, 1, 1);
  EXPECT_FLOAT_EQ(1, b1.min[0]);
  EXPECT_FLOAT_EQ(1, b1.min[1]);
  EXPECT_FLOAT_EQ(1, b1.min[2]);
  EXPECT_FLOAT_EQ(1, b1.max[0]);
  EXPECT_FLOAT_EQ(1, b1.max[1]);
  EXPECT_FLOAT_EQ(1, b1.max[2]);

  b1.add(-1, -2, -3);
  EXPECT_FLOAT_EQ(-1, b1.min[0]);
  EXPECT_FLOAT_EQ(-2, b1.min[1]);
  EXPECT_FLOAT_EQ(-3, b1.min[2]);
  EXPECT_FLOAT_EQ(1, b1.max[0]);
  EXPECT_FLOAT_EQ(1, b1.max[1]);
  EXPECT_FLOAT_EQ(1, b1.max[2]);

  b1.add(-5, 4, -2);
  EXPECT_FLOAT_EQ(-5, b1.min[0]);
  EXPECT_FLOAT_EQ(-2, b1.min[1]);
  EXPECT_FLOAT_EQ(-3, b1.min[2]);
  EXPECT_FLOAT_EQ(1, b1.max[0]);
  EXPECT_FLOAT_EQ(4, b1.max[1]);
  EXPECT_FLOAT_EQ(1, b1.max[2]);

  // Adding the same vector should leave the bounding box the same
  b1.add(-5, 4, -2);
  EXPECT_FLOAT_EQ(-5, b1.min[0]);
  EXPECT_FLOAT_EQ(-2, b1.min[1]);
  EXPECT_FLOAT_EQ(-3, b1.min[2]);
  EXPECT_FLOAT_EQ(1, b1.max[0]);
  EXPECT_FLOAT_EQ(4, b1.max[1]);
  EXPECT_FLOAT_EQ(1, b1.max[2]);
}

//////////////////////////////////////////////////////////////////////////////
// etMat3

TEST(etMat3, ConstructAndSetRow) {
  // This constructor sets nothing, so there are no guarantees to check
  etMat3d m1;

  etMat3d m2;
  m2.setRow(etVec3d(1, 2, 3), 0);
  m2.setRow(etVec3d(4, 5, 6), 1);
  m2.setRow(etVec3d(7, 8, 9), 2);
  EXPECT_EQ(1, m2.elem[0][0]);
  EXPECT_EQ(2, m2.elem[0][1]);
  EXPECT_EQ(3, m2.elem[0][2]);
  EXPECT_EQ(4, m2.elem[1][0]);
  EXPECT_EQ(5, m2.elem[1][1]);
  EXPECT_EQ(6, m2.elem[1][2]);
  EXPECT_EQ(7, m2.elem[2][0]);
  EXPECT_EQ(8, m2.elem[2][1]);
  EXPECT_EQ(9, m2.elem[2][2]);

  etMat3d m3(m2);
  EXPECT_EQ(1, m3.elem[0][0]);
  EXPECT_EQ(2, m3.elem[0][1]);
  EXPECT_EQ(3, m3.elem[0][2]);
  EXPECT_EQ(4, m3.elem[1][0]);
  EXPECT_EQ(5, m3.elem[1][1]);
  EXPECT_EQ(6, m3.elem[1][2]);
  EXPECT_EQ(7, m3.elem[2][0]);
  EXPECT_EQ(8, m3.elem[2][1]);
  EXPECT_EQ(9, m3.elem[2][2]);
}

TEST(etMat3, SetColAndTransform) {
  etMat3d m1;
  m1.setCol(etVec3d(1, 2, 3), 0);
  m1.setCol(etVec3d(4, 5, 6), 1);
  m1.setCol(etVec3d(7, 8, 9), 2);
  EXPECT_EQ(1, m1.elem[0][0]);
  EXPECT_EQ(2, m1.elem[1][0]);
  EXPECT_EQ(3, m1.elem[2][0]);
  EXPECT_EQ(4, m1.elem[0][1]);
  EXPECT_EQ(5, m1.elem[1][1]);
  EXPECT_EQ(6, m1.elem[2][1]);
  EXPECT_EQ(7, m1.elem[0][2]);
  EXPECT_EQ(8, m1.elem[1][2]);
  EXPECT_EQ(9, m1.elem[2][2]);

  etVec3d v1(-1, -2, -3);
  etVec3d v2;
  m1.transform(v1, v2);
  EXPECT_EQ(-30, v2.elem[0]);
  EXPECT_EQ(-36, v2.elem[1]);
  EXPECT_EQ(-42, v2.elem[2]);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
