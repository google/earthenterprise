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


#include "common/tile_resampler.h"
#include <string>
#include <vector>

#include "common/compressor.h"
#include "common/generic_utils.h"
#include "common/quadtreepath.h"
#include "common/khSimpleException.h"
#include "common/geindex/Reader.h"
#include "common/khEndian.h"
#include "common/khGuard.h"


TileResampler::TileResampler(int tile_width, int num_channels, bool is_png)
    : compressor_(0),
      tile_width_(tile_width),
      num_channels_(num_channels),
      is_png_(is_png) {
  int byte_count = tile_width_ * tile_width_ * num_channels_;
  if (is_png_) {
    // Note: We don't want to handle endian-ness here since we do not need to
    // unpack RGBA tuples.
    const unsigned int png_byte_order_option = 0;
    compressor_ = NewPNGCompressor(
        tile_width_, tile_width_, num_channels_, png_byte_order_option);
  } else {
    int quality = 25;  // Standard quality is 75, but for resampled tiles we
                       // can afford some quality loss.
    compressor_ = new JPEGCompressor(
        tile_width_, tile_width_, num_channels_, quality);
  }
  // Probably unnecessary, but adding for safety.
  raw_buffer_.resize(byte_count);
  result_buffer_.resize(byte_count);
}

TileResampler::~TileResampler() {
  if (compressor_) {
    delete compressor_;
  }
}

bool TileResampler::ResampleTileInBuffer(
    geindex::UnifiedReader::ReadBuffer* const buffer,
    const QuadtreePath& qpath_parent,
    const QuadtreePath& qpath_child) {
  // We're going to use contents of buffer for jpeg/png decompression.
  int count = buffer->size();
  char* input_buffer = static_cast<char*>(&((*buffer)[0]));
  if (ResampleTileHelper(input_buffer, count, qpath_parent, qpath_child)) {
    // Compress the tile and copy it into our buffer.
    char* result_buffer = static_cast<char*>(&(result_buffer_[0]));
    std::uint32_t count_compressed = compressor_->compress(result_buffer);

    // Copy the compressed result back into the read buffer.
    buffer->SetValueFromBuffer(compressor_->data(), count_compressed);
    return true;
  }
  return false;
}

bool TileResampler::ResampleTileInString(
    std::string* const buffer,
    const QuadtreePath& qpath_parent,
    const QuadtreePath& qpath_child) {
  // We're going to use contents of buffer for jpeg decompression.
  int count = buffer->size();
  char* input_buffer = static_cast<char*>(&((*buffer)[0]));
  if (ResampleTileHelper(input_buffer, count, qpath_parent, qpath_child)) {
    // Compress the tile and copy it into our buffer.
    char* result_buffer = static_cast<char*>(&(result_buffer_[0]));
    std::uint32_t count_compressed = compressor_->compress(result_buffer);

    // Copy the compressed result back into the read buffer.
    buffer->assign(compressor_->data(), count_compressed);
    return true;
  }
  return false;
}

bool TileResampler::ResampleTileInString(
    std::string* const buffer,
    const std::string& qpath_parent_str,
    const std::string& qpath_child_str) {
  QuadtreePath qpath_parent(qpath_parent_str.c_str());
  QuadtreePath qpath_child(qpath_child_str.c_str());
  return ResampleTileInString(buffer, qpath_parent, qpath_child);
}

bool TileResampler::ResampleTileHelper(char* input_buffer,
                                       int count,
                                       const QuadtreePath& qpath_parent,
                                       const QuadtreePath& qpath_child) {
  // Decompress into a raw byte buffer.
  char* raw_buffer = static_cast<char*>(&(raw_buffer_[0]));

  // We support jpeg and png at the moment.
  if (is_png_) {
    if (!gecommon::IsPngBuffer(input_buffer)) {
      return false;
    }
  } else {  // jpeg resampling.
    if (!gecommon::IsJpegBuffer(input_buffer)) {
      return false;
    }
  }

  std::uint32_t count_raw = compressor_->decompress(input_buffer, count, raw_buffer);

  // Make sure the raw byte count is correct.
  if ((count_raw != static_cast<std::uint32_t>(raw_buffer_.size())) ||
      (count_raw != static_cast<std::uint32_t>(result_buffer_.size()))) {
    return false;  // Our assumption is wrong about tile size, return false.
  }

  // Buffer for the resampled results.
  char* result_buffer = static_cast<char*>(&(result_buffer_[0]));

  if (!ResampleTile(raw_buffer, qpath_parent, qpath_child, result_buffer)) {
    return false;  // Failure likely due to bad quadtree paths.
  }

  return true;
}

bool TileResampler::ResampleTile(const char* raw_buffer,
                                 const QuadtreePath& qpath_parent,
                                 const QuadtreePath& qpath_child,
                                 char* output_buffer) {
  int row_start_index;
  int col_start_index;
  int width;
  if (!qpath_parent.ChildTileCoordinates(
          tile_width_, qpath_child,
          &row_start_index, &col_start_index, &width)) {
    return false;  // Bad paths.
  }

  int offset = tile_width_ / width;
  // We iterate over the pixels from row/col_start_index and width
  // rows and cols.
  // Copy those pixels into blocks of pixels in the output_buffer.
  int row_end_index = row_start_index + width;
  int col_end_index = col_start_index + width;
  char pixel_value_[num_channels_];
  for (int row_in = row_start_index, row_out = 0; row_in < row_end_index;
      ++row_in, row_out += offset) {
    const char* in_row_ptr = raw_buffer +
      (row_in * tile_width_ + col_start_index) * num_channels_;
    for (int col_in = col_start_index, col_out = 0; col_in < col_end_index;
        ++col_in, col_out += offset) {
      // Get the band values at the current [col_in, row_in] coordinates.
      for (int k = 0; k < num_channels_; ++k) {
        pixel_value_[k] = *in_row_ptr++;
      }

      // Copy band values into the corresponding block of pixels in
      // the re-sampled output image.
      // No smoothing for now...want this to be fast.
      for (int j = row_out; j < row_out + offset; ++j) {
        char* out_row_ptr = output_buffer +
          (j * tile_width_ + col_out) * num_channels_;
        for (int i = col_out; i < col_out + offset; ++i) {
          for (int k = 0; k < num_channels_; ++k) {
           *out_row_ptr++ = pixel_value_[k];
          }
        }
      }
    }
  }

  return true;
}
