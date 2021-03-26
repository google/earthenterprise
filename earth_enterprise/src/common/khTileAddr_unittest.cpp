// Copyright 2021 The Open GEE Contributors
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

#include <gtest/gtest.h>

#include "common/khTileAddr.h"

#include "common/khGeomUtils.h"

extern const khTilespaceFlat RasterProductTilespaceFlat;
extern const khTilespaceMercator RasterProductTilespaceMercator;

double MetersToDegrees(double meters) {
  return meters / khGeomUtils::khEarthCircumferencePerDegree;
}

double MetersToDegreesMerc(double meters) {
  return meters / khGeomUtilsMercator::khEarthCircumferencePerDegree;
}

TEST(TileAddrTest, FlatLowRes) {
  double degrees = MetersToDegrees(400000000);
  unsigned int level = RasterProductTilespaceFlat.LevelFromDegPixelSize(degrees);
  ASSERT_EQ(0, level);
}

TEST(TileAddrTest, FlatMedRes) {
  double degrees = MetersToDegrees(4000); // Same as bluemarble
  unsigned int level = RasterProductTilespaceFlat.LevelFromDegPixelSize(degrees);
  // The reported level will be 8 levels off from the level in the Fusion UI
  ASSERT_EQ(6 + 8, level);
}

TEST(TileAddrTest, FlatHighRes) {
  double degrees = MetersToDegrees(0.0187);
  unsigned int level = RasterProductTilespaceFlat.LevelFromDegPixelSize(degrees);
  ASSERT_EQ(31, level);
}

// Make sure the level tops out at 31
TEST(TileAddrTest, FlatTooHighRes) {
  double degrees = MetersToDegrees(0.00000001);
  unsigned int level = RasterProductTilespaceFlat.LevelFromDegPixelSize(degrees);
  ASSERT_EQ(31, level);
}

TEST(TileAddrTest, MercLowRes) {
  unsigned int level = RasterProductTilespaceMercator.LevelFromPixelSizeInMeters(400000000);
  ASSERT_EQ(0, level);
}

TEST(TileAddrTest, MercMedRes) {
  // Same resolution as bluemarble
  unsigned int level = RasterProductTilespaceMercator.LevelFromPixelSizeInMeters(4000);
  // The reported level will be 8 levels off from the level in the Fusion UI
  ASSERT_EQ(6 + 8, level);
}

TEST(TileAddrTest, MercHighRes) {
  unsigned int level = RasterProductTilespaceMercator.LevelFromPixelSizeInMeters(0.0187);
  ASSERT_EQ(31, level);
}

// Make sure the level tops out at 31
TEST(TileAddrTest, MercTooHighRes) {
  unsigned int level = RasterProductTilespaceMercator.LevelFromPixelSizeInMeters(0.00000001);
  ASSERT_EQ(31, level);
}

// The purpose of this test is to demonstrate that flat and mercator behave
// differently.
TEST(TileAddrTest, FlatVsMercDiff) {
  double meters = 2504689.303265145;
  double degrees = MetersToDegrees(meters);
  unsigned int mercLevel = RasterProductTilespaceMercator.LevelFromPixelSizeInMeters(meters);
  unsigned int flatLevel = RasterProductTilespaceFlat.LevelFromDegPixelSize(degrees);
  ASSERT_EQ(4, mercLevel);
  ASSERT_EQ(5, flatLevel);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
