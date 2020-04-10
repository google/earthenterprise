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

// Classes writing filebundles for the portable server. This class is
// used for portable globe creation and should reside on Linux with
// access to Fusion libraries.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_PACKETBUNDLE_WRITER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_PACKETBUNDLE_WRITER_H_

#include <fstream>  // NOLINT(readability/streams)
#include <string>
#include <vector>
//#include "common/khTypes.h"
#include <cstdint>
#include "fusion/portableglobe/shared/packetbundle.h"
#include "fusion/portableglobe/shared/file_packer.h"
#include "fusion/portableglobe/shared/file_unpacker.h"

namespace fusion_portableglobe {

/**
 * Class for writing packets to packet bundle files and their
 * associated index.
 */
class PacketBundleWriter : public PacketBundle {
 public:
  /**
   * The program is responsible for ensuring the existence of the given
   * directory before instantiating a packet bundle writer.
   */

  /**
   * Constructor for a packet bundle writer with a particular
   * file index.
   */
  PacketBundleWriter(const std::string& directory,
                     std::uint64_t max_file_size = kMaxFileSize,
                     std::uint16_t file_id = 0);
  /**
   * Constructor for a delta packet bundle writer with a particular
   * file index. If a packet is only written if it does not exist
   * or is different from the one in the base file.
   */
  PacketBundleWriter(const std::string& directory,
                     const std::string& base_file,
                     bool is_qtp_bundle = false,
                     std::uint64_t max_file_size = kMaxFileSize,
                     std::uint16_t file_id = 0);
  ~PacketBundleWriter();

  /**
   * Append packet to the next position in the open packetbundle. If that
   * packetbundle is full, close it and start a new one.
   */
  bool AppendPacket(const std::string qtpath,
                    std::uint8_t packet_type,
                    std::uint16_t channel,
                    const std::string& data) {
    return AppendPacket(qtpath, packet_type, channel,
                        data.c_str(), data.size());
  }

  bool AppendPacket(const std::string qtpath,
                    std::uint8_t packet_type,
                    std::uint16_t channel,
                    const char* const data,
                    const size_t size);

 private:
  void Init();

  IncrementalCrcCalculator crc_calculator_;
  CrcWrappingOfstream packet_file_;
  std::ofstream index_file_;

  // Id of the packetbundle file we are currently writing.
  std::uint16_t file_id_;
  // Maximum size of the packetbundle file before starting a new file.
  std::uint64_t max_file_size_;
  // Size of the packetbundle file we are currently writing.
  std::uint64_t current_file_size_;
  // Keep track of last index item so we can detect if a packet
  // somehow comes out of order. Then we fail.
  IndexItem last_index_item_;
  // Unpacker used to check if packets should be added to delta.
  FileUnpacker* unpacker_;
  // Whether bundle is a quadtree packet bundle.
  bool is_qtp_bundle_;
  // Whether bundle being created is a delta bundle.
  bool is_delta_;
  // Input stream from which base file data can be read.
  std::ifstream base_file_ptr_;
  // Buffer for reading in packet data.
  std::string packet_data_;
};

}  // namespace fusion_portableglobe


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_PACKETBUNDLE_WRITER_H_
