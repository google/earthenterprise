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

#include "./file_unpacker.h"
#include <cstring>
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <map>
#include <string>
#include "./file_package.h"
#include "./glc_reader.h"
#include "./packetbundle_finder.h"

const char* const GLB_DBROOT_PREFIX = "dbroot/dbroot_localhost_";

/**
 * Constructor. Load the index to all of the files and
 * packets in the package file.
 */
FileUnpacker::FileUnpacker(
    const GlcReader& glc_reader, std::uint64_t offset, std::uint64_t size)
    : glc_reader_(glc_reader),
      base_offset_(offset),
      base_size_(size),
      data_packet_finder_(0),
      mapdata_packet_finder_(0),
      qtp_packet_finder_(0) {
  // TODO: Consider a quick check of the file integrity here.
  // TODO: Maybe add a magic number or redundant info
  // TODO: such as the index size.
  // TODO: Double-check version number is correct.

  // Get offset to the index
  std::uint64_t index_offset_loc =
      base_offset_ + base_size_ - Package::kIndexOffsetOffset;
  glc_reader_.ReadData(
      &reader_offset_, index_offset_loc, Package::kIndexOffsetSize);

  // Read in the index header.
  std::uint32_t num_files;
  reader_offset_ += base_offset_;
  if (!Read(&num_files, sizeof(num_files))) {
    std::cerr << "Unable to read number of files." << std::endl;
    return;
  }

  // Read in the index entries.
  std::string path;
  PackageFileLoc file_loc;
  for (std::uint32_t i = 0; i < num_files; ++i) {
    // Read in the relative path of the index entry.
    std::uint16_t path_len;
    if (!Read(&path_len, sizeof(path_len))) {
      std::cerr << "Unable to read file name size." << std::endl;
      return;
    }

    path.resize(path_len);
    if (!Read(&path[0], path_len)) {
      std::cerr << "Unable to read file name." << std::endl;
      return;
    }

     // Read in the size and offset of the index entry.
    if (!Read(&file_loc, sizeof(PackageFileLoc))) {
      std::cerr << "Unable to read file offset and size." << std::endl;
      return;
    }
    file_loc.AddBaseToOffset(base_offset_);
    index_[path] = file_loc;

    // Look for dbroot file entry and cache its path.
    if (dbroot_path_.empty() &&
        path.length() > std::strlen(GLB_DBROOT_PREFIX) && !std::strncmp(
        path.c_str(), GLB_DBROOT_PREFIX, std::strlen(GLB_DBROOT_PREFIX))) {
      dbroot_path_ = path;
    }
  }

  std::map<std::string, PackageFileLoc>::iterator it;

  // Get names of the data packet and index files.
  std::string packet_file = "data/pbundle_";

  // Get location(s) of the 3d data packet bundle(s).
  has_3d_data_ = false;
  int file_id = 0;
  while (true) {
    char bundle_index[9];
    snprintf(bundle_index, sizeof(bundle_index), "%04d", file_id++);
    it = index_.find(packet_file + bundle_index);
    if (it != index_.end()) {
      data_packet_file_loc_.push_back(it->second);
      has_3d_data_ = true;
    } else {
      break;
    }
  }

  // Get location(s) of the 2d data packet bundle(s).
  packet_file = "mapdata/pbundle_";
  has_2d_data_ = false;
  file_id = 0;
  while (true) {
    char bundle_index[9];
    snprintf(bundle_index, sizeof(bundle_index), "%04d", file_id++);
    it = index_.find(packet_file + bundle_index);
    if (it != index_.end()) {
      mapdata_packet_file_loc_.push_back(it->second);
      has_2d_data_ = true;
    } else {
      break;
    }
  }

  // Get locations-of-the-packet index.
  std::string packet_index = "data/index";
  if (has_3d_data_) {
    it = index_.find(packet_index);
    if (it != index_.end()) {
      data_packet_index_loc_ = it->second;
    } else {
      std::cerr << "Unable to find: " << packet_index << std::endl;
      return;
    }

    // Get names of the quadtree packet and index files.
    packet_file = "qtp/pbundle_0000";
    packet_index = "qtp/index";

    file_loc = index_[packet_file];
    // Get location of the packet file.
    it = index_.find(packet_file);
    bool has_qtp_data = true;
    if (it != index_.end()) {
      qtp_packet_file_loc_ = it->second;
    } else {
      // TODO: Allow case where there are no new quadtree packets
      // TODO: but instead all inherited from the parent layer.
      std::cerr << "Warning: Unable to find: " << packet_file << std::endl;
      has_qtp_data = false;
    }

    // Get locations-of-the-packet index.
    it = index_.find(packet_index);
    if (it != index_.end()) {
      qtp_packet_index_loc_ = it->second;
    } else {
      // Allow case where there are no new quadtree packets but
      // instead all inherited from the parent layer.
      std::cerr << "Warning: Unable to find: " << packet_index << std::endl;
      has_qtp_data = false;
    }

    // Set up the packet finders.
    if (has_qtp_data) {
      qtp_packet_finder_ = new PacketBundleFinder(
          glc_reader_,
          qtp_packet_index_loc_.Offset(),
          qtp_packet_index_loc_.Size());
    } else {
      qtp_packet_finder_ = 0;
    }
    data_packet_finder_ = new PacketBundleFinder(
        glc_reader_,
        data_packet_index_loc_.Offset(),
        data_packet_index_loc_.Size());
  }

  // Get locations-of-the-packet index.
  if (has_2d_data_) {
    packet_index = "mapdata/index";
    it = index_.find(packet_index);
    if (it != index_.end()) {
      mapdata_packet_index_loc_ = it->second;
    } else {
      has_2d_data_ = false;
      std::cerr << "Unable to find: " << packet_index << std::endl;
      return;
    }

    mapdata_packet_finder_ = new PacketBundleFinder(
        glc_reader_,
        mapdata_packet_index_loc_.Offset(),
        mapdata_packet_index_loc_.Size());
  }
}

