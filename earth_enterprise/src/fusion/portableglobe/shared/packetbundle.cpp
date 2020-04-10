// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


#include <notify.h>
#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include <utility>
#include <vector>
#include "fusion/portableglobe/shared/packetbundle.h"

namespace fusion_portableglobe {

/**
 * Equality operator for index items. Necessary for finding the index
 * item that we are searching for.
 */
bool IndexItem::operator==(const IndexItem& other) const {
  return (btree_high == other.btree_high) &&
         (btree_low == other.btree_low) &&
         (level == other.level) &&
         (packet_type == other.packet_type) &&
         (channel == other.channel);
}

/**
 * Less Than operator for index items. Necessary for search for the index
 * item that in an ordered list of index items.
 */
bool IndexItem::operator<(const IndexItem& other) const {
  if (btree_high < other.btree_high) {
    return true;
  } else if (btree_high == other.btree_high) {
    if (btree_low < other.btree_low) {
      return true;
    } else if (btree_low == other.btree_low) {
      if (level < other.level) {
        return true;
      } else if (level == other.level) {
        if (packet_type < other.packet_type) {
          return true;
        } else if (packet_type == other.packet_type) {
          if (channel < other.channel) {
            return true;
          }
        }
      }
    }
  }

  return false;
}

/**
 * Fills in values for an index item.
 * TODO: Derive btree and level values from a qt packet string.
 */
void IndexItem::Fill(std::string qtpath, std::uint8_t packet_type, std::uint16_t channel) {
  level = qtpath.size() - 1;
  btree_high = 0x00000000;
  btree_low = 0x0000;

  // Skip first character. It is always 0 at level 0.
  int str_idx = 1;

  // Fill up btree high values.
  for (int i = 0; (i < 16) && (i < level); ++i) {
    // Shift qt path bits into btree value.
    btree_high |= (qtpath[str_idx++] - '0') << (30 - i*2);
  }

  // Fill up btree low values.
  for (int i = 0; (i < 8) && (i < level - 16); ++i) {
    // Shift qt path bits into btree value.
    btree_low |= (qtpath[str_idx++] - '0') << (14 - i*2);
  }

  this->packet_type = packet_type;
  this->channel = channel;
}

/**
 * Get quadtree address that index item points to. Address includes the
 * leading zero. Quadtree address is stored in the
 * btree_high (32 bits - 16 quadtree digits) and
 * btree_low (16 bits - 8 quadtree digits).
 * The number of digits to pay
 * attention to are determined by the level of the index item.
 */
std::string IndexItem::QuadtreeAddress() const {
  std::string address = "0";
  for (int i = 0; i < std::min(16, 0 + level); ++i) {
    int local_quadnode = (btree_high >> ((15 - i) << 1)) & 0x03;
    char ch = '0' + local_quadnode;
    address += ch;
  }

  for (int i = 0; i < std::min(8, level - 16); ++i) {
    int local_quadnode = (btree_low >> ((7 - i) << 1)) & 0x03;
    char ch = '0' + local_quadnode;
    address += ch;
  }

  return address;
}


// Constants associated with packetbundles.
// Set really high so we never get more than one bundle
// for single file package.
const std::uint64_t PacketBundle::kMaxFileSize = 0x7fffffffffffffffLLU;
const std::string PacketBundle::kIndexFile = "index";
const std::string PacketBundle::kPacketbundlePrefix = "pbundle_";
const std::string PacketBundle::kDirectoryDelimiter = "/";

/**
 * Returns packetbundle file's name for the given file id.
 */
std::string PacketBundle::PacketBundleFileName(std::uint16_t file_id) const {
  char str[8];
  snprintf(str, sizeof(str), "%04d", file_id);
  return directory_ + kDirectoryDelimiter + kPacketbundlePrefix + str;
}

/**
 * Returns index file's name.
 */
std::string PacketBundle::IndexFileName() const {
  return directory_ + kDirectoryDelimiter + kIndexFile;
}

}  // namespace fusion_portableglobe
