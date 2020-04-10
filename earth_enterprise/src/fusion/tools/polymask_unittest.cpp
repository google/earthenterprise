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


// Unit Tests for PolyMask

#include <limits.h>
#include <string>
#include <vector>
#include <stdlib.h>
#include <gtest/gtest.h>
#include "common/khFileUtils.h"
#include "fusion/tools/polymask.h"

namespace fusion_tools {

// Test class for PolyMask. Provides simple tools for
// comparing generated masks.
class PolyMaskTest : public testing::Test {
  protected:
    PolyMask* poly_mask_;
    int mask_width_;
    int mask_height_;
    std::string image_file_;
    std::string output_mask1_;
    std::string output_mask2_;
    std::string gold_standard_mask_;
    std::string maskA_;
    std::string maskAa_;
    std::string maskAb_;
    std::string maskB_;
    std::string maskBa_;
    std::string maskC_;

    virtual void SetUp() {
      image_file_ = "fusion/testdata/i3SF15-meter.tif";
      output_mask1_ = "fusion/testdata/output_mask1.tif";
      output_mask2_ = "fusion/testdata/output_mask2.tif";
      gold_standard_mask_ = "fusion/testdata/gold_mask.tif";

      // Kml and tiff masks
      // Uppercase indicates the masks sit alone at the top level.
      // Uppercase then lowercase indicates mask sits within the uppercase mask.
      maskA_ = "fusion/testdata/maskA.kml";
      maskAa_ = "fusion/testdata/maskAa.kml";
      maskAb_ = "fusion/testdata/maskAb.kml";
      maskB_ = "fusion/testdata/maskB.kml";
      maskBa_ = "fusion/testdata/maskBa.tif";
      maskC_ = "fusion/testdata/maskC.tif";

      poly_mask_ = new PolyMask();
    }

    virtual void TearDown() {
      if (khExists(output_mask1_)) {
        // khPruneFileOrDir(output_mask1_);
      }
      if (khExists(output_mask2_)) {
        // khPruneFileOrDir(output_mask2_);
      }

      delete poly_mask_;
    }

    // Helper function indicates if all bytes in image match the given byte.
    bool EqualMaskData(std::vector<unsigned char> data, unsigned char match_byte, size_t size) {
      for (size_t i = 0; i < size; ++i) {
        if (data[i] != match_byte) {
          return false;
        }
      }
      return true;
    }

    // Helper function counts number of occurrences of a given byte in the
    // given mask.
    int CountBytes(std::string mask,
                   unsigned char match_byte,
                   int width,
                   int height) {
      std::vector<unsigned char> data(width * height);
      int count = 0;

      // Load mask to count from file.
      PolyMask::LoadMask(mask, &data, width, height);

      // Check if all bytes are equal.
      for (int i = 0; i < width * height; ++i) {
        if (data[i] == match_byte) {
          ++count;
        }
      }

      return count;
    }

    // Helper function checks if the xor of every byte in two mask files
    // is equal to the given byte.
    bool XorMaskMatch(std::string mask1,
                      std::string mask2,
                      unsigned char match_byte,
                      int width,
                      int height) {
      std::vector<unsigned char> data1(width * height);
      std::vector<unsigned char> data2(width * height);

      // Load two masks from given files.
      PolyMask::LoadMask(mask1, &data1, width, height);
      PolyMask::LoadMask(mask2, &data2, width, height);

      // Check if all bytes are equal.
      bool equal = true;
      for (int i = 0; i < width * height; ++i) {
        unsigned char xor_byte = data1[i] ^ data2[i];
        if (xor_byte != match_byte) {
          notify(NFY_NOTICE,
                 "failed at %d (d1: %x d2: %x xor: %x match: %x)",
                 i, data1[i], data2[i], xor_byte, match_byte);
          equal = false;
          break;
        }
      }

      notify(NFY_NOTICE,
             "Finished. Compared %d (%d)", width * height, equal);
      return equal;
    }

