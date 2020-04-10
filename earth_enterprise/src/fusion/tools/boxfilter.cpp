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


// Implementation:
//
// The filtering is computed using a summed area table (SAT).  For
// information on SATs, see for example
// http://www.blackwell-synergy.com/doi/abs/10.1111/j.1467-8659.2005.00880.x
//
// See BoxFilter routine below for detailed implementation description.


#include "boxfilter.h"

#include <limits>
#include <numeric>

#include "khGuard.h"
#include "khProgressMeter.h"
#include "notify.h"

// Four tiles are cached in non-single-tile-mode. This allows a
// 2-tile-wide image to never have a cache miss (except when a tile is
// loaded the first time).  See GetImageTile() for details.
const int kNumTilesCached = 4;

BoxFilterTiledImage::BoxFilterTiledImage(int image_width, int image_height,
                                         int tile_width, int tile_height)
    : image_width_(image_width),
      image_height_(image_height),
      tile_width_(tile_width),
      tile_height_(tile_height),
      num_tiles_x_((image_width + tile_width - 1) / tile_width),
      num_tiles_y_((image_height + tile_height - 1) / tile_height),
      single_tile_mode_(false),
      cached_tiles_(kNumTilesCached, static_cast<uchar *>(NULL)),
      cached_tile_ids_(kNumTilesCached, -1) {
  if (image_width < 2 || image_height < 2 || tile_width < 2 || tile_height < 2)
    notify(NFY_FATAL, "BoxFilterTiledImage: bad parameters");
}


BoxFilterTiledImage::~BoxFilterTiledImage() {
  if (!single_tile_mode_)
    for (unsigned int i = 0; i < cached_tiles_.size(); ++i)
      delete [] cached_tiles_[i];
}

void BoxFilterTiledImage::SingleImageTile(unsigned char *image) {
  if (tile_width_ != image_width_ || tile_height_ != image_height_ ||
      cached_tiles_[0] != NULL)
    notify(NFY_FATAL, "BoxFilterTiledImage: bad single-tile parameters");
  cached_tiles_[0] = image;
  cached_tile_ids_[0] = 0;
  single_tile_mode_ = true;
}

const unsigned char *BoxFilterTiledImage::GetImageTile(int tile_x, int tile_y,
                                               unsigned char border) {
  notify(NFY_VERBOSE, "Get tile %d %d\n", tile_x, tile_y);
  if (tile_x < num_tiles_x_ && tile_y < num_tiles_y_) {
    // Re-use slots so if ntilesx<=2, don't load tiles more than once.
    int slot = (tile_x % 2) * 2 + tile_y % 2;
    int this_tile_id = tile_x + num_tiles_x_ * tile_y;
    if (cached_tile_ids_[slot] != this_tile_id) {
      // About to load image.  Should never happen in single-tile mode.
      if (single_tile_mode_)
        notify(NFY_FATAL,
               "BoxFilterTiledImage: tile cache miss in single-tile mode");
      cached_tile_ids_[slot] = this_tile_id;
      if (cached_tiles_[slot] == NULL)
        cached_tiles_[slot] = new unsigned char[tile_width_ * tile_height_];
      LoadImageTile(tile_x, tile_y, cached_tiles_[slot]);

      // Fill borders of tile if needed.
      if (tile_x == num_tiles_x_ - 1) {
        int gap_x = num_tiles_x_ * tile_width_ - image_width_;
        if (gap_x > 0)
          for (int y = 0; y < tile_height_; ++y)
            memset(cached_tiles_[slot] + y * tile_width_ + tile_width_ - gap_x,
                   border, gap_x);
      }
      if (tile_y == num_tiles_y_ - 1) {
        int gap_y = num_tiles_y_ * tile_height_ - image_height_;
        for (int y = tile_height_ - gap_y; y < tile_height_; ++y)
          memset(cached_tiles_[slot] + y * tile_width_, border, tile_width_);
      }
    }
    return cached_tiles_[slot];
  } else {
    return NULL;
  }
}

// Computes the left SAT element of a given row and advances the pointer. We
// don't have a similar function for the top row of the SAT because the
// calculation will just wrap around the horizontal band which is set to 0,
// so the extra elements will have no effect.
inline void ComputeLeftSAT(int **elem,  // pointer to element in SAT table
                           unsigned char value,  // corresponding image value
                           int prev_row_offset) {  // offset to previous row elem
  **elem =
      + *(*elem + prev_row_offset)
      + value;
  ++(*elem);
}

