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

// Based on an algorithm and initial implementation by emilp

#ifndef _FUSION_TOOLS_TILEDFLOODFILL_H__
#define _FUSION_TOOLS_TILEDFLOODFILL_H__

// Tiled Flood Fill
//
// Flood fill when the entire image can't be loaded into memory at
// once.  Pixels are flooded when they are within a tolerance of input
// fill values.  Flooding happens from pixel to pixel vertically and
// horizontally, but not diagonally.
//
// The fill algorithm also finds and starts a seed in holes of a given
// diameter or larger.  A hole is defined as a vertical OR horizontal
// line of pixels of at least the diameter in length; it then floods
// into any connected pixels.
//
// This implementation is designed to work well for large fill areas
// that are almost convex. Tiles are loaded one at a time, flooded as
// much as possible within the tile, and boundary information is
// stored until the next tile is loaded.  It probably won't be as
// efficient for lots of small winding flood fill channels that weave
// their way across tile boundaries (though it should produce a
// correct result).

// TODO: remove requirement on adding seeds in a particular order

#include <deque>
#include <cstdint>
#include <stack>
#include <vector>

#include <qdatetime.h>


// A Span is a horizontal segment of len pixels to flood into. It is
// used to keep track of future flooding locations.
class Span {
 public:
  int row_in_image, col_in_tile, len;
  Span() {}
  Span(int r, int c, int l) : row_in_image(r), col_in_tile(c), len(l) {}
  ~Span() {}
};

// ExaminableStack is a stack subclass that allows us to examine all the
// elements in the stack (read-only) to allow progress tracking in this code.
template<class T>
class ExaminableStack : public std::stack<T, std::deque<T> > {
 public:
  const std::deque<T> &GetContainer() const {
    return this->c;
  }
};


// TiledFloodFill computes a flood fill of the image as described at
// the top of this file.  Code to use it must derive a class from
// TiledFloodFill that defines the LoadImageTile, LoadMaskTile, and
// SaveMaskTile functions to load and save data.  Sample use of the
// derived class MyTiledFloodFill:
// MyTiledFloodFill flood(image_width, image_ht, tile_width, tile_ht,
//                        tolerance, min_hole_dia);
// flood.AddFillValue(0);
// flood.AddFillValue(1);
// flood.AddSeed(0, image_ht - 1);  // seed at top left of image
// flood.AddSeed(0, 0);  // seed at lower left of image
// tell MyTiledFloodFill where to get and save tiles
// flood.FloodFill();
//
class TiledFloodFill {
 public:
  TiledFloodFill(int image_width, int image_height,
                 int tile_width, int tile_height,
                 int tolerance, int min_hole_dia);
  virtual ~TiledFloodFill();

  // Adds a fill value.  A pixel in the image may be filled if is
  // within tolerance_ of a fill value.
  void AddFillValue(unsigned char fv);

  // Adds a seed point to flood from.  Seeds must be added in
  // decreasing y order.
  void AddSeed(int x, int y);

  // Perform the flood fill, reading and writing tiles using the
  // methods provided by the derived class.
  void FloodFill();

  static const int kNoFillValueSpecified;  // value to represent no value
                                           // specified.
 protected:
  // Gets a pointer to the image for a tile. Caller will not free.
  virtual const unsigned char *LoadImageTile(int tx, int ty) = 0;

  // Gets a pointer to tile mask data. Should be initially all
  // kNotFilled.  Caller will not free the image.  old_opacity should
  // be kNotFilled the first time the tile is loaded, and then the
  // value from SaveMaskTile after that. opacity is the average value
  // of the mask tile.
  virtual unsigned char *LoadMaskTile(int tx, int ty, double *old_opacity) = 0;

  // Saves data (supplied in LoadMaskTile) to store.  See LoadMaskTile
  // for opacity.
  virtual void SaveMaskTile(int tx, int ty, double opacity) = 0;

  int image_width_, image_height_;  // full image size
  int tile_width_, tile_height_;             // size of each tile
  int num_tiles_x_, num_tiles_y_;   // number of tiles in each direction

