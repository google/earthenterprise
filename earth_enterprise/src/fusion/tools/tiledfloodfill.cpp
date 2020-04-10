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


// The basic approach is to sweep up and down the overall image until
// flooding stops.  A row of tiles is processed, reloading the tiles
// in the row as many times as needed, until no more flooding can be
// accomplished between the tiles in the row.  Then the algorithm
// moves to the next row of tiles.  Processing a row is done lazily,
// only loading a tile when it has propagation into it.  Flooding
// within a single tile is similarly done in sweeps up and down until
// there is no more propagation within the tile.  A stack of
// propagation spans is kept for each direction (up/down) in each
// column of tiles.
//
// Hole detection is done by dynamic programming.  The first time to
// sweep across all the tiles, the algorithm makes sure all tiles get
// visited.  A horizontal line of vertical-length counts gets swept
// down the entire image.  In addition, vertical lines of
// horizontal-length counts get propagated left and right.  The
// algorithm assumes that the hole diameter is smaller than the width
// of a tile - otherwise the horizontal hole length calc might fail to
// detect because it loads and searches the tiles in a row in unknown
// order.

#include "tiledfloodfill.h"

#include <limits>
#include <stack>
#include <vector>

#include "khProgressMeter.h"
#include "notify.h"

using std::vector;
using std::stack;


// If a fill value is not specified, we use the values at the corners of the
// image as fill values.  The following is a flag value to know that a fill
// value hasn't been specified.
const int TiledFloodFill::kNoFillValueSpecified =
  std::numeric_limits<int>::min();

// The filled areas will be masked out, so kFilled = 0 to form the
// alpha mask.
const unsigned char TiledFloodFill::kNotFilled = 255;
const unsigned char TiledFloodFill::kFilled = 0;

// The number of seconds between progress reports for long computations.
const int PROGRESS_REPORT_INTERVAL_SECONDS = 30;

TiledFloodFill::TiledFloodFill(int image_width, int image_height,
                               int tile_width, int tile_height,
                               int tolerance, int min_hole_dia)
    : image_width_(image_width), image_height_(image_height),
      tile_width_(tile_width), tile_height_(tile_height),
      num_tiles_x_((image_width + tile_width - 1) / tile_width),
      num_tiles_y_((image_height + tile_height - 1) / tile_height),
      tolerance_(tolerance),
      min_hole_dia_(min_hole_dia) {
  tile_col_to_do_[0].resize(num_tiles_x_);
  tile_col_to_do_[1].resize(num_tiles_x_);

  // Check params for validity.
  if (tile_width_ < 2 || tile_height_ < 2 ||
      tolerance_ < 0)
    notify(NFY_FATAL, "TiledFloodFill: Bad inputs");
  // Horizontal run length propagation algorithm cannot detect holes
  // wider than a tile.
  if (min_hole_dia_ > tile_width_)
    notify(NFY_FATAL, "TiledFloodFill: Min hole dia must be <= tile width");
  for (int i = 0; i < 256; ++i)
    fill_value_lookup_[i] = false;
}

TiledFloodFill::~TiledFloodFill() {}


void TiledFloodFill::AddSeed(int x, int y) {
  if (x < 0 || x > image_width_ ||
      y < 0 || y > image_height_)
    notify(NFY_FATAL, "TiledFloodFill: seed outside image");
  stack<Span> &seed_to_do_list = tile_col_to_do_[DIRECTION_PLUS][x/tile_width_];
  if (!seed_to_do_list.empty() && seed_to_do_list.top().row_in_image < y)
    notify(NFY_FATAL,
           "TiledFloodFill: seeds must be added in decreasing y order");
  seed_to_do_list.push(Span(y, x % tile_width_,
                            0));  // 0-len: propagates both dirs
}


void TiledFloodFill::AddFillValue(unsigned char fv) {
  for (int i = std::max(0, fv - tolerance_);
       i <= std::min(255, fv + tolerance_); ++i)
    fill_value_lookup_[i] = true;
}

inline bool TiledFloodFill::FillOk(unsigned char u) const {
  return fill_value_lookup_[u];
}


