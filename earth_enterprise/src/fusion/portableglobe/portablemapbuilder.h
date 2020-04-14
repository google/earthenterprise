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


// Classes for pruning away all quadtree nodes that are not at or below
// the given default level or encompassing the given set of
// quadtree nodes. Data referenced by quadtree nodes that are not pruned
// are stored in packet bundles.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PORTABLEMAPBUILDER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PORTABLEMAPBUILDER_H_

#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include "common/qtpacket/quadtreepacket.h"
#include "fusion/gst/gstSimpleStream.h"
#include "fusion/portableglobe/portableglobebuilder.h"
#include "fusion/portableglobe/shared/packetbundle_reader.h"
#include "fusion/portableglobe/shared/packetbundle_writer.h"

namespace fusion_portableglobe {

class LayerInfo {
  public:
    const std::string type;
    const std::string channel;
    const std::string version;
    // Same channel as above but in integer form.
    std::uint32_t channel_num;
    // Type converted to integer form.
    std::uint32_t type_id;

    /**
     * Constructor
     */
    LayerInfo(const std::string& type_,
              const std::string& channel_,
              const std::string& version_) :
              type(type_),
              channel(channel_),
              version(version_) {
      sscanf(channel.c_str(), "%u", &channel_num);
      if ((type_ == "ImageryMaps") || (type_ == "ImageryMapsMercator")) {
        type_id = kImagePacket;
      } else {
        type_id = kVectorPacket;
      }
    }
};

/**
 * Class for constructing a portable globe.
 * Traverses quadtree packets and prunes out the ones that
 * are not required. Writes all of packet bundles of the nodes
 * that are kept.
 */
class PortableMapBuilder {
 public:
  // Names of subdirectories within the portable globe.
  static const std::string kMapDataDirectory;
  static const size_t      kBundleSizeForSizeQuery;
  static const size_t      kBundleSizeForPacketQuery;

  // Used to accumulate size during a "no write" run.
  std::uint64_t total_size;
  std::uint64_t num_image_packets;
  std::uint64_t num_terrain_packets;
  std::uint64_t num_vector_packets;
  std::uint64_t image_size;
  std::uint64_t terrain_size;
  std::uint64_t vector_size;

  /**
   * Production constructor.
   */
  PortableMapBuilder(const std::string& source,
                     std::uint16_t default_level,
                     std::uint16_t max_level,
                     const std::string& hires_qt_nodes_file,
                     const std::string& map_directory,
                     const std::string& additional_args,
                     const std::string& metadata_file,
                     bool ignore_imagery_depth,
                     bool no_write);

  // For testing ...
  PortableMapBuilder():server_(NULL) {}

  ~PortableMapBuilder();

  /**
   * Build packet bundles for all specified data types. Add data based on
   * whether it is available for the 2d map.
   */
  void BuildMap();

  /**
   * Returns whether quadtree node and the data it points to should be
   * included in the portable globe.
   */
  bool KeepNode(const std::string& qtpath) const;

 private:
  static const std::uint16_t kMaxPacketDepth;

  void AddWriter(PacketType packet_type,
                 PacketBundleWriter* writer);

  void WritePacket(PacketType packet_type,
                   std::string qtpath, std::string data);

  void GetLevelRowCol(const std::string& qtpath_str,
                      std::uint32_t* level,
                      std::uint32_t* col,
                      std::uint32_t* row);

  friend class PortableMapBuilderTest_TestJsExtract_Test;
  std::string ExtractValue(const std::string& str,
                           const std::string& key);
  void ParseServerDefs(const std::string& json);

  bool WriteMapPackets(const std::string& qtpath_str,
                       std::uint32_t level,
                       std::uint32_t col,
                       std::uint32_t row);

  bool WriteImagePacket(const std::string& qtpath,
                        const std::string&  channel,
                        const std::string&  version,
                        std::uint32_t level,
                        std::uint32_t col,
                        std::uint32_t row);

  bool WriteVectorPacket(const std::string& qtpath,
                         const std::string& channel,
                         const std::string& version,
                         std::uint32_t level,
                         std::uint32_t col,
                         std::uint32_t row);

  void AddWriter(const std::string& sub_directory);

  void DeleteWriter();

  // Main directory that contains all portable globe data.
  std::string map_directory_;
  // Level at or below which all assets are kept.
  std::uint16_t default_level_;
  // Level above which no assets are kept.
  std::uint16_t max_level_;
  // Url to master globe server.
  const std::string source_;
  // Additional args to be appended to each query.
  std::string additional_args_;
  // Master map server reader service.
  gstSimpleStream* server_;
  // Forest of trees identifying quadtree nodes to keep.
  HiresTree* hires_tree_;

  // Info about layers within the map.
  std::vector<LayerInfo*> layers_;

  // Packet bundle writers for the different asset types.
  PacketBundleWriter* writer_;

  // Path to file that will hold metadata.
  std::string metadata_file_;
  // Structure to keep track of map boundaries.
  BoundsTracker bounds_tracker_;

  // Flag indicating whether Cutter should continue looking for (vector) data
  // or stop cut in that region when end of imagery data is found.
  bool ignore_imagery_depth_;
  // Flag indicating whether data shouldn't be written, just checked for size.
  bool is_no_write_;


  DISALLOW_COPY_AND_ASSIGN(PortableMapBuilder);
};

}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PORTABLEMAPBUILDER_H_
