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


// Unit Tests for Featherer

#include <string>
#include <gtest/gtest.h>

#include "common/notify.h"
#include "fusion/tools/Featherer.h"

class FeathererTest : public testing::Test {
  protected:
    virtual void SetUp() {
    }

  virtual void TearDown() {
  }

  // Test if two images are equal (identical pixal values).
  int numUnequalPixels(unsigned char* img1, unsigned char* img2, int size) {
    int cnt = 0;
    for (int i = 0; i < size; ++i) {
      if (*img1++ != *img2++) {
        ++cnt;
      }
    }
    return cnt;
  }

  // Test for trapezoid in pixels.
  bool isTrapezoid(
      unsigned char* img, int len, int step, int offset, int maxTransition) {
    enum States {PRE_TRAP, RISE, PLATEAU, FALL, POST_TRAP} state;
    state = PRE_TRAP;

    img += offset;
    unsigned char lastPixel = 0x00;
    int transitionSize = 0;
    for (int i = 0; i < len; ++i) {
      switch (state) {
        // Flat before trapezoid begins
        case PRE_TRAP:
          if (*img != 0x00) {
            state = RISE;
            transitionSize = 0;
          }
          break;

        // Leading edge must continuously rise
        case RISE:
          if (*img == 0xff) {
            state = PLATEAU;
          } else if (*img <= lastPixel) {
            notify(NFY_NOTICE, "Leading edge didn't continuously rise.");
            return false;
          }

          if (++transitionSize > maxTransition) {
            notify(NFY_NOTICE, "Leading edge  was too long.");
            return false;
          }
          break;

        // Flat plateau of trapezoid
        case PLATEAU:
          if (*img != 0xff) {
            state = FALL;
            transitionSize = 0;
          }
          break;

       // Trailing edge must continuously fall
       case FALL:
          if (*img == 0x00) {
            state = POST_TRAP;
          } else if (*img >= lastPixel) {
            notify(NFY_NOTICE, "Trailing edge didn't continuously fall.");
            return false;
          }

          if (++transitionSize > maxTransition) {
            notify(NFY_NOTICE, "Trailing edge  was too long.");
            return false;
          }
         break;

        case POST_TRAP:
          if (*img != 0x00) {
            notify(NFY_NOTICE, "Non-zero pixel after trapezoid. %d", i);
            return false;
          }
         break;
      }

      lastPixel = *img;
      img += step;
    }

    if (state != POST_TRAP) {
      notify(NFY_NOTICE, "Did not reach end of trapezoid.");
      return false;
    }

    notify(NFY_NOTICE, "Good trapezoid.");
    return true;
  }

  // Show image
  void show(unsigned char* img, int width, int height) {
    for (int y = 0; y < height; ++y) {
      std::string str;
      for (int x = 0; x < width; ++x) {
        char s[64];
        snprintf(s, sizeof(s), " %02x", (unsigned int) *img++);
        str += s;
      }

      notify(NFY_NOTICE, "%s", str.c_str());
    }
  }
};


// Basic test in in-memory feathering.
TEST_F(FeathererTest, InMemoryFeatherBasicTest) {
  const int kImageHeight = 18;
  const int kImageWidth = 15;
  const int kImageSize = kImageHeight * kImageWidth;
  unsigned char testImage[kImageSize];
  unsigned char originalImage[kImageSize];

  // Set up simple rectangle image
  int idx = 0;
  for (int y = 0; y < kImageHeight; ++y) {
    for (int x = 0; x < kImageWidth; ++x) {
      if ((y >= 6) && (y < 12) && (x >= 5) && (x < 10)) {
        originalImage[idx] = 0xff;
      } else {
        originalImage[idx] = 0x00;
      }
      ++idx;
    }  // x
  }  // y

  // No change if feather is 0
  memcpy(testImage, originalImage, kImageSize);
  InMemoryFeatherer imf0(testImage, kImageWidth, kImageHeight, 0, 0x00);
  show(testImage, kImageWidth, kImageHeight);
  EXPECT_EQ(0, numUnequalPixels(testImage, originalImage, kImageSize));

  // Image should change if feather is greater than 0
  memcpy(testImage, originalImage, kImageSize);
  InMemoryFeatherer imf1(testImage, kImageWidth, kImageHeight, 1, 0x00);
  show(testImage, kImageWidth, kImageHeight);
  int feather1Diff = numUnequalPixels(testImage, originalImage, kImageSize);
  EXPECT_LT(0, feather1Diff);
  EXPECT_TRUE(isTrapezoid(testImage, kImageWidth, 1, kImageWidth * 8, 2));
  EXPECT_TRUE(isTrapezoid(testImage, kImageHeight, kImageWidth, 7, 2));
  notify(NFY_NOTICE, "Num unequal pixels = %d", feather1Diff);

  // Image should change more as feather size increases
  memcpy(testImage, originalImage, kImageSize);
  InMemoryFeatherer imf2(testImage, kImageWidth, kImageHeight, 2, 0x00);
  show(testImage, kImageWidth, kImageHeight);
  int feather2Diff = numUnequalPixels(testImage, originalImage, kImageSize);
  EXPECT_LT(feather1Diff, feather2Diff);
  EXPECT_TRUE(isTrapezoid(testImage, kImageWidth, 1, kImageWidth * 8, 3));
  EXPECT_TRUE(isTrapezoid(testImage, kImageHeight, kImageWidth, 7, 3));
  notify(NFY_NOTICE, "Num unequal pixels = %d", feather2Diff);
}

// TODO: Test disk-based feathering when better mocks are available.

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
