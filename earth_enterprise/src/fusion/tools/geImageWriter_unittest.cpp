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


// Unit Tests for geImageWriter

#include <string>
#include <gtest/gtest.h>

#include "common/khFileUtils.h"
#include "common/notify.h"
#include "fusion/khgdal/khGeoExtents.h"
#include "fusion/tools/Featherer.h"
#include "fusion/tools/geImageWriter.h"
#include "fusion/khraster/LevelRasterBand.h"

class geImageWriterTest : public testing::Test {
  protected:
    std::string testInputFile_;
    std::string testOutputAlphaFile_;
    std::string testOutputPreviewFile_;
    int maskHeight_;
    int maskWidth_;
    GDALRasterBand* alphaBand_;
    GDALDriver* driver_;
    unsigned char* alphaBuf_;

    virtual void SetUp() {
      testInputFile_ = "fusion/testdata/testmask.tif";
      testOutputAlphaFile_ = "fusion/testdata/testmaskoutAlpha.tif";
      testOutputPreviewFile_ = "fusion/testdata/testmaskoutPreview.tif";
      maskHeight_ = 5856;
      maskWidth_ = 8206;

      // Read the mask so we can try to write it back out
      khGDALInit();
      driver_ = reinterpret_cast<GDALDriver*>(GDALGetDriverByName("GTiff"));
      GDALDataset* dataset = reinterpret_cast<GDALDataset*>(
          GDALOpen(testInputFile_.c_str(), GA_ReadOnly));
      alphaBand_ = dataset->GetRasterBand(1);
      alphaBuf_ = new unsigned char[maskWidth_ * maskHeight_];

      if (alphaBand_->RasterIO(GF_Read, 0, 0,
                               maskWidth_, maskHeight_,
                               alphaBuf_,
                               maskWidth_, maskHeight_,
                               GDT_Byte, 0, maskWidth_)
          == CE_Failure) {
        notify(NFY_FATAL, "Could not read in mask.");
      }
    }

    virtual void TearDown() {
      if (khExists(testOutputAlphaFile_)) {
        khPruneFileOrDir(testOutputAlphaFile_);
      }
      if (khExists(testOutputPreviewFile_)) {
        khPruneFileOrDir(testOutputPreviewFile_);
      }
      delete [] alphaBuf_;
    }
};

// Test writing an alpha image.
TEST_F(geImageWriterTest, ImageWriterBasicTest) {
  khOffset<double> imageDOffset(XYOrder, 0, 0);
  khSize<double> imageDSize(maskWidth_, maskHeight_);
  khExtents<double> imageDExtents(imageDOffset, imageDSize);
  khGeoExtents imageDGeoExtents(
      7, imageDExtents, true /* top->bottom */, false);
  khSize<std::uint32_t> imageSize(maskWidth_, maskHeight_);

  geImageWriter::WriteAlphaImage(imageSize, alphaBuf_, driver_,
                                 testOutputAlphaFile_, imageDGeoExtents,
                                 false, NULL);

  EXPECT_TRUE(khExists(testOutputAlphaFile_));
  // Header info should make it bigger than the data itself
  EXPECT_LT(static_cast<std::uint64_t>(maskHeight_ * maskWidth_),
            khGetFileSizeOrThrow(testOutputAlphaFile_));
}

// TODO: Test other writes when we have better mocks for
//                  raster products.

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
