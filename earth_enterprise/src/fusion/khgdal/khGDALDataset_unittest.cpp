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

// The ostream functions make any failure messages more informative and easier
// to read.
template <typename T>
std::ostream& operator<<(std::ostream& os, const khExtents<T>& e) {
  os << e.north() << ", " << e.south() << ", " << e.east() << ", " << e.west();
  return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const khSize<T>& s) {
  os << s.width << ", " << s.height;
  return os;
}

std::ostream& operator<<(std::ostream& os, const khGeoExtents& ge) {
  khSize<std::uint32_t> raster_size(
      (ge.extents().width() / ge.absPixelWidth() + 0.5),
      (ge.extents().height() / ge.absPixelHeight() + 0.5));
  os << ge.extents() << std::endl << raster_size;
  return os;
}

bool geoExtentsEqual(const khExtents<double> & extents, const khSize<std::uint32_t> & size, const khGeoExtents & ge) {
  bool status = true;
  khGeoExtents expected(extents, size);
  if (expected.extents() != ge.extents()) {
    status = false;
  }
  const double epsilon = 0.000001;
  for (size_t i = 0; i < 6; ++i) {
    if (fabs(expected.geoTransform()[i] - ge.geoTransform()[i]) > epsilon) {
      status = false;
    }
  }
  return status;
}

TEST(GDALDatasetTest, FlatLowRes) {
  khGDALDataset ds("fusion/testdata/lowres.tiff");
  EXPECT_EQ(6, ds.normalizedTopLevel());
  EXPECT_FALSE(ds.IsMercator());
}

TEST(GDALDatasetTest, FlatHighRes) {
  khGDALDataset ds("fusion/testdata/highres.tiff");
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -0.187, 166021.63009607795, 166021.44309607794),
      khSize<std::uint32_t>(10, 10),
      ds.geoExtents());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.rasterSize());
  EXPECT_EQ(
      khExtents<std::int64_t>(NSEWOrder, 1073741824, 1073741814, 1073741834, 1073741824),
      ds.alignedPixelExtents());
  EXPECT_EQ(
      khExtents<std::int64_t>(NSEWOrder, 1073741824, 1073741814, 1073741834, 1073741824),
      ds.croppedPixelExtents());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.alignedRasterSize());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.croppedRasterSize());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -1.6763806343078613e-06, 1.6763806343078613e-06, 0),
      khSize<std::uint32_t>(10, 10),
      ds.alignedGeoExtents());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -1.6763806343078613e-06, 1.6763806343078613e-06, 0),
      khSize<std::uint32_t>(10, 10),
      ds.croppedGeoExtents());
  EXPECT_EQ(31, ds.normalizedTopLevel());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.normalizedRasterSize());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -1.6763806343078613e-06, 1.6763806343078613e-06, 0),
      khSize<std::uint32_t>(10, 10),
      ds.normalizedGeoExtents());
  EXPECT_TRUE(ds.needReproject());
  EXPECT_FALSE(ds.needSnapUp());
  EXPECT_FALSE(ds.normIsCropped());
}

// This verifies that the resolution tops out at the maximum zoom level
TEST(GDALDatasetTest, FlatTooHighRes) {
  khGDALDataset ds("fusion/testdata/toohighres.tiff");
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -0.185, 166021.62809607794, 166021.44309607794),
      khSize<std::uint32_t>(10, 10),
      ds.geoExtents());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.rasterSize());
  EXPECT_EQ(
      khExtents<std::int64_t>(NSEWOrder, 1073741824, 1073741814, 1073741834, 1073741824),
      ds.alignedPixelExtents());
  EXPECT_EQ(
      khExtents<std::int64_t>(NSEWOrder, 1073741824, 1073741814, 1073741834, 1073741824),
      ds.croppedPixelExtents());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.alignedRasterSize());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.croppedRasterSize());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -1.6763806343078613e-06, 1.6763806343078613e-06, 0),
      khSize<std::uint32_t>(10, 10),
      ds.alignedGeoExtents());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -1.6763806343078613e-06, 1.6763806343078613e-06, 0),
      khSize<std::uint32_t>(10, 10),
      ds.croppedGeoExtents());
  EXPECT_EQ(31, ds.normalizedTopLevel());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.normalizedRasterSize());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -1.6763806343078613e-06, 1.6763806343078613e-06, 0),
      khSize<std::uint32_t>(10, 10),
      ds.normalizedGeoExtents());
  EXPECT_TRUE(ds.needReproject());
  EXPECT_FALSE(ds.needSnapUp());
  EXPECT_FALSE(ds.normIsCropped());
}

