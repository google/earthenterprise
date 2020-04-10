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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_PACKETBUNDLE_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_PACKETBUNDLE_H_

#include <fstream>  // NOLINT(readability/streams)
#include <string>
#include <vector>

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

  /**
   * Returns whether the given IndexItem points to the same packet.
   */
  bool operator==(const IndexItem& other);

  /**
   * Returns whether the given IndexItem points to a different packet
   * that follows this one with respect to its quadtree address and
   * channel information.
   */
  bool operator<(const IndexItem& other);

  /**
   * Sets the information needed to read the packet from a file.
   * I.e. file_id, offset, and packet_size
   */
  void SetLookupInfo(const IndexItem& other);

  /**
   * Fills in values for an index item.
   */
  void Fill(const std::string& qtpath, std::uint8_t packet_type, std::uint16_t channel);

  // Special value for packet_type indicating channel
  // and type  should be ignored.
  static const std::uint8_t kIgnoreChannelAndType;

  // Maximum level (lod) supported by the indexing system.
  static const std::uint8_t kMaxLevel;
};

/**
 * Class containing constants for the reader and writer. Should not
 * be instantiated directly, but rather derived from a reader or a
 * writer.
 */
class PacketBundle {
 protected:
  /**
   * Returns packetbundle file's name for the given file id.
   */
  std::string PacketBundleFileName(std::uint16_t file_id) const;

  /**
   * Returns index file's name.
   */
  std::string IndexFileName() const;

  PacketBundle() { }
  static const std::uint64_t kMaxFileSize;                 // 0x7fffffffffffffff;
  static const std::string kIndexFile;              // "index"
  static const std::string kPacketbundlePrefix;     // "pbundle_"
  static const std::string kDirectoryDelimiter;     // "/"

  std::string directory_;
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_PACKETBUNDLE_H_
