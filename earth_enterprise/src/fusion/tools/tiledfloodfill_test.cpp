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


#include <stdlib.h>

#include <vector>
#include <iostream>
#include <sstream>

#include "notify.h"
#include "tiledfloodfill.h"
#include <fusion/tools/in_memory_floodfill.h>

#include "UnitTest.h"

using std::vector;
const uchar kUnmaskedValue = 255;

// Class to hold test image and output for TiledFloodFill.
class SimpleTiledFloodFill : public TiledFloodFill {
 public:
  SimpleTiledFloodFill(int image_width, int image_height,
                       int tile_width, int tile_height,
                       int tolerance, int minHoleDiam,
                       const uchar *image)
      : TiledFloodFill(image_width, image_height, tile_width, tile_height,
                       tolerance, minHoleDiam),
        image_(image, image + image_width * image_height),
        mask_(image_width * image_height, kNotFilled),
        opacity_(num_tiles_x_ * num_tiles_y_, kNotFilled),
        image_tile_(tile_width * tile_height, 0),
        mask_tile_(tile_width * tile_height, 0) {
  }
  virtual ~SimpleTiledFloodFill() {}

  const uchar *get_mask() const {return &mask_[0];}
  void FillSeedFlood(int fill_v, int seed_x, int seed_y) {
    AddFillValue(fill_v);
    AddSeed(seed_x, seed_y);
    FloodFill();
  }

  virtual const uchar *LoadImageTile(int tx, int ty) {
    image_tile_.assign(tile_width_ * tile_height_, 0);
    for (int x = 0;
         x < tile_width_ && tx*tile_width_ + x < image_width_; ++x)
      for (int y = 0;
           y < tile_height_ && ty*tile_height_ + y < image_height_; ++y)
        image_tile_.at(y*tile_width_ + x) =
          image_.at(ty*tile_height_*image_width_ + y*image_width_
                    + tx*tile_width_ + x);
    return &image_tile_[0];
  }

  virtual uchar *LoadMaskTile(int tx, int ty, double *old_opacity) {
    mask_tile_.assign(tile_width_ * tile_height_, kNotFilled);
    for (int x = 0;
         x < tile_width_ && tx*tile_width_ + x < image_width_; ++x)
      for (int y = 0;
           y < tile_height_ && ty*tile_height_ + y < image_height_; ++y)
        mask_tile_.at(y*tile_width_ + x) =
          mask_.at(ty*tile_height_*image_width_ + y*image_width_
                   + tx*tile_width_ + x);
    *old_opacity = opacity_.at(ty * num_tiles_x_ + tx);
    return &mask_tile_[0];
  }

  virtual void SaveMaskTile(int tx, int ty, double opacity) {
    for (int x = 0;
         x < tile_width_ && tx*tile_width_ + x < image_width_; ++x)
      for (int y = 0;
           y < tile_height_ && ty*tile_height_ + y < image_height_; ++y)
        mask_.at(ty*tile_height_*image_width_ + y*image_width_
                 + tx*tile_width_ + x) =
          mask_tile_.at(y*tile_width_ + x);
    opacity_.at(ty * num_tiles_x_ + tx) = opacity;
  }


  vector<uchar> image_;       // the full-size image
  vector<uchar> mask_;        // the flooded result
  vector<double> opacity_;    // opacities for all tiles

  vector<uchar> image_tile_;  // buffer to pass image tiles to alg
  vector<uchar> mask_tile_;   // buffer to pass mask tiles to alg
};

// Here is the unit test code for TiledFloodFill
class TiledFloodFillUnitTest : public UnitTest<TiledFloodFillUnitTest> {
 public:
  TiledFloodFillUnitTest() : BaseClass("TiledFloodFillUnitTest") {
    REGISTER(FloodFillGeometricTest);
    REGISTER(SimpleImageStorageTest);
    REGISTER(SimpleMaskStorageTest);
    REGISTER(SimpleEdgeTest);
    REGISTER(NoFillTest);
    REGISTER(FloodFillTestBasic);
    REGISTER(FloodFillTestTolerance);
    REGISTER(FloodFillTestRandom);
  }

