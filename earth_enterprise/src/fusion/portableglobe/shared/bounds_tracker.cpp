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

void BoundsTracker::update(const std::string& qtpath, PacketType type, uint16_t packet_channel) {
  std::string real_path = "0" + qtpath;

  uint32_t column, row, zoom;
  ConvertFromQtNode(real_path, &column, &row, &zoom);

  uint32_t level = qtpath.size();

  if (channels.find(packet_channel) == channels.end()) {
    channels[packet_channel] = {row, column, level, packet_channel, type};

  } else {
    channel_info& channel = channels[packet_channel];

    if (level < channel.min_level) {
      channel.min_level = level;

    } else if (level > channel.max_level) {
      channel.max_level = level;
      channel.left = column;
      channel.right = column;
      channel.top = row;
      channel.bottom = row;

    } else if (level == channel.max_level) {
      channel.left = std::min(column, channel.left);
      channel.right = std::max(column, channel.right);
      channel.top = std::min(row, channel.top);
      channel.bottom = std::max(row, channel.bottom);
    }
  }
}

void BoundsTracker::write_json_file(const std::string& filename) {
  khEnsureParentDir(filename);
  static std::map<PacketType, std::string> channel_type_strings;
  if (channel_type_strings.size() == 0) {
    channel_type_strings[kDbRootPacket] = "DbRoot";
    channel_type_strings[kDbRoot2Packet] = "DbRoot2";
    channel_type_strings[kQtpPacket] = "Qtp";
    channel_type_strings[kQtp2Packet] = "Qtp2";
    channel_type_strings[kImagePacket] = "Image";
    channel_type_strings[kTerrainPacket] = "Terrain";
    channel_type_strings[kVectorPacket] = "Vector";
  }
    
  std::ofstream fout(filename.c_str());
  fout << "[\n";
  auto last_entry = channels.end();
  --last_entry;

  for (auto iter =  channels.begin(); iter != channels.end(); ++iter) {
    const std::pair<uint16_t, channel_info> channel = *iter;

    fout << "  {\n"
         << "    \"layer_id\": " << channel.first << ",\n"
         << "    \"top\": " << channel.second.top << ",\n"
         << "    \"bottom\": " << channel.second.bottom << ",\n"
         << "    \"left\": " << channel.second.left << ",\n"
         << "    \"right\": " << channel.second.right << ",\n"
         << "    \"min_image_level\": " << channel.second.min_level << ",\n"
         << "    \"max_image_level\": " << channel.second.max_level << ",\n"
         << "    \"channel\": " << channel.second.channel << ",\n"
         << "    \"type\": \"" << channel_type_strings[channel.second.type] << "\"\n"
         << "  }";
    
    if (iter != last_entry) {
      fout << ",";
    }
    fout << "\n";
  }
  fout << "]\n";
  fout.close();
}

}  // namespace fusion_portableglobe
