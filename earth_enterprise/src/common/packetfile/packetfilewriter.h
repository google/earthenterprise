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


#ifndef COMMON_PACKETFILE_PACKETFILEWRITER_H__
#define COMMON_PACKETFILE_PACKETFILEWRITER_H__

#include <geFilePool.h>
#include <khThread.h>
#include <quadtreepath.h>
#include <filebundle/filebundlewriter.h>
#include <packetfile/packetindexwriter.h>

// PacketFileWriter - create packet file with index.  The path
// specified to the constructor is the directory within which the
// packet file will be created.  The constructor parameters are passed
// to the FileBundleWriter constructor.
//
// Records can be written to the packet file in any of three orders of
// quadtree path:
//
//  (1) Records are written in quadtree path preorder. This is the most
//  efficient, as no sorting is needed.
//
//  (2) Records from multiple levels are written to the file, in
//  quadtree path lexical order within each level.  All records for a
//  level must be written together.  This requires a merge pass after
//  the index records have been created.  It is not as efficient as (1),
//  but much more efficient than (3).
//
//  (3) Records are written in no particular order.  In this case all
//  the entries are sorted after creation.
//
// The writer keeps track of the order of the incoming records.  The
// index records are written to a temporary file in the destination
// directory.  When the writer is closed, it renames, merges, or sorts
// the index as needed to produce the final, preorder-sorted index.
//
//  Note that only the index is sorted, not the data itself.

class PacketFileWriter : private FileBundleWriter {
 public:
  // Define an opaque token pointing to an allocated entry.  All
  // fields are private, accessible only to the PacketFileWriter
  // class.
  class AllocatedBlock {
   public:
    // need to expose copy constructor so a copy can be held for later
    // use by PacketFileWriter::WriteDuplicate() 
    inline AllocatedBlock(const AllocatedBlock& that) :
      data_pos_(that.data_pos_), index_pos_(that.index_pos_), data_size_(that.data_size_) {}
   private:
    std::uint64_t data_pos_;
    std::uint64_t index_pos_;
    size_t data_size_;
    inline AllocatedBlock(std::uint64_t data_pos, std::uint64_t index_pos, size_t data_size) :
        data_pos_(data_pos), index_pos_(index_pos), data_size_(data_size) { }
    friend class PacketFileWriter;
  };

  PacketFileWriter(geFilePool &file_pool,
                   const std::string &path,
                   bool overwrite = true,
                   mode_t mode = FileBundleWriter::kDefaultMode,
                   std::uint64_t seg_break
                   = FileBundleWriter::kDefaultSegmentBreak);
  virtual ~PacketFileWriter();

  // Start write buffering (0 or off by default) in the underlying filebundle
  // and index. Should speed up writes by WriteAppendCRC but slow down MT access
  // through WriteAt calls.
  void BufferWrites(size_t write_buffer_size, size_t index_buffer_size) {
    FileBundleWriter::BufferWrites(write_buffer_size);
    index_writer_->BufferWrites(index_buffer_size);
  }

  // Write a buffer (with CRC) to the packet file.  The last 4 bytes
  // of the supplied buffer are overwritten with the CRC32, so the
  // buffer_size should include 4 extra bytes beyond the end of the
  // actual data.  Returns data position as file bundle offset.
  std::uint64_t WriteAppendCRC(const QuadtreePath &qt_path,
                        void *buffer,
                        size_t buffer_size,
                        std::uint32_t index_extra = 0);

  // "split-writing" - Allocation routine is separate from actual disk
  // writing. This allows MT apps to allocate from a single thread 9to
  // preserve ordering requirements, but do the blocking disk I/O from
  // multiple threads
  AllocatedBlock AllocateAppend(const QuadtreePath &qt_path,
                                size_t buffer_size);
  void WriteAtCRC(const QuadtreePath &qt_path,
                  void *buffer, size_t buffer_size,
                  const AllocatedBlock &allocation,
                  std::uint32_t index_extra = 0);
  // Write an additional index entry pointing to a previously written
  // record. This is used primarily by map tiles so that uniform tiles
  // are only stored once.
  void WriteDuplicate(const QuadtreePath &qt_path,
                      const AllocatedBlock &allocation,
                      std::uint32_t index_extra = 0);

  void Close(size_t max_sort_buffer = PacketIndexWriter::kDefaultMaxSortBuffer);
  const std::string &abs_path() const { return FileBundleWriter::abs_path(); }
 private:

  khDeleteGuard<PacketIndexWriter> index_writer_;

  // For unit testing
  friend class PacketFileUnitTest;
};

// PacketFilePackIndexer - open old-style pack.nn file group to write
// an index for reading using PackFileReader.  The pack files are
// opened read-only.

class PacketFilePackIndexer {
 public:
  PacketFilePackIndexer(geFilePool &file_pool,
                   const std::string &path,
                   bool overwrite = true,
                   mode_t mode = FileBundleWriter::kDefaultMode,
                   std::uint64_t seg_break
                   = FileBundleWriter::kDefaultSegmentBreak);
  ~PacketFilePackIndexer();

  // Read next record from pack file group.  Returns position of record
  // in file bundle, throws exception if error.
  std::uint64_t ReadNext(void *buffer, size_t read_len);

  // Read record at specified linear position. Returns position of
  // record in file bundle, throws exception if error.  Sets position
  // to just after this record;
  std::uint64_t ReadAtLinear(std::uint64_t linear_pos, void *buffer, size_t read_len);

  bool AtEnd() { return data_pos_ >= data_reader_.data_size(); }
  const std::string &abs_path() const { return data_reader_.abs_path(); }
  std::uint64_t data_size() const { return data_reader_.data_size(); }

  // Write a record to the index file.
  void WriteAppendIndex(const QuadtreePath &qt_path,
                        std::uint64_t position,
                        std::int32_t record_size,
                        std::uint32_t index_extra = 0) {
    index_writer_->WriteAppend(PacketIndexEntry(qt_path,
                                                position,
                                                record_size,
                                                index_extra));
  }

  void Close(size_t max_sort_buffer = PacketIndexWriter::kDefaultMaxSortBuffer);

  // Translate old pack file linear position (addressing continuous
  // across segment boundary) to FileBundle position (segment start
  // addresses are multiples of segment break).  Throws exception if
  // invalid position.
  std::uint64_t LinearToBundlePosition(std::uint64_t linear_position) const {
    return data_reader_.LinearToBundlePosition(linear_position);
  }
 private:
  std::uint64_t data_pos_;                     // linear address position
  FileBundlePackImport data_reader_;
  khMutex modify_lock_;                 // protects data_pos_

  khDeleteGuard<PacketIndexWriter> index_writer_;

  // For unit testing
  friend class PacketFileUnitTest;
};


#endif  // COMMON_PACKETFILE_PACKETFILEWRITER_H__
