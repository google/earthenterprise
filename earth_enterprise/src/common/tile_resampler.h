/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */



#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_TILE_RESAMPLER_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_TILE_RESAMPLER_H_

#include <string>
#include <vector>
#include "common/quadtreepath.h"
#include "common/geindex/Reader.h"

class Compressor;
class QuadtreePath;


// TileResampler is a private utility class that provides a basic
// function of resampling jpg/png tile.
class TileResampler {
 public:
  // This class will be reused over and over with the same buffers.
  explicit TileResampler(int tile_width,
                         int num_channels = 3,
                         bool is_png = false);
  ~TileResampler();

  // Given a jpg compressed buffer and a parent and child quadtree paths,
  // return the buffer containing a jpg re-sampled tile such that the contents
  // of the tile are the zoomed in pixels of the child quad node within the
  // original tile.
  // buffer: the input and output ReadBuffer. Contains the jpeg tile on input
  //         and the re-sampled jpeg tile on output.
  // qpath_parent: the quadtree path of the input tile.
  // qpath_child: the quadtree path of the result tile.
  //               the qpath_child must be a child of input_path.
  // returns false if the parent quadtree path is not the ancestor of the
  //         child quadtree path.
  bool ResampleTileInBuffer(geindex::UnifiedReader::ReadBuffer* const buffer,
                            const QuadtreePath& qpath_parent,
                            const QuadtreePath& qpath_child);

  bool ResampleTileInString(std::string* const buffer,
                            const QuadtreePath& qpath_parent,
                            const QuadtreePath& qpath_child);

  bool ResampleTileInString(std::string* const buffer,
                            const std::string& qpath_parent_str,
                            const std::string& qpath_child_str);

  // Given a raw uncompressed buffer, re-sample the image such that the contents
  // of the tile are the zoomed in pixels of the child quad node within the
  // original tile.
  // raw_buffer: a pointer to the raw uncompressed image buffer.
  //             Note: the buffer is assumed to be tile_width_^2 * 3 bytes.
  // qpath_parent: the quadtree path of the input tile.
  // qpath_child: the quadtree path of the result tile.
  //               the qpath_child must be a child of input_path.
  // output_buffer: a pointer to the raw uncompressed re-sampled image buffer.
  // returns false if the parent quadtree path is not the ancestor of the
  //         child quadtree path.
  bool ResampleTile(const char* raw_buffer,
                    const QuadtreePath& qpath_parent,
                    const QuadtreePath& qpath_child,
                    char* output_buffer);

 private:
  Compressor* compressor_;

  // Keep raw buffer caches for the specified tile size
  // (tile_width x tile_width).
  const int tile_width_;
  const int num_channels_;
  const bool is_png_;
  std::string raw_buffer_;
  std::string result_buffer_;

  bool ResampleTileHelper(char* input_buffer,
                          int count,
                          const QuadtreePath& qpath_parent,
                          const QuadtreePath& qpath_child);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_TILE_RESAMPLER_H_
