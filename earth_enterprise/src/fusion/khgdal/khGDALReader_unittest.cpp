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

#include <gtest/gtest.h>
#include "khGDALReaderImpl.h"

// Point the dataset at a valid image. We won't use this, but it prevents crashes
// due to an invalid dataset.
khGDALDataset dataset("fusion/testdata/i3SF15-meter.tif");

class MockReader : public khGDALReader {
  public:
    double no_data_val;
    int nodata_exists_val;
    uint num_gets;
    MockReader(double no_data, int nodata_exists) :
        khGDALReader(dataset, 3), // Values don't matter; just making the constructor happy
        no_data_val(no_data), nodata_exists_val(nodata_exists), num_gets(0) { }

    // Give callers access to the functions we want to test
    template<class SrcPixelType>
    SrcPixelType CallGetNoDataOrZero() {
      return GetNoDataOrZero<SrcPixelType>();
    }
  private:
    virtual void GetNoDataFromSrc(double & no_data, int & nodata_exists) override {
      no_data = no_data_val;
      nodata_exists = nodata_exists_val;
      ++num_gets;
    }

    // Override abstract functions so we can instantiate the class
    virtual void FetchPixels(const khExtents<uint32> &srcExtents) override {}
};

TEST(khGDALReaderTest, NoDataDoesntExist) {
  MockReader reader(255, false);
  int8 nodata = reader.CallGetNoDataOrZero<int8>();
  ASSERT_EQ(0, nodata);
}

TEST(khGDALReaderTest, NoDataZero) {
  MockReader reader(0, true);
  uint32 nodata = reader.CallGetNoDataOrZero<uint32>();
  ASSERT_EQ(0, nodata);
}

TEST(khGDALReaderTest, NoDataNonZero) {
  MockReader reader(-9999, true);
  float32 nodata = reader.CallGetNoDataOrZero<float32>();
  ASSERT_EQ(-9999, nodata);
}

TEST(khGDALReaderTest, NoDataTooBigPositive) {
  MockReader reader(256, true);
  uint8 nodata = reader.CallGetNoDataOrZero<uint8>();
  ASSERT_EQ(0, nodata);
}

TEST(khGDALReaderTest, NoDataTooBigNegative) {
  MockReader reader(-100000, true);
  int16 nodata = reader.CallGetNoDataOrZero<int16>();
  ASSERT_EQ(0, nodata);
}

TEST(khGDALReaderTest, NoDataMultipleCalls) {
  MockReader reader(16, true);
  float64 nodata = reader.CallGetNoDataOrZero<float64>();
  ASSERT_EQ(16, nodata);
  ASSERT_EQ(1, reader.num_gets);
  nodata = reader.CallGetNoDataOrZero<float64>();
  ASSERT_EQ(16, nodata);
  ASSERT_EQ(2, reader.num_gets);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
