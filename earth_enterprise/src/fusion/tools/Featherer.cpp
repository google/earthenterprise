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


// Feathers a given image. Ported out of gemaskgen.cpp.

#include "fusion/tools/Featherer.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <ios>

#include "common/notify.h"
#include "fusion/tools/boxfilter.h"


// The feathering code attempts to only feather "into" the image data
// (from the border or floodfilled or NoData regions). So for example
// data that goes all the way to the edge of an image gets faded to 0
// at the edge of the image.
const float Featherer::kFeatherPRatio = 0.6f;
const float Featherer::kFeatherRatio =
    1.0 / (2.0 - kFeatherPRatio) / 2.0;


inline unsigned char NonlinearFilter(int ksum, float normalize_factor) {
  // from old gemaskgen: feather only inside the mask (do not expand)
  // see also: constructor
  int kvalue =
    static_cast<int>((ksum*normalize_factor - (1.0 - Featherer::kFeatherRatio))
                     / Featherer::kFeatherRatio * 255.0 + 0.5);

  return std::max(0, kvalue);
}


InMemoryFeatherer::InMemoryFeatherer(unsigned char *image, int image_width,
                                     int image_height, int feather_radius,
                                     unsigned char border_value)
    : BoxFilterTiledImage(image_width, image_height,
                          image_width, image_height),
      image_(image) {
  SingleImageTile(image);

  // from old gemaskgen: feather only towards inside of mask (do not expand)
  // see also: Filter()
  int box_width = feather_radius * 2 + 1;
  normalize_factor_ = 1.0 / (box_width*box_width) / 255.0;

  BoxFilter(box_width, box_width, border_value);
}

void InMemoryFeatherer::LoadImageTile(int tile_x, int tile_y, unsigned char *image) {
  notify(NFY_FATAL, "should never get here");
}

void InMemoryFeatherer::SaveFilteredTile(int tile_x, int tile_y,
                                         const unsigned char *image) {
  if (!tile_x && !tile_y && image != image_)
    memcpy(image_, image, image_width_ * image_height_);
}

void InMemoryFeatherer::Filter(int ksum, unsigned char *pixel) const {
  *pixel = std::min(*pixel, NonlinearFilter(ksum, normalize_factor_));
}


Featherer::Featherer(GDALRasterBand *poBand, int block_w, int block_h,
                       int feather_radius, unsigned char border_value)
    : BoxFilterTiledImage(poBand->GetXSize(),
                          poBand->GetYSize(),
                          block_w, block_h),
      poBand_(poBand) {
  if (poBand->GetRasterDataType() != GDT_Byte)
    notify(NFY_FATAL, "Raster band data type must be byte.");

  // Set up parameters for filtering.  See Filter().
  int box_width = feather_radius * 2 + 1;
  normalize_factor_ = 1.0 / (box_width*box_width) / 255.0;

  BoxFilter(box_width, box_width, border_value);
}

void Featherer::LoadImageTile(int tile_x, int tile_y, unsigned char *image) {
  int xsz = std::min(tile_width_, image_width_ - tile_x * tile_width_),
      ysz = std::min(tile_height_, image_height_ - tile_y * tile_height_);
  if (poBand_->RasterIO(GF_Read, tile_x * tile_width_, tile_y * tile_height_,
                        xsz, ysz,
                        image,
                        xsz, ysz,
                        GDT_Byte, 0, tile_width_) == CE_Failure)
    notify(NFY_FATAL, "Could not load tile %d, %d to feather.",
           tile_x, tile_y);
}

void Featherer::SaveFilteredTile(int tile_x, int tile_y, const unsigned char *image) {
  int xsz = std::min(tile_width_, image_width_ - tile_x * tile_width_),
      ysz = std::min(tile_height_, image_height_ - tile_y * tile_height_);
  if (poBand_->RasterIO(GF_Write, tile_x * tile_width_, tile_y * tile_height_,
                        xsz, ysz,
                        const_cast<unsigned char *>(image),
                        xsz, ysz,
                        GDT_Byte, 0, tile_width_) == CE_Failure)
    notify(NFY_FATAL, "Could not save feathered tile %d, %d.",
           tile_x, tile_y);
}

// Override boxfilter's standard Filter function. Never increase the alpha
// value of a pixel: doing so could allow NoData values to be blended in,
// causing terrain spikes for example.
void Featherer::Filter(int ksum, unsigned char *pixel) const {
  *pixel = std::min(*pixel, NonlinearFilter(ksum, normalize_factor_));
}