  // Convert a Mask Value (0 or 255) to an expected mask value (0 or 1).
  // This is to simplify expected result specification.
  uchar MaskValueToExpectedMaskValue(uchar mask_value) {
    return (mask_value == kUnmaskedValue ? 1 : 0);
  }

  // Helper function for debugging
  void printArray(const uchar *a, const int sizex, const int sizey) {
    for (int y = 0; y < sizey; ++y) {
      for (int x = 0; x < sizex; ++x)
        fprintf(stderr, "%4d", a[y*sizex + x]);
      fprintf(stderr, "\n");
    }
  }
  // Helper function for debugging
  // Takes a mask array of values 0,255 and prints out the array as a
  // binary array 0,1
  void printMaskArray(const uchar *a, const int sizex, const int sizey) {
    for (int y = 0; y < sizey; ++y) {
      for (int x = 0; x < sizex; ++x)
        fprintf(stderr, "%4d", MaskValueToExpectedMaskValue(a[y*sizex + x]));
      fprintf(stderr, "\n");
    }
  }

  // Utility to check a mask array against a vector of expected values.
  // The mask_array values are either 0 or 255 (kUnmaskedValue) while
  // the expected_result values are either 0 or 1.
  // mask_array: the array of mask values (value is either 0 or 255)
  // image_width: width of the mask array
  // height: height of the mask array
  // expected_array: the vector of expected values (value is either 0 or 1)
  // message: the failure message to be printed if the mask does not match
  //          the expected.
  // tile_width: the tile_width for the test (used for output on error)
  // tile_height: the tile_height for the test (used for output on error)
  // hole_dia: the hole_dia for the test (used for output on error)
  // return: true if the mask matches the expected, false otherwise.
  bool CheckMaskResult(const uchar* mask_array,
                       uint image_width, uint image_height,
                       const uchar* expected_array,
                       const std::string& message,
                       uint tile_width, uint tile_height, uint hole_dia) {
    // Scale the mask values (either 255 or 0) so that unmasked pixels are 1
    // to match the image_data for simple comparison of the correct result.
    uint pixel_count = image_width * image_height;
    for(uint i = 0; i < pixel_count; ++i) {
      if (MaskValueToExpectedMaskValue(mask_array[i]) != expected_array[i]) {
        fprintf(stderr,
                "FAIL in %s\n"
                "  tile_width %d tile_height %d hole_dia %d\n",
                message.c_str(), tile_width, tile_height, hole_dia);
        fprintf(stderr, "  Result:\n");
        printMaskArray(mask_array, image_width, image_height);
        fprintf(stderr, "  Expected:\n");
        printArray(expected_array, image_width, image_height);
        return false;
      }
    }
    return true;
  }

  // tests for SimpleTiledFloodFill first
  bool SimpleImageStorageTest() {
    uchar img_data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    SimpleTiledFloodFill t(4, 4, 2, 2, 0, 0, img_data);

    const uchar *tile = t.LoadImageTile(0, 0);
    uchar expect1[] = {1, 2, 5, 6};
    EXPECT_TRUE((vector<uchar>(tile, tile+4)) ==
                (vector<uchar>(expect1, expect1+4)));

    tile = t.LoadImageTile(1, 0);
    uchar expect2[] = {3, 4, 7, 8};
    EXPECT_TRUE((vector<uchar>(tile, tile+4)) ==
                (vector<uchar>(expect2, expect2+4)));

    tile = t.LoadImageTile(0, 1);
    uchar expect3[] = {9, 10, 13, 14};
    EXPECT_TRUE((vector<uchar>(tile, tile+4)) ==
                (vector<uchar>(expect3, expect3+4)));

    tile = t.LoadImageTile(1, 1);
    uchar expect4[] = {11, 12, 15, 16};
    EXPECT_TRUE((vector<uchar>(tile, tile+4)) ==
                (vector<uchar>(expect4, expect4+4)));

    return true;
  }