TEST(GDALDatasetTest, MercLowRes) {
  khGDALDataset ds("fusion/testdata/lowres.tiff",
      std::string(),
      khExtents<double>(),
      khTilespace::MERCATOR_PROJECTION);
  EXPECT_EQ(4, ds.normalizedTopLevel());
  EXPECT_TRUE(ds.IsMercator());
}

TEST(GDALDatasetTest, MercHighRes) {
  khGDALDataset ds("fusion/testdata/highres.tiff",
      std::string(),
      khExtents<double>(),
      khTilespace::MERCATOR_PROJECTION);
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -0.187, 166021.63009607795, 166021.44309607794),
      khSize<std::uint32_t>(10, 10),
      ds.geoExtents());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.rasterSize());
  EXPECT_EQ(
      khExtents<std::int64_t>(NSEWOrder, 1073741824, 1073741814, 1073741834, 1073741824),
      ds.alignedPixelExtents());
  EXPECT_EQ(
      khExtents<std::int64_t>(NSEWOrder, 1073741824, 1073741814, 1073741834, 1073741824),
      ds.croppedPixelExtents());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.alignedRasterSize());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.croppedRasterSize());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -0.18661383911967278, 0.18661383911967278, 0),
      khSize<std::uint32_t>(10, 10),
      ds.alignedGeoExtents());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -0.18661383911967278, 0.18661383911967278, 0),
      khSize<std::uint32_t>(10, 10),
      ds.croppedGeoExtents());
  EXPECT_EQ(31, ds.normalizedTopLevel());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.normalizedRasterSize());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -0.18661383911967278, 0.18661383911967278, 0),
      khSize<std::uint32_t>(10, 10),
      ds.normalizedGeoExtents());
  EXPECT_TRUE(ds.needReproject());
  EXPECT_FALSE(ds.needSnapUp());
  EXPECT_FALSE(ds.normIsCropped());
}

// This verifies that the resolution tops out at the maximum zoom level
TEST(GDALDatasetTest, MercTooHighRes) {
  khGDALDataset ds("fusion/testdata/toohighres.tiff",
      std::string(),
      khExtents<double>(),
      khTilespace::MERCATOR_PROJECTION);
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -0.185, 166021.62809607794, 166021.44309607794),
      khSize<std::uint32_t>(10, 10),
      ds.geoExtents());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.rasterSize());
  EXPECT_EQ(
      khExtents<std::int64_t>(NSEWOrder, 1073741824, 1073741814, 1073741834, 1073741824),
      ds.alignedPixelExtents());
  EXPECT_EQ(
      khExtents<std::int64_t>(NSEWOrder, 1073741824, 1073741814, 1073741834, 1073741824),
      ds.croppedPixelExtents());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.alignedRasterSize());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.croppedRasterSize());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -0.18661383911967278, 0.18661383911967278, 0),
      khSize<std::uint32_t>(10, 10),
      ds.alignedGeoExtents());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -0.18661383911967278, 0.18661383911967278, 0),
      khSize<std::uint32_t>(10, 10),
      ds.croppedGeoExtents());
  EXPECT_EQ(31, ds.normalizedTopLevel());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.normalizedRasterSize());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -0.18661383911967278, 0.18661383911967278, 0),
      khSize<std::uint32_t>(10, 10),
      ds.normalizedGeoExtents());
  EXPECT_TRUE(ds.needReproject());
  EXPECT_FALSE(ds.needSnapUp());
  EXPECT_FALSE(ds.normIsCropped());
}