// See top of file comment for basic operation of TiledFloodFill.
void TiledFloodFill::FloodFill() {
  Direction y_direction = DIRECTION_PLUS;

  compute_holes_ = min_hole_dia_ > 0;
  v_empty_pix_run_.resize(image_width_, 0);

  ProgressStartFlooding();

  // Sweep up and down rows of tiles propagating the flood in each
  // tile row until the flooding stops.
  bool need_work;
  do {
    for (int tile_row = (y_direction == DIRECTION_PLUS ? 0 : (num_tiles_y_-1));
         tile_row >= 0 && tile_row < num_tiles_y_;
         tile_row += DirToDelta(y_direction)) {
      // Mark all tiles in row as not visited.
      visited_.assign(num_tiles_x_, false);

      // Clear horizontal run counts for hole detection.
      h_empty_pix_run_.assign(num_tiles_x_ + 1, vector<int>());

      // While we have flood spans to propagate, do it.
      Span s;
      for (int col = 0; col < num_tiles_x_; ++col) {
        stack<Span> &col_todo = tile_col_to_do_[y_direction][col];
        if (!col_todo.empty() &&
            col_todo.top().row_in_image >= tile_height_ * tile_row &&
            col_todo.top().row_in_image < tile_height_ * (tile_row + 1)) {
          // The following is used to pass flood-fill info
          // horizontally between tiles.
          vector<std::vector<unsigned char> > tile_vert_sides;
          ProcessTile(col, tile_row,
                      DIRECTION_PLUS,  // x_direction
                      y_direction, &tile_vert_sides);
        }
      }

      if (compute_holes_)
        // Do hole detection for all unvisited tiles, left to right.
        for (int col = 0; col < num_tiles_x_; ++col)
          if (!visited_[col]) {
            vector<std::vector<unsigned char> > tile_vert_sides;
            ProcessTile(col, tile_row, DIRECTION_PLUS, y_direction,
                        &tile_vert_sides);
          }
    }

    // Clear stacks for current direction and see if opposite one needs work.
    need_work = false;
    for (int col = 0; col < num_tiles_x_; ++col) {
      // Discard propagation in this direction past the last row.
      while (!tile_col_to_do_[y_direction][col].empty())
        tile_col_to_do_[y_direction][col].pop();
      if (!tile_col_to_do_[ReverseDirection(y_direction)][col].empty())
        need_work = true;
    }

    compute_holes_ = false;  // Only need 1 sweep to detect holes.
    y_direction = ReverseDirection(y_direction);
  } while (need_work);
}

