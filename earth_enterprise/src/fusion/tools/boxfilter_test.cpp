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


// TODO: explicitly test 4 cases described at top of
// boxfilter.h, plus one large stress test with significant size.

#include <stdlib.h>

#include <vector>

#include "notify.h"
#include "boxfilter.h"
#include "UnitTest.h"

using std::vector;

// A class to hold test data in memory to test BoxFilterTiledImage.
class BoxFilterTestFixture : public AveragingBoxFilter {
 public:
  BoxFilterTestFixture(int image_width, int image_height,
                       int tile_width, int tile_height,
                       const unsigned char *image)
      : AveragingBoxFilter(image_width, image_height, tile_width, tile_height),
        orig_image_(image, image + image_width * image_height),
        filt_image_(image_width * image_height, 0) {
  }
  virtual ~BoxFilterTestFixture() {}

  // Serves an image tile to the filter from the in-memory image.
  virtual void LoadImageTile(int tx, int ty, unsigned char *image_tile) {
    ++tile_load_count_;
    notify(NFY_VERBOSE, "Loading tile %d %d\n", tx, ty);
    for (int x = 0; x < tile_width_ && tx*tile_width_ + x < image_width_;
         ++x)
      for (int y = 0; y < tile_height_ && ty*tile_height_ + y < image_height_;
           ++y)
        image_tile[y*tile_width_ + x] =
          orig_image_.at(ty*tile_height_*image_width_ + y*image_width_ +
                         tx*tile_width_ + x);
  }
  // Saves an output tile to the in-memory output image.
  virtual void SaveFilteredTile(int tx, int ty,
                                const unsigned char *output_tile) {
    for (int x = 0; x < tile_width_ && tx*tile_width_ + x < image_width_; ++x)
      for (int y = 0; y < tile_height_ && ty*tile_height_ + y < image_height_;
           ++y)
        filt_image_.at(ty*tile_height_*image_width_ + y*image_width_ +
                       tx*tile_width_ + x) =
          output_tile[y*tile_width_ + x];
  }

  // This function wraps BoxFilter() so that we can count tile loads
  // to make sure they're not excessive.
  void TestBoxFilter(int box_width, int box_height, unsigned char border_value) {
    tile_load_count_ = 0;
    BoxFilter(box_width, box_height, border_value);
  }

  // Each tile should be loaded no more than 2 times in certain cases
  // determined by the test data.
  bool TileLoadCountOK() const {
    return (tile_load_count_ <= 2 * num_tiles_x_ * num_tiles_y_);
  }

  int tile_load_count_;       // how many tiles have been loaded
  vector<unsigned char> orig_image_;  // the full-size input image
  vector<unsigned char> filt_image_;  // the filtered output image
};


class BoxFilterUnitTest : public UnitTest<BoxFilterUnitTest> {
 public:
  BoxFilterUnitTest() : BaseClass("BoxFilterUnitTest") {
    REGISTER(ImageStorageTest);
    REGISTER(BlankTest);
    REGISTER(BoxFilterTestRandom);
  }

  // Helper function to print arrays for debugging.
  void printArray(const unsigned char *a, const int size_x, const int size_y) {
    for (int y = 0; y < size_y; ++y) {
      for (int x = 0; x < size_x; ++x)
        fprintf(stderr, "%4d", a[y*size_x + x]);
      fprintf(stderr, "\n");
    }
  }

  void printArray(const vector<unsigned char> &a, int size_x, int size_y) {
    printArray(&a[0], size_x, size_y);
  }

  // First test BoxFilterTestFixture test class.
  bool ImageStorageTest() {
    unsigned char img_data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    BoxFilterTestFixture t(4, 4, 2, 2, img_data);

    vector<unsigned char> tile(4, 0);
    t.LoadImageTile(0, 0, &tile[0]);
    unsigned char expect1[] = {1, 2, 5, 6};
    EXPECT_TRUE((tile) == (vector<unsigned char>(expect1, expect1+4)));

    t.LoadImageTile(1, 0, &tile[0]);
    unsigned char expect2[] = {3, 4, 7, 8};
    EXPECT_TRUE((tile) == (vector<unsigned char>(expect2, expect2+4)));

    t.LoadImageTile(0, 1, &tile[0]);
    unsigned char expect3[] = {9, 10, 13, 14};
    EXPECT_TRUE((tile) == (vector<unsigned char>(expect3, expect3+4)));

    t.LoadImageTile(1, 1, &tile[0]);
    unsigned char expect4[] = {11, 12, 15, 16};
    EXPECT_TRUE((tile) == (vector<unsigned char>(expect4, expect4+4)));

    return true;
  }

