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


// PacketIndex - internal class for the packet file index.  Used by
// PacketFileWriter and PacketFileReader.

#include <string.h>
#include <string>
#include <khEndian.h>
#include <khSimpleException.h>
#include <khFileUtils.h>
#include <third_party/rsa_md5/crc32.h>
#include "packetindex.h"
#include "packetfile.h"

// Pull - fetch from endian buffer (checking CRC)
// Format is:
//
// Bytes   Contents
//   8     QuadtreePath
//   8     Position of corresponding data in file bundle
//   4     Size of data (in bytes)
//   4     CRC32 of above data
void PacketIndexEntry::Pull(EndianReadBuffer& buf) {
  buf.CheckCRC(kStoreSize, "PacketIndexEntry");
  buf >> qt_path_
      >> position_
      >> record_size_
      >> extra_;
  buf.Skip(sizeof(std::uint32_t)); // stored crc
}

// Push - store to endian buffer with CRC
void PacketIndexEntry::Push(EndianWriteBuffer& buf) const {
  EndianWriteBuffer::size_type start = buf.size();
  buf << qt_path_
      << position_
      << record_size_
      << extra_;
  EndianWriteBuffer::size_type end = buf.size();
  buf << Crc32((const char*)buf.data() + start, end - start);
  assert((buf.size() - start) == kStoreSize);
}

// PacketIndexReader constructor

PacketIndexReader::PacketIndexReader(geFilePool &file_pool,
                                     const std::string &index_path)
    : file_path_(index_path),
      index_pos_(PacketFile::kIndexHeaderSize),
      index_reader_(TransferOwnership(
                        new geFilePool::Reader(file_pool, index_path))),
      file_size_(index_reader_->Filesize()){
  // Read header
  LittleEndianReadBuffer buffer;
  index_reader_->Pread(buffer, PacketFile::kIndexHeaderSize, 0);

  try {
    buffer.CheckCRC(PacketFile::kIndexHeaderSize, "PacketIndexHeader");
  }
  catch (khSimpleException e) {
    // Add file name to exception
    throw khSimpleException(e.what()) << ": " << index_path;
  }

  // Extract fields and check validity
  std::string signatureCheck;
  buffer >> FixedLengthString(signatureCheck, PacketFile::kSignature.size());
  if (signatureCheck != PacketFile::kSignature) {
    throw khSimpleException("PacketIndexReader: not a PacketFile index: ")
      << index_path;
  }

  std::uint16_t format_version;
  buffer >> format_version;
  if (format_version != PacketFile::kFormatVersion) {
    throw khSimpleException("PacketIndexReader: expected version ")
      << PacketFile::kFormatVersion << ", file is version "
      << format_version << " in " << index_path;
  }

  buffer >> DecodeAs<std::uint16_t>(data_has_crc_);
  buffer.Skip(sizeof(std::uint32_t)); // stored crc
}

// PacketIndexReader::Seek - set position for ReadNext

void PacketIndexReader::Seek(off64_t position) {
  khLockGuard lock(modify_lock_);
  index_pos_ = position;
}

// PacketIndexReader::ReadNext - read next index entry.
// Returns true if success, false if at end of index.

bool PacketIndexReader::ReadNext(PacketIndexEntry *entry) {
  off64_t read_pos;
  {
    khLockGuard lock(modify_lock_);
    if (AtEnd()) {
      return false;
    }
    read_pos = index_pos_;
    index_pos_ += PacketIndexEntry::kStoreSize;
  }

  // Convert to little endian from file format
  LittleEndianReadBuffer buffer;
  index_reader_->Pread(buffer, PacketIndexEntry::kStoreSize, read_pos);
  try {
    buffer >> *entry;
  }
  catch (khSimpleException e) {
    // Throw new exception with file name
    throw khSimpleException("PacketIndexReader::ReadNext entry decode error")
        << " (" << e.what() << ")"
        << " at index position " << read_pos
        << " in file " << file_path();
  }

  return true;
}

 std::uint32_t PacketIndexReader::ReadNextN(PacketIndexEntry *entries, std::uint32_t count,
                                    LittleEndianReadBuffer &buffer) {
  off64_t read_pos;
  std::uint64_t num_left;
  {
    khLockGuard lock(modify_lock_);
    num_left = (file_size_ - index_pos_) / PacketIndexEntry::kStoreSize;
    if (num_left < count) {
      count = num_left;
    }
    read_pos = index_pos_;
    index_pos_ += count * PacketIndexEntry::kStoreSize;
  }

  buffer.Seek(0);
  index_reader_->Pread(buffer, count * PacketIndexEntry::kStoreSize, read_pos);
  for (std::uint32_t i = 0; i < count; ++i) {
    try {
      buffer >> entries[i];
    }
    catch (khSimpleException e) {
      // Throw new exception with file name
      throw khSimpleException("PacketIndexReader::ReadNextN entry decode error")
          << " (" << e.what() << ")"
          << " at index position "
          << read_pos + i * PacketIndexEntry::kStoreSize
          << " in file " << file_path();
    }
  }

  return count;
}

 std::uint64_t PacketIndexReader::NumPackets(void) const {
  return (Filesize() - PacketFile::kIndexHeaderSize) /
    PacketIndexEntry::kStoreSize;
}

 std::uint64_t PacketIndexReader::NumPackets(const std::string &index_path) {
  std::uint64_t file_size;
  time_t mtime;
  if (khGetFileInfo(index_path, file_size, mtime)) {
    return (file_size - PacketFile::kIndexHeaderSize) /
        PacketIndexEntry::kStoreSize;
  } else {
    return 0;
  }
}
