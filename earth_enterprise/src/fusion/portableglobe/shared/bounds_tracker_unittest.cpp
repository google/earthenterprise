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

  EXPECT_TRUE(bt.channels.find(123) != bt.channels.end());

  EXPECT_EQ(bt.channels[123].top, 2);
  EXPECT_EQ(bt.channels[123].bottom, 3);
  EXPECT_EQ(bt.channels[123].left, 0);
  EXPECT_EQ(bt.channels[123].right, 1);

  EXPECT_EQ(bt.channels[123].min_level, 1);
  EXPECT_EQ(bt.channels[123].max_level, 2);
  EXPECT_EQ(bt.channels[123].channel, 123);
  EXPECT_EQ(bt.channels[123].type, kImagePacket);
}

TEST_F(BoundsTrackerTest, SimpleVectorBounds) {
  BoundsTracker bt;
  bt.update("0",  kVectorPacket, 123);
  bt.update("00", kVectorPacket, 123);
  bt.update("01", kVectorPacket, 123);
  bt.update("02", kVectorPacket, 123);

  EXPECT_EQ(bt.channels[123].top, 2);
  EXPECT_EQ(bt.channels[123].bottom, 3);
  EXPECT_EQ(bt.channels[123].left, 0);
  EXPECT_EQ(bt.channels[123].right, 1);

  EXPECT_EQ(bt.channels[123].min_level, 1);
  EXPECT_EQ(bt.channels[123].max_level, 2);
  EXPECT_EQ(bt.channels[123].channel, 123);
  EXPECT_EQ(bt.channels[123].type, kVectorPacket);
}

TEST_F(BoundsTrackerTest, SimpleTerrainBounds) {
  BoundsTracker bt;
  bt.update("0",  kTerrainPacket, 123);
  bt.update("00", kTerrainPacket, 123);
  bt.update("01", kTerrainPacket, 123);
  bt.update("02", kTerrainPacket, 123);

  EXPECT_EQ(bt.channels[123].top, 2);
  EXPECT_EQ(bt.channels[123].bottom, 3);
  EXPECT_EQ(bt.channels[123].left, 0);
  EXPECT_EQ(bt.channels[123].right, 1);

  EXPECT_EQ(bt.channels[123].min_level, 1);
  EXPECT_EQ(bt.channels[123].max_level, 2);
  EXPECT_EQ(bt.channels[123].channel, 123);
  EXPECT_EQ(bt.channels[123].type, kTerrainPacket);
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

    EXPECT_EQ(bt.channels[123].top, 8);
    EXPECT_EQ(bt.channels[123].bottom, 9);
    EXPECT_EQ(bt.channels[123].left, 5);
    EXPECT_EQ(bt.channels[123].right, 5);

    EXPECT_EQ(bt.channels[123].min_level, 1);
    EXPECT_EQ(bt.channels[123].max_level, 4);
    EXPECT_EQ(bt.channels[123].channel, 123);
    EXPECT_EQ(bt.channels[123].type, kImagePacket);


    EXPECT_EQ(bt.channels[24].top, 8);
    EXPECT_EQ(bt.channels[24].bottom, 9);
    EXPECT_EQ(bt.channels[24].left, 5);
    EXPECT_EQ(bt.channels[24].right, 5);

    EXPECT_EQ(bt.channels[24].min_level, 1);
    EXPECT_EQ(bt.channels[24].max_level, 4);
    EXPECT_EQ(bt.channels[24].channel, 24);
    EXPECT_EQ(bt.channels[24].type, kVectorPacket);

    EXPECT_EQ(bt.channels[1].top, 2);
    EXPECT_EQ(bt.channels[1].bottom, 3);
    EXPECT_EQ(bt.channels[1].left, 0);
    EXPECT_EQ(bt.channels[1].right, 1);

    EXPECT_EQ(bt.channels[1].min_level, 1);
    EXPECT_EQ(bt.channels[1].max_level, 2);
    EXPECT_EQ(bt.channels[1].channel, 1);
    EXPECT_EQ(bt.channels[1].type, kTerrainPacket);
}

  TEST_F(BoundsTrackerTest, TestWriteJson) {

    // This is kind of hacky, but easier than adding a new JSON parser dependency.
    const std::string expected_json = "[\n"
      "  {\n"
      "    \"layer_id\": 1,\n"
      "    \"top\": 2,\n"
      "    \"bottom\": 3,\n"
      "    \"left\": 0,\n"
      "    \"right\": 1,\n"
      "    \"min_image_level\": 1,\n"
      "    \"max_image_level\": 2,\n"
      "    \"channel\": 1,\n"
      "    \"type\": \"Terrain\"\n"
      "  },\n"
      "  {\n"
      "    \"layer_id\": 24,\n"
      "    \"top\": 8,\n"
      "    \"bottom\": 9,\n"
      "    \"left\": 5,\n"
      "    \"right\": 5,\n"
      "    \"min_image_level\": 1,\n"
      "    \"max_image_level\": 4,\n"
      "    \"channel\": 24,\n"
      "    \"type\": \"Vector\"\n"
      "  },\n"
      "  {\n"
      "    \"layer_id\": 123,\n"
      "    \"top\": 8,\n"
      "    \"bottom\": 9,\n"
      "    \"left\": 5,\n"
      "    \"right\": 5,\n"
      "    \"min_image_level\": 1,\n"
      "    \"max_image_level\": 4,\n"
      "    \"channel\": 123,\n"
      "    \"type\": \"Image\"\n"
      "  }\n"
      "]\n";

    BoundsTracker bt;
    populateBoundsTracker(bt);

    std::string filename = test_directory + "test.json";
    
    bt.write_json_file(filename);
    std::ifstream ins(filename.c_str());

    std::string str{ std::istreambuf_iterator<char>(ins),
                     std::istreambuf_iterator<char>()};


    EXPECT_EQ(str, expected_json);
  }

}  // namespace fusion_portableglobe

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
