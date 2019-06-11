// Copyright 2019 Open GEE Contributors
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


// Methods for keeping track of quadtree bounds and writing them to a JSON file.

#include "fusion/portableglobe/shared/bounds_tracker.h"
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include "common/khFileUtils.h"
#include "common/khSimpleException.h"
#include "fusion/portableglobe/quadtree/qtutils.h"
#include "fusion/portableglobe/shared/packetbundle.h"

namespace fusion_portableglobe {

BoundsTracker::BoundsTracker() {}

void BoundsTracker::update(uint16_t channel_id, PacketType type, const std::string& qtpath) {
  std::string real_path = "0" + qtpath;

  uint32_t column, row, zoom;
  ConvertFromQtNode(real_path, &column, &row, &zoom);

  uint32_t level = qtpath.size();

  if (channels.count(channel_id) == 0) {
    channels[channel_id] = {channel_id, type, row, column, level};

  } else {
    channel_info& channel = channels[channel_id];

    // BoundsTracker only tracks bounds at the maximum zoom level (max_level)
    if (level < channel.min_level) {
      channel.min_level = level;

      // If this node is at a new max_level then set max_level and also update the bounds
      // to be the current row and column.
    } else if (level > channel.max_level) {
      channel.max_level = level;
      channel.left = channel.right = column;
      channel.top = channel.bottom = row;

      // A new node at max_level, so expand the bounding box if necessary.
    } else if (level == channel.max_level) {
      channel.left = std::min(column, channel.left);
      channel.right = std::max(column, channel.right);
      channel.top = std::min(row, channel.top);
      channel.bottom = std::max(row, channel.bottom);
    }
  }
}

void BoundsTracker::write_json_file(const std::string& filename) const {
  khEnsureParentDir(filename);

  static const std::map<PacketType, std::string> channel_type_strings =
    {{ kDbRootPacket,  "DbRoot"},
     { kDbRoot2Packet, "DbRoot2"},
     { kQtpPacket,     "Qtp"},
     { kQtp2Packet,    "Qtp2"},
     { kImagePacket,   "Image"},
     { kTerrainPacket, "Terrain"},
     { kVectorPacket,  "Vector"}};
    
  std::ofstream fout(filename.c_str());

  // Find the last entry so the last comma can be skipped
  auto last_entry = channels.end();
  --last_entry;

  fout << "[\n";
  for (auto iter =  channels.begin(); iter != channels.end(); ++iter) {

    const std::pair<uint16_t, channel_info> &channel_pair = *iter;
    const auto &channel = channel_pair.second;

    fout << "  {\n"
         << "    \"channel_id\": "      << channel.channel_id << ",\n"
         << "    \"type\": \""          << channel_type_strings.at(channel.type) << "\",\n"
         << "    \"top\": "             << channel.top << ",\n"
         << "    \"bottom\": "          << channel.bottom << ",\n"
         << "    \"left\": "            << channel.left << ",\n"
         << "    \"right\": "           << channel.right << ",\n"
         << "    \"min_level\": "       << channel.min_level << ",\n"
         << "    \"max_level\": "       << channel.max_level << "\n"
         << "  }";

    if (iter != last_entry) {
      fout << ",";
    }
    fout << "\n";
  }
  fout << "]\n";
  fout.close();
}

const channel_info& BoundsTracker::channel(uint16_t channel_id) const {
  if (channels.count(channel_id) == 0) {
    throw khSimpleException("Channel not found in BoundsTracker: ") << channel_id;
  }
  return channels.at(channel_id);
}

}  // namespace fusion_portableglobe
