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
#include "./glc_reader.h"
#include "./packetbundle.h"
#include "./packetbundle_finder.h"

/**
 * Constructor sets the packetbundle's directory and initializes index info.
 */
PacketBundleFinder::PacketBundleFinder(const GlcReader& glc_reader,
                                       std::uint64_t index_offset,
                                       std::uint64_t index_size)
    : glc_reader_(glc_reader),
    index_offset_(index_offset),
    index_size_(index_size),
    num_index_items_(index_size_ / sizeof(IndexItem)) { }

/**
 * Find the index in the index file. We know that the packets are in order.
 * @param index_item Index item with btree, level, and packet_type info set.
 * @return whether the index item was found.
 */
bool PacketBundleFinder::FindPacketInIndex(IndexItem* index_item) {
  if (num_index_items_ == 0) {
    return false;
  }

  // Do a binary search for the index item.
  IndexItem next_item;
  std::uint64_t top = num_index_items_ - 1;
  std::uint64_t bottom = 0;
  while (top >= bottom) {
    // Look in the middle.
    std::uint64_t idx = bottom + ((top - bottom) >> 1);
    std::uint64_t offset = index_offset_ + idx * sizeof(IndexItem);
    glc_reader_.ReadData(reinterpret_cast<char*>(&next_item),
                          offset, sizeof(IndexItem));

    // If we found the item, get the file id and offset.
    if (*index_item == next_item) {
      index_item->file_id = next_item.file_id;
      index_item->offset = next_item.offset;
      index_item->packet_size = next_item.packet_size;
      return true;
    }

    // If we are too high, search bottom half.
    if (*index_item < next_item) {
      if (idx == 0) {
        return false;
      }

      top = idx - 1;

    // If we are too low, search top half.
    } else {
      bottom = idx + 1;
    }
  }

  return false;
}


bool PacketBundleFinder::MapDataPacketWalker(int layer, const map_packet_walker& walker)
{
  IndexItem next_item;
  for(std::uint64_t idx = 0; idx < num_index_items_; ++idx) {
    std::uint64_t offset = index_offset_ + idx * sizeof(IndexItem);
    glc_reader_.ReadData(reinterpret_cast<char*>(&next_item),
                         offset, sizeof(IndexItem));
    if ( walker(layer, next_item) == true ) {
      return true;
    }
  }

  return false;
}

