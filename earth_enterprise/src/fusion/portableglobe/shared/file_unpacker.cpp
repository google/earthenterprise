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

// Methods for packing up all files in a globe into a single
// package file.

#include "fusion/portableglobe/shared/file_unpacker.h"
#include <notify.h>
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <map>
#include <sstream>  // NOLINT(readability/streams)
#include <string>
#include "common/khConstants.h"
#include "common/khEndian.h"
#include "common/khFileUtils.h"
#include "fusion/portableglobe/shared/file_package.h"
#include "fusion/portableglobe/shared/packetbundle_finder.h"

namespace fusion_portableglobe {

/**
 * Constructor. Load the index to all of the files and
 * packets in the package file.
 */
FileUnpacker::FileUnpacker(const char* package_file) {
  // Get length of file.
  length_ = Package::FileSize(package_file);
  if (!length_) {
    std::cout << "Unable to find: " << package_file << std::endl;
    return;
  }

  // Get offset to the index
  source_.open(package_file, std::ios::binary);
  source_.seekg(length_ - Package::kIndexOffsetOffset, std::ios::beg);
  std::uint64_t index_offset;
  source_.read(reinterpret_cast<char*>(&index_offset),
               Package::kIndexOffsetSize);

  // Read in the index header.
  source_.seekg(index_offset, std::ios::beg);
  std::uint32_t num_files;
  source_.read(reinterpret_cast<char*>(&num_files), 4);

  std::map<std::string, PackageFileLoc>::iterator it;

  // Read in the index entries.
  std::string path;
  PackageFileLoc file_loc;
  for (std::uint32_t i = 0; i < num_files; ++i) {
    // Read in the relative path of the index entry.
    std::uint16_t path_len;
    source_.read(reinterpret_cast<char*>(&path_len),
                 sizeof(path_len));
    path.resize(path_len);
    source_.read(&path[0], path_len);
    // Read in the size and offset of the index entry.
    source_.read(reinterpret_cast<char*>(&file_loc), sizeof(PackageFileLoc));
    it = index_.find(path);
    if (it != index_.end()) {
      std::cout << "Replacing existing " << path << std::endl;
    }
    index_[path] = file_loc;
  }

  // Read in earth/info.txt
  it = index_.find("earth/info.txt");
  if (it != index_.end()) {
    file_loc = it->second;
    info_.resize(file_loc.Size());
    source_.seekg(file_loc.Offset(), std::ios::beg);
    source_.read(&info_[0], file_loc.Size());
  }

  // Get names of the data packet and index files.
  std::string packet_file = "data/pbundle_0000";
  std::string packet_index = "data/index";

  // Get location of the packet file.
  it = index_.find(packet_file);
  if (it != index_.end()) {
    data_packet_file_loc_ = it->second;
    has_3d_data_ = true;
  } else {
    has_3d_data_ = false;
  }

  // Get locations-of-the-packet index.
  if (has_3d_data_) {
    it = index_.find(packet_index);
    if (it != index_.end()) {
      data_packet_index_loc_ = it->second;
    } else {
      std::cout << "Unable to find: " << packet_index << std::endl;
      return;
    }

    // Get names of the quadtree packet and index files.
    packet_file = "qtp/pbundle_0000";
    packet_index = "qtp/index";

    file_loc = index_[packet_file];
    // Get location of the packet file.
    it = index_.find(packet_file);
    if (it != index_.end()) {
      qtp_packet_file_loc_ = it->second;
    } else {
      std::cout << "Unable to find: " << packet_file << std::endl;
      return;
    }

    // Get locations-of-the-packet index.
    it = index_.find(packet_index);
    if (it != index_.end()) {
      qtp_packet_index_loc_ = it->second;
    } else {
      std::cout << "Unable to find: " << packet_index << std::endl;
      return;
    }

    // Set up the packet finders.
    qtp_packet_finder_ = PacketBundleFinder(&source_,
                                            qtp_packet_index_loc_.Offset(),
                                            qtp_packet_index_loc_.Size());
    data_packet_finder_ = PacketBundleFinder(&source_,
                                             data_packet_index_loc_.Offset(),
                                             data_packet_index_loc_.Size());
  }

  // Get names of the map data packet and index files.
  packet_file = "mapdata/pbundle_0000";
  packet_index = "mapdata/index";

  // Get location of the packet file.
  it = index_.find(packet_file);
  if (it != index_.end()) {
    mapdata_packet_file_loc_ = it->second;
    has_2d_data_ = true;
  } else {
    has_2d_data_ = false;
  }

  // Get locations-of-the-packet index.
  if (has_2d_data_) {
    it = index_.find(packet_index);
    if (it != index_.end()) {
      mapdata_packet_index_loc_ = it->second;
    } else {
      std::cout << "Unable to find: " << packet_index << std::endl;
      return;
    }

    mapdata_packet_finder_ = PacketBundleFinder(
        &source_,
        mapdata_packet_index_loc_.Offset(),
        mapdata_packet_index_loc_.Size());
  }
}

/**
 * Find data packet and set offset and size for the packet. Data packets can
 * be imagery, terrain or vectors.
 */
bool FileUnpacker::FindDataPacket(const char* qtpath,
                                  int packet_type,
                                  int channel) {
  if (!has_3d_data_) {
    return false;
  }

  offset_ = data_packet_file_loc_.Offset();
  IndexItem index_item;

  index_item.Fill(qtpath, packet_type, channel);
  if (data_packet_finder_.FindPacketInIndex(&index_item)) {
    offset_ += index_item.offset;
    size_ = index_item.packet_size;
    return true;
  }
  return false;
}

/**
 * Find map data packet and set offset and size for the packet. Map ata packets
 * can be imagery or vectors.
 */
bool FileUnpacker::FindMapDataPacket(const char* qtpath,
                                     int packet_type,
                                     int channel) {
  if (!has_2d_data_) {
    return false;
  }

  offset_ = mapdata_packet_file_loc_.Offset();
  IndexItem index_item;

  index_item.Fill(qtpath, packet_type, channel);
  if (mapdata_packet_finder_.FindPacketInIndex(&index_item)) {
    offset_ += index_item.offset;
    size_ = index_item.packet_size;
    return true;
  }
  return false;
}

/**
 * Find qtp packet and set offset and size for the packet. Qtp packets can
 * be quadtree packets or a dbroot packet.
 */
bool FileUnpacker::FindQtpPacket(const char* qtpath,
                                 int packet_type,
                                 int channel) {
  offset_ = qtp_packet_file_loc_.Offset();
  IndexItem index_item;

  index_item.Fill(qtpath, packet_type, channel);
  if (qtp_packet_finder_.FindPacketInIndex(&index_item)) {
    offset_ += index_item.offset;
    size_ = index_item.packet_size;
    return true;
  }
  return false;
}

/**
 * Find file and set the offset and size for retrieving the file from the
 * package.
 */
bool FileUnpacker::FindFile(const char* file_name) {
  std::map<std::string, PackageFileLoc>::iterator it;
  it = index_.find(file_name);
  if (it != index_.end()) {
    offset_ = it->second.Offset();
    size_ = it->second.Size();
    return true;
  } else {
    std::cout << "Unable to find: " << file_name << std::endl;
    return false;
  }
}

/**
 * Get idx-th file in index. Assumes iteration order is stable if
 * index is unchanged.
 */
const char* FileUnpacker::IndexFile(int idx) {
  std::map<std::string, PackageFileLoc>::iterator it;
  it = index_.begin();
  for (int i = 0; (i < idx) && (it != index_.end()); ++i, ++it) ;

  if (it != index_.end()) {
    return it->first.c_str();
  }

  return "";
}

/**
 * Extracts the first timestamp from the info file, which should
 * be a GMT timestamp. Assumes date begins with 20 (fails in 2100).
 * The beginning of the info file is quite structured so this
 * should be reliable. It starts with 3 lines similar to the
 * following:
 *   Portable Globe
 *   Copyright 2011 Google Inc. All Rights Reserved.
 *   Copyright 2020 The Open GEE Contributors
 *   2012-02-24 01:26:07 GMT
 */
std::string FileUnpacker::ExtractDateFromInfo() const {
  size_t idx = info_.find("\n20");
  return info_.substr(idx + 1, 4)
       + info_.substr(idx + 6, 2)
       + info_.substr(idx + 9, 2)
       + info_.substr(idx + 12, 2)
       + info_.substr(idx + 15, 2)
       + info_.substr(idx + 18, 2);
}

/**
 * Calculates a crc for the info data.
 */
 std::uint32_t FileUnpacker::InfoCrc() const {
  std::uint32_t crc = 0;
  const unsigned char* ptr = reinterpret_cast<const unsigned char*>(&info_[0]);
  for (std::uint32_t i = 0; i < info_.size(); ++i) {
    // Use the full bit space by shifting to one of the
    // four byte boundaries.
    std::uint32_t value = *ptr++ << ((i & 3) * 8);
    crc += value;
  }
  return crc;
}

/**
 * Calculate the id associated with this file. Assumes that the bundle
 * contains a file called "earth/info.txt" which uniquely identifies
 * the file. If not, it returns "noinfo-<file_size>".
 */
const char* FileUnpacker::Id() {
  std::stringstream sstream;
  std::string info;
  if (id_.empty()) {
    if (info_.empty()) {
      sstream << "noinfo";
    } else {
      sstream << ExtractDateFromInfo();
      sstream << "-" << std::hex << InfoCrc();
    }
    sstream << "-" << std::hex << length_;
  }

  id_ = sstream.str();
  return id_.c_str();
}

}  // namespace fusion_portableglobe