  // Now do actual tests of BoxFilter.

  // Single-value input should yield unchanged output.
  bool BlankTest() {
    for (unsigned char val = 0; val < 200; val += 50) {
      vector<unsigned char> image = vector<unsigned char>(36, val);

      for (int tile_width = 2; tile_width < 8; ++tile_width)
        for (int tile_height = 2; tile_height < 8; ++tile_height)
          for (int box_width = 1; box_width <= tile_width; box_width += 2)
            for (int box_height = 1; box_height <= tile_height;
                 box_height += 2) {
              BoxFilterTestFixture t(6, 6, tile_width, tile_height, &image[0]);
              t.TestBoxFilter(box_width, box_height, val);
              EXPECT_TRUE((t.filt_image_) == (vector<unsigned char>(36, val)));
              if (!t.TileLoadCountOK()) {
                fprintf(stderr, "FAILURE: too many tiles loaded %d\n",
                        t.tile_load_count_);
                EXPECT_TRUE(false);
              }
            }
    }

    return true;
  }

  int RandomInt(int low, int high) {
    return low + static_cast<int>((high - low) * (rand() / (RAND_MAX + 1.0)));
  }

  // Generate random data, calculate result the slow and simple way,
  // and make sure the fast algorithm gets the same result.
  bool BoxFilterTestRandom() {
    const int kIterations = 1000;   // 1000 == ~1 sec optimized

    for (int it = 0; it < kIterations; ++it) {
      // Generate random parameters and image.
      int image_width = RandomInt(5, 50);
      int image_height = RandomInt(5, 50);
      int box_width = 1 + 2 * RandomInt(0, image_width / 2 - 1);
      int box_height = 1 + 2 * RandomInt(0, image_height / 2 + 5);
      int box_area = box_width * box_height;
      int border = RandomInt(0, 255);
      vector<unsigned char> image_orig(image_width * image_height, 0);
      for (int x = 0; x < image_width; ++x)
        for (int y = 0; y < image_height; ++y)
          image_orig[y*image_width + x] = RandomInt(0, 255);

      // Calculate filtered image the slow way.
      vector<unsigned char> expected(image_width * image_height, 0);
      for (int x = 0; x < image_width; ++x)
        for (int y = 0; y < image_height; ++y) {
          int sum = 0;
          for (int dx = x-box_width/2; dx <= x+box_width/2; ++dx)
            for (int dy = y-box_height/2; dy <= y+box_height/2; ++dy)
              if (dx < 0 || dx >= image_width ||
                  dy < 0 || dy >= image_height)
                sum += border;
              else
                sum += image_orig[dy*image_width + dx];
          expected[y*image_width + x] = sum / box_area;
        }

      // Faster tiled computation must get same result as slow version.
      const int kIterations2 = 5;  // try 5 different tile sizes for each
      for (int it2 = 0; it2 < kIterations2; ++it2) {
        // Copy the image because the single-tile-mode modifies it in
        // place.
        vector<unsigned char> image = image_orig;
        int tile_width = RandomInt(std::max(box_width/2 + 1, 2), image_width+5);
        int tile_height = RandomInt(3, image_height + 5);
        if (RandomInt(0, 10) == 0) {   // Sometimes we test single-tile mode.
          tile_width = image_width;
          tile_height = image_height;
        }

        BoxFilterTestFixture t(image_width, image_height,
                               tile_width, tile_height, &image[0]);
        if (tile_width == image_width && tile_height == image_height &&
            RandomInt(0, 1) == 0)
          t.SingleImageTile(&image[0]);
        t.TestBoxFilter(box_width, box_height, border);
        if (tile_height > box_height && !t.TileLoadCountOK()) {
          fprintf(stderr, "FAILURE: too many tiles loaded %d\n",
                  t.tile_load_count_);
          EXPECT_TRUE(false);
          return false;
        }
        if (t.filt_image_ != expected) {
          fprintf(stderr, "FAILED: box_wh %d %d, tile_wh %d %d, border %d\n",
                  box_width, box_height, tile_width, tile_height, border);
          fprintf(stderr, "Image:\n");
          printArray(image, image_width, image_height);
          fprintf(stderr, "Expected:\n");
          printArray(expected, image_width, image_height);
          fprintf(stderr, "Filtered:\n");
          printArray(t.filt_image_, image_width, image_height);
          EXPECT_TRUE(false);
        }
      }
    }

    return true;
  }
};  // class TiledFloodFillUnitTest


int main(int argc, char **argv) {
  BoxFilterUnitTest tests;
  return tests.Run();
}
