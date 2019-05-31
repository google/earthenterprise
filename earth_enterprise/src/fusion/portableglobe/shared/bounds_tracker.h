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
#include <map>
#include "common/base/macros.h"
#include "common/khTypes.h"
#include "fusion/portableglobe/shared/packetbundle.h"

namespace fusion_portableglobe {


struct channel_info {
  uint32_t top = 0;
  uint32_t bottom = UINT32_MAX;
  uint32_t left = UINT32_MAX;
  uint32_t right = 0;

  uint32_t min_level = UINT32_MAX;
  uint32_t max_level = 0;
  uint16_t channel = UINT16_MAX;
  PacketType type;
  channel_info() : top(0),
                   bottom(UINT32_MAX),
                   left(UINT32_MAX),
                   right(0),
                   min_level(UINT32_MAX),
                   max_level(0),
                   channel(UINT16_MAX)
  {};
  channel_info(uint32_t row,
               uint32_t column,
               uint32_t level,
               uint16_t channel,
               PacketType type) :
    top(row), bottom(row),
    left(column), right(column),
    min_level(level), max_level(level),
    channel(channel),
    type(type)
  {}
};

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
  // Minimum and maximum zoom level and channel for imagery, terrain, and vector data
  std::map<uint16_t, channel_info> channels;

private:
  DISALLOW_COPY_AND_ASSIGN(BoundsTracker);
};

}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PORTABLEGLOBEBUILDER_H_
