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


#include <khGuard.h>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>
#include <khSimpleException.h>
#include <khFileUtils.h>
#include <khThread.h>
#include <packetfile/packetfilereader.h>
#include <packetfile/packetfile.h>

// PacketFile - shared constants for Reader and Writer
//
// The index file starts with a header in the following format
// (numeric data is little-endian):
//  Bytes   Description
//    8     "PktIndex"
//    2     Index format version number
//    2     FileBundle data has CRC32? 1=yes (new files), 0=no (converted files)
//    4     CRC-32 of above data

const std::string PacketFileReaderBase::kIndexBase = "packetindex";

namespace PacketFile {
  const std::string kIndexBase = PacketFileReaderBase::kIndexBase;
  const std::string kSignature("PktIndex");
  const std::uint16_t kFormatVersion = 1;
  const size_t kIndexHeaderSize(
    kSignature.size() + sizeof(kFormatVersion)
    + sizeof(std::uint16_t) + sizeof(std::uint32_t));

bool IsPacketFile(const std::string &path) {
  std::string indexpath = khComposePath(path, kIndexBase);
  if (khExists(indexpath)) {
    std::string signature;
    if (khReadStringFromFile(indexpath, signature, kSignature.size()) &&
        (signature == kSignature)) {
      return true;
    }
  }
  return false;
}

std::string IndexFilename(const std::string &path) {
  return khComposePath(path, kIndexBase);
}


}

// PacketFileReader
PacketFileReaderBase::PacketFileReaderBase(
    geFilePool &file_pool, const std::string &path)
    : FileBundleReader(file_pool, path),
      data_has_crc_(
          khExists(IndexFilename()) ?
          PacketIndexReader(
              file_pool, IndexFilename()).
          data_has_crc()
          : true) {
}

PacketFileReaderBase::PacketFileReaderBase(
    geFilePool &file_pool, const std::string &path, const std::string &prefix)
    : FileBundleReader(file_pool, path, prefix),
      data_has_crc_(
          khExists(IndexFilename()) ?
          PacketIndexReader(
              file_pool, IndexFilename()).
          data_has_crc()
          : true) {
}

PacketFileReaderBase::PacketFileReaderBase(
    geFilePool &file_pool, const std::string &path, bool data_has_crc)
    : FileBundleReader(file_pool, path),
      data_has_crc_(data_has_crc) {
}

PacketFileReader::PacketFileReader(
    geFilePool &file_pool, const std::string &path)
    : PacketFileReaderBase(file_pool, path, false),
      index_reader_(file_pool, IndexFilename()) {
  if (index_reader_.data_has_crc()) {
    PacketFileReaderBase::SetDataHasCrc();
  }
}

size_t PacketFileReader::ReadNextCRC(QuadtreePath *qt_path,
                                     void *buffer,
                                     size_t buf_size) {
  // Read index entry
  PacketIndexEntry index_entry;
  if (index_reader_.ReadNext(&index_entry)) {
    *qt_path = index_entry.qt_path();

    // Read data
    size_t record_size = index_entry.record_size();
    if (record_size <= buf_size) {
      if (index_reader_.data_has_crc()) {
        FileBundleReader::ReadAtCRC(
            index_entry.position(), buffer, record_size);
        record_size -= FileBundle::kCRCsize;
      } else {
        FileBundleReader::ReadAt(index_entry.position(), buffer, record_size);
      }
      return record_size;
    } else {
      throw khSimpleException("PacketFileReader::ReadNextCRC: ")
        << "record size (" << record_size
        << ") > buffer size (" << buf_size << ")";
    }
  } else {                              // end of index
    return 0;
  }
}

size_t PacketFileReader::ReadNextCRC(QuadtreePath *qt_path,
                                     std::string &buffer) {
  // Read index entry
  PacketIndexEntry index_entry;
  if (index_reader_.ReadNext(&index_entry)) {
    *qt_path = index_entry.qt_path();

    return ReadAtCRC(index_entry.position(), buffer,
                     index_entry.record_size());
  } else {                              // end of index
    return 0;
  }
}

void PacketFileReaderBase::EnableReadCache(
    std::uint32_t max_blocks, std::uint32_t block_size) {
  // Only enable caching if a minimum of 2 blocks are set.
  if (max_blocks >= 2) {
    FileBundleReader::EnableReadCache(max_blocks, block_size);
  }
}

PacketFileReader::~PacketFileReader() {}

PacketFileReaderBase::~PacketFileReaderBase() {}
