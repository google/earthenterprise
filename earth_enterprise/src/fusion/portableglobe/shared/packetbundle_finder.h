/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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


// Class for finding packets in index of filebundles for the portable server.
// This class is used for deployed portable globes and should minimize
// dependencies so that it can be easily compiled on multiple platforms.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_PACKETBUNDLE_FINDER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_PACKETBUNDLE_FINDER_H_

#include <fstream>  // NOLINT(readability/streams)
#include <string>
#include <vector>
#include "fusion/portableglobe/shared/packetbundle.h"

namespace fusion_portableglobe {

/**
 * Class for reading in packets from a directory of packet bundle
 * files and their associated index.
 *
 * TODO: Add a second finder that implements FindPacketInIndex
 * TODO: by cacheing the index in memory. E.g. for qt packets.
 * TODO: depending on available ram.
 */
class PacketBundleFinder : public PacketBundle {
 public:
  PacketBundleFinder() { }
  PacketBundleFinder(std::ifstream* source,
                     std::uint64_t index_offset,
                     std::uint64_t index_size);

  /**
   * Find the index in the index file. Set index item fields if it
   * is found.
   * @param index_item Index item with btree, level, packet_type, offset and
   * packet size info set.
   * @return whether the index item was found.
   */
  bool FindPacketInIndex(IndexItem* index_item);

 private:
  std::ifstream* source_;
  std::uint64_t index_offset_;
  std::uint64_t index_size_;

  std::uint64_t num_index_items_;
  std::vector<IndexItem> index_cache_;
};

}  // namespace fusion_portableglobe


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_PACKETBUNDLE_FINDER_H_
