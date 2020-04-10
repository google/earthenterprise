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

// Classes reading and writing filebundles for the portable server.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_PACKETBUNDLE_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_PACKETBUNDLE_H_

#include <fstream>  // NOLINT(readability/streams)
#include <string>
#include <vector>

namespace fusion_portableglobe {

// We have a similar enum in other places, but I am adding dbroot here.
// Order needs to match the order of the packetbundle writer if
// packet types are mixed within a single bundle.
// Start at 1 so everything is greater than 0 for initial conditions
// of packet order comparison.
enum PacketType {
  kDbRootPacket = 1,
  kDbRoot2Packet,
  kQtpPacket,
  kQtp2Packet,
  kImagePacket,
  kTerrainPacket,
  kVectorPacket,
};


/**
 * Class for index entries that describe location of packets in the
 * packet bundle files.
 */
class IndexItem {
 public:
  // Order matters here. Do NOT cross 64 bit boundaries,
  // or the structure may be laid out differently of 32-bit
  // and 64-bit machines.
  std::uint32_t btree_high;
  std::uint16_t btree_low;
  std::uint8_t level;
  std::uint8_t packet_type;
  // Channel is vector channel or time machine date channel.
  std::uint16_t channel;
  std::uint16_t file_id;
  std::uint32_t packet_size;
  std::uint64_t offset;

  bool operator==(const IndexItem& other) const;
  bool operator<(const IndexItem& other) const;

  /**
   * Fills in values for an index item.
   */
  void Fill(const std::string qtpath, std::uint8_t packet_type, std::uint16_t channel);
  /**
   * Get quadtree address (with leading 0) as string.
   */
  std::string QuadtreeAddress() const;
};

/**
 * Class containing constants for the reader and writer. Should not
 * be instantiated directly, but rather derived from a reader or a
 * writer.
 */
class PacketBundle {
 public:
  static const std::uint64_t kMaxFileSize;                 // 0x7fffffffffffffff;
  static const std::string kPacketbundlePrefix;     // "pbundle_"
  static const std::string kDirectoryDelimiter;     // "/"

  virtual ~PacketBundle() { }

 protected:
  explicit PacketBundle(const std::string& directory)
      : directory_(directory) { }

  PacketBundle() { }

  /**
   * Returns packetbundle file's name for the given file id.
   */
  std::string PacketBundleFileName(std::uint16_t file_id) const;

  /**
   * Returns index file's name.
   */
  std::string IndexFileName() const;

  static const std::string kIndexFile;              // "index"

  std::string directory_;
};

}  // namespace fusion_portableglobe


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_PACKETBUNDLE_H_
