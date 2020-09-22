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

// Class for unpacking packets and files from from a single package file.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_FILE_UNPACKER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_FILE_UNPACKER_H_

#include <fstream>  // NOLINT(readability/streams)
#include <map>
#include <string>
#include <vector>
#include "common/khConstants.h"
#include "fusion/portableglobe/shared/file_package.h"
#include "fusion/portableglobe/shared/packetbundle_finder.h"

namespace fusion_portableglobe {

/**
 * Class for unpacking files from a single package file.
 */
class FileUnpacker {
 public:
  explicit FileUnpacker(const char* package_file);

  /**
   * Find data packet and set offset and size for the packet. Data packets can
   * be imagery, terrain or vectors.
   * @return whether data packet was found.
   */
  bool FindDataPacket(const char* qtpath, int packet_type, int channel);

  /**
   * Find map data packet and set offset and size for the packet. Data packets
   * can be map imagery or vectors.
   * @return whether data packet was found.
   */
  bool FindMapDataPacket(const char* qtpath, int packet_type, int channel);

  /**
   * Find qtp packet and set offset and size for the packet. Qtp packets can
   * be quadtree packets or a dbroot packet.
   * @return whether qtp packet was found.
   */
  bool FindQtpPacket(const char* qtpath, int packet_type, int channel);

  /**
   * Find file and set offset and size for the file content.
   * @return whether file was found.
   */
  bool FindFile(const char* file_name);

  /**
   * Get low 32 bits of offset to file or packet that was last found.
   */
  int LowOffset() { return (offset_ & 0x000000000ffffffff); }

  /**
   * Get high 32 bits of offset to file or packet that was last found.
   */
  int HighOffset() { return ((offset_ >> 32) & 0x000000000ffffffff); }

  /**
   * Get offset to file or packet that was last found.
   */
  std::uint64_t Offset() { return offset_; }

  /**
   * Get size of file or packet that was last found.
   */
  int Size() { return size_; }

  /**
   * Get idx-th file in index.
   */
  const char* IndexFile(int idx);

  /**
   * Get path of file in index.
   */
  int IndexSize() { return index_.size(); }

  /**
   * Return the files id. The id is based on contents of the file
   * stored at earth/info.txt and the file's length.
   */
  const char* Id();

  /**
   * Return the data from earth/info.txt. Returns an empty string
   * if there isn't any or the file is not contained within the
   * bundle.
   */
  const char* Info() const { return info_.c_str(); }

  /**
   * Get the index.
   */
  const std::map<std::string, PackageFileLoc>& Index() { return index_; }

 private:
  std::string ExtractDateFromInfo() const;
  std::uint32_t InfoCrc() const;

  // Length of full packed file.
  std::uint64_t length_;
  std::string info_;
  std::string id_;

  std::map<std::string, PackageFileLoc> index_;
  std::ifstream source_;
  std::uint64_t offset_;
  std::uint32_t size_;

  PackageFileLoc data_packet_index_loc_;
  PackageFileLoc data_packet_file_loc_;
  PackageFileLoc mapdata_packet_index_loc_;
  PackageFileLoc mapdata_packet_file_loc_;
  PackageFileLoc qtp_packet_index_loc_;
  PackageFileLoc qtp_packet_file_loc_;

  PacketBundleFinder data_packet_finder_;
  PacketBundleFinder mapdata_packet_finder_;
  PacketBundleFinder qtp_packet_finder_;

  bool has_3d_data_;
  bool has_2d_data_;
};

}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_FILE_UNPACKER_H_