// Computes a single SAT element given current values and an offset to
// the previous row of values in the table.
inline void ComputeSAT(int *elem,  // pointer to element in SAT table
                       unsigned char value,  // corresponding image value
                       int prev_row_offset) {  // offset to previous row elem
  *elem = - *(elem + prev_row_offset - 1) + *(elem + prev_row_offset)
          + *(elem - 1)                   + value;
}

// Computes a horizontal span of entries in the SAT with a single input
// value, moving ptrs forward.
inline void ComputeSATSpanValue(int **psat,   // ptr to first SAT element
                                int length,   // length of the span
                                unsigned char value,  // constant image value (border)
                                int prev_row_offset) {
  for (int *psat_end = (*psat) + length; (*psat) < psat_end; ++(*psat))
    ComputeSAT((*psat), value, prev_row_offset);
}

// Computes a horizontal span of entries in the SAT with a row of
// input values, moving ptrs forward.
inline void ComputeSATSpanValues(int **psat,  // ptr to first SAT element
                                 int length,  // length of the span
                                 const unsigned char **values,  // ptr to image values
                                 int prev_row_offset) {
  for (int *psat_end = (*psat) + length; (*psat) < psat_end;
       ++(*psat), ++(*values))
    ComputeSAT((*psat), **values, prev_row_offset);
}

// Computes a horizonal span of entries in the SAT that starts from the left
// edge of the image and covers the width of the box while moving pointers
// forward. The first box_halfw+1 elements are outside the image and are
// computed from the border value.  The remaining box_halfw elements are
// computed from the image.
inline void ComputeLeftSATSpan(int **psat,  // ptr to the row in the SAT
                               unsigned char border,  // border vlaue
                               int prev_row_offset,  // offset to the prev row
                               int box_halfw,  // (width of box -1) / 2
                               const unsigned char **pimg) {  // ptr to image row
  // Compute the far left value
  ComputeLeftSAT(psat, border, prev_row_offset);
  // Compute the padding outside the image
  ComputeSATSpanValue(psat, box_halfw, border, prev_row_offset);
  // Compute the remaining half of the box using values from the image
  ComputeSATSpanValues(psat, box_halfw, pimg, prev_row_offset);
}


// Main function to do box filtering.  It is used, for example, when feathering
// a mask - in that case, each output pixel is the average of all the input
// pixels within a box centered on the pixel being calculated.
//
// This function is heavily optimized.  It uses a Summed Area Table (SAT) to
// speed up the calculation of the sum of the pixels within the box.
// For more information about SATs, see https://en.wikipedia.org/wiki/Summed-area_table.
// We don't keep the whole SAT in memory, only one sliding horizontal band
// across the whole image width (it slides down as we process each image tile)
// and a vertical band (sliding right) for the current row of tiles.
// The region in the SAT corresponding to the current image tile is padded
// (conceptually) on the sides:
// box_halfw+1 left, box_halfw right, box_halfh+1 top, box_halfh bottom.
// The vertical band contains the right box_width entries of each row of the
// padded SAT tile above the row being processed and the left box_width entries
// of all rows below it.
//
// The horizontal sliding band for tiles to the left of the current
// one is at the bottom of the tile row. For tiles to the right, it is
// at the top. For the current tile, it slides from top (including
// padding) to bottom (including padding), being centered on the
// current pixel. The width of the update window is a full padded tile
// width with box_halfw+1 padding on the left and box_halfw on the
// right.  After the current tile is processed and when we move to the
// one to the right the right side of the old update window (the
// box_halfw of padding and box_halfw inside this tile) are moved back
// to the top of the tile row (from the saved vertical band).
//
// Precision: ints should be ok, even for huge images. It's ok if the
// total count rolls over MAXINT: 2's complement arithmetic should
// give the right result for a box filter footprint.

