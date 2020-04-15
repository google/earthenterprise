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


#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include "./packetbundle.h"

const std::uint8_t IndexItem::kIgnoreChannelAndType = 255;
const std::uint8_t IndexItem::kMaxLevel = 24;

/**
 * Equality operator for index items. Necessary for finding the index
 * item that we are searching for.
 */
bool IndexItem::operator==(const IndexItem& other) {
  return (btree_high == other.btree_high) &&
         (btree_low == other.btree_low) &&
         (level == other.level) &&
         ((packet_type == kIgnoreChannelAndType) ||
         ((packet_type == other.packet_type) &&
          (channel == other.channel)));
}

/**
 * Less Than operator for index items. Necessary for search for the index
 * item that in an ordered list of index items.
 */
bool IndexItem::operator<(const IndexItem& other) {
  if (btree_high < other.btree_high) {
    return true;
  } else if (btree_high == other.btree_high) {
    if (btree_low < other.btree_low) {
      return true;
    } else if (btree_low == other.btree_low) {
      if (level < other.level) {
        return true;
      } else if (level == other.level) {
        if (packet_type == kIgnoreChannelAndType) {
          return false;
        } else if (packet_type < other.packet_type) {
          return true;
        } else if (packet_type == other.packet_type) {
          return (channel < other.channel);
        }
      }
    }
  }

  return false;
}

/**
 * Sets the information needed to read the packet from a file.
 */
void IndexItem::SetLookupInfo(const IndexItem& other) {
  file_id = other.file_id;
  offset = other.offset;
  packet_size = other.packet_size;
}

/**
 * Fills in values for an index item.
 * TODO: Derive btree and level values from a qt packet string.
 */
void IndexItem::Fill(
    const std::string& qtpath, std::uint8_t packet_type, std::uint16_t channel) {
  level = qtpath.size() - 1;
  // Not much we can do with the current structure except ignore the rest of
  // the string.
  if (level > kMaxLevel) {
    level = kMaxLevel;
  }
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
