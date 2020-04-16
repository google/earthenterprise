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

// Feathers a given image. Ported out of gemaskgen.cpp.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_TOOLS_FEATHERER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_TOOLS_FEATHERER_H_

#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>

#include "fusion/tools/boxfilter.h"
#include "khgdal/khgdal.h"

class Featherer : public BoxFilterTiledImage {
 public:
  static const float kFeatherPRatio;
  static const float kFeatherRatio;

  Featherer(GDALRasterBand* poBand, int block_w, int block_h,
             int feather_radius, unsigned char border_value);
  virtual ~Featherer() {}

  virtual void LoadImageTile(int tile_x, int tile_y, unsigned char* image);
  virtual void SaveFilteredTile(int tile_x, int tile_y, const unsigned char* image);

  // Override boxfilter's standard Filter function. Never increase the alpha
  // value of a pixel: doing so could allow NoData values to be blended in,
  // causing terrain spikes for example.
  virtual void Filter(int ksum, unsigned char *pixel) const;

 private:
  GDALRasterBand *poBand_;
  float normalize_factor_;  // for sum of box area pixels, normalizes to 0-1
};


class InMemoryFeatherer : public BoxFilterTiledImage {
 public:
  InMemoryFeatherer(unsigned char *image, int image_width, int image_height,
                     int feather_radius, unsigned char border_value);
  virtual ~InMemoryFeatherer() {}

  virtual void LoadImageTile(int tile_x, int tile_y, unsigned char *image);
  virtual void SaveFilteredTile(int tile_x, int tile_y, const unsigned char *image);
  virtual void Filter(int ksum, unsigned char *pixel) const;

 private:
  unsigned char *image_;
  float normalize_factor_;  // for sum of box area pixels, normalizes to 0-1
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_TOOLS_FEATHERER_H_