  bool SimpleMaskStorageTest() {
    uchar img_data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    SimpleTiledFloodFill t(4, 4, 2, 2, 0, 0, img_data);

    uchar tile_data[] = {1, 2, 5, 7};
    double opacity = 15;
    uchar *mask = t.LoadMaskTile(0, 1, &opacity);
    for (int i = 0; i < 4; ++i)
      mask[i] = tile_data[i];
    opacity = 17;
    t.SaveMaskTile(0, 1, opacity);
    mask[3] = 1;
    opacity = 10;
    mask = t.LoadMaskTile(0, 1, &opacity);
    EXPECT_TRUE(vector<uchar>(mask, mask+4) ==
                vector<uchar>(tile_data, tile_data+4));
    EXPECT_EQ(opacity, 17);

    return true;
  }

  bool SimpleEdgeTest() {
    uchar image_data[] = {1, 2, 3, 4, 5,
                          6, 7, 8, 9, 10,
                          11, 12, 13, 14, 15,
                          16, 17, 18, 19, 20,
                          21, 22, 23, 24, 25};
    SimpleTiledFloodFill t(5, 5, 2, 2, 0, 0, image_data);

    const uchar *tile = t.LoadImageTile(0, 0);
    uchar expect1[] = {1, 2, 6, 7};
    EXPECT_TRUE((vector<uchar>(tile, tile+4)) ==
                (vector<uchar>(expect1, expect1+4)));

    tile = t.LoadImageTile(1, 0);
    uchar expect2[] = {3, 4, 8, 9};
    EXPECT_TRUE((vector<uchar>(tile, tile+4)) ==
                (vector<uchar>(expect2, expect2+4)));

    tile = t.LoadImageTile(0, 1);
    uchar expect3[] = {11, 12, 16, 17};
    EXPECT_TRUE((vector<uchar>(tile, tile+4)) ==
                (vector<uchar>(expect3, expect3+4)));

    tile = t.LoadImageTile(2, 0);
    uchar expect4[] = {5, 0, 10, 0};
    EXPECT_TRUE((vector<uchar>(tile, tile+4)) ==
                (vector<uchar>(expect4, expect4+4)));

    tile = t.LoadImageTile(2, 1);
    uchar expect5[] = {15, 0, 20, 0};
    EXPECT_TRUE((vector<uchar>(tile, tile+4)) ==
                (vector<uchar>(expect5, expect5+4)));

    tile = t.LoadImageTile(2, 2);
    uchar expect6[] = {25, 0, 0, 0};
    EXPECT_TRUE((vector<uchar>(tile, tile+4)) ==
                (vector<uchar>(expect6, expect6+4)));

    tile = t.LoadImageTile(0, 2);
    uchar expect7[] = {21, 22, 0, 0};
    EXPECT_TRUE((vector<uchar>(tile, tile+4)) ==
                (vector<uchar>(expect7, expect7+4)));

    return true;
  }

  // Now actual tests of TiledFloodFill

  // Shouldn't flood at all
  bool NoFillTest() {
    uchar image_data[] = {1,   2,  3,  4,  5,  6,
                          6,  7,  8,  9, 10, 11,
                          12, 13, 14, 15, 16, 17,
                          11, 12, 13, 14, 15, 16,
                          16, 17, 18, 19, 20, 21,
                          22, 23, 24, 25, 26, 27,
                          28, 29, 30, 31, 32, 33};
    uchar expect[] = {1, 1, 1, 1, 1, 1,
                      1, 1, 1, 1, 1, 1,
                      1, 1, 1, 1, 1, 1,
                      1, 1, 1, 1, 1, 1,
                      1, 1, 1, 1, 1, 1,
                      1, 1, 1, 1, 1, 1};
    for (int tile_width = 2; tile_width < 8; ++tile_width) {
      for (int tile_height = 2; tile_height < 8; ++tile_height) {
        for (int hole_dia = 0; hole_dia < tile_width && hole_dia < tile_height;
             hole_dia += 2) {
          SimpleTiledFloodFill t(6, 6, tile_width, tile_height, 0,
                                 hole_dia, image_data);
          t.FillSeedFlood(0, 0, 0);
          const uchar *mask = t.get_mask();
          if (!CheckMaskResult(mask, 6, 6, expect, "NoFillTest", tile_width,
                               tile_height, hole_dia)) {
            return false;
          }
        }
      }
    }
    return true;
  };