// This tests a case where the level is different between flat and mercator
TEST(GDALDatasetTest, FlatVsMerc) {
  khGDALDataset flat("fusion/testdata/flatvsmerc.tiff");
  khGDALDataset merc("fusion/testdata/flatvsmerc.tiff",
      std::string(),
      khExtents<double>(),
      khTilespace::MERCATOR_PROJECTION);
  EXPECT_EQ(6, flat.normalizedTopLevel());
  EXPECT_EQ(3, merc.normalizedTopLevel());
}

// Test a case using a mercator image that doesn't need reprojection as an
// input. This also tests images that need snap-up.
TEST(GDALDatasetTest, NativeMerc) {
  khGDALDataset ds("fusion/testdata/medresmerc.tiff",
      std::string(),
      khExtents<double>(),
      khTilespace::MERCATOR_PROJECTION);
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -40000, 206021.44309607794, 166021.44309607794),
      khSize<std::uint32_t>(10, 10),
      ds.geoExtents());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.rasterSize());
  EXPECT_EQ(
      khExtents<std::int64_t>(NSEWOrder, 8192, 8176, 8276, 8260),
      ds.alignedPixelExtents());
  EXPECT_EQ(
      khExtents<std::int64_t>(NSEWOrder, 8192, 8176, 8276, 8260),
      ds.croppedPixelExtents());
  EXPECT_EQ(khSize<std::uint32_t>(16, 16), ds.alignedRasterSize());
  EXPECT_EQ(khSize<std::uint32_t>(16, 16), ds.croppedRasterSize());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -39135.758482009172, 205462.73203055188, 166326.97354854271),
      khSize<std::uint32_t>(16, 16),
      ds.alignedGeoExtents());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -39135.758482009172, 205462.73203055188, 166326.97354854271),
      khSize<std::uint32_t>(16, 16),
      ds.croppedGeoExtents());
  EXPECT_EQ(14, ds.normalizedTopLevel());
  EXPECT_EQ(khSize<std::uint32_t>(16, 16), ds.normalizedRasterSize());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 0, -39135.758482009172, 205462.73203055188, 166326.97354854271),
      khSize<std::uint32_t>(16, 16),
      ds.normalizedGeoExtents());
  EXPECT_TRUE(ds.needReproject());
  EXPECT_FALSE(ds.needSnapUp());
  EXPECT_FALSE(ds.normIsCropped());
}

// This tests a case that doesn't require reprojection or snapup. The image is
// a small section of the i3SF15-meter.tif tutorial image.
TEST(GDALDatasetTest, FlatMedRes) {
  khGDALDataset ds("fusion/testdata/flatnoproj.tiff");
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 37.873761032714846, 37.87290272583008, -122.52983138159179, -122.53068968847656),
      khSize<std::uint32_t>(10, 10),
      ds.geoExtents());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.rasterSize());
  EXPECT_EQ(
      khExtents<std::int64_t>(NSEWOrder, 2538413, 2538403, 669576, 669566),
      ds.alignedPixelExtents());
  EXPECT_EQ(
      khExtents<std::int64_t>(NSEWOrder, 2538413, 2538403, 669576, 669566),
      ds.croppedPixelExtents());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.alignedRasterSize());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.croppedRasterSize());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 37.873735427856445, 37.87287712097168, -122.52983093261719, -122.53068923950195),
      khSize<std::uint32_t>(10, 10),
      ds.alignedGeoExtents());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 37.873735427856445, 37.87287712097168, -122.52983093261719, -122.53068923950195),
      khSize<std::uint32_t>(10, 10),
      ds.croppedGeoExtents());
  EXPECT_EQ(22, ds.normalizedTopLevel());
  EXPECT_EQ(khSize<std::uint32_t>(10, 10), ds.normalizedRasterSize());
  EXPECT_PRED3(geoExtentsEqual,
      khExtents<double>(NSEWOrder, 37.873735427856445, 37.87287712097168, -122.52983093261719, -122.53068923950195),
      khSize<std::uint32_t>(10, 10),
      ds.normalizedGeoExtents());
  EXPECT_FALSE(ds.needReproject());
  EXPECT_FALSE(ds.needSnapUp());
  EXPECT_FALSE(ds.normIsCropped());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
