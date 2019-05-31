/*
 * Copyright 2019 Google Inc.
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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_BOUNDS_TRACKER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_BOUNDS_TRACKER_H_

#include <fstream>  // NOLINT(readability/streams)
#include <string>
#include <vector>
#include "common/base/macros.h"
#include "common/khTypes.h"
#include "fusion/portableglobe/shared/packetbundle.h"

namespace fusion_portableglobe {

/**
   A class to keep track of imagery, terrain, and vector boundaries while
   building maps and globes.
 */
class BoundsTracker {
public:
  BoundsTracker();

  /**
     Update min_level and max_level in/out parameters to incorporate qtpath.
     If update is true, also update the top, bottom, left, and right boundaries.
     @param qtpath The quadtree path to incorporate
     @param type   The type of node being added.
  */
  void update(const std::string& qtpath, PacketType type, uint16_t channel);

  /**
     Write current boundary information to a JSON file.
     @param filename The name of the file to create.
  */
  void write_json_file(const std::string& filename);

public:
  // top, left, bottom, and right keep track of the bounding box for imagery
  uint32_t top = 0;
  uint32_t bottom = UINT32_MAX;
  uint32_t left = UINT32_MAX;
  uint32_t right = 0;

  // Minimum and maximum zoom levels with imagery packets
  uint32_t min_image_level = 32;
  uint32_t max_image_level = 0;

  // Minimum and maximum zoom levels with terrain packets
  uint32_t min_terrain_level = 32;
  uint32_t max_terrain_level = 0;

  // Minimum and maximum zoom levels with vector packets
  uint32_t min_vector_level = 32;
  uint32_t max_vector_level = 0;

  // Keep track of which channels contain imagery, terrain, and vector data.
  uint16_t image_tile_channel = UINT16_MAX;
  uint16_t terrain_tile_channel = UINT16_MAX;
  uint16_t vector_tile_channel = UINT16_MAX;

private:
  void update(const std::string& qtpath, uint32_t& min_level, uint32_t& max_level, bool update_bounding_box=false);

  DISALLOW_COPY_AND_ASSIGN(BoundsTracker);
};

}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PORTABLEGLOBEBUILDER_H_
