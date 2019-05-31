// Copyright 2019 Google Inc.
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


// Methods keeping track of quadtree bounds and writing to a JSON file.

#include "fusion/portableglobe/shared/bounds_tracker.h"
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include "common/khFileUtils.h"
#include "fusion/portableglobe/quadtree/qtutils.h"
#include "fusion/portableglobe/shared/packetbundle.h"

namespace fusion_portableglobe {

BoundsTracker::BoundsTracker() {}

void BoundsTracker::update(const std::string& qtpath, PacketType type, uint16_t channel) {

  // Pass min_level and max_level parameters based on type
  switch (type) {

  case kImagePacket:
    update(qtpath, this->min_image_level, this->max_image_level, true);
    image_tile_channel = channel;
    break;

  case kTerrainPacket:
    update(qtpath, this->min_terrain_level, this->max_terrain_level);
    terrain_tile_channel = channel;
    break;

  case kVectorPacket:
    update(qtpath, this->min_vector_level, this->max_vector_level);
    vector_tile_channel = channel;
    break;

  default:
    return;
  }
}
void BoundsTracker::write_json_file(const std::string& filename) {
  khEnsureParentDir(filename);
  std::ofstream fout(filename.c_str());
  fout << "[\n"
       << "  {\n"
       << "    \"layer_id\": 0\n"
       << "    \"top\": " << this->top << ",\n"
       << "    \"bottom\": " << this->bottom << ",\n"
       << "    \"left\": " << this->left << ",\n"
       << "    \"right\": " << this->right << ",\n"
       << "    \"min_image_level\": " << this->min_image_level << ",\n"
       << "    \"max_image_level\": " << this->max_image_level << ",\n"
       << "    \"min_terrain_level\": " << this->min_terrain_level << ",\n"
       << "    \"max_terrain_level\": " << this->max_terrain_level << ",\n"
       << "    \"min_vector_level\": " << this->min_vector_level << ",\n"
       << "    \"max_vector_level\": " << this->max_vector_level << ",\n"
       << "    \"image_tile_channel\": " << this->image_tile_channel << ",\n"
       << "    \"terrain_tile_channel\": " << this->terrain_tile_channel << ",\n"
       << "    \"vector_tile_channel\": " << this->vector_tile_channel << ",\n"
       << "  }\n"
       << "]\n";

  fout.close();
}

void BoundsTracker::update(const std::string& qtpath, uint32_t& min_level, uint32_t& max_level, bool update_bounding_box) {
  std::string real_path = "0" + qtpath;

  uint32_t column, row, zoom;
  ConvertFromQtNode(real_path, &column, &row, &zoom);

  if (zoom < min_level) {
    min_level = zoom;
  }
  if (zoom > max_level) {
    max_level = zoom;
    if (update_bounding_box) {
      this->left = column;
      this->right = column;
      this->top = row;
      this->bottom = row;
    }
  }

  if (zoom == max_level && update_bounding_box) {
    this->left = std::min(column, this->left);
    this->right = std::max(column, this->right);

    this->top = std::min(row, this->top);
    this->bottom = std::max(row, this->bottom);
  }
}
  
}  // namespace fusion_portableglobe
