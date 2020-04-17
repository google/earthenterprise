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


// Class for packing up a globe into a single file.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_FILE_UNPACKER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_FILE_UNPACKER_H_

#include <fstream>  // NOLINT(readability/streams)
#include <map>
#include <string>
#include <vector>
#include "./file_package.h"
#include "./glc_reader.h"
#include "./packetbundle_finder.h"
#include "./callbacks.h"

class GlcReader;

/**
 * Class for unpacking files from a single package file.
 */
class FileUnpacker {
 public:
  FileUnpacker(const GlcReader& glc_reader, std::uint64_t offset, std::uint64_t size);
  ~FileUnpacker();
  FileUnpacker(const FileUnpacker&) = delete;
  FileUnpacker(FileUnpacker&&) = delete;
  FileUnpacker& operator=(const FileUnpacker&) = delete;
  FileUnpacker& operator=(FileUnpacker&&) = delete;

  /**
   * Find data packet and set offset and size for the packet. Data packets can
   * be imagery, terrain or vectors.
   * @return whether data packet was found.
   */
  bool FindDataPacket(const char* qtpath,
                      int packet_type,
                      int channel,
                      PackageFileLoc* data_loc);

  /**
   * Find map data packet and set offset and size for the packet. Data packets
   * can be map imagery or vectors.
   * @return whether data packet was found.
   */
  bool FindMapDataPacket(const char* qtpath,
                         int packet_type,
                         int channel,
                         PackageFileLoc* data_loc);

  bool MapDataPacketWalker(int layer, const map_packet_walker& walker);

  /**
   * Find qtp packet and set offset and size for the packet. Qtp packets can
   * be quadtree packets or a dbroot packet.
   * @return whether qtp packet was found.
   */
  bool FindQtpPacket(const char* qtpath,
                     int packet_type,
                     int channel,
                     PackageFileLoc* data_loc);

  /**
   * Returns whether there are any data at the given address.
   */
  bool HasData(const char* qtpath, bool is_2d);

  /**
   * Find dbroot and set offset and size for the content.
   * @param data_loc Returns location of dbroot data.
   * @return whether dbroot was found.
   */
  bool FindDbRoot(PackageFileLoc* data_loc);

  /**
   * Find file and set offset and size for the file content.
   * @return whether file was found.
   */
  bool FindFile(const char* file_name, PackageFileLoc* file_loc);

  /**
   * Get idx-th file in index.
   */
  const char* IndexFile(int idx);

  /**
   * Get path of file in index.
   */
  int IndexSize() { return index_.size(); }

  /**
   * Get the index for debugging tools.
   */
  const std::map<std::string, PackageFileLoc>& Index() { return index_; }

 private:
  /**
   * Helper for reading in consecutive fields from the glc file.
   */
  bool Read(void* data, std::uint64_t size);

  const GlcReader& glc_reader_;
  std::uint64_t base_offset_;  // Base offset to package within the composite.
  std::uint64_t base_size_;    // Size of package within the composite.

  std::map<std::string, PackageFileLoc> index_;

  PackageFileLoc data_packet_index_loc_;
  std::vector<PackageFileLoc> data_packet_file_loc_;
  PackageFileLoc mapdata_packet_index_loc_;
  std::vector<PackageFileLoc> mapdata_packet_file_loc_;
  PackageFileLoc qtp_packet_index_loc_;
  PackageFileLoc qtp_packet_file_loc_;

  PacketBundleFinder* data_packet_finder_;
  PacketBundleFinder* mapdata_packet_finder_;
  PacketBundleFinder* qtp_packet_finder_;

  bool has_3d_data_;
  bool has_2d_data_;

  std::string dbroot_path_;

  // Offset for consecutive reads from the reader.
  std::uint64_t reader_offset_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_FILE_UNPACKER_H_

