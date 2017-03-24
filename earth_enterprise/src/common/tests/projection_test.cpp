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
// Unit tests for khStringUtils.

#include <string>
#include "common/projection.h"
#include "common/UnitTest.h"

class ProjectionTest: public UnitTest<ProjectionTest> {
 public:
  bool FromFlatDegLatitudeToMercatorMeterLatitude(void) {
    double data[][2] = {
      {0,0},
      {38.46461111111111111111, 4645269.29793149698525667191},
      {85.051128779806575, 20037508.34278913587331771851},
    };
    const size_t data_pairs = sizeof(data) / (sizeof(double) * 2);
    for (size_t i = 0; i < data_pairs; ++i) {
      EXPECT_EQ(MercatorProjection::FromFlatDegLatitudeToMercatorMeterLatitude(
          data[i][0]), data[i][1]);
      EXPECT_EQ(MercatorProjection::FromFlatDegLatitudeToMercatorMeterLatitude(
          -data[i][0]), -data[i][1]);
    }
    return true;
  }

  bool FromMercatorMeterLatitudeToFlatDegLatitude(void) {
    double data[][2] = {
      {0,0},
      {38.46461111111111819127, 4645269.29793149698525667191},
      {85.05112877980656094223, 20037508.34278913587331771851},
    };
    const size_t data_pairs = sizeof(data) / (sizeof(double) * 2);
    for (size_t i = 0; i < data_pairs; ++i) {
      EXPECT_EQ(MercatorProjection::FromMercatorMeterLatitudeToFlatDegLatitude(
          data[i][1]), data[i][0]);
      EXPECT_EQ(MercatorProjection::FromMercatorMeterLatitudeToFlatDegLatitude(
          -data[i][1]), -data[i][0]);
    }
    return true;
  }


  ProjectionTest(void) : BaseClass("ProjectionTest") {
    REGISTER(FromFlatDegLatitudeToMercatorMeterLatitude);
    REGISTER(FromMercatorMeterLatitudeToFlatDegLatitude);
  }
};

int main(int argc, char *argv[]) {
  ProjectionTest tests;

  return tests.Run();
}
