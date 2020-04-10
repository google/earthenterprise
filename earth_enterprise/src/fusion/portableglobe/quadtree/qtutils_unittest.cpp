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


// Unit Tests for qtutils.cpp.

#include "fusion/portableglobe/quadtree/qtutils.h"

#include <math.h>
#include <algorithm>
#include <string>
#include <vector>
#include <gtest/gtest.h>

namespace fusion_portableglobe {

// Test helper functions in quadtree utilities.
class QtUtilsTest : public testing::Test {
 protected:
  virtual void SetUp() {
  }
};

// Tests the conversion of a plate carree address to
// all of the the Mercator addresses for nodes that
// touch some part of the plate carree node.
TEST_F(QtUtilsTest, ConvertFlatToMercatorQtAddressesTest) {
  std::vector<std::string> mercator_qtaddresses;
  std::string qtaddress;

  // NW corner of plate carree
  qtaddress = "030333333";
  ConvertFlatToMercatorQtAddresses(qtaddress, &mercator_qtaddresses);
  // ... should correspond to NW corner of Mercator.
  EXPECT_EQ("033333333", mercator_qtaddresses[0]);
  mercator_qtaddresses.clear();

  // SW corner of plate carree
  qtaddress = "00300000000";
  ConvertFlatToMercatorQtAddresses(qtaddress, &mercator_qtaddresses);
  // ... should correspond to SW corner of Mercator.
  EXPECT_EQ("00000000000", mercator_qtaddresses[0]);
  mercator_qtaddresses.clear();

  // NE corner of plate carree
  qtaddress = "0212222222222";
  ConvertFlatToMercatorQtAddresses(qtaddress, &mercator_qtaddresses);
  // ... should correspond to NE corner of Mercator.
  EXPECT_EQ("0222222222222", mercator_qtaddresses[0]);
  mercator_qtaddresses.clear();

  // SE corner of plate carree
  qtaddress = "0121111111";
  ConvertFlatToMercatorQtAddresses(qtaddress, &mercator_qtaddresses);
  // ... should correspond to SE corner of Mercator.
  EXPECT_EQ("0111111111", mercator_qtaddresses[0]);
  mercator_qtaddresses.clear();

  // Center of plate carree (try all 4)
  qtaddress = "00222222222222";
  ConvertFlatToMercatorQtAddresses(qtaddress, &mercator_qtaddresses);
  // ... should correspond to center of Mercator.
  bool found = false;
  for (size_t i = 0; i < mercator_qtaddresses.size(); ++i) {
    found |= (mercator_qtaddresses[i] == qtaddress);
  }
  EXPECT_TRUE(found);
  mercator_qtaddresses.clear();

  qtaddress = "013333333333";
  ConvertFlatToMercatorQtAddresses(qtaddress, &mercator_qtaddresses);
  // ... should correspond to center of Mercator.
  found = false;
  for (size_t i = 0; i < mercator_qtaddresses.size(); ++i) {
    found |= (mercator_qtaddresses[i] == qtaddress);
  }
  EXPECT_TRUE(found);
  mercator_qtaddresses.clear();


  qtaddress = "0200000000000000";
  ConvertFlatToMercatorQtAddresses(qtaddress, &mercator_qtaddresses);
  // ... should correspond to center of Mercator.
  found = false;
  for (size_t i = 0; i < mercator_qtaddresses.size(); ++i) {
    found |= (mercator_qtaddresses[i] == qtaddress);
  }
  EXPECT_TRUE(found);
  mercator_qtaddresses.clear();


  qtaddress = "03111111111";
  ConvertFlatToMercatorQtAddresses(qtaddress, &mercator_qtaddresses);
  // ... should correspond to center of Mercator.
  found = false;
  for (size_t i = 0; i < mercator_qtaddresses.size(); ++i) {
    found |= (mercator_qtaddresses[i] == qtaddress);
  }
  EXPECT_TRUE(found);
  mercator_qtaddresses.clear();

  // Sanity check
  // Make sure x and z don't change and that
  // y values are contiguous.
  qtaddress = "0312301233";
  std::uint32_t flat_x;
  std::uint32_t flat_y;
  std::uint32_t flat_z;
  ConvertFromQtNode(qtaddress, &flat_x, &flat_y, &flat_z);

  ConvertFlatToMercatorQtAddresses(qtaddress, &mercator_qtaddresses);

  std::uint32_t merc_x;
  std::uint32_t merc_y;
  std::uint32_t merc_z;
  std::uint32_t last_merc_y = 0;
  for (size_t i = 0; i < mercator_qtaddresses.size(); ++i) {
    ConvertFromQtNode(mercator_qtaddresses[i], &merc_x, &merc_y, &merc_z);
    EXPECT_EQ(flat_x, merc_x);
    EXPECT_EQ(flat_z, merc_z);
    if (i > 0) {
      EXPECT_EQ(last_merc_y + 1, merc_y);
    }
    last_merc_y = merc_y;
  }
  mercator_qtaddresses.clear();

  // Check that all values of y are covered
  // in Mercator space for the y values in
  // plate carree space.

  flat_z = 10;
  // Y dimension in map space at LOD 10.
  std::uint32_t ydim = 1 << flat_z;
  std::uint32_t qtr_ydim = ydim >> 2;
  flat_x = 93;
  std::uint32_t highest_merc_y = 0;
  // Loop over all plate carree y values in one column.
  for (flat_y = qtr_ydim; flat_y < ydim - qtr_ydim; ++flat_y) {
    qtaddress = ConvertToQtNode(flat_x, flat_y, flat_z);
    mercator_qtaddresses.clear();
    ConvertFlatToMercatorQtAddresses(qtaddress, &mercator_qtaddresses);
    for (size_t i = 0; i < mercator_qtaddresses.size(); ++i) {
      ConvertFromQtNode(mercator_qtaddresses[i], &merc_x, &merc_y, &merc_z);
      EXPECT_EQ(flat_x, merc_x);
      EXPECT_EQ(flat_z, merc_z);
      if (merc_y > highest_merc_y) {
        EXPECT_EQ(highest_merc_y + 1, merc_y);
        highest_merc_y = merc_y;
      }
    }
  }
  EXPECT_EQ(ydim - 1, highest_merc_y);
  mercator_qtaddresses.clear();
}

// Tests that values are at least reasonable (monotonic) and that
// inverse function works.
TEST_F(QtUtilsTest, LatToYPosTest) {
  std::uint32_t ypos;
  // Y dimension in map space at LOD 12.
  std::uint32_t size_at_12 = 1 << 12;
  // Half of y dimension in map space at LOD 12.
  std::uint32_t halfsize_at_12 = size_at_12 >> 1;
  // Quarter of y dimension in map space at LOD 12.
  std::uint32_t qtrsize_at_12 = halfsize_at_12 >> 1;

  // At equator, both Mercator and flat should be the same.
  // Just above the equator.
  double lat = 1e-12;
  ypos = LatToYPos(lat, 12, true);
  EXPECT_EQ(halfsize_at_12 - 1, ypos);
  ypos = LatToYPos(lat, 12, false);
  EXPECT_EQ(halfsize_at_12 - 1, ypos);

  // Just below the equator.
  lat = -lat;
  ypos = LatToYPos(lat, 12, true);
  EXPECT_EQ(halfsize_at_12, ypos);
  ypos = LatToYPos(lat, 12, false);
  EXPECT_EQ(halfsize_at_12, ypos);

  // Top of the world.
  lat = 90.0;
  ypos = LatToYPos(lat, 12, true);
  EXPECT_EQ(0u, ypos);
  ypos = LatToYPos(lat, 12, false);
  EXPECT_EQ(qtrsize_at_12, ypos);

  // Bottom of the world.
  lat = -90.0;
  ypos = LatToYPos(lat, 12, true);
  EXPECT_EQ(size_at_12 - 1, ypos);
  ypos = LatToYPos(lat, 12, false);
  EXPECT_EQ(halfsize_at_12 + qtrsize_at_12 - 1, ypos);

  // Try somewhere in the middle with known values.
  lat = 45.0;
  ypos = LatToYPos(lat, 12, true);
  EXPECT_EQ(1473u, ypos);
  ypos = LatToYPos(lat, 12, false);
  EXPECT_EQ(1536u, ypos);

  lat = -45.0;
  ypos = LatToYPos(lat, 12, true);
  EXPECT_EQ(2622u, ypos);
  ypos = LatToYPos(lat, 12, false);
  EXPECT_EQ(2560u, ypos);
}

// Tests that values are at least reasonable (monotonic) and that
// inverse function works.
TEST_F(QtUtilsTest, TestLatToMercatorToLat) {
  double last_lat = -MAX_MERCATOR_LATITUDE - 0.1;
  double last_y = -PI - 0.1;
  for (double lat = -MAX_MERCATOR_LATITUDE;
       lat < MAX_MERCATOR_LATITUDE; lat += 0.4) {
    double y = MercatorLatToY(lat);
    double new_lat = MercatorYToLat(y);
    EXPECT_NEAR(lat, new_lat, 1e-12);
    EXPECT_LT(last_y, y);
    EXPECT_LT(last_lat, lat);
    last_lat = lat;
    last_y = y;
  }
}

// Make sure we are getting a linear distribution across the
// normalized y values that correspond to ypos.
TEST_F(QtUtilsTest, TestYToYPos) {
  for (std::uint32_t depth = 5; depth <= 22; ++depth) {
    std::uint32_t size = 1 << depth;

    // Check midline.
    std::uint32_t ypos = YToYPos(1.0e-15, depth);
    EXPECT_EQ(size >> 1, ypos);
    ypos = YToYPos(-1.0e-15, depth);
    EXPECT_EQ((size >> 1) - 1, ypos);

    double step = 2 * PI / size;
    double y = -PI - step / 2.0;
    ypos = YToYPos(y, depth);

    // Check underflow.
    EXPECT_LE(y, -PI);
    EXPECT_EQ(0u, ypos);
    // Check legal positions.
    for (std::uint32_t i = 0; i < size; ++i) {
      y += step;
      ypos = YToYPos(y, depth);
      EXPECT_EQ(i, ypos);
    }
    // Check overflow.
    y += step;
    EXPECT_GE(y, PI);
    ypos = YToYPos(y, depth);
    EXPECT_EQ(size - 1, ypos);
  }
}

// Check conversion between x, y and z and qt addresses.
TEST_F(QtUtilsTest, TestConvertToAndFromQtNode) {
  std::uint32_t x;
  std::uint32_t y;
  std::uint32_t z;
  std::string result;

  // Test a couple a simple of known cases.
  std::string qtnode = "03210";
  ConvertFromQtNode(qtnode, &x, &y, &z);
  EXPECT_EQ(4u, z);
  EXPECT_EQ(6u, x);
  EXPECT_EQ(3u, y);

  qtnode = "03333";
  result = ConvertToQtNode(0, 0, 4);
  EXPECT_EQ(qtnode, result);

  qtnode = "01111";
  result = ConvertToQtNode(15, 15, 4);
  EXPECT_EQ(qtnode, result);

  // Test inversion on a few random cases.
  qtnode = "0323131";
  ConvertFromQtNode(qtnode, &x, &y, &z);
  result = ConvertToQtNode(x, y, z);
  EXPECT_EQ(qtnode, result);

  qtnode = "0002310302012131021312";
  ConvertFromQtNode(qtnode, &x, &y, &z);
  result = ConvertToQtNode(x, y, z);
  EXPECT_EQ(qtnode, result);

  qtnode = "0231231312312012101";
  ConvertFromQtNode(qtnode, &x, &y, &z);
  result = ConvertToQtNode(x, y, z);
  EXPECT_EQ(qtnode, result);

  // Test degenerate cases.
  qtnode = "";
  ConvertFromQtNode(qtnode, &x, &y, &z);
  EXPECT_EQ(MAX_LEVEL, z);

  qtnode = "0123123141123";
  ConvertFromQtNode(qtnode, &x, &y, &z);
  EXPECT_EQ(MAX_LEVEL, z);

  qtnode = "0bob";
  ConvertFromQtNode(qtnode, &x, &y, &z);
  EXPECT_EQ(MAX_LEVEL, z);

  result = ConvertToQtNode(10u, 16u, 4u);
  EXPECT_EQ("", result);

  result = ConvertToQtNode(128u, 3u, 7u);
  EXPECT_EQ("", result);
}

}  // namespace fusion_portableglobe

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
