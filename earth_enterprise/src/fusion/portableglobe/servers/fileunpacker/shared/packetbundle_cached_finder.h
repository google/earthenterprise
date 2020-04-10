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


// Classes for finding packets in index of filebundles for the portable server.
// This class is used for deployed portable globes and should minimize
// dependencies so that it can be easily compiled on multiple platforms.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_PACKETBUNDLE_FINDER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_PACKETBUNDLE_FINDER_H_

#include <fstream>  // NOLINT(readability/streams)
#include <string>
#include <vector>
#include "./packetbundle.h"

class GlcReader;

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
  PacketBundleFinder(
      const GlcReader& glc_reader, std::uint64_t index_offset, std::uint64_t index_size);

  /**
   * Find the index in the index file. Set index item fields if it
   * is found.
   * @param reader Object for safely reading the packet file.
   * @param index_item Index item with btree, level, packet_type, offset and
   * packet size info set.
   * @return whether the index item was found.
   */
  bool FindPacketInIndex(IndexItem* index_item);

 private:
  static const int MAX_CACHE_SIZE;
  const GlcReader& glc_reader_;
  std::uint64_t index_offset_;
  std::uint64_t index_size_;
  std::uint64_t num_index_items_;

  // The cache uses a very simple idea: if you are looking at a particular region,
  // most of your binary search through the packet index will be indentical to what
  // it was for the previous packet. Since this is very cheap in terms of memory,
  // i.e. O(log2(# of packets)), it should be fine for multi-globe situations.
  // If you are on a large quadtree boundary, the cache may not help much, but it
  // shouldn't hurt performance either.
  // Assumes that the number of packets will not exceed 2^64 - 1.
  // Assumes that the globe does not change underneath it.
  // Both of these assumptions are already made in other parts of the code.
  // The cache is not thread safe. Using the cache in a multi-threaded context
  // might not be very useful since it defeats the local neighborhood idea if
  // lots of people are looking at different parts of the globe at the same time.
  std::vector<std::uint64_t> index_cache_location_;
  std::vector<IndexItem> index_cache_;
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_PACKETBUNDLE_FINDER_H_