void BoxFilterTiledImage::BoxFilter(int box_width, int box_height,
                                    unsigned char border) {
  // Check params
  if (box_width <= 0 || box_width % 2 == 0 ||
      box_height <= 0 || box_height % 2 == 0)
    notify(NFY_FATAL, "BoxFilterTiledImage: Bad parameters in BoxFilter()");
  box_area_ = box_width * box_height;
  // Avoid overflow: see above comment about precision.
  if (box_area_ >= std::numeric_limits<int>::max() / 255)
    notify(NFY_FATAL,
           "BoxFilterTiledImage: box area too large for int precision");
  int hband_w = num_tiles_x_ * tile_width_ + box_width;
  int hband_h = box_height + 2;
  int box_halfw = box_width / 2;
  int box_halfh = box_height / 2;
  if (box_halfw > tile_width_)
    notify(NFY_FATAL, "BoxFilterTiledImage: box width / 2 > tile width");

  khProgressMeter progress(num_tiles_x_ * num_tiles_y_);

  // Setup Summed Area Table bands.
  khDeleteGuard<int, ArrayDeleter> sat_hband(
    TransferOwnership(new int[hband_w * hband_h]));
  memset(&sat_hband[0], 0, hband_w * hband_h * sizeof(*sat_hband));
  khDeleteGuard<int, ArrayDeleter> sat_vband(
    TransferOwnership(new int[box_width * (tile_height_ + box_height)]));
  int sat_prevrow = 0;
  khDeleteGuard<unsigned char, ArrayDeleter> border_row(
    TransferOwnership(new unsigned char[tile_width_]));
  memset(&border_row[0], border, tile_width_);

  // In single-tile mode, we compute the output image in place of the
  // input image to save the double memory.
  khDeleteGuard<unsigned char, ArrayDeleter> output_tile_buffer;
  unsigned char *output_tile;
  if (single_tile_mode_) {
    output_tile = cached_tiles_[0];
  } else {
    output_tile_buffer =
      TransferOwnership(new unsigned char[tile_width_ * tile_height_]);
    output_tile = output_tile_buffer;
  }

  // Process output tiles in rows, left to right in each row.
  for (int tile_row = 0; tile_row < num_tiles_y_; ++tile_row) {
    for (int tile_col = 0; tile_col < num_tiles_x_; ++tile_col) {
      // The image tiles start out non-valid until we get past the
      // border or previously-computed values.
      const unsigned char *left_tile = NULL, *right_tile = NULL;
      int max_row_in_tile = box_halfh + 1;  // forces tile load when we get here
      int loaded_tile_row = tile_row - 1;

      // Restore beginning of previous SAT row from vertical band.
      // We will need it when computing the current SAT row.
      int prev_row = tile_row * tile_height_ - 1 + box_height;
      int *prev_row_sat = (&sat_hband[0] + tile_col * tile_width_ +
                           hband_w * (prev_row%hband_h));
      if (tile_col > 0)
        prev_row_sat[box_width - 1] = sat_prevrow;
      // Now prepare the vband for the next tile to the right.
      sat_prevrow = prev_row_sat[box_width - 1 + tile_width_];

      // Compute rows in the SAT.  The data for each row in the SAT is
      // in image row (row - box_height/2), or in the border.  Once we have
      // computed at least box_height rows in this tile, then we can start
      // also computing output tile pixels at (row - box_height/2, col -
      // box_width/2) using the four corner SAT values from the table.
      for (int row = 0; row < tile_height_ + box_height; ++row) {
        // Get pointer to the SAT row and offset to previous
        // row. Modulo arithmetic wraps around the hband vertically.
        const int srow = tile_row * tile_height_ + row + box_height;
        const int sat_prev_row_offset = hband_w * ((srow - 1) % hband_h -
                                                   srow % hband_h);
        // Pointer to the elements in the SAT table we're computing
        int *psat = &sat_hband[0] + tile_col*tile_width_ +
                    hband_w * (srow % hband_h);

        // Load image tiles if needed.  The image data is only loaded
        // when needed to avoid extra reads.
        if (tile_row == 0 || row >= box_height) {
          if (row >= max_row_in_tile) {
            // We have gone beyond the currently-loaded input image
            // tile, or from the border into the image itself, so we
            // need to load new image tiles.
            int skip_tile_rows = (row - max_row_in_tile) / tile_height_ + 1;
            loaded_tile_row += skip_tile_rows;
            max_row_in_tile += skip_tile_rows * tile_height_;
            left_tile = GetImageTile(tile_col, loaded_tile_row, border);
            right_tile = GetImageTile(tile_col + 1, loaded_tile_row, border);
          }
        }

        // The proper tiles are now loaded.  Get pointers to row
        // inside original image tiles corresponding to current SAT
        // row.  When the image data is outside the image, use border
        // values instead.
        const unsigned char *pimg = 0;  // pointer to image data
        const unsigned char *r_tile_row = 0;  // pointer to row in right image tile
        if (row < box_halfh + 1) {
          // Image data is in the border.
          pimg = r_tile_row = border_row;
        } else {
          if (left_tile)
            // Valid left image tile, so use that image data.
            pimg = left_tile + (((row - box_halfh - 1) % tile_height_)
                                * tile_width_);
          else
            // No valid left tile, so border image data.
            pimg = border_row;
          if (right_tile)
            // Valid right image tile, so use that image data.
            r_tile_row = right_tile + (((row-box_halfh-1) % tile_height_)
                                       * tile_width_);
          else
            // No valid right image, so we must be in the border.
            r_tile_row = border_row;
        }

        // SAT rows computed in a single tile are tile_height + box_height.  So
        // when we get to a new row of output tiles, the first box_height
        // rows are either computed from the border or were computed
        // during the previous tile row.  Starting with row box_height we
        // also compute the box-filtered output image.
        if (row < box_height) {
          // Take care of the first box_width elements of SAT row
          // (box_halfw+1 tile padding on left, box_halfw inside image
          // tile).
          if (tile_col == 0) {
            if (tile_row == 0) {
              // Compute first part of top tile row.
              ComputeLeftSATSpan(&psat, border, sat_prev_row_offset,
                                     box_halfw, &pimg);
            } else {  // It is already computed from tile above us.
              psat += box_width;
            }
          } else {  // Need to restore these since tile to left messed them.
            memcpy(psat, &sat_vband[0] + row * box_width,
                   box_width * sizeof(*psat));
            psat += box_width;
            pimg += box_halfw;  // Don't need these to compute SAT.
          }

          // Compute the rest of the row.
          if (tile_row == 0) {
            if (row < box_halfh + 1) {
              // Compute SAT using lower border values.
              ComputeSATSpanValue(&psat, tile_width_, border,
                                  sat_prev_row_offset);
            } else {
              ComputeSATSpanValues(&psat, tile_width_ - box_halfw,
                                   &pimg, sat_prev_row_offset);
              pimg = r_tile_row;
              ComputeSATSpanValues(&psat, box_halfw, &pimg,
                                   sat_prev_row_offset);
            }
          } else {
            psat += tile_width_;
          }
        } else {
          // Handle case where row >= box_height.
          // Take care of the first box_width elements of SAT row.
          if (tile_col == 0) {  // Leftmost tile column: compute.
            ComputeLeftSATSpan(&psat, border, sat_prev_row_offset,
                                   box_halfw, &pimg);
          } else {  // Copy box_width elements from vertical band.
            memcpy(psat, &sat_vband[0] + row * box_width,
                   box_width * sizeof(*psat));
            psat += box_width;
            pimg += box_halfw;  // Don't need these to compute SAT.
          }

          // Now compute the rest of the SAT row.  Filter the input
          // image in place (do it here rather than separate loop in
          // order to improve cache behavior).  Current SAT pointer
          // <psat> will be Bottom Right corner of filter box in
          // original image tile.
          //
          // Get top-left corner of filter box in SAT.
          int *sat_topleft = (&sat_hband[0] + tile_col * tile_width_ +
                              hband_w * ((srow - box_height) % hband_h));
          unsigned char *f = &output_tile[0] + (row - box_height) * tile_width_;
          int *psat_end = psat + tile_width_ - box_halfw;
          // First time through loop does left image tile; second time
          // does right tile
          for (int times = 0; times < 2; ++times) {
            for (; psat < psat_end; ++psat, ++pimg) {
              ComputeSAT(psat, *pimg, sat_prev_row_offset);

              // Use Summed Area Table to box-filter image.
              Filter((*sat_topleft          - *(sat_topleft + box_width)
                      - *(psat - box_width) + *psat),
                     f);
              ++sat_topleft;
              ++f;
            }
            pimg = r_tile_row;
            psat_end += box_halfw;
          }
        }

        // If not last column, copy end of tile row to vertical band.
        if (tile_col != num_tiles_x_ - 1)
          memcpy(&sat_vband[0] + row * box_width, psat - box_width,
                 box_width * sizeof(*psat));
      }  // Row inside tile.

      // Done processing tile so save it
      SaveFilteredTile(tile_col, tile_row, output_tile);
      progress.incrementDone();
    }  // Column of tiles loop
  }  // Row of tiles loop.
}
