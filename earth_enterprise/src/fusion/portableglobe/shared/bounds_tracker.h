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

namespace fusion_portableglobe {

class BoundsTracker {
public:
  BoundsTracker();

  void update_imagery(const std::string& qtpath);
  void update_terrain(const std::string& qtpath);
  void update_vector(const std::string& qtpath);

  void write_json_file(const std::string& filename);

public:
  uint32_t top = 0;
  uint32_t left = UINT32_MAX;
  uint32_t bottom = UINT32_MAX;
  uint32_t right = 0;

  uint32_t min_image_level = 32;
  uint32_t max_image_level = 0;

  uint32_t min_terrain_level = 32;
  uint32_t max_terrain_level = 0;

  uint32_t min_vector_level = 32;
  uint32_t max_vector_level = 0;

  // save off the location and size of the first packet encountered at the highest level we
  // find, so that we can check if there is another layer after that
  uint64_t terrain_packet_offset = UINT64_MAX;
  uint32_t terrain_packet_size = UINT32_MAX;

  uint16_t image_tile_channel = UINT16_MAX;
  uint16_t terrain_tile_channel = UINT16_MAX;
  uint16_t vector_tile_channel = UINT16_MAX;

private:
  void update(const std::string& qtpath, uint32_t& min_level, uint32_t& max_level, bool update=false);

  DISALLOW_COPY_AND_ASSIGN(BoundsTracker);
};

}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PORTABLEGLOBEBUILDER_H_