void TiledFloodFill::ProcessTile(int tile_col, int tile_row,
                                 Direction x_direction, Direction y_direction,
                                 std::vector<std::vector<unsigned char> > *vert_sides) {
  // Store horizontal-propagation info.  The sides of tile i are
  // vert_sides[i] and vert_sides[i+1].  Each value is set to kFilled
  // if a pixel has been set on either side of the tile border during
  // this tile row of propagation.
  (*vert_sides).resize(num_tiles_x_ + 1, vector<unsigned char>());

  do {
    notify(NFY_VERBOSE, "flood fill tile %d, %d, dir %d",
           tile_col, tile_row, DirToDelta(y_direction));

    // Load tile with image data & previous fill data.
    const unsigned char *tile = LoadImageTile(tile_col, tile_row);
    double old_opacity;
    unsigned char *alpha = LoadMaskTile(tile_col, tile_row, &old_opacity);

    // Get or create side bars for propagation in/out.
    // side_bar bit is set if pixel on either side of border has
    // been set during propagation in this row of tiles.
    vector<unsigned char> *side_bar[2] = {&(*vert_sides)[tile_col],
                                  &(*vert_sides)[tile_col + 1]};
    side_bar[0]->resize(tile_height_, kNotFilled);
    side_bar[1]->resize(tile_height_, kNotFilled);

    // Save old sides so we know what's changed.
    int last_row = std::min(tile_height_,
                            image_height_ - tile_row * tile_height_);
    int row_len = std::min(tile_width_, image_width_ - tile_col * tile_width_);
    int sidecols[2] = {0, row_len - 1};
    vector<unsigned char> old_sides[2];
    for (int side = 0; side < 2;  ++side) {
      old_sides[side].resize(tile_height_);
      for (int row = 0; row < last_row; ++row)
        old_sides[side][row] = alpha[sidecols[side] + row * tile_width_];
    }

    // Alternate upwards and downwards sweeps inside the tile.  During
    // the first sweep, check for holes and also check propagation
    // into this tile from side tiles.
    bool first_sweep = true;
    Direction tile_y_direction = y_direction;
    stack<Span> *to_do = &tile_col_to_do_[tile_y_direction][tile_col];
    int filled_pixels = 0;
    do {
      for (int row = (tile_y_direction == DIRECTION_PLUS ? 0 : last_row - 1);
          row >= 0 && row < last_row;
          row += DirToDelta(tile_y_direction)) {
        // If not the first sweep, we can jump the row ahead to the
        // next todo item.
        if (!first_sweep && !to_do->empty() &&
            to_do->top().row_in_image >= tile_height_ * tile_row &&
            to_do->top().row_in_image < tile_height_ * tile_row + last_row)
          row = to_do->top().row_in_image % tile_height_;

        int global_row = row + tile_row * tile_height_;
        const unsigned char *row_beg = tile + row * tile_width_;
        const unsigned char *row_end = row_beg + row_len;
        unsigned char *fill_row = alpha + row * tile_width_;

        // Propagate spans on todo list for this row.  Must pop row
        // into temporary because ProcessSpan pushes next row.
        vector<Span> temp_to_do;
        while (!to_do->empty() && to_do->top().row_in_image == global_row) {
          temp_to_do.push_back(to_do->top());
          to_do->pop();
        }
        for (unsigned int i = 0; i < temp_to_do.size(); ++i)
          filled_pixels += ProcessSpan(global_row, tile_col,
                                       row_beg, row_end, fill_row,
                                       tile_y_direction,
                                       temp_to_do[i].col_in_tile,
                                       temp_to_do[i].len);

        if (first_sweep) {
          // Check row endpoints for propagation from side neighbors.
          for (int side = 0; side < 2; ++side) {
            if ((*side_bar[side])[row] == kFilled &&
                fill_row[sidecols[side]] != kFilled)
              filled_pixels += ProcessSpan(global_row, tile_col,
                                           row_beg, row_end, fill_row,
                                           tile_y_direction,
                                           sidecols[side], 0);
          }

          // If filling holes, update empty run counts, both
          // horizontal and vertical.
          if (compute_holes_ && !visited_[tile_col]) {
            // Get or create side counts of runs of empty pixels.
            vector<int> *h_counts_left = &h_empty_pix_run_[tile_col],
                       *h_counts_right = &h_empty_pix_run_[tile_col + 1];
            h_counts_left->resize(last_row, 0);
            h_counts_right->resize(last_row, 0);

            // We count the horizontal run lengths using the counter
            // for the left side to auto-update it until the first run
            // break; after that we use mid_row_h_count.
            int *h_count = &(*h_counts_left)[row];
            int mid_row_h_count = 0;
            int *v_count = &v_empty_pix_run_[tile_col * tile_width_];
            for (int i = 0; i < row_len; ++i, ++v_count) {
              if (fill_row[i] == kFilled || !FillOk(row_beg[i])) {
                h_count = &mid_row_h_count;
                *h_count = 0;
                *v_count = 0;
              } else {
                ++*h_count;
                ++*v_count;
                if (i == row_len - 1)
                  *h_count += (*h_counts_right)[row];
                // If trigger hole detection, expand and propagate +/- y.
                if (*h_count >= min_hole_dia_ || *v_count >= min_hole_dia_) {
                  filled_pixels += ProcessSpan(global_row, tile_col,
                                               row_beg, row_end, fill_row,
                                               tile_y_direction, i, 0);
                }
              }
            }
            // Save empty run counts for last column.
            (*h_counts_right)[row] = *h_count;
          }
        }
      }

      // We're done with this direction of sweeping. Don't detect
      // holes or flood into this tile from the sides. Sweep other
      // direction if there are spans to do.
      first_sweep = false;
      visited_[tile_col] = true;
      tile_y_direction = ReverseDirection(tile_y_direction);
      to_do = &tile_col_to_do_[tile_y_direction][tile_col];
    } while (!to_do->empty() &&
             to_do->top().row_in_image >= tile_height_ * tile_row &&
             to_do->top().row_in_image < tile_height_ * (tile_row + 1));

    // We're done processing the tile, so save it if changed.
    if (filled_pixels > 0) {
      double r = static_cast<double>(filled_pixels) / (row_len * last_row);
      SaveMaskTile(tile_col, tile_row,
                   old_opacity + r * (static_cast<int>(kFilled) -
                                      static_cast<int>(kNotFilled)));
    }

    // Check row ends for propagation to side neighbors. Place results
    // in side bars to propagate them.
    bool recurse_side[2] = {false, false};
    for (int row = 0; row < last_row; ++row)
      for (int side = 0; side < 2; ++side) {
        if (old_sides[side][row] != kFilled &&
            (alpha + row * tile_width_)[sidecols[side]] == kFilled &&
            (*side_bar[side])[row] != kFilled) {
          (*side_bar[side])[row] = kFilled;
          recurse_side[side] = true;
        }
      }

    ProgressTileDone(tile_row);

    // Recurse to modified tiles as needed.
    notify(NFY_VERBOSE, "recurse sides: %s, %s",
           (recurse_side[0] ? "left" : ""),
           (recurse_side[1] ? "right": ""));
    if (!recurse_side[0] && !recurse_side[1]) {
      // We're done.
      return;
    } else if (recurse_side[0] && recurse_side[1]) {
      // Recurse first on neighbor from which we came (likely shorter).
      // Tail recursion handles the other side.
      int newcol = tile_col - DirToDelta(x_direction);
      if (newcol >= 0 && newcol < num_tiles_x_)
        ProcessTile(tile_col - DirToDelta(x_direction), tile_row,
                    ReverseDirection(x_direction), y_direction, vert_sides);
    } else if (recurse_side[0]) {
      // Let tail recursion proceed to tile on the left.
      x_direction = DIRECTION_MINUS;
    } else {
      // Let tail recursion proceed to tile on the right.
      x_direction = DIRECTION_PLUS;
    }
    tile_col += DirToDelta(x_direction);
  } while (tile_col >= 0 && tile_col < num_tiles_x_);
}


