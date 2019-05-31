// Copyright 2019 Google Inc.
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

// Unit Tests for bounds tracker class

#include <limits.h>
#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include <utility>
#include <vector>
#include <gtest/gtest.h>
#include "common/khFileUtils.h"
#include "fusion/portableglobe/shared/bounds_tracker.h"

namespace fusion_portableglobe {

// Test class for the BoundsTracker
class BoundsTrackerTest : public testing::Test {
 protected:
  std::string test_directory;

  virtual void SetUp() {
    test_directory = khTmpDirPath() + "/bounds_tracker_test/";
    khEnsureParentDir(test_directory);
  }

  virtual void TearDown() {
    khPruneDir(test_directory);
  }
};

TEST_F(BoundsTrackerTest, SimpleImageryBounds) {
  BoundsTracker bt;
  bt.update("0", kImagePacket, 123);
  bt.update("00", kImagePacket, 123);
  bt.update("01", kImagePacket, 123);
  bt.update("02", kImagePacket, 123);

  EXPECT_EQ(bt.top, 2);
  EXPECT_EQ(bt.bottom, 3);
  EXPECT_EQ(bt.left, 0);
  EXPECT_EQ(bt.right, 1);

  EXPECT_EQ(bt.min_image_level, 1);
  EXPECT_EQ(bt.max_image_level, 2);
  EXPECT_EQ(bt.image_tile_channel, 123);

  // Expect default values everywhere else
  EXPECT_EQ(bt.terrain_tile_channel, UINT16_MAX);
  EXPECT_EQ(bt.vector_tile_channel, UINT16_MAX);

  EXPECT_EQ(bt.min_terrain_level, 32);
  EXPECT_EQ(bt.max_terrain_level, 0);
  EXPECT_EQ(bt.min_vector_level, 32);
  EXPECT_EQ(bt.max_vector_level, 0);
}

TEST_F(BoundsTrackerTest, SimpleVectorBounds) {
  BoundsTracker bt;
  bt.update("0",  kVectorPacket, 123);
  bt.update("00", kVectorPacket, 123);
  bt.update("01", kVectorPacket, 123);
  bt.update("02", kVectorPacket, 123);

  EXPECT_EQ(bt.min_vector_level, 1);
  EXPECT_EQ(bt.max_vector_level, 2);
  EXPECT_EQ(bt.vector_tile_channel, 123);

  // Expect default values everywhere else
  EXPECT_EQ(bt.top, 0);
  EXPECT_EQ(bt.bottom, UINT32_MAX);
  EXPECT_EQ(bt.left, UINT32_MAX);
  EXPECT_EQ(bt.right, 0);

  EXPECT_EQ(bt.image_tile_channel, UINT16_MAX);
  EXPECT_EQ(bt.terrain_tile_channel, UINT16_MAX);

  EXPECT_EQ(bt.min_image_level, 32);
  EXPECT_EQ(bt.max_image_level, 0);
  EXPECT_EQ(bt.min_terrain_level, 32);
  EXPECT_EQ(bt.max_terrain_level, 0);
}

TEST_F(BoundsTrackerTest, SimpleTerrainBounds) {
  BoundsTracker bt;
  bt.update("0",  kTerrainPacket, 123);
  bt.update("00", kTerrainPacket, 123);
  bt.update("01", kTerrainPacket, 123);
  bt.update("02", kTerrainPacket, 123);

  EXPECT_EQ(bt.min_terrain_level, 1);
  EXPECT_EQ(bt.max_terrain_level, 2);
  EXPECT_EQ(bt.terrain_tile_channel, 123);

  // Expect default values everywhere else
  EXPECT_EQ(bt.top, 0);
  EXPECT_EQ(bt.bottom, UINT32_MAX);
  EXPECT_EQ(bt.left, UINT32_MAX);
  EXPECT_EQ(bt.right, 0);

  EXPECT_EQ(bt.image_tile_channel, UINT16_MAX);
  EXPECT_EQ(bt.vector_tile_channel, UINT16_MAX);

  EXPECT_EQ(bt.min_image_level, 32);
  EXPECT_EQ(bt.max_image_level, 0);
  EXPECT_EQ(bt.min_vector_level, 32);
  EXPECT_EQ(bt.max_vector_level, 0);
}

void populateBoundsTracker(BoundsTracker& bt) {
    bt.update("0", kImagePacket, 123);
    bt.update("00", kImagePacket, 123);
    bt.update("01", kImagePacket, 123);
    bt.update("02", kImagePacket, 123);
    bt.update("023", kImagePacket, 123);
    bt.update("0231", kImagePacket, 123);
    bt.update("0232", kImagePacket, 123);

    bt.update("0", kVectorPacket, 24);
    bt.update("00", kVectorPacket, 24);
    bt.update("01", kVectorPacket, 24);
    bt.update("02", kVectorPacket, 24);
    bt.update("023", kVectorPacket, 24);
    bt.update("0231", kVectorPacket, 24);
    bt.update("0232", kVectorPacket, 24);

    // Terrain stops at a lower level than vector and imagery.
    bt.update("0", kTerrainPacket, 1);
    bt.update("00", kTerrainPacket, 1);
    bt.update("01", kTerrainPacket, 1);
    bt.update("02", kTerrainPacket, 1);
}

TEST_F(BoundsTrackerTest, MultipleBounds) {
    BoundsTracker bt;
    populateBoundsTracker(bt);

    EXPECT_EQ(bt.top, 8);
    EXPECT_EQ(bt.bottom, 9);
    EXPECT_EQ(bt.left, 5);
    EXPECT_EQ(bt.right, 5);

    EXPECT_EQ(bt.min_image_level, 1);
    EXPECT_EQ(bt.max_image_level, 4);
    EXPECT_EQ(bt.min_terrain_level, 1);
    EXPECT_EQ(bt.max_terrain_level, 2);
    EXPECT_EQ(bt.min_vector_level, 1);
    EXPECT_EQ(bt.max_vector_level, 4);

    EXPECT_EQ(bt.image_tile_channel, 123);
    EXPECT_EQ(bt.terrain_tile_channel, 1);
    EXPECT_EQ(bt.vector_tile_channel, 24);
}

TEST_F(BoundsTrackerTest, TestWriteJson) {
    BoundsTracker bt;
    populateBoundsTracker(bt);

    std::string filename = test_directory + "test.json";
    
    bt.write_json_file(filename);

    std::ifstream ins(filename.c_str());
    std::string str((std::istreambuf_iterator<char>(ins)),
                    std::istreambuf_iterator<char>());
    const std::string expected_json = "[\n"
      "  {\n"
      "    \"layer_id\": 0,\n"
      "    \"top\": 8,\n"
      "    \"bottom\": 9,\n"
      "    \"left\": 5,\n"
      "    \"right\": 5,\n"
      "    \"min_image_level\": 1,\n"
      "    \"max_image_level\": 4,\n"
      "    \"min_terrain_level\": 1,\n"
      "    \"max_terrain_level\": 2,\n"
      "    \"min_vector_level\": 1,\n"
      "    \"max_vector_level\": 4,\n"
      "    \"image_tile_channel\": 123,\n"
      "    \"terrain_tile_channel\": 1,\n"
      "    \"vector_tile_channel\": 24\n"
      "  }\n"
      "]\n";
    EXPECT_EQ(str, expected_json);
}

}  // namespace fusion_portableglobe

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