  // Mixed flooding test
  bool FloodFillTestBasic() {
    uchar image_data[] = {1,   3,  3,  3,  5,  2,
                          6,  7,  8,  3,  2,  4,
                          4, 13,  3,  3, 16,  3,
                          4,  3, 15, 14,  3,  2,
                          3,  4, 18,  4, 20,  3,
                          5,  3, 30,  2,  3,  3};

    {  // Flood fill with value 3, no holes, tolerance 0
      uchar expect[] = {1, 0, 0, 0, 1, 1,
                        1, 1, 1, 0, 1, 1,
                        1, 1, 0, 0, 1, 1,
                        1, 1, 1, 1, 1, 1,
                        1, 1, 1, 1, 1, 1,
                        1, 1, 1, 1, 1, 1};
      for (int tile_width = 2; tile_width < 8; ++tile_width)
        for (int tile_height = 2; tile_height < 8; ++tile_height) {
          SimpleTiledFloodFill t(6, 6, tile_width, tile_height,
                                 0, 0, image_data);
          t.FillSeedFlood(3, 1, 0);
          const uchar *mask = t.get_mask();
          if (!CheckMaskResult(mask, 6, 6, expect, "FloodFillTestBasic", tile_width,
                               tile_height, 0)) {
            return false;
          }
        }
    }

    {  // Flood fill with value 3, hole dia 1, tolerance 0
      uchar expect[] = {1, 0, 0, 0, 1, 1,
                        1, 1, 1, 0, 1, 1,
                        1, 1, 0, 0, 1, 0,
                        1, 0, 1, 1, 0, 1,
                        0, 1, 1, 1, 1, 0,
                        1, 0, 1, 1, 0, 0};
      vector<uchar> expect_v(expect, expect+36);
      const int hole_dia = 1;
      for (int tile_width = 2; tile_width < 8; ++tile_width)
        for (int tile_height = 2; tile_height < 8; ++tile_height) {
          SimpleTiledFloodFill t(6, 6, tile_width, tile_height, 0,
                                 hole_dia, image_data);
          t.FillSeedFlood(3, 1, 0);
          const uchar *mask = t.get_mask();
          if (!CheckMaskResult(mask, 6, 6, expect, "FloodFillTestBasic 2",
                               tile_width, tile_height, hole_dia)) {
            return false;
          }
        }
    }

    {  // Flood fill with value 3, hole dia 2, tolerance 0
      uchar expect[] = {1, 0, 0, 0, 1, 1,
                        1, 1, 1, 0, 1, 1,
                        1, 1, 0, 0, 1, 1,
                        1, 1, 1, 1, 1, 1,
                        1, 1, 1, 1, 1, 0,
                        1, 1, 1, 1, 0, 0};
      vector<uchar> expect_v(expect, expect+36);
      const int hole_dia = 2;
      for (int tile_width = 2; tile_width < 8; ++tile_width)
        for (int tile_height = 2; tile_height < 8; ++tile_height) {
          SimpleTiledFloodFill t(6, 6, tile_width, tile_height, 0,
                                 hole_dia, image_data);
          t.FillSeedFlood(3, 1, 0);
          const uchar *mask = t.get_mask();
          if (!CheckMaskResult(mask, 6, 6, expect, "FloodFillTestBasic 3",
                               tile_width, tile_height, hole_dia)) {
            return false;
          }
        }
    }

    {  // Flood fill with value 3, hole dia 3, tolerance 0
      uchar expect[] = {1, 0, 0, 0, 1, 1,
                        1, 1, 1, 0, 1, 1,
                        1, 1, 0, 0, 1, 1,
                        1, 1, 1, 1, 1, 1,
                        1, 1, 1, 1, 1, 1,
                        1, 1, 1, 1, 1, 1};
      vector<uchar> expect_v(expect, expect+36);
      const int hole_dia = 3;
      for (int tile_width = 3; tile_width < 8; ++tile_width)
        for (int tile_height = 3; tile_height < 8; ++tile_height) {
          SimpleTiledFloodFill t(6, 6, tile_width, tile_height, 0,
                                 hole_dia, image_data);
          t.FillSeedFlood(3, 1, 0);
          const uchar *mask = t.get_mask();
          if (!CheckMaskResult(mask, 6, 6, expect, "FloodFillTestBasic 4",
                               tile_width, tile_height, hole_dia)) {
            return false;
          }
        }
    }

    return true;
  }


