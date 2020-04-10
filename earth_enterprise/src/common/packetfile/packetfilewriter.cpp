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


#include <string>
#include <vector>
#include <khGuard.h>
#include <khSimpleException.h>
#include <khFileUtils.h>
#include <notify.h>
#include <packetfile/packetindex.h>
#include <packetfile/packetfilewriter.h>

// PacketFileWriter - create packet file with index.

PacketFileWriter::PacketFileWriter(geFilePool &file_pool,
                                   const std::string &path,
                                   bool overwrite,
                                   mode_t mode,
                                   std::uint64_t seg_break)
    : FileBundleWriter(file_pool, path, overwrite, mode, seg_break),
      index_writer_(TransferOwnership(
                        new PacketIndexWriter(file_pool,
                                              FileBundleWriter::abs_path(),
                                              mode))) {
}

// Destructor - all cleanup should be automatic

PacketFileWriter::~PacketFileWriter() {
}

// WriteAppendCRC - write a packet (with CRC) to the file, and also
// write an index entry.  The buffer size must include 4 unused bytes
// at the end where the CRC32 will be stored.  Returns data position
// as file bundle offset.

 std::uint64_t PacketFileWriter::WriteAppendCRC(const QuadtreePath &qt_path,
                                        void *buffer,
                                        size_t buffer_size,
                                        std::uint32_t index_extra) {

  AllocatedBlock allocation = AllocateAppend(qt_path, buffer_size);
  WriteAtCRC(qt_path, buffer, buffer_size, allocation, index_extra);
  return allocation.data_pos_;
}

PacketFileWriter::AllocatedBlock
PacketFileWriter::AllocateAppend(const QuadtreePath &qt_path,
                                 size_t buffer_size) {
  return AllocatedBlock(FileBundleWriter::AllocateAppend(buffer_size),
                        index_writer_->AllocateAppend(qt_path),
                        buffer_size);
}

void PacketFileWriter::WriteAtCRC(const QuadtreePath &qt_path,
                                  void *buffer,
                                  size_t buffer_size,
                                  const AllocatedBlock &allocation,
                                  std::uint32_t index_extra) {
  assert(buffer_size == allocation.data_size_);
  FileBundleWriter::WriteAtCRC(allocation.data_pos_, buffer, buffer_size);
  index_writer_->WriteAt(allocation.index_pos_,
                         PacketIndexEntry(qt_path,
                                          allocation.data_pos_,
                                          buffer_size, index_extra));
}

void PacketFileWriter::WriteDuplicate(const QuadtreePath &qt_path,
                                      const AllocatedBlock &allocation,
                                      std::uint32_t index_extra) {
  index_writer_->WriteAppend(PacketIndexEntry(qt_path,
                                              allocation.data_pos_,
                                              allocation.data_size_,
                                              index_extra));
}



// Close - close the file bundle and the index.

void PacketFileWriter::Close(size_t max_sort_buffer) {
  // Flush and close the data file bundle and the temporary index file
  FileBundleWriter::Close();
  index_writer_->Close(max_sort_buffer);
  index_writer_.clear();
}

// PacketFilePackIndexer - create packet file with index.

PacketFilePackIndexer::PacketFilePackIndexer(geFilePool &file_pool,
                                             const std::string &path,
                                             bool overwrite,
                                             mode_t mode,
                                             std::uint64_t seg_break)
    : data_pos_(0),
      data_reader_(file_pool, path, mode, seg_break),
      index_writer_(TransferOwnership(
                        new PacketIndexWriter(file_pool,
                                              data_reader_.abs_path(),
                                              mode,
                                              false))) {
}

// Destructor - all cleanup should be automatic

PacketFilePackIndexer::~PacketFilePackIndexer() {
}

// Read next record from the pack file and return position in bundle

 std::uint64_t PacketFilePackIndexer::ReadNext(void *buffer, size_t read_len) {
  std::uint64_t bundle_pos;                    // position in bundle addressing
  {
    khLockGuard lock(modify_lock_);

    bundle_pos = data_reader_.LinearToBundlePosition(data_pos_);
    data_pos_ += read_len;
  }

  data_reader_.ReadAt(bundle_pos, buffer, read_len);
  return bundle_pos;
}

// ReadAtLinear - read record at specified linear position.

 std::uint64_t PacketFilePackIndexer::ReadAtLinear(std::uint64_t linear_pos,
                                           void *buffer,
                                           size_t read_len) {
  std::uint64_t bundle_pos = data_reader_.LinearToBundlePosition(linear_pos);
  {
    khLockGuard lock(modify_lock_);
    data_pos_ = linear_pos + read_len;
  }

  data_reader_.ReadAt(bundle_pos, buffer, read_len);
  return bundle_pos;
}

void PacketFilePackIndexer::Close(size_t max_sort_buffer) {
  // Flush and close the data file and temporary index file
  data_reader_.Close();
  index_writer_->Close(max_sort_buffer);
  index_writer_.clear();
}