int TiledFloodFill::ProcessSpan(int global_row, int tile_col,
                                const unsigned char *row_beg, const unsigned char *row_end,
                                unsigned char *fill_row,
                                Direction y_direction,
                                int span_beg, int span_len) {
  const int next_row = global_row + DirToDelta(y_direction);
  const int prev_row = global_row + DirToDelta(ReverseDirection(y_direction));

  stack<Span> &to_do = tile_col_to_do_[y_direction][tile_col],
          &rev_to_do = tile_col_to_do_[ReverseDirection(y_direction)][tile_col];
  const unsigned char * const left = row_beg + span_beg;
  const unsigned char * const right = left + span_len;
  int filled_count = 0;

  if (!to_do.empty() &&
      (next_row - to_do.top().row_in_image) * DirToDelta(y_direction) > 0)
    notify(NFY_FATAL, "TiledFloodFill: to_do list not in order");
  if (!rev_to_do.empty() &&
      (prev_row - rev_to_do.top().row_in_image) * DirToDelta(y_direction) < 0)
    notify(NFY_FATAL, "TiledFloodFill: reverse to_do list not in order");

  // If the first pixel is valid, expand left.
  const unsigned char *exp_left = left;
  unsigned char *fill = fill_row + span_beg;
  if ((*fill != kFilled) && FillOk(*exp_left)) {
    for (--exp_left, --fill;
         exp_left >= row_beg && (*fill != kFilled) && FillOk(*exp_left);
         --exp_left, --fill)
      *fill = kFilled;
    ++exp_left;
  }
  // At this point exp_left is min(leftmost valid pixel, left).

  // Flood fill the middle. Also update To_Do list for next row.
  const unsigned char *exp_right = left;
  const unsigned char *last = exp_left;
  for (fill = fill_row + span_beg; exp_right < right; ++fill, ++exp_right)
    if ((*fill != kFilled) && FillOk(*exp_right)) {
      *fill = kFilled;
    } else {
      if (last < exp_right) {
        // Pass the span (last-exp_right) to the next row.
        to_do.push(Span(next_row, last - row_beg, exp_right - last));
        filled_count += exp_right - last;
      }
      last = exp_right + 1;
    }

  // If we filled the rightmost pixel, expand right.
  if (span_len == 0 || last < exp_right) {
    for (; exp_right < row_end && (*fill != kFilled) && FillOk(*exp_right);
         ++exp_right, ++fill)
      *fill = kFilled;
    if (last < exp_right) {
      to_do.push(Span(next_row, last - row_beg, exp_right - last));
      filled_count += exp_right - last;
    }
  }

  // Above code handles propagation in the sweep direction. If we
  // expanded, we need to propagate in the reverse direction as well.
  if (span_len == 0) {
    if (exp_left < exp_right)
      rev_to_do.push(Span(prev_row, exp_left - row_beg, exp_right - exp_left));
  } else {
    if (exp_right > right)
      rev_to_do.push(Span(prev_row, right - row_beg, exp_right - right));
    if (exp_left < left)
      rev_to_do.push(Span(prev_row, exp_left - row_beg, left - exp_left));
  }
  return filled_count;
}


