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

#include "khgdal/khGDALDataset.h"

TEST(GDALDatasetTest, FlatLowRes) {
  khGDALDataset ds("fusion/testdata/lowres.tiff");
  ASSERT_EQ(6, ds.normalizedTopLevel());
}

TEST(GDALDatasetTest, FlatHighRes) {
  khGDALDataset ds("fusion/testdata/highres.tiff");
  ASSERT_EQ(31, ds.normalizedTopLevel());
}

// This verifies that the resolution tops out at the maximum zoom level
TEST(GDALDatasetTest, FlatTooHighRes) {
  khGDALDataset ds("fusion/testdata/toohighres.tiff");
  ASSERT_EQ(31, ds.normalizedTopLevel());
}

TEST(GDALDatasetTest, MercLowRes) {
  khGDALDataset ds("fusion/testdata/lowres.tiff",
      std::string(),
      khExtents<double>(),
      khTilespace::MERCATOR_PROJECTION);
  ASSERT_EQ(4, ds.normalizedTopLevel());
}

TEST(GDALDatasetTest, MercHighRes) {
  khGDALDataset ds("fusion/testdata/highres.tiff",
      std::string(),
      khExtents<double>(),
      khTilespace::MERCATOR_PROJECTION);
  ASSERT_EQ(31, ds.normalizedTopLevel());
}

// This verifies that the resolution tops out at the maximum zoom level
TEST(GDALDatasetTest, MercTooHighRes) {
  khGDALDataset ds("fusion/testdata/toohighres.tiff",
      std::string(),
      khExtents<double>(),
      khTilespace::MERCATOR_PROJECTION);
  ASSERT_EQ(31, ds.normalizedTopLevel());
}

// This tests a case where the level is different between flat and mercator
TEST(GDALDatasetTest, FlatVsMerc) {
  khGDALDataset flat("fusion/testdata/flatvsmerc.tiff");
  khGDALDataset merc("fusion/testdata/flatvsmerc.tiff",
      std::string(),
      khExtents<double>(),
      khTilespace::MERCATOR_PROJECTION);
  ASSERT_EQ(6, flat.normalizedTopLevel());
  ASSERT_EQ(3, merc.normalizedTopLevel());
}

// Test a case using a mercator image that doesn't need reprojection as an
// input. This also tests images that need snap-up.
TEST(GDALDatasetTest, NativeMerc) {
  khGDALDataset ds("fusion/testdata/medresmerc.tiff",
      std::string(),
      khExtents<double>(),
      khTilespace::MERCATOR_PROJECTION);
  ASSERT_EQ(14, ds.normalizedTopLevel());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