    // Helper function indicates if two masks are equal.
    bool EqualMasks(
        std::string mask1, std::string mask2, int width, int height) {
      return XorMaskMatch(mask1, mask2, 0x00, width, height);
    }

    // Helper function indicates if two masks are negatives of each other.
    bool NegativeMasks(
        std::string mask1, std::string mask2, int width, int height) {
      return XorMaskMatch(mask1, mask2, 0xff, width, height);
    }

    // Helper function extracts mask file name.
    std::string MaskFile(PolyMask* poly_mask) {
      return poly_mask->output_file_;
    }

    // Helper function extracts image file name.
    std::string ImageFile(PolyMask* poly_mask) {
      return poly_mask->base_image_file_;
    }

    // Clear command buffers so a new mask can be created.
    void ClearCommands(PolyMask* poly_mask) {
      poly_mask->commands_.clear();
      poly_mask->command_string_args_.clear();
      poly_mask->command_int_args_.clear();
      poly_mask->feather_ = 0;
    }

    // Helper function to set image dimensions.
    void SetDimensions(PolyMask* poly_mask) {
      mask_width_ = poly_mask->mask_width_;
      mask_height_ = poly_mask->mask_height_;
    }

    // Helper function to get image info.
    void ImageInfo(PolyMask* poly_mask,
                   std::string image_file,
                   int* width,
                   int* height,
                   khGeoExtents* geo_extents,
                   std::string* projection) {
      poly_mask->ImageInfo(image_file, width, height, geo_extents, projection);
    }
};

// Test against a pre-saved feathered mask.
// The gold standard was created by this tool so this is more a unit test
// to detect change rather than to verify current functionality.
//
// Gold standard file generated with the command:
// gepolymaskgen --feather -30 --base_image /path/i3SF15-meter.tif
//               --feather 20 --or_mask /path/maskA.kml
//                            --or_mask /path/maskB.kml
//               --feather 15 --or_mask /path/maskC.tif
//               --feather 5  --and_mask /path/maskAa.kml
//                            --and_mask /path/maskAb.kml
//               --feather 40 --and_mask /path/maskBa.tif
//               --output_mask /path/gold_standard.tif
TEST_F(PolyMaskTest, GoldStandardTest) {
  // TODO: Fix this for pulse build.
  return;
  poly_mask_->AddCommandSetFeather(-30);
  poly_mask_->AddCommandSetBaseImage(image_file_);

  poly_mask_->AddCommandSetFeather(20);
  poly_mask_->AddCommandOrMask(maskA_);
  poly_mask_->AddCommandOrMask(maskB_);

  poly_mask_->AddCommandSetFeather(15);
  poly_mask_->AddCommandOrMask(maskC_);

  poly_mask_->AddCommandSetFeather(5);
  poly_mask_->AddCommandAndMask(maskAa_);
  poly_mask_->AddCommandAndMask(maskAb_);

  poly_mask_->AddCommandSetFeather(40);
  poly_mask_->AddCommandAndMask(maskBa_);

  poly_mask_->AddCommandSetOutputMask(output_mask1_);

  // Build the mask
  poly_mask_->BuildMask();

  // Compare the mask to the gold standard
  SetDimensions(poly_mask_);
  EXPECT_TRUE(EqualMasks(gold_standard_mask_, output_mask1_,
                         mask_width_, mask_height_));
}

// Test preservation of geodata from image in mask.
TEST_F(PolyMaskTest, GeodataPreservationTest) {
  poly_mask_->AddCommandSetBaseImage(image_file_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);

  // Build the empty mask file.
  poly_mask_->BuildMask();

  // Get dimensions and geo extent of the original image.
  int image_height;
  int image_width;
  khGeoExtents image_geo_extents;
  std::string image_projection;
  ImageInfo(poly_mask_, image_file_, &image_width, &image_height,
            &image_geo_extents, &image_projection);
  // Get dimensions and geo extent of the generated mask.
  int mask_height;
  int mask_width;
  khGeoExtents mask_geo_extents;
  std::string mask_projection;
  ImageInfo(poly_mask_, output_mask1_, &mask_width, &mask_height,
            &mask_geo_extents, &mask_projection);

  // Make sure dimensions, geo extent and projection of image and mask are the
  // same.
  EXPECT_EQ(image_width, mask_width);
  EXPECT_EQ(image_height, mask_height);
  EXPECT_EQ(image_geo_extents, mask_geo_extents);
  EXPECT_EQ(image_projection, mask_projection);
}

// Test mask against its negative.
TEST_F(PolyMaskTest, NegativeMaskTest) {
  poly_mask_->AddCommandSetBaseImage(image_file_);

  poly_mask_->AddCommandSetFeather(25);
  poly_mask_->AddCommandOrMask(maskA_);
  poly_mask_->AddCommandOrMask(maskB_);

  poly_mask_->AddCommandSetFeather(8);
  poly_mask_->AddCommandAndMask(maskAa_);
  poly_mask_->AddCommandAndMask(maskAb_);

  poly_mask_->AddCommandSetOutputMask(output_mask1_);

  // Build the first mask.
  poly_mask_->BuildMask();

  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseImage(image_file_);

  poly_mask_->AddCommandSetFeather(25);
  poly_mask_->AddCommandOrMask(maskA_);
  poly_mask_->AddCommandOrMask(maskB_);

  poly_mask_->AddCommandSetFeather(8);
  poly_mask_->AddCommandAndMask(maskAa_);
  poly_mask_->AddCommandAndMask(maskAb_);

  // Invert the mask to get the negative.
  poly_mask_->AddCommandInvert();

  poly_mask_->AddCommandSetOutputMask(output_mask2_);

  // Build the second mask.
  poly_mask_->BuildMask();

  SetDimensions(poly_mask_);
  EXPECT_TRUE(NegativeMasks(output_mask1_, output_mask2_,
                            mask_width_, mask_height_));
}

// Test progression of masks as polygons are added and subtracted.
TEST_F(PolyMaskTest, MaskProgressionTest) {
  // Start by building blank mask.
  poly_mask_->AddCommandSetBaseImage(image_file_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  poly_mask_->BuildMask();

  SetDimensions(poly_mask_);
  int num00 = CountBytes(output_mask1_, 0x00, mask_width_, mask_height_);
  int numff = CountBytes(output_mask1_, 0xff, mask_width_, mask_height_);
  // Expect all 0's initially.
  EXPECT_EQ(0, numff);
  EXPECT_EQ(mask_width_ * mask_height_, num00);

  // Add first level polygons (add A and B).
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseImage(image_file_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  poly_mask_->AddCommandOrMask(maskA_);
  poly_mask_->AddCommandOrMask(maskB_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  // Rebuild mask.
  poly_mask_->BuildMask();

  int pass0_num00 = CountBytes(output_mask1_, 0x00, mask_width_, mask_height_);
  int pass0_numff = CountBytes(output_mask1_, 0xff, mask_width_, mask_height_);
  // Now some 0's and some 0xff's.
  // A + B
  EXPECT_LT(0, pass0_numff);
  EXPECT_LT(pass0_numff, mask_width_ * mask_height_);
  EXPECT_LT(0, pass0_num00);
  EXPECT_LT(pass0_num00, mask_width_ * mask_height_);
  EXPECT_EQ(mask_width_ * mask_height_, pass0_num00 + pass0_numff);

  // Subtract second level polygons  (subtract Aa and Ab).
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseImage(image_file_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  poly_mask_->AddCommandOrMask(maskA_);
  poly_mask_->AddCommandOrMask(maskB_);
  poly_mask_->AddCommandAndMask(maskAa_);
  poly_mask_->AddCommandAndMask(maskAb_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  // Rebuild mask.
  poly_mask_->BuildMask();

  int pass1_num00 = CountBytes(output_mask1_, 0x00, mask_width_, mask_height_);
  int pass1_numff = CountBytes(output_mask1_, 0xff, mask_width_, mask_height_);

  // The second level should have subtracted pixels from the first level.
  // A + B - Aa - Ab
  EXPECT_LT(pass1_numff, pass0_numff);
  EXPECT_LT(pass1_numff, mask_width_ * mask_height_);
  EXPECT_LT(pass0_num00, pass1_num00);
  EXPECT_LT(pass1_num00, mask_width_ * mask_height_);
  EXPECT_EQ(mask_width_ * mask_height_, pass0_num00 + pass0_numff);

  // Add second level polygons back in (add Aa and Ab).
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseImage(image_file_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  poly_mask_->AddCommandOrMask(maskA_);
  poly_mask_->AddCommandOrMask(maskB_);
  poly_mask_->AddCommandAndMask(maskAa_);
  poly_mask_->AddCommandAndMask(maskAb_);
  poly_mask_->AddCommandOrMask(maskAa_);
  poly_mask_->AddCommandOrMask(maskAb_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  // Rebuild mask.
  poly_mask_->BuildMask();

  int pass2_num00 = CountBytes(output_mask1_, 0x00, mask_width_, mask_height_);
  int pass2_numff = CountBytes(output_mask1_, 0xff, mask_width_, mask_height_);

  // The second level should have been removed.
  // A + B - Aa - Ab + Aa + Ab = A + B
  EXPECT_EQ(pass2_num00, pass0_num00);
  EXPECT_EQ(pass2_numff, pass0_numff);
}

// Simple test using base image.
TEST_F(PolyMaskTest, SimpleBaseImageTest) {
  // Create a base mask with first pass polygons and no feathering.
  poly_mask_->AddCommandSetBaseImage(image_file_);

  poly_mask_->AddCommandOrMask(maskA_);
  poly_mask_->AddCommandOrMask(maskB_);

  poly_mask_->AddCommandSetOutputMask(output_mask1_);

  // Build the mask
  poly_mask_->BuildMask();

  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseMask(output_mask1_);
  poly_mask_->AddCommandSetOutputMask(output_mask2_);
  poly_mask_->BuildMask();

  SetDimensions(poly_mask_);
  EXPECT_TRUE(EqualMasks(output_mask1_, output_mask2_,
                         mask_width_, mask_height_));
}

// Test using base image with feathering.
TEST_F(PolyMaskTest, BaseImageWithFeatherTest) {
  // Create the feathered mask.
  poly_mask_->AddCommandSetBaseImage(image_file_);

  poly_mask_->AddCommandSetFeather(25);
  poly_mask_->AddCommandOrMask(maskA_);

  poly_mask_->AddCommandSetOutputMask(output_mask1_);

  // Build the mask.
  poly_mask_->BuildMask();

  // Create the unfeathered mask.
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseImage(image_file_);

  poly_mask_->AddCommandOrMask(maskA_);

  poly_mask_->AddCommandSetOutputMask(output_mask2_);

  // Build the mask
  poly_mask_->BuildMask();

  SetDimensions(poly_mask_);
  EXPECT_FALSE(EqualMasks(output_mask1_, output_mask2_,
                          mask_width_, mask_height_));

  // Now create the feathered mask from the unfeathered mask.
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetFeather(25);
  poly_mask_->AddCommandSetBaseMask(output_mask2_);
  poly_mask_->AddCommandSetOutputMask(output_mask2_);

  // Build the mask.
  poly_mask_->BuildMask();

  EXPECT_TRUE(EqualMasks(output_mask1_, output_mask2_,
                         mask_width_, mask_height_));
}

// Test ANDing of masks.
TEST_F(PolyMaskTest, MaskANDTest) {
  // Start by building blank mask (all 0x00s).
  poly_mask_->AddCommandSetBaseImage(image_file_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  poly_mask_->BuildMask();

  SetDimensions(poly_mask_);
  int num00 = CountBytes(output_mask1_, 0x00, mask_width_, mask_height_);
  EXPECT_EQ(mask_width_ * mask_height_, num00);

  // 0x00 AND A' = 0x00
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseMask(output_mask1_);
  poly_mask_->AddCommandAndMask(maskA_);
  poly_mask_->AddCommandSetOutputMask(output_mask2_);
  poly_mask_->BuildMask();

  // Both generated masks should be blank.
  EXPECT_TRUE(EqualMasks(output_mask1_, output_mask2_,
                         mask_width_, mask_height_));

  // Build mask A' as tiff image.
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseImage(image_file_);
  poly_mask_->AddCommandOrMask(maskA_);
  poly_mask_->AddCommandInvert();
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  poly_mask_->BuildMask();

  // A' AND A' = A'
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseMask(output_mask1_);
  poly_mask_->AddCommandAndMask(maskA_);
  poly_mask_->AddCommandSetOutputMask(output_mask2_);
  poly_mask_->BuildMask();

  // Generated mask should equal A.
  EXPECT_TRUE(EqualMasks(output_mask1_, output_mask2_,
                         mask_width_, mask_height_));
}

// Test ORing of masks.
TEST_F(PolyMaskTest, MaskORTest) {
  // Build blank mask (all 0x00s).
  poly_mask_->AddCommandSetBaseImage(image_file_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  poly_mask_->BuildMask();

  SetDimensions(poly_mask_);
  int num00 = CountBytes(output_mask1_, 0x00, mask_width_, mask_height_);
  EXPECT_EQ(mask_width_ * mask_height_, num00);

  // Build mask A as tiff image.
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseImage(image_file_);
  poly_mask_->AddCommandOrMask(maskA_);
  poly_mask_->AddCommandSetOutputMask(output_mask2_);
  poly_mask_->BuildMask();

  // 0x00 OR A = A
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseMask(output_mask1_);
  poly_mask_->AddCommandOrMask(maskA_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  poly_mask_->BuildMask();

  // Both generated masks should be blank
  EXPECT_TRUE(EqualMasks(output_mask1_, output_mask2_,
                         mask_width_, mask_height_));

  // A OR A' = 0xff
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseMask(output_mask1_);
  poly_mask_->AddCommandInvert();
  poly_mask_->AddCommandOrMask(maskA_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  poly_mask_->BuildMask();

  int numff = CountBytes(output_mask1_, 0xff, mask_width_, mask_height_);
  EXPECT_EQ(mask_width_ * mask_height_, numff);
}

// Test Threshold of masks.
TEST_F(PolyMaskTest, MaskThresholdTest) {
  // Build mask A.
  poly_mask_->AddCommandSetBaseImage(image_file_);
  poly_mask_->AddCommandOrMask(maskA_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  poly_mask_->BuildMask();

  SetDimensions(poly_mask_);
  int num_A_00 = CountBytes(output_mask1_, 0x00, mask_width_, mask_height_);
  int num_A_ff = CountBytes(output_mask1_, 0xff, mask_width_, mask_height_);
  EXPECT_EQ(mask_width_ * mask_height_, num_A_00 + num_A_ff);

  // Feather and threshold mask A as tiff image.
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseImage(image_file_);
  // Use an erosive feather.
  poly_mask_->AddCommandSetFeather(30);
  poly_mask_->AddCommandOrMask(maskA_);
  poly_mask_->AddCommandThreshold(0xfe);
  poly_mask_->AddCommandSetOutputMask(output_mask2_);
  poly_mask_->BuildMask();
  int num_Af_00 = CountBytes(output_mask2_, 0x00, mask_width_, mask_height_);
  int num_Af_ff = CountBytes(output_mask2_, 0xff, mask_width_, mask_height_);

  EXPECT_EQ(mask_width_ * mask_height_, num_A_00 + num_A_ff);
  // Because of the erosive filtering, the white should shrink ...
  EXPECT_TRUE(num_Af_ff < num_A_ff);
  // ... and the black should expand.
  EXPECT_TRUE(num_Af_00 > num_A_00);

  // Feather and threshold mask A as tiff image.
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseImage(image_file_);
  // Use an expansive feather.
  poly_mask_->AddCommandSetFeather(-30);
  poly_mask_->AddCommandOrMask(maskA_);
  poly_mask_->AddCommandThreshold(0x01);
  poly_mask_->AddCommandSetOutputMask(output_mask2_);
  poly_mask_->BuildMask();
  num_Af_00 = CountBytes(output_mask2_, 0x00, mask_width_, mask_height_);
  num_Af_ff = CountBytes(output_mask2_, 0xff, mask_width_, mask_height_);

  EXPECT_EQ(mask_width_ * mask_height_, num_A_00 + num_A_ff);
  // Because of the expansive filtering, the white should expand ...
  EXPECT_TRUE(num_Af_ff > num_A_ff);
  // ... and the black should shrink.
  EXPECT_TRUE(num_Af_00 < num_A_00);

  // Threshold mask A at 255.
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseImage(image_file_);
  poly_mask_->AddCommandOrMask(maskA_);
  // If threshold is 0xff, everything should go black.
  poly_mask_->AddCommandThreshold(0xff);
  poly_mask_->AddCommandSetOutputMask(output_mask2_);
  poly_mask_->BuildMask();
  num_Af_00 = CountBytes(output_mask2_, 0x00, mask_width_, mask_height_);
  num_Af_ff = CountBytes(output_mask2_, 0xff, mask_width_, mask_height_);

  EXPECT_EQ(mask_width_ * mask_height_, num_Af_00);
  EXPECT_EQ(0, num_Af_ff);

  // Threshold mask A at 0.
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseImage(image_file_);
  poly_mask_->AddCommandOrMask(maskA_);
  // If threshold is 0x00, nothing should change in a b&w mask.
  poly_mask_->AddCommandThreshold(0x00);
  poly_mask_->AddCommandSetOutputMask(output_mask2_);
  poly_mask_->BuildMask();

  EXPECT_TRUE(EqualMasks(output_mask1_, output_mask2_,
                         mask_width_, mask_height_));
}

// Test diff-ing of masks.
TEST_F(PolyMaskTest, MaskDiffTest) {
  // Build blank mask (all 0x00s).
  poly_mask_->AddCommandSetBaseImage(image_file_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  poly_mask_->BuildMask();

  SetDimensions(poly_mask_);
  int num00 = CountBytes(output_mask1_, 0x00, mask_width_, mask_height_);
  EXPECT_EQ(mask_width_ * mask_height_, num00);

  // Build mask A as tiff image.
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseImage(image_file_);
  poly_mask_->AddCommandOrMask(maskA_);
  poly_mask_->AddCommandSetOutputMask(output_mask2_);
  poly_mask_->BuildMask();

  // 0x00 diff A = A
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseMask(output_mask1_);
  poly_mask_->AddCommandDiffMask(maskA_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  poly_mask_->BuildMask();

  // Both generated masks should be blank
  EXPECT_TRUE(EqualMasks(output_mask1_, output_mask2_,
                         mask_width_, mask_height_));

  // A diff A' = 0xff
  ClearCommands(poly_mask_);
  poly_mask_->AddCommandSetBaseMask(output_mask1_);
  poly_mask_->AddCommandInvert();
  poly_mask_->AddCommandDiffMask(maskA_);
  poly_mask_->AddCommandSetOutputMask(output_mask1_);
  poly_mask_->BuildMask();

  int numff = CountBytes(output_mask1_, 0xff, mask_width_, mask_height_);
  EXPECT_EQ(mask_width_ * mask_height_, numff);
}

}  // namespace fusion_tools

int main(int argc, char *argv[]) {
  char* currentpath = getenv("PATH");
  std::string expandedpath = std::string(currentpath)+":/opt/google/bin";
  setenv("PATH", expandedpath.c_str(), 1);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