void TiledFloodFill::ProgressStartFlooding() {
  tiles_processed_ = 0;
  timer_.start();
  ms_elapsed_reported_ = 0;
}

void TiledFloodFill::ProgressTileDone(int tile_row) {
  // Report status to user, including estimated number of tiles left, as
  // long as tiles remain and no more often than once per minute.
  ++tiles_processed_;
  int tiles_to_process = CountTilesToDo();
  if (compute_holes_)
    tiles_to_process = std::max(num_tiles_x_ * (num_tiles_y_ -
                                                tile_row - 1),
                                tiles_to_process);
  int ms_elapsed = std::max(1, timer_.elapsed());  // milliseconds
  if (tiles_to_process == 0 ||
      ms_elapsed > (ms_elapsed_reported_ +
                    PROGRESS_REPORT_INTERVAL_SECONDS * 1000)) {
    float tiles_per_ms = (static_cast<float>(tiles_processed_) /
                          ms_elapsed);
    std::int64_t ms_remaining = static_cast<std::int64_t>(tiles_to_process /
                                            tiles_per_ms);
    if (tiles_to_process > 0) {
      fprintf(stderr, "Flooded %d tiles (%.1f/sec),"
              " at least %d tiles remain (time %s)\n",
              tiles_processed_, tiles_per_ms * 1000,
              tiles_to_process,
              khProgressMeter::msToString(ms_remaining).latin1());
    } else {
      fprintf(stderr, "Flooded %d tiles (%.1f/sec) in %s\n",
              tiles_processed_, tiles_per_ms * 1000,
              khProgressMeter::msToString(ms_elapsed).latin1());
    }
    ms_elapsed_reported_ = ms_elapsed;
  }
}


// CountTilesToDo loops over all spans in all the column to-do lists and counts
// unique tiles.  Due to the way the to-do lists are created and emptied, a
// single tile should never appear in both directions.
int TiledFloodFill::CountTilesToDo() const {
  int tiles_to_do = 0;
  for (int direction = 0; direction < NUM_DIRECTIONS; ++direction)
    for (int tile_col = 0; tile_col < num_tiles_x_; ++tile_col) {
      const ExaminableStack<Span>::container_type &col_to_do =
        tile_col_to_do_[direction][tile_col].GetContainer();
      int last_tile_row = -1;
      ExaminableStack<Span>::container_type::const_iterator span;
      for (span = col_to_do.begin(); span != col_to_do.end(); ++span) {
        int tile_row = span->row_in_image % tile_height_;
        if (tile_row >= 0 && span->row_in_image < image_height_ &&
            tile_row != last_tile_row) {
          ++tiles_to_do;
          last_tile_row = tile_row;
        }
      }
    }
  return tiles_to_do;
}
