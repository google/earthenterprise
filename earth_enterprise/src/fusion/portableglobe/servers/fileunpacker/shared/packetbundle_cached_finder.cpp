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

// Depth shouldn't exceed number of bits in binary search.
const int PacketBundleFinder::MAX_CACHE_SIZE = 64;

/**
 * Constructor sets the packetbundle's directory and initializes index info.
 */
PacketBundleFinder::PacketBundleFinder(const GlcReader& glc_reader,
                                       std::uint64_t index_offset,
                                       std::uint64_t index_size)
    : glc_reader_(glc_reader),
    index_offset_(index_offset),
    index_size_(index_size),
    num_index_items_(index_size_ / sizeof(IndexItem)) {
  // Fill with impossible value (assuming less than 2^64 packets).
  const std::uint64_t ILLEGAL_LOCATION = 0xffffffffffffffffLLU;
  index_cache_location_.assign(MAX_CACHE_SIZE, ILLEGAL_LOCATION);
  index_cache_.resize(MAX_CACHE_SIZE);
}

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
  std::uint64_t top = num_index_items_ - 1;
  std::uint64_t bottom = 0;
  // Keep track of depth in binary search for the cache.
  int i = 0;
  while (top >= bottom) {
    // Look in the middle.
    std::uint64_t idx = bottom + ((top - bottom) >> 1);
    // If it's not in our cache, load it.
    // TODO: start lock here for thread safety.
    if (idx != index_cache_location_[i]) {
      index_cache_location_[i] = idx;
      std::uint64_t offset = index_offset_ + idx * sizeof(IndexItem);
      glc_reader_.ReadData(reinterpret_cast<char*>(&index_cache_[i]),
                           offset, sizeof(IndexItem));
    }

    // If we found the item, get the file id and offset.
    if (*index_item == index_cache_[i]) {
      index_item->SetLookupInfo(index_cache_[i]);
      // TODO: release lock here for thread safety.
      return true;
    }

    // If we are too high, search bottom half.
    if (*index_item < index_cache_[i]) {
      top = idx - 1;

    // TODO: release lock here for thread safety.

    // If we are too low, search top half.
    } else {
      bottom = idx + 1;
    }

    ++i;
  }

  return false;
}

