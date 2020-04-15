/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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

#ifndef _FUSION_TOOLS_BOXFILTER_H__
#define _FUSION_TOOLS_BOXFILTER_H__

// Performs box-filtering on a grayscale image.  The value of each
// output pixel is the average of the values in a box around the
// pixel.  A border value gives the value of any pixels outside the
// image itself, so that the output image can be the same size as the
// input image.
//
// The filtering is computed on a per-tile basis, so the whole image doesn't
// have to fit in memory.  The tile width must be larger than the box
// width / 2.
//
// The implementation caches up to 4 tiles in memory plus a single
// output tile.  Each output tile is saved only once.  In-memory
// processing is O(image_area + box_area), so the computation is
// dominated by I/O. Assuming that I/O is expensive, here are the best
// methods for performing box filtering, most efficient first, if the
// memory size (height*width) of each tile is M:
//
// - Single tile for entire image, calling SingleImageTile(). Loads the
//     image only once, uses M memory (output tile computed in place).
// - Image is 1 tile wide, with tile height > box filter ht / 2.
//     Loads each tile only once and uses M*3 memory.
// - Image is 2 tiles wide, with tile height > box filter ht / 2.
//     Loads each tile only once and uses M*5 memory.
// - Many tiles, but ensuring tile height > box filter ht / 2.
//     Loads each tile up to 2 times and uses M*5 memory.
// - Many tiles, but with tile height <= box filter ht / 2.  Tiles may
//     be loaded many times, but still uses M*5 memory.
//
// To use, create a derived class that implements LoadImageTile and
// SaveFilteredTile.  If desired, you can override the behavior of the
// Filter() function.  Construct an object of the class, call
// SingleImageTile() if possible, then call BoxFilter().

#include <vector>

#include <common/base/macros.h>
#include "khTypes.h"
#include <cstdint>


class BoxFilterTiledImage {
 public:
  BoxFilterTiledImage(int image_width, int image_height,
                      int tile_width, int tile_height);
  virtual ~BoxFilterTiledImage();

  // If a single input image tile is already in memory, calling
  // SingleImageTile avoids the copying and extra memory of tiles. The
  // output image is computed in place, and SaveFilteredTile() is
  // still called.  The class will not delete the image.
  void SingleImageTile(unsigned char *image);

  // Box filters the image.  box_width and box_height must be odd.
  // tile_width must be >= box_width/2.
  void BoxFilter(int box_width, int box_height, unsigned char border_value);

 protected:
  // Called by base impl GetTile to load image tiles from store.
  virtual void LoadImageTile(int tile_x, int tile_y,
                             unsigned char *image_tile) = 0;

  // Called by BoxFilter() to save filtered image tiles. Define in
  // derived class to save tiles.  Function should not delete the
  // output_tile.
  virtual void SaveFilteredTile(int tile_x, int tile_y,
                                const unsigned char *output_tile) = 0;

  // Given the sum of pixels in the box and a pointer to the destination pixel,
  // set the pixel to its final value. A derived class might want to handle
  // differently, for example for non-linear response or to avoid raising any
  // values.
  virtual void Filter(int box_sum, unsigned char *pixel) const = 0;

  int image_width_, image_height_;  // full image size
  int tile_width_, tile_height_;    // size of each tile
  int num_tiles_x_, num_tiles_y_;   // number of tiles in each direction
  int box_area_;                    // Caches box_w * box_h

 private:
  // BoxFilter() calls GetImageTile(), which caches up to 4 tiles in
  // memory and calls LoadImageTile() on cache miss.  Returns NULL if
  // tile is outside image bounds.  border value is used to fill in
  // extra pixels in case of unfilled edge tiles.
  virtual const unsigned char *GetImageTile(int tile_x, int tile_y,
                                    unsigned char border);

  bool single_tile_mode_;              // true iff SingleImageTile() was called
  std::vector<unsigned char *> cached_tiles_;  // 4 tiles cached
  std::vector<int> cached_tile_ids_;   // Holds cache id of each tile in tile_

  // Disallow copying: do not define these
  DISALLOW_COPY_AND_ASSIGN(BoxFilterTiledImage);
};


// This is an instantiation of the above class to do standard averaging of the
// pixels in the box.
class AveragingBoxFilter : public BoxFilterTiledImage {
 public:
  AveragingBoxFilter(int image_width, int image_height,
                      int tile_width, int tile_height)
      : BoxFilterTiledImage(image_width, image_height,
                            tile_width, tile_height) {
  }
  virtual ~AveragingBoxFilter() {}

 protected:
  // Sets the pixel to be the average of the pixel values in the box.
  virtual void Filter(int box_sum, unsigned char *pixel) const {
    *pixel = box_sum / box_area_;
  }
 private:
};

#endif  // _FUSION_TOOLS_BOXFILTER_H__
