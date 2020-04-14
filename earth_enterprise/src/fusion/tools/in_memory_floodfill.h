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

#ifndef FUSION_TOOLS_IN_MEMORY_FLOODFILL_H_
#define FUSION_TOOLS_IN_MEMORY_FLOODFILL_H_

#include "fusion/tools/tiledfloodfill.h"

// InMemoryFloodFill implements a flood fill of the image in memory,
// placing the resulting filled image in alpha.  It does not allocate
// or free image or alpha.  See tiledfloodfill.h for the meaning of
// the other arguments.
class InMemoryFloodFill : public TiledFloodFill {
 public:
  InMemoryFloodFill(const unsigned char *image, unsigned char *alpha,
                    int image_width, int image_height,
                    int fill_value, int tolerance, bool fill_white,
                    int hole_size)
      : TiledFloodFill(image_width, image_height,
                       image_width, image_height,
                       tolerance,
                       hole_size),
        image_(image),
        alpha_(alpha),
        opacity_(kNotFilled) {
    AddSeed(0, image_height_ - 1);
    AddSeed(image_width_ - 1, image_height_ - 1);
    AddSeed(0, 0);
    AddSeed(image_width_ - 1, 0);

    if (fill_value != kNoFillValueSpecified) {
      AddFillValue(fill_value);
    } else {
      AddFillValue(image_[0]);
      AddFillValue(image_[image_width_ - 1]);
      AddFillValue(image_[image_height_ * image_width_ - image_width_]);
      AddFillValue(image_[image_height_ * image_width_ - 1]);
    }
    if (fill_white) AddFillValue(255);
    memset(alpha, kNotFilled, image_width_ * image_height_);
  }
  virtual ~InMemoryFloodFill() {}
  virtual const unsigned char *LoadImageTile(int tile_x, int tile_y) {return image_;}
  virtual unsigned char *LoadMaskTile(int tile_x, int tile_y, double *old_opacity) {
    *old_opacity = opacity_;
    return alpha_;
  }
  virtual void SaveMaskTile(int tile_x, int tile_y, double opacity) {
    opacity_ = opacity;
  }

 protected:
  const unsigned char *image_;
  unsigned char *alpha_;
  double opacity_;
};

#endif // FUSION_TOOLS_IN_MEMORY_FLOODFILL_H_