  // These must be defined in your derived class .cpp file, for example:
  // const unsigned char TiledFloodFill::kNotFilled = 0;
  // const unsigned char TiledFloodFill::kFilled = 255;
  static const unsigned char kFilled;     // values to represent filled pixels
  static const unsigned char kNotFilled;  // and unfilled pixels

 private:
  // FillOk returns true iff the unsigned char should be filled, i.e. whether
  // it's within tolerance_ of a fill value.
  inline bool FillOk(unsigned char u) const;

  // A direction of sweeping, used for both x and y.
  enum Direction {DIRECTION_MINUS = 0, DIRECTION_PLUS, NUM_DIRECTIONS};
  inline int DirToDelta(Direction d) {return -1 + 2 * static_cast<int>(d);}
  inline Direction ReverseDirection(Direction d) {
    return (d == DIRECTION_PLUS ? DIRECTION_MINUS : DIRECTION_PLUS);
  }

  // ProcessTile floods within a single tile, in the given directions
  // of overall sweeping. It recurses on tiles to the left and right
  // as needed.  vert_sides is used to propagate information between
  // tiles within the row and should be blank when first called: see
  // definition for details.
  void ProcessTile(int tile_col, int tile_row,
                   Direction x_direction, Direction y_direction,
                   std::vector<std::vector<unsigned char> > *vert_sides);

  // ProcessSpan floods a single span, adding Spans to tile_col_todo_
  // as needed. y_direction is the direction of sweep (up or down) in
  // the tile.  span_len=0 results in propagation both up and down.
  // Returns the number of pixels filled.
  int ProcessSpan(int global_row, int tile_col,
                  const unsigned char *image_row_beg, const unsigned char *image_row_end,
                  unsigned char *fill_row, Direction y_direction,
                  int span_beg_offset_in_row, int span_len);

  int tolerance_;     // see FillOk() for description
  int min_hole_dia_;  // see file comments above

  // A lookup table for fast determination of valid fill
  // values.  lookup[uchar] is true iff we should fill the uchar.
  bool fill_value_lookup_[256];

  // Spans to be flooded, organized by direction (up and down), then
  // by tile column, and then as a stack in increasing/decreasing row
  // order so that it can be popped in row order when sweeping in the
  // direction. A span with length 0 is propagated in both directions.
  std::vector<ExaminableStack<Span> > tile_col_to_do_[2];

  // The following member variables support detecting holes by
  // counting horizontal and vertical runs of empty pixels. We only
  // search for holes during the first vertical sweep over the image,
  // and during that sweep this value is true.
  bool compute_holes_;

  // For each pixel column in the full image, this array keeps a count
  // of how many pixels vertically have been fillable values.  If any
  // column reaches min_hole_dia_, then we seed a flood there.
  std::vector<int> v_empty_pix_run_;

  // Between each pair of tiles in the current tile row, for each
  // pixel row in the current row of tiles, keep track of the number
  // of fillable pixels in a horizontal row next to that edge of the
  // tile. h_empty_pix_run_[2] is the boundary between tiles 1 and 2.
  // Since each tile is only searched for holes once, we only need a
  // single count between each pair of tiles.
  std::vector<std::vector<int> > h_empty_pix_run_;

  // For each tile in the current row, this vector holds whether we
  // have searched for holes in it.
  std::vector<bool> visited_;

  // Disallow copying: do not define these
  TiledFloodFill(const TiledFloodFill &);
  TiledFloodFill& operator=(const TiledFloodFill &);

  // Starts progress metering. We can't use a khProgressMeter because we don't
  // know how many times we'll have to visit each tile.
  void ProgressStartFlooding();
  // TileDone() increments tiles finished and reports progress if needed.
  void ProgressTileDone(int tile_row);
  // Variables in support of progress metering.
  int tiles_processed_;           // total tiles processed.
  QTime timer_;                   // a timer for the flood filling.
  int ms_elapsed_reported_;       // elapsed milliseconds reported so far.

  // Returns the number of tiles referenced in the ToDo lists in both
  // directions.  Used to report progress.
  int CountTilesToDo() const;
};


#endif  // _FUSION_TOOLS_TILEDFLOODFILL_H__
