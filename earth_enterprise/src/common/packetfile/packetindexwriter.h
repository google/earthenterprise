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


#ifndef COMMON_PACKETFILE_PACKETINDEXWRITER_H__
#define COMMON_PACKETFILE_PACKETINDEXWRITER_H__

#include <geFilePool.h>
#include <khThread.h>
#include <quadtreepath.h>
#include <packetfile/packetindex.h>
#include <packetfile/packetfile.h>

// PacketIndexWriter - create index for packet file.  Used internally
// by PacketFileWriter and PacketFilePackIndexer.  See packetwriter.h
// for information about index ordering.

class PacketIndexWriter {
 public:
  // Define default max amount of memory to use for final sort.  This
  // is just the amount for the records to be sorted, doesn't include
  // memory needed for other purposes.
  static const size_t kDefaultMaxSortBuffer = 16 * 1024 * 1024;
  // Define the bare minimum memory for sorting.  This is set
  // ridiculously small to allow small sizes for testing.  Don't use
  // anything nearly this small for real work.
  static const size_t kMinSortBuffer = 16 * 16;

  // Constructor - path is the path to the PacketFile directory.
  // Execute permissions are stripped from mode before creating the
  // index file.
  // data_has_crc remains because of historical reasons. 3.0 onwards
  // data_has_crc is true always. But prior to that it was false always.
  // We left this flag so as to avoid rewriting the packetfiles in
  // gerasterpackupgrade.
  PacketIndexWriter(geFilePool &file_pool,
                    const std::string &path,
                    mode_t mode,
                    bool data_has_crc = true);
  ~PacketIndexWriter();

  // Start buffering writes. Should speed up single-threaded appends, but
  // multi-threaded and random location writes might be slower.
  void BufferWrites(size_t write_buffer_size) {
    index_writer_->BufferWrites(write_buffer_size);
  }

  void WriteAppend(const PacketIndexEntry &index_entry);

  // "split-writing" - allocation separate from blocking disk I/O
  std::uint64_t AllocateAppend(const QuadtreePath &qt_path);
  void   WriteAt(std::uint64_t pos, const PacketIndexEntry &index_entry);

  void Close(size_t max_sort_buffer = kDefaultMaxSortBuffer);
  std::uint64_t entry_count() const { return entry_count_; }
  off64_t index_pos() const { return index_pos_; }
 private:
  // SortedRegion structure to keep track of contiguous areas in index
  // which are already sorted.
  struct SortedRegion {
   public:
    SortedRegion() : position(0), count(0) {}
    SortedRegion(off64_t position_, std::uint32_t count_)
        : position(position_),
          count(count_) {}
    off64_t position;                   // position in index file
    std::uint32_t count;                       // number of records in region
  };
  class SortedRegionMergeSource;

  geFilePool &file_pool_;
  mode_t mode_;                         // file creation mode
  bool data_has_crc_;                   // header flag
  std::string index_path_;              // full path to final index
  std::string index_temp_path_;         // full path to temp index

  khDeleteGuard<geFilePool::Writer> index_writer_;
  off64_t index_pos_;                   // position in index for next write
  std::uint64_t entry_count_;

  khMutex modify_lock_;

  void CreateIndexWriter(const std::string &index_filename,
                         khDeleteGuard<geFilePool::Writer> &index_writer,
                         off64_t *position);

  // Order tracking
  bool is_preorder_;
  bool is_level_ordered_;
  std::vector<SortedRegion> level_region_;  // sorted regions if level ordered
  QuadtreePath last_path_;                  // last path seen
  void UpdatePreorder(const QuadtreePath &new_path);
  void UpdateLevelOrdered(const QuadtreePath &new_path, off64_t index_position);
  void MergeIndex(const std::string &source_path,
                  const std::vector<SortedRegion> &regions);
  void SortIndex(size_t max_sort_memory);

  // For unit testing
  friend class PacketFileUnitTest;
};

#endif // COMMON_PACKETFILE_PACKETINDEXWRITER_H__
