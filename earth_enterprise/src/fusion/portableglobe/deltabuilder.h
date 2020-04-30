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


// Classes for creating the files for a delta glb given the files for the
// original glb and the latest glb.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_DELTABUILDER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_DELTABUILDER_H_

#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include <cstdint>
#include "common/khTypes.h"
#include "fusion/portableglobe/shared/packetbundle.h"

namespace fusion_portableglobe {

class PacketBundleSource : public PacketBundle {
 public:
  explicit PacketBundleSource(const std::string &directory) {
    directory_ = directory + "/data";
  }
  /**
   * Returns packetbundle file's name for the given file id.
   */
  std::string SourcePacketBundleFileName(std::uint16_t file_id) const {
    return PacketBundleFileName(file_id);
  }

  /**
   * Returns index file's name.
   */
  std::string SourceIndexFileName() const {
    return IndexFileName();
  }

  /**
   * Returns number of packets referenced in index.
   */
  std::uint64_t SourceIndexSize() const {
    std::ifstream fp;

    fp.open(SourceIndexFileName().c_str(), std::ios::binary);
    fp.seekg(0, std::ios::end);
    std::int64_t file_size = fp.tellg();
    if (file_size < 0) {
      return 0;
    }
    return file_size / sizeof(IndexItem);
  }
};

/**
 * Class for constructing a portable globe.
 * Traverses quadtree packets and prunes out the ones that
 * are not required. Writes all of packet bundles of the nodes
 * that are kept.
 * TODO: Always use gepartialglobebuilder.
 * TODO: I.e. Remove geportableglobebuilder.
 */
class PortableDeltaBuilder {
 public:
  /**
   * Production constructor.
   */
  PortableDeltaBuilder(const std::string& original_directory,
                       const std::string& latest_directory,
                       const std::string& delta_directory);

  ~PortableDeltaBuilder();

  /**
   * Display nodes that were added to the delta index. This is mostly
   * for debugging.
   */
  void DisplayDeltaQtNodes();

  /**
   * Build packet bundle for all data types between the given starting
   * and ending quadtree addresses. Add data based on
   * whether corresponding quadtree node is to be kept in the globe.
   */
  void BuildDeltaGlobe();

 private:
  void LoadPacket(
      const char* file_name, const IndexItem& index_item, std::string* buffer);

  bool IsLatestPacketDifferent(
      const IndexItem& original_index_item,
      const std::ifstream& original_index,
      std::string* original_buffer,
      const IndexItem& latest_index_item,
      const std::ifstream& latest_index,
      std::string* latest_buffer,
      std::string* qtpath,
      bool* read_original_data);

  // Directory where delta files should be created.
  std::string delta_directory_;

  // Source of files from the original cut for the glb
  // that is out in the field.
  PacketBundleSource original_;
  // Source of files from the latest cut for the glb.
  PacketBundleSource latest_;
};

}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_DELTABUILDER_H_
