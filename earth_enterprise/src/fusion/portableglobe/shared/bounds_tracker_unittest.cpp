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

void populateSimpleBoundsTracker(BoundsTracker& bt, PacketType type) {
  bt.update(123, type, "0");
  bt.update(123, type, "00");
  bt.update(123, type, "01");
  bt.update(123, type, "02");
}

void populateBoundsTracker(BoundsTracker& bt) {
  bt.update(123, kImagePacket, "0");
  bt.update(123, kImagePacket, "00");
  bt.update(123, kImagePacket, "01");
  bt.update(123, kImagePacket, "02");
  bt.update(123, kImagePacket, "023");
  bt.update(123, kImagePacket, "0231");
  bt.update(123, kImagePacket, "0232");

  bt.update(24, kVectorPacket, "0");
  bt.update(24, kVectorPacket, "00");
  bt.update(24, kVectorPacket, "01");
  bt.update(24, kVectorPacket, "02");
  bt.update(24, kVectorPacket, "023");
  bt.update(24, kVectorPacket, "0231");
  bt.update(24, kVectorPacket, "0232");

  // Terrain stops at a lower level than vector and imagery.
  bt.update(1, kTerrainPacket, "0");
  bt.update(1, kTerrainPacket, "00");
  bt.update(1, kTerrainPacket, "01");
  bt.update(1, kTerrainPacket, "02");
}

TEST_F(BoundsTrackerTest, SimpleImageryBounds) {
  BoundsTracker bt;
  populateSimpleBoundsTracker(bt, kImagePacket);

  const channel_info& channel = bt.channel(123);
  EXPECT_EQ(channel.channel_id, 123);
  EXPECT_EQ(channel.type, kImagePacket);

  EXPECT_EQ(channel.top, 2);
  EXPECT_EQ(channel.bottom, 3);
  EXPECT_EQ(channel.left, 0);
  EXPECT_EQ(channel.right, 1);

  EXPECT_EQ(channel.min_level, 1);
  EXPECT_EQ(channel.max_level, 2);
}

TEST_F(BoundsTrackerTest, SimpleVectorBounds) {
  BoundsTracker bt;
  populateSimpleBoundsTracker(bt, kVectorPacket);

  const channel_info& channel = bt.channel(123);
  EXPECT_EQ(channel.channel_id, 123);
  EXPECT_EQ(channel.type, kVectorPacket);

  EXPECT_EQ(channel.top, 2);
  EXPECT_EQ(channel.bottom, 3);
  EXPECT_EQ(channel.left, 0);
  EXPECT_EQ(channel.right, 1);

  EXPECT_EQ(channel.min_level, 1);
  EXPECT_EQ(channel.max_level, 2);
}

TEST_F(BoundsTrackerTest, SimpleTerrainBounds) {
  BoundsTracker bt;
  populateSimpleBoundsTracker(bt, kTerrainPacket);

  const channel_info& channel = bt.channel(123);
  EXPECT_EQ(channel.channel_id, 123);
  EXPECT_EQ(channel.type, kTerrainPacket);

  EXPECT_EQ(channel.top, 2);
  EXPECT_EQ(channel.bottom, 3);
  EXPECT_EQ(channel.left, 0);
  EXPECT_EQ(channel.right, 1);

  EXPECT_EQ(channel.min_level, 1);
  EXPECT_EQ(channel.max_level, 2);
}

TEST_F(BoundsTrackerTest, MultipleBounds) {
  BoundsTracker bt;
  populateBoundsTracker(bt);

  const channel_info& imagery = bt.channel(123);
  EXPECT_EQ(imagery.channel_id, 123);
  EXPECT_EQ(imagery.type, kImagePacket);

  EXPECT_EQ(imagery.top, 8);
  EXPECT_EQ(imagery.bottom, 9);
  EXPECT_EQ(imagery.left, 5);
  EXPECT_EQ(imagery.right, 5);

  EXPECT_EQ(imagery.min_level, 1);
  EXPECT_EQ(imagery.max_level, 4);

  const channel_info& vector = bt.channel(24);
  EXPECT_EQ(vector.channel_id, 24);
  EXPECT_EQ(vector.type, kVectorPacket);

  EXPECT_EQ(vector.top, 8);
  EXPECT_EQ(vector.bottom, 9);
  EXPECT_EQ(vector.left, 5);
  EXPECT_EQ(vector.right, 5);

  EXPECT_EQ(vector.min_level, 1);
  EXPECT_EQ(vector.max_level, 4);

  const channel_info& terrain = bt.channel(1);
  EXPECT_EQ(terrain.channel_id, 1);
  EXPECT_EQ(terrain.type, kTerrainPacket);

  EXPECT_EQ(terrain.top, 2);
  EXPECT_EQ(terrain.bottom, 3);
  EXPECT_EQ(terrain.left, 0);
  EXPECT_EQ(terrain.right, 1);

  EXPECT_EQ(terrain.min_level, 1);
  EXPECT_EQ(terrain.max_level, 2);
}

TEST_F(BoundsTrackerTest, TestWriteJson) {
  // This is lame, but easier than adding a new JSON parser dependency.
  const std::string expected_json = "[\n"
    "  {\n"
    "    \"channel_id\": 1,\n"
    "    \"type\": \"Terrain\",\n"
    "    \"top\": 2,\n"
    "    \"bottom\": 3,\n"
    "    \"left\": 0,\n"
    "    \"right\": 1,\n"
    "    \"min_level\": 1,\n"
    "    \"max_level\": 2\n"
    "  },\n"
    "  {\n"
    "    \"channel_id\": 24,\n"
    "    \"type\": \"Vector\",\n"
    "    \"top\": 8,\n"
    "    \"bottom\": 9,\n"
    "    \"left\": 5,\n"
    "    \"right\": 5,\n"
    "    \"min_level\": 1,\n"
    "    \"max_level\": 4\n"
    "  },\n"
    "  {\n"
    "    \"channel_id\": 123,\n"
    "    \"type\": \"Image\",\n"
    "    \"top\": 8,\n"
    "    \"bottom\": 9,\n"
    "    \"left\": 5,\n"
    "    \"right\": 5,\n"
    "    \"min_level\": 1,\n"
    "    \"max_level\": 4\n"
    "  }\n"
    "]\n";

  BoundsTracker bt;
  populateBoundsTracker(bt);

  std::string filename = test_directory + "test.json";

  bt.write_json_file(filename);

  std::ifstream ins(filename.c_str());
  std::string str{ std::istreambuf_iterator<char>(ins),
                   std::istreambuf_iterator<char>()};
  bt.write_json_file("/home/jlarocco/testing.json");
  EXPECT_EQ(str, expected_json);
}

}  // namespace fusion_portableglobe

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