FileUnpacker::~FileUnpacker() {
  if (data_packet_finder_) {
    delete data_packet_finder_;
  }
  if (mapdata_packet_finder_) {
    delete mapdata_packet_finder_;
  }
  if (qtp_packet_finder_) {
    delete qtp_packet_finder_;
  }
}

/**
 * Helper for reading in consecutive fields from the glc file.
 */
bool FileUnpacker::Read(void* data, std::uint64_t size) {
  if (glc_reader_.ReadData(data, reader_offset_, size)) {
    reader_offset_ += size;
    return true;
  }
  return false;
}

/**
 * Find data packet and set offset and size for the packet. Data packets can
 * be imagery, terrain or vectors.
 */
bool FileUnpacker::FindDataPacket(const char* qtpath,
                                  int packet_type,
                                  int channel,
                                  PackageFileLoc* data_loc) {
  if (!has_3d_data_) {
    return false;
  }

  IndexItem index_item;
  std::string qtpath_str = qtpath;
  if (qtpath_str.size() > IndexItem::kMaxLevel) {
    return false;
  }
  index_item.Fill(qtpath_str, packet_type, channel);
  if (data_packet_finder_->FindPacketInIndex(&index_item)) {
    // Get base address of packets.
    if (index_item.file_id >= data_packet_file_loc_.size()) {
      std::cerr << "File id exceeds max packet bundle." << std::endl;
      return false;
    }

    std::uint64_t offset = data_packet_file_loc_[index_item.file_id].Offset();
    // Add offset to base address.
    offset += index_item.offset;
    data_loc->Set(offset, index_item.packet_size);
    return true;
  }
  return false;
}

/**
 * Find map data packet and set offset and size for the packet. Map data packets
 * can be imagery or vectors.
 */
bool FileUnpacker::FindMapDataPacket(const char* qtpath,
                                     int packet_type,
                                     int channel,
                                     PackageFileLoc* data_loc) {
  if (!has_2d_data_) {
    return false;
  }

  IndexItem index_item;
  std::string qtpath_str = qtpath;
  if (qtpath_str.size() > IndexItem::kMaxLevel) {
    return false;
  }
  index_item.Fill(qtpath_str, packet_type, channel);
  if (mapdata_packet_finder_->FindPacketInIndex(&index_item)) {
    // Get base address of packets.
    if (index_item.file_id >= mapdata_packet_file_loc_.size()) {
      std::cerr << "File id exceeds max packet bundle." << std::endl;
      return false;
    }

    std::uint64_t offset = mapdata_packet_file_loc_[index_item.file_id].Offset();
    // Add offset to base address.
    offset += index_item.offset;
    data_loc->Set(offset, index_item.packet_size);
    return true;
  }
  return false;
}

bool FileUnpacker::MapDataPacketWalker(int layer, const map_packet_walker& walker)
{
  PacketBundleFinder* finder = mapdata_packet_finder_;
  if (has_3d_data_) {
    finder = data_packet_finder_;
  }
  return finder->MapDataPacketWalker(layer, walker);
}

/**
 * Find qtp packet and set offset and size for the packet. Qtp packets can
 * be quadtree packets or a dbroot packet.
 */
bool FileUnpacker::FindQtpPacket(const char* qtpath,
                                 int packet_type,
                                 int channel,
                                 PackageFileLoc* data_loc) {
  if (!has_3d_data_) {
    return false;
  }

  // Get base address of packets.
  std::uint64_t offset = qtp_packet_file_loc_.Offset();

  IndexItem index_item;
  std::string qtpath_str = qtpath;
  if (qtpath_str.size() > IndexItem::kMaxLevel) {
    return false;
  }
  index_item.Fill(qtpath_str, packet_type, channel);
  if (qtp_packet_finder_
      && qtp_packet_finder_->FindPacketInIndex(&index_item)) {
    // Add offset to base address.
    offset += index_item.offset;
    data_loc->Set(offset, index_item.packet_size);
    return true;
  }
  return false;
}

/**
 * Returns whether there are any data at the given address.
 */
bool FileUnpacker::HasData(const char* qtpath, bool is_2d) {
  PacketBundleFinder* finder;
  if (is_2d) {
    if (!has_2d_data_) {
      return false;
    }
    finder = mapdata_packet_finder_;
  } else {
    if (!has_3d_data_) {
      return false;
    }
    finder = data_packet_finder_;
  }

  IndexItem index_item;
  index_item.Fill(qtpath, IndexItem::kIgnoreChannelAndType, 0);
  return (finder->FindPacketInIndex(&index_item));
}

/**
 * Find dbroot for the globe.
 */
bool FileUnpacker::FindDbRoot(PackageFileLoc* data_loc) {
  if (!dbroot_path_.empty()) {
    return FindFile(dbroot_path_.c_str(), data_loc);
  } else if (FindQtpPacket("0", kDbRootPacket, 0, data_loc)) {
    // Found it as first packet in quadtree bundle.
    return true;
  }

  data_loc->Set(0, 0);
  return false;
}

/**
 * Find file and set the offset and size for retrieving the file from the
 * package.
 */
bool FileUnpacker::FindFile(const char* file_name,
                            PackageFileLoc* file_loc) {
  std::map<std::string, PackageFileLoc>::iterator it;
  it = index_.find(file_name);
  if (it != index_.end()) {
    file_loc->Set(it->second.Offset(), it->second.Size());
    return true;
  } else {
    std::cout << "Unable to find: " << file_name << std::endl;
    file_loc->Set(0, 0);
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
