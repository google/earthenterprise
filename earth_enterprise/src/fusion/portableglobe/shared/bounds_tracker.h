/*
 * Copyright 2020 Open GEE Contributors
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
#include <cstdint>
#include "common/base/macros.h"
#include <cstdint>
#include "fusion/portableglobe/shared/packetbundle.h"

namespace fusion_portableglobe {

/**
   The boundary and zoom level information for a channel/layer.
*/
struct channel_info {
  // Channel ID and type
  std::uint16_t channel_id = UINT16_MAX;
  PacketType type;

  // Map tile bounding box
  std::uint32_t top = 0;
  std::uint32_t bottom = UINT32_MAX;
  std::uint32_t left = UINT32_MAX;
  std::uint32_t right = 0;

  // Zoom levels where this channel is available
  std::uint32_t min_level = UINT32_MAX;
  std::uint32_t max_level = 0;

  /**
     Default constructor that creates an empty bounds, where
     bottom > top, left > right, and min_level > max_level.
  */
  channel_info() : channel_id(UINT16_MAX),
                   top(0),
                   bottom(UINT32_MAX),
                   left(UINT32_MAX),
                   right(0),
                   min_level(UINT32_MAX),
                   max_level(0)
  {};

  /**
     Create a channel_info with the given channel_id and type, bounding the
     given row, column, and level.
     @param channel
     @param type
     @param row
     @param column
     @param level
  */
  channel_info(std::uint16_t channel_id,
               PacketType type,
               std::uint32_t row,
               std::uint32_t column,
               std::uint32_t level) :
    channel_id(channel_id),
    type(type),
    top(row), bottom(row),
    left(column), right(column),
    min_level(level), max_level(level)
  {}
};

/**
   A class for tracking the map tile boundaries and zoom levels of a portable file
   as it's being built. Once the globe is finished, the information is saved to
   earth/metadata.json and included in the final *.glb or *.glm file.

   Only the tile boundaries at max_level are stored by the tracker.
 */

class BoundsTracker {
public:

  /// Default constructor
  BoundsTracker();

  /**
     Update the channel_info for the specified channel to be of the given type and
     include qtpath in its bounds.
     @param channel ID of the channel to be updated.
     @param type    The packet type of the channel.
     @param qtpath  The quadtree address to update.
  */
  void update(std::uint16_t channel, PacketType type, const std::string& qtpath);

  /**
     Write current boundary information to the specified JSON file.
     @param filename
  */
  void write_json_file(const std::string& filename) const;

  /**
     Return a const reference to the specified channel_info object.
     Throws a khSimpleException if the channel isn't found.
     @param channel_id
  */
  const channel_info& channel(std::uint16_t channel_id) const;


private:

  // Minimum and maximum zoom level and channel for imagery, terrain, and vector data
  std::map<std::uint16_t, channel_info> channels;

  DISALLOW_COPY_AND_ASSIGN(BoundsTracker);
};

}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PORTABLEGLOBEBUILDER_H_