  // Mixed flooding test with tolerances
  bool FloodFillTestTolerance() {
    uchar image_data[] = {1,   3,  3,  3,  5,  2,
                          6,  7,  8,  3,  2,  4,
                          4, 13,  3,  3, 16,  3,
                          4,  3, 15, 14,  3,  2,
                          3,  4, 18,  4, 20,  3,
                          5,  3, 30,  2,  3,  3};

    {  // Flood fill with value 3, hole dia 0, tolerance 1
      uchar expect[] = {1, 0, 0, 0, 1, 0,
                        1, 1, 1, 0, 0, 0,
                        1, 1, 0, 0, 1, 0,
                        1, 1, 1, 1, 0, 0,
                        1, 1, 1, 0, 1, 0,
                        1, 1, 1, 0, 0, 0};
      vector<uchar> expect_v(expect, expect+36);
      const int hole_dia = 0;
      const int tolerance = 1;
      for (int tile_width = 2; tile_width < 8; ++tile_width)
        for (int tile_height = 2; tile_height < 8; ++tile_height) {
          SimpleTiledFloodFill t(6, 6, tile_width, tile_height, tolerance,
                                 hole_dia, image_data);
          t.FillSeedFlood(3, 1, 0);
          const uchar *mask = t.get_mask();
          if (!CheckMaskResult(mask, 6, 6, expect, "FloodFillTestTolerance 1",
                               tile_width, tile_height, hole_dia)) {
            return false;
          }
        }
    }

    {  // Flood fill with value 3, hole dia 2-3, tolerance 1
      uchar expect[] = {1, 0, 0, 0, 1, 0,
                        1, 1, 1, 0, 0, 0,
                        0, 1, 0, 0, 1, 0,
                        0, 0, 1, 1, 0, 0,
                        0, 0, 1, 0, 1, 0,
                        1, 0, 1, 0, 0, 0};
      vector<uchar> expect_v(expect, expect+36);
      const int tolerance = 1;
      for (int hole_dia = 2; hole_dia < 4; ++hole_dia)
        for (int tile_width = hole_dia; tile_width < 8; ++tile_width)
          for (int tile_height = hole_dia; tile_height < 8; ++tile_height) {
            SimpleTiledFloodFill t(6, 6, tile_width, tile_height, tolerance,
                                   hole_dia, image_data);
            t.FillSeedFlood(3, 1, 0);
            const uchar *mask = t.get_mask();
            if (!CheckMaskResult(mask, 6, 6, expect, "FloodFillTestTolerance 2",
                                 tile_width, tile_height, hole_dia)) {
              return false;
            }
        }
    }

    {  // Flood fill with value 3, hole dia 4, tolerance 1
      uchar expect[] = {1, 0, 0, 0, 1, 0,
                        1, 1, 1, 0, 0, 0,
                        1, 1, 0, 0, 1, 0,
                        1, 1, 1, 1, 0, 0,
                        1, 1, 1, 0, 1, 0,
                        1, 1, 1, 0, 0, 0};
      vector<uchar> expect_v(expect, expect+36);
      const int hole_dia = 4;
      const int tolerance = 1;
      for (int tile_width = hole_dia; tile_width < 8; ++tile_width)
        for (int tile_height = hole_dia; tile_height < 8; ++tile_height) {
          SimpleTiledFloodFill t(6, 6, tile_width, tile_height, tolerance,
                                 hole_dia, image_data);
          t.FillSeedFlood(3, 1, 0);
          const uchar *mask = t.get_mask();
          if (!CheckMaskResult(mask, 6, 6, expect, "FloodFillTestTolerance 3",
                               tile_width, tile_height, hole_dia)) {
            return false;
          }
        }
    }

    {  // Flood fill with value 3, hole dia 0, tolerance 1, 2 seeds
      uchar expect[] = {1, 0, 0, 0, 1, 0,
                        1, 1, 1, 0, 0, 0,
                        0, 1, 0, 0, 1, 0,
                        0, 0, 1, 1, 0, 0,
                        0, 0, 1, 0, 1, 0,
                        1, 0, 1, 0, 0, 0};
      vector<uchar> expect_v(expect, expect+36);
      const int tolerance = 1;
      for (int tile_width = 2; tile_width < 8; ++tile_width)
        for (int tile_height = 2; tile_height < 8; ++tile_height) {
          SimpleTiledFloodFill t(6, 6, tile_width, tile_height, tolerance,
                                 0, image_data);
          t.AddSeed(0, 4);
          t.FillSeedFlood(3, 1, 0);
          const uchar *mask = t.get_mask();
          if (!CheckMaskResult(mask, 6, 6, expect, "FloodFillTestTolerance 4",
                               tile_width, tile_height, 0)) {
            return false;
          }
        }
    }

    return true;
  };

