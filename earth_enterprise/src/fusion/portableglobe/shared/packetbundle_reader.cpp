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
#include "common/khFileUtils.h"
#include "fusion/portableglobe/shared/packetbundle.h"
#include "fusion/portableglobe/shared/packetbundle_reader.h"

namespace fusion_portableglobe {

/**
 * Constructor sets the packetbundle's directory and initializes index info.
 */
PacketBundleReader::PacketBundleReader(std::string directory)
    : PacketBundle(directory) {

  // Determine number of index items (i.e. number of packets) in the index.
  std::ifstream index_file;
  index_file.open(IndexFileName().c_str(), std::ios::binary);

  index_file.seekg(0, std::ios::end);
  num_index_items_ =  static_cast<int>(index_file.tellg() / sizeof(IndexItem));

  // TODO: Set up file pool for index and bundle files.
}

/**
 * Find location of packet in the index, and then read in the packet
 * into the data string.
 */
bool PacketBundleReader::ReadPacket(std::string qtpath,
                                    std::uint8_t packet_type,
                                    std::uint16_t channel,
                                    std::string* data) {
  IndexItem index_item;
  index_item.Fill(qtpath, packet_type, channel);
  if (FindPacketInIndex(&index_item)) {
    std::ifstream packet_file;
    packet_file.open(PacketBundleFileName(index_item.file_id).c_str(),
                                          std::ios::binary);

    packet_file.seekg(index_item.offset, std::ios::beg);

    // In practice, we might want to reserve MAX_PACKET_SIZE for this var.
    data->resize(index_item.packet_size);
    packet_file.read(&(*data)[0], index_item.packet_size);

    packet_file.close();
    return true;
  }

  return false;
}

/**
 * Find the index in the index file. We know that the packets are in order.
 * @param index_item Index item with btree, level, and packet_type info set.
 * @return whether the index item was found.
 */
bool PacketBundleReader::FindPacketInIndex(IndexItem* index_item) {
  if (num_index_items_ == 0) {
    return false;
  }

  std::ifstream index_file;
  index_file.open(IndexFileName().c_str(), std::ios::binary);

  // Do a binary search for the index item.
  IndexItem next_item;
  std::int32_t top = num_index_items_ - 1;
  std::int32_t bottom = 0;
  while (top >= bottom) {
    // Look in the middle.
    std::int32_t idx = bottom + ((top - bottom) >> 1);
    index_file.seekg(idx * sizeof(IndexItem), std::ios::beg);
    index_file.read(reinterpret_cast<char*>(&next_item), sizeof(IndexItem));

    // If we found the item, get the file id and offset.
    if (*index_item == next_item) {
      index_item->file_id = next_item.file_id;
      index_item->offset = next_item.offset;
      index_item->packet_size = next_item.packet_size;
      index_file.close();
      return true;
    }

    // If we are too high, search bottom half.
    if (*index_item < next_item) {
      top = idx - 1;

    // If we are too low, search top half.
    } else {
      bottom = idx + 1;
    }
  }

  index_file.close();
  return false;
}

/**
 * Find the index in the index file. Search the whole index just in case one
 * of the items is out of order.
 * @param index_item Index item with btree, level, and packet_type info set.
 * @return whether the index item was found.
 */
bool PacketBundleReader::BruteForceFindPacketInIndexForDebug(
    IndexItem* index_item) {
  std::ifstream index_file;
  index_file.open(IndexFileName().c_str(), std::ios::binary);

  // Sequential search.
  IndexItem next_item;
  for (std::uint32_t i = 0; i < num_index_items_; ++i) {
    index_file.read(reinterpret_cast<char*>(&next_item), sizeof(IndexItem));

    // If we found the item, get the file id and offset.
    if (*index_item == next_item) {
      index_item->file_id = next_item.file_id;
      index_item->offset = next_item.offset;
      index_file.close();
      std::cout << i << " searches." << std::endl;
      return true;
    }
  }

  index_file.close();
  return false;
}

}  // namespace fusion_portableglobe
