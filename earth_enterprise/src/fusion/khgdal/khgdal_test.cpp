// Copyright 2017 Google Inc.
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

//
// Unit tests for khgdal.

#include <gtest/gtest.h>

#include "khgdal.h"
#include <khGeomUtils.h>

#include <dlfcn.h>

// We round upto an integral meter and then compare
bool NearlyEqual(double gold, double other) {
  return ( static_cast<int>(gold + 0.5) == static_cast<int>(other + 0.5) );
}

TEST(khGdalTest, ConvertToGdalMeterForMercator) {
  // Expected value for gdal_lat_meter is
  // ln(tan(lat_deg*pi/180) + sec(lat_deg*pi/180)) * 40075016.6855784 / (2*pi)
  // In case more inputs are needed the above can be put to google calculator
  // e.g ln(tan(30*pi/180)+sec(30*pi/180))*40075016.6855784/(2*pi)=3503549.84
  // e.g ln(tan(50*pi/180)+sec(50*pi/180))*40075016.6855784/(2*pi)=6446275.84
  const double half_circumference =
      khGeomUtilsMercator::khEarthCircumference / 2.0;
  struct {
    const double deg_latitude;
    const double gdal_mercator_meter_latitude;
  } latitudes[] = {
    {0, 0},
    {30, 3503549.84},
    {50, 6446275.84},
      // If we draw world extents from (85.05113, 180) to (-180, -85.05113)
      // using Mercator projection, the map shows up as a square
      // Test for various longitudes at 85.051128779806575 deg latitude
    {khGeomUtilsMercator::khMaxLatitude, half_circumference},
  };

  struct {
    const double deg_longitude;
    const double gdal_mercator_meter_longitude;
  } longitudes[] = {
    {0, 0},
    {10, khGeomUtilsMercator::khEarthCircumference * 10 / 360},
    {63, khGeomUtilsMercator::khEarthCircumference * 63 / 360},
    {90, khGeomUtilsMercator::khEarthCircumference * 90 / 360},
    {139, khGeomUtilsMercator::khEarthCircumference * 139 / 360},
    {180, khGeomUtilsMercator::khEarthCircumference * 180 / 360},
  };

  struct {
    double lat_multiplier;
    double lon_multiplier;
  } sign_multipliers[] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};

  const int num_latitudes = sizeof(latitudes) / sizeof(latitudes[0]);
  const int num_longitudes = sizeof(longitudes) / sizeof(longitudes[0]);
  const int num_sign_multipliers =
      sizeof(sign_multipliers) / sizeof(sign_multipliers[0]);

  for (int i = 0; i < num_latitudes; ++i) {
    for (int j = 0; j < num_longitudes; ++j) {
      for (int k = 0; k < num_sign_multipliers; ++k) {
        const double lat = latitudes[i].deg_latitude *
                      sign_multipliers[k].lat_multiplier;
        const double lon = longitudes[j].deg_longitude *
                      sign_multipliers[k].lon_multiplier;
        double lat_converted = lat;
        double lon_converted = lon;
        ConvertToGdalMeterForMercator(&lat_converted, &lon_converted);
        const double lat_expected =
            latitudes[i].gdal_mercator_meter_latitude *
            sign_multipliers[k].lat_multiplier;
        const double lon_expected =
            longitudes[j].gdal_mercator_meter_longitude *
            sign_multipliers[k].lon_multiplier;
        EXPECT_TRUE(NearlyEqual(lat_expected, lat_converted))
            << "Latitiude mismatch: expected " << lat_expected << ", got " << lat_converted;
        EXPECT_TRUE(NearlyEqual(lon_expected, lon_converted))
            << "Longitude mismatch: expected " << lon_expected << ", got " << lon_converted;
      }
    }
  }
}

class DLWrapper {
 private:
  void * handle;
 public:
  DLWrapper(const char * filename, int flags) {
    handle = dlopen(filename, flags);
  }
  ~DLWrapper() {
    if(IsLoaded()) {
      dlclose(handle);
    }
  }
  bool IsLoaded() {
    return handle;
  }
};

TEST(khGdalTest, CheckMrSID) {
  // Only run this test if the MrSID library is available
  DLWrapper mrsid("libltidsdk.so", RTLD_NOW);
  if (mrsid.IsLoaded()) {
    khGDALInit();
    auto manager = GetGDALDriverManager();

    // Verify that the MrSID driver is available
    auto driver = manager->GetDriverByName("MrSID");
    ASSERT_NE(driver, nullptr) << "Could not load MrSID driver";
  }
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