  int RandomInt(int low, int high) {
    return low + static_cast<int>((high - low) * (rand() / (RAND_MAX + 1.0)));
  }

  // Generate random data, and make sure the different tile sizes all
  // get the same result.  Makes sure the tile boundaries are handled
  // the same as a single image.
  bool FloodFillTestRandom() {
    const int kIterations = 1000;
    const int kImgSize = 20;

    srand(1235);  // set random seed for repeatable q
    for (int iteration = 0; iteration < kIterations; ++iteration) {
      uchar image_data[kImgSize * kImgSize];
      for (int x = 0; x < kImgSize; ++x)
        for (int y = 0; y < kImgSize; ++y)
          image_data[y*kImgSize + x] = RandomInt(0, 255);
      int hole_dia = (RandomInt(0, 2) == 0 ? 0 : RandomInt(1, kImgSize/2));
      int tolerance = RandomInt(0, 50);
      int num_fill_values = RandomInt(1, 5);
      vector<int> fill_values;
      for (int i = 0; i < num_fill_values; ++i)
        fill_values.push_back(RandomInt(0, 255));


      int seed_x = RandomInt(0, kImgSize - 1);
      int seed_y = RandomInt(0, kImgSize - 1);

      SimpleTiledFloodFill base(kImgSize, kImgSize, kImgSize, kImgSize,
                                tolerance, hole_dia, image_data);
      for (uint f = 0; f < fill_values.size(); ++f)
        base.AddFillValue(fill_values[f]);
      base.AddSeed(seed_x, seed_y);
      base.FloodFill();
      const uchar *base_mask = base.get_mask();

      for (int tile_width = std::max(hole_dia, 2); tile_width <= kImgSize;
           ++tile_width) {
        for (int tile_height = std::max(hole_dia, 2); tile_width <= kImgSize;
             ++tile_width) {
          SimpleTiledFloodFill t(kImgSize, kImgSize, tile_width, tile_height,
                                 tolerance,
                                 hole_dia, image_data);
          for (uint f = 0; f < fill_values.size(); ++f)
            t.AddFillValue(fill_values[f]);
          t.AddSeed(seed_x, seed_y);
          t.FloodFill();
          const uchar *t_mask = t.get_mask();
          for (int x = 0; x < kImgSize; ++x) {
            for (int y = 0; y < kImgSize; ++y) {
              if (t_mask[y*kImgSize + x] != base_mask[y*kImgSize + x]) {
                fprintf(stderr, "Failed.  hole_dia %d\n",
                        hole_dia);
                fprintf(stderr, "tol %d seed %d %d tile_width %d %d\n",
                        tolerance, seed_x, seed_y, tile_width, tile_height);
                fprintf(stderr, "diff x y %d %d\n", x, y);
                fprintf(stderr, "Image:\n");
                printArray(image_data, kImgSize, kImgSize);
                fprintf(stderr, "Base:\n");
                printArray(base_mask, kImgSize, kImgSize);
                fprintf(stderr, "T:\n");
                printArray(t_mask, kImgSize, kImgSize);
                return false;
              }
            }
          }
        }
      }
    }
    return true;
  }

