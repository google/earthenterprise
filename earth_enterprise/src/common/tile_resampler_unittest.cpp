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


// Unit Tests for TileResampler

#include <string>
#include <gtest/gtest.h>
#include "common/tile_resampler.h"
#include "common/khTileAddr.h"

namespace {

class TileResamplerTest : public testing::Test { };


struct ResampleData {
  int tile_width;
  std::string qpath_string;
  std::string child_step;
};

// Utility for testing TileResampler::Resample.
void CheckResample(TileResampler& resampler,
                   const ResampleData& data,
                   char* input_buffer,
                   char* output_buffer) {

  QuadtreePath qpath_parent(data.qpath_string);
  QuadtreePath qpath_child(data.qpath_string + data.child_step);
  int row_start_index;
  int col_start_index;
  int width;
  qpath_parent.ChildTileCoordinates(data.tile_width, qpath_child,
                                    &row_start_index, &col_start_index, &width);

  // We resample the image and then use the brute force mapping from
  // output pixels to input pixels to test it.
  EXPECT_TRUE(resampler.ResampleTile(input_buffer,
                                     qpath_parent,
                                     qpath_child,
                                     output_buffer));
  int tile_width = data.tile_width;
  int offset = tile_width / width;
  char* out_ptr = output_buffer;
  for(int row = 0; row < data.tile_width; ++row) {
    for(int col = 0; col < data.tile_width; ++col, out_ptr += 3) {
      // Here's the brute force mapping from output pixel coordinates to
      // input pixel coordinates.
      int row_in = row / offset + row_start_index;
      int col_in = col / offset + col_start_index;
      char* in_ptr = input_buffer + (row_in * tile_width + col_in) * 3;
      if (in_ptr[0] != out_ptr[0] || in_ptr[1] != out_ptr[1] ||
          in_ptr[2] != out_ptr[2]) {
        fprintf(stderr, "Failed ResampleTile Test of %s, %s at "
                "[%d,%d] to [%d,%d]\n",
                data.qpath_string.c_str(), data.child_step.c_str(),
                col_in, row_in, col, row);
        ASSERT_EQ(in_ptr[0], out_ptr[0]);
        ASSERT_EQ(in_ptr[1], out_ptr[1]);
        ASSERT_EQ(in_ptr[2], out_ptr[2]);
      }
    }
  }
}

TEST_F(TileResamplerTest, ResampleTile) {
  TileResampler tile_resampler(ImageryQuadnodeResolution);

  int tile_width = ImageryQuadnodeResolution;
  int bytes_per_pixel = 3;
  std::string input_image;
  int byte_count = tile_width * tile_width * bytes_per_pixel;
  input_image.resize(byte_count);
  char* input_buffer = static_cast<char*>(&(input_image[0]));
  // Fill the input buffer with garbage.
  for(int i = 0; i < byte_count; ++i) {
    input_buffer[i] = static_cast<char>((i % 512) - 256);  // Truncate to 8bits.
  }
  std::string output_image;
  output_image.resize(byte_count);
  char* output_buffer = static_cast<char*>(&(output_image[0]));


  ResampleData test_data[] = {
      {tile_width, "0321", ""},
      {tile_width, "10", "021"},
      {tile_width, "000111", "32132"},
  };
  int count = static_cast<int>(sizeof(test_data)/sizeof(ResampleData));
  for(int i = 0; i < count; ++i) {
    CheckResample(tile_resampler, test_data[i], input_buffer, output_buffer);
  }
}

}  // namespace

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