  // Utility for testing that an image of 1's and 0's is masked properly.
  // mask result should equal the image data.
  // Basic flood fill of 0 values and no tolerance is applied.
  bool FloodFillBasicTest(uchar* image_data,
                          uint image_width, uint image_height,
                          const std::string& message) {
    const uint pixel_count = image_width * image_height;
    uchar mask_result[pixel_count];
    vector<uchar> expect_v(image_data, image_data+pixel_count);
    const int fill_value = 0;
    const int hole_diameter = 1;
    const int tolerance = 0;
    const bool fill_white = false;
    InMemoryFloodFill maskgen(image_data, mask_result,
                              image_width, image_height,
                              fill_value, tolerance,
                              fill_white,
                              hole_diameter);
    maskgen.FloodFill();

    if (!CheckMaskResult(mask_result, image_width, image_height, image_data,
                         message, 0, 0, 0)) {
      return false;
    }
    return true;
  }

  // Testing that the geometric basic flood filling is correct.
  // Flood fill with value 0, hole dia 0, tolerance 0.
  // In this test, the image_data and expected are the same.
  bool FloodFillGeometricTest() {
    const uint image_width = 6;
    const uint image_height = 6;
    const uint pixel_count = image_width * image_height;
    {  // A case with mosaiced images may leave a gap in the middle.
      uchar image_data[pixel_count] = {
          1, 1, 1, 1, 1, 1,
          1, 1, 1, 1, 1, 1,
          1, 1, 0, 0, 0, 1,
          1, 1, 0, 0, 0, 1,
          1, 1, 0, 0, 0, 1,
          1, 1, 1, 1, 1, 1};
      if (!FloodFillBasicTest(image_data, image_width, image_height,
                              "Hole In Middle")) {
        return false;
      }
    }
    {  // Simple rotated image (common case) exposing the corners as mask.
      uchar image_data[pixel_count] = {
          0, 0, 1, 1, 0, 0,
          0, 0, 1, 1, 1, 0,
          0, 1, 1, 1, 1, 1,
          1, 1, 1, 1, 1, 0,
          1, 1, 1, 1, 0, 0,
          0, 1, 1, 1, 0, 0};
      if (!FloodFillBasicTest(image_data, image_width, image_height,
                              "Corners")) {
        return false;
      }
    }
    {  // An image in the middle.
      uchar image_data[pixel_count] = {
          0, 0, 0, 0, 0, 0,
          0, 1, 1, 1, 1, 0,
          0, 1, 1, 1, 1, 0,
          0, 1, 1, 1, 1, 0,
          0, 1, 1, 1, 1, 0,
          0, 0, 0, 0, 0, 0};
      if (!FloodFillBasicTest(image_data, image_width, image_height,
                              "Middle")) {
        return false;
      }
    }
    {  // An image on one side.
      // We break this into 4 tests, one for each side.
      // Use 4 entries to identify when to zero out a row or column.
      int zero_row[] = {0, image_height - 1, -1, -1};
      int zero_col[] = {-1, -1, 0, image_width - 1};
      for(uint i = 0; i < 4; ++i) {
        // Init the image data with 1's except where zero_row/col[i] is non-zero.
        uchar image_data[pixel_count];
        uint count = 0;
        for(int row = 0; row < static_cast<int>(image_height); ++row) {
          for(int column = 0; column < static_cast<int>(image_width);
              ++column, ++count) {
            image_data[count] = (row == zero_row[i] || column == zero_col[i]) ?
                0 : 1;
          }
        }
        std::ostringstream message;
        message << "Sides " << i;
        if (!FloodFillBasicTest(image_data, image_width, image_height,
                                message.str())) {
          return false;
        }
      }
    }
    return true;
  }
};  // class TiledFloodFillUnitTest


int main(int argc, char **argv) {
  TiledFloodFillUnitTest tests;
  return tests.Run();
}

