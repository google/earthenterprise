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
#include <algorithm>
#include <sstream>
#include <khGuard.h>
#include <khSimpleException.h>
#include <khFileUtils.h>
#include <notify.h>
#include <third_party/rsa_md5/crc32.h>
#include <merge/merge.h>
#include <filebundle/filebundlewriter.h>
#include <packetfile/packetindexwriter.h>
#include <packetfile/packetfile.h>
#include <khEndian.h>

static const int kWriteBufferSize = 10 * 1024 * 1024;

// PacketIndexWriter - create index for packet file.  Used internally
// by PacketFileWriter and PacketFilePackIndexer.  See packetwriter.h
// for information about index ordering.

// Constructor - create the writer and write the index header.  Path
// is the path to the PacketFile directory.  Execute permissions are
// stripped from mode before creating the index file.  Always
// overwrites any existing index.
PacketIndexWriter::PacketIndexWriter(geFilePool &file_pool,
                                     const std::string &path,
                                     mode_t mode,
                                     bool data_has_crc)
    : file_pool_(file_pool),
      mode_(mode),
      data_has_crc_(data_has_crc),
      index_path_(path + PacketFile::kIndexBase),
      index_temp_path_(index_path_ + ".tmp"),
      index_pos_(0),
      entry_count_(0),
      is_preorder_(true),
      is_level_ordered_(true),
      level_region_(QuadtreePath::kMaxLevel + 1) {
  // Delete the final version of the index if it already exists.  This
  // will avoid problems if the writer dies before completion.
  if (khExists(index_path_)) {
    khUnlink(index_path_);
  }

  // Create a writer for the temporary index file
  CreateIndexWriter(index_temp_path_, index_writer_, &index_pos_);
}

PacketIndexWriter::~PacketIndexWriter() {
  if (index_writer_) {
    notify(NFY_WARN, "PacketIndexWriter: error, not closed properly: %s",
           index_path_.c_str());
  }
}

// CreateIndexWriter - create the writer, write the index header, and return
// position just after header.
void PacketIndexWriter::CreateIndexWriter(
    const std::string &index_path,
    khDeleteGuard<geFilePool::Writer> &new_writer,
    off64_t *position) {
  // Create a writer for the index file
  new_writer = TransferOwnership(
      new geFilePool::Writer(geFilePool::Writer::ReadWrite,
                             geFilePool::Writer::Truncate,
                             file_pool_,
                             index_path,
                             mode_ & ~FileBundleWriter::kExecuteMode));
  LittleEndianWriteBuffer le_buffer;
  le_buffer << FixedLengthString(PacketFile::kSignature,
                                 PacketFile::kSignature.size())
            << PacketFile::kFormatVersion
            << EncodeAs<std::uint16_t>(data_has_crc_);
  le_buffer << Crc32(le_buffer.data(), le_buffer.size());
  new_writer->Pwrite(le_buffer.data(), le_buffer.size(), 0);

  *position = le_buffer.size();
  assert(*position == (off64_t)PacketFile::kIndexHeaderSize);
}

// WriteAppend - add entry to end of index, return position written

void PacketIndexWriter::WriteAppend(const PacketIndexEntry &index_entry) {
  WriteAt(AllocateAppend(index_entry.qt_path()), index_entry);
}


 std::uint64_t PacketIndexWriter::AllocateAppend(const QuadtreePath &qt_path) {
  // Update index progress
  off64_t index_write_pos;
  {
    khLockGuard lock(modify_lock_);

    // Reserve position in index
    index_write_pos = index_pos_;
    index_pos_ += PacketIndexEntry::kStoreSize;

    // Update input ordering information
    UpdatePreorder(qt_path);
    UpdateLevelOrdered(qt_path, index_write_pos);
    last_path_ = qt_path;
  }

  return index_write_pos;
}

void   PacketIndexWriter::WriteAt(std::uint64_t pos,
                                  const PacketIndexEntry &index_entry) {
  // Create buffer for index entry
  LittleEndianWriteBuffer le_buffer;
  le_buffer << index_entry;
  assert(le_buffer.size() == PacketIndexEntry::kStoreSize);

  // Write new record to index
  if (!index_writer_) {
    throw khSimpleException(
        "PacketIndexWriter::WriteAt write to closed pack file ")
          << index_temp_path_;
  }
  index_writer_->Pwrite(le_buffer.data(), le_buffer.size(), pos);
}


// UpdatePreorder - check if the new record is still in preorder,
// updating the is_preorder_ flag.

void PacketIndexWriter::UpdatePreorder(const QuadtreePath &new_path) {
  is_preorder_ = is_preorder_
                 && !(new_path < last_path_);
}

// UpdateLevelOrdered - check if the new record is consistent with a
// level ordered file.  If it is the same level as the previous
// record, it must not precede that record.  If at a different level,
// no records from that level must have been seen previously.
//
// The vector level_region_ is used to track the start of index records
// for each level seen.  This information is used to merge the records
// when the writer is closed.

void PacketIndexWriter::UpdateLevelOrdered(const QuadtreePath &new_path,
                                           off64_t index_pos) {
  if (is_level_ordered_) {
    std::uint32_t level = new_path.Level();
    SortedRegion &region = level_region_.at(level);
    if ((new_path.Level() == last_path_.Level())
        ? (new_path < last_path_)
        : (region.count != 0)) { // if out of order
        is_level_ordered_ = false;
    } else {
      if (0 == region.count) { // none previous at this level
        assert(index_pos != 0);
        region.position = index_pos;
      }
      ++region.count;
    }
  }
}

//-------------------------------------------------------------------------------

// SortIndex - takes an unsorted index in index_temp_path_ and
// produces an index sorted in preorder in index_path_.  If index is
// too big to sort in memory, a second temporary file will be
// generated to hold intermediate results.
//
// TODO: the intermediate results could be written back in-place in
// the temporary index, but right now geFilePool doesn't support read
// and write on the same file, or opening a file for rewrite.

void PacketIndexWriter::SortIndex(size_t max_sort_buffer) {
  // Make sure we are allowed at least a minimum amount of memory
  if (max_sort_buffer < kMinSortBuffer) {
    notify(NFY_NOTICE,
           "PacketIndexWriter::SortIndex: buffer size limit specified (%lu) "
           "is less than minimum (%lu), using minimum",
           static_cast<unsigned long>(max_sort_buffer),
           static_cast<unsigned long>(kMinSortBuffer));
    max_sort_buffer = kMinSortBuffer;
  }

  // Open the unsorted index for reading, and create a secondary temp
  // index

  khDeleteGuard<PacketIndexReader> unsorted_index(
      TransferOwnership(new PacketIndexReader(file_pool_, index_temp_path_)));
  const std::string region_index_path(index_path_ + ".sort.tmp");

  khDeleteGuard<geFilePool::Writer> region_index;
  off64_t write_pos;
  CreateIndexWriter(region_index_path, region_index, &write_pos);
  region_index->BufferWrites(kWriteBufferSize);

  // Set up memory buffer and vector of regions.  entry_count is the
  // total number of index entries, buffer_count is the count of
  // entries which will fit in the memory buffer.

  ssize_t entry_count = (unsorted_index->Filesize() - PacketFile::kIndexHeaderSize)
                        / PacketIndexEntry::kStoreSize;
  ssize_t buffer_count =
    std::min(entry_count,
             static_cast<ssize_t>(max_sort_buffer / PacketIndexEntry::kStoreSize));
  ssize_t region_count = (entry_count + buffer_count - 1) / buffer_count;

  std::vector<SortedRegion> region;
  region.reserve(region_count);
  std::vector<PacketIndexEntry> index_entry(buffer_count);

  // Sort each region of the file, and write to intermediate file
  for (ssize_t remaining = entry_count; remaining > 0;) {
    ssize_t read_count = std::min(remaining, buffer_count);

    // Read index entries into memory from input file
    for (ssize_t i = 0; i < read_count; ++i) {
      unsorted_index->ReadNext(&index_entry[i]);
    }

    std::sort(index_entry.begin(), index_entry.begin() + read_count);

    // Save region data for merge, and write sorted index entries to
    // intermediate output file
    region.push_back(SortedRegion(write_pos, read_count));
    LittleEndianWriteBuffer le_buffer(PacketIndexEntry::kStoreSize);
    for (ssize_t i = 0; i < read_count; ++i) {
      le_buffer.reset();
      le_buffer << index_entry.at(i);
      region_index->Pwrite(le_buffer.data(), le_buffer.size(), write_pos);
      write_pos += le_buffer.size();
    }

    remaining -= read_count;
  }

  region_index->SyncAndClose();         // done writing
  region_index.clear();
  unsorted_index.clear();               // done reading

  // Delete the input file
  if (!khUnlink(index_temp_path_)) {
    notify(NFY_WARN, "PacketIndexWriter::SortIndex: failed to unlink %s",
           index_temp_path_.c_str());
  }

  // Move intermediate sort results to final index
  if (region.size() == 1) {
    // Only one segment, just rename to final index file
    if (!khRename(region_index_path, index_path_)) {
      throw khSimpleException("PacketIndexWriter::SortIndex: index rename failed, ")
        << region_index_path << " -> " << index_path_;
    }
    notify(NFY_DEBUG, "PacketIndexWriter::SortIndex: sorted %ld entries in memory",
           static_cast<long int>(entry_count));
  } else {
    // Multiple segments, merge into final index file
    MergeIndex(region_index_path, region);
    if (!khUnlink(region_index_path)) {
      notify(NFY_WARN, "PacketIndexWriter::SortIndex: failed to unlink %s",
             region_index_path.c_str());
    }
    notify(NFY_DEBUG, "PacketIndexWriter::SortIndex: sorted %ld entries "
           "using merge", static_cast<long int>(entry_count));
  }
}

//-------------------------------------------------------------------------------

// MergeIndex and related classes - input is a temporary index file
// with multiple sorted regions.  Output is a merged final index file.

class PacketIndexWriter::SortedRegionMergeSource
    : public MergeSource<PacketIndexEntry> {
 public:
  SortedRegionMergeSource(const std::string &name,
                          geFilePool &file_pool,
                          const std::string &index_path,
                          const SortedRegion &region)
      : MergeSource<PacketIndexEntry>(name),
        valid_(true),
        index_path_(index_path),
        index_reader_(TransferOwnership(
                          new PacketIndexReader(file_pool, index_path))),
        remaining_(region.count)
  {
    assert(remaining_ > 0);
    index_reader_->Seek(region.position);
    index_reader_->ReadNext(&current_);
    --remaining_;
  }
  virtual ~SortedRegionMergeSource() {}
  virtual const PacketIndexEntry &Current() const {
    if (valid_) {
      return current_;
    } else {
      throw khSimpleException("SortedRegionMergeSource::Current: no valid value")
        << " in source " << name() << " reading index " << index_path_;
    }
  }
  virtual bool Advance() {
    if (valid_  &&  remaining_ > 0) {
      index_reader_->ReadNext(&current_);
      --remaining_;
    } else {
      valid_ = false;
    }
    return valid_;
  }
  virtual void Close() {
    index_reader_.clear();
    remaining_ = 0;
  }
 private:
  bool valid_;
  std::string index_path_;
  khDeleteGuard<PacketIndexReader> index_reader_;
  std::uint32_t remaining_;                    // remaining records
  PacketIndexEntry current_;

  DISALLOW_COPY_AND_ASSIGN(SortedRegionMergeSource);
};

void PacketIndexWriter::MergeIndex(const std::string &source_path,
                                   const std::vector<SortedRegion> &regions) {
  // Build a Merge with a source for each region
  Merge<PacketIndexEntry> index_merge("Merge_to_" + index_path_);

  size_t source_count = 0;
  for (size_t i = 0; i < regions.size(); ++i) {
    const SortedRegion &region = regions.at(i);
    if (region.count != 0) {
      std::ostringstream source_name;
      source_name << "IndexMerge:"
                  << region.position << ":" << region.count;
      index_merge.AddSource(
          TransferOwnership(
              new SortedRegionMergeSource(source_name.str(),
                                          file_pool_,
                                          source_path,
                                          region)));
      ++source_count;
    }
  }
  notify(NFY_DEBUG, "PacketIndexWriter::MergeIndex: merging %llu regions",
         static_cast<long long unsigned>(source_count));
  index_merge.Start();

  // Create the final index output file
  khDeleteGuard<geFilePool::Writer> index_writer;
  off64_t index_pos;
  CreateIndexWriter(index_path_, index_writer, &index_pos);
  index_writer->BufferWrites(kWriteBufferSize);

  // Write the merged records to the final index
  LittleEndianWriteBuffer le_buffer(PacketIndexEntry::kStoreSize);
  do {
    le_buffer.reset();
    le_buffer << index_merge.Current();
    index_writer->Pwrite(le_buffer.data(), le_buffer.size(), index_pos);
    index_pos += le_buffer.size();
  } while (index_merge.Advance());
  index_merge.Close();

  index_writer->SyncAndClose();
}

// Close - close the index.  If the index was written in sorted order,
// just rename the temporary index to be the permanent index.  If the
// index was written sorted by levels, do a merge into the final
// index.  If the index was not sorted in any recognizable way, do a
// full sort.

void PacketIndexWriter::Close(size_t max_sort_buffer) {
  khLockGuard lock(modify_lock_);

  // Flush and close the temporary index file
  index_writer_->SyncAndClose();
  index_writer_.clear();

  // Check if index is empty
  if (index_pos_ == (off64_t)PacketFile::kIndexHeaderSize) {
    // no sorting to do when empty, just rename
    if (!khRename(index_temp_path_, index_path_)) {
      throw khSimpleException("PacketIndexWriter::Close: index rename failed, ")
        << index_temp_path_ << " -> " << index_path_;
    }
  } else {  // not empty, we need to sort it
    // Determine if sorting is needed
    if (is_preorder_) {
      // Already in preorder, just rename the temporary index
      if (!khRename(index_temp_path_, index_path_)) {
        throw khSimpleException("PacketIndexWriter::Close: index rename failed, ")
          << index_temp_path_ << " -> " << index_path_;
      }
    } else if (is_level_ordered_) {
      // Multiple levels, sorted within each level. Do a merge of sorted
      // levels.
      MergeIndex(index_temp_path_, level_region_);
      // Delete the temporary index file
      if (!khUnlink(index_temp_path_)) {
        notify(NFY_WARN, "PacketIndexWriter::Close: failed to unlink %s",
               index_temp_path_.c_str());
      }
    } else {
      // Not written in any recognizeable order - sort whole index
      SortIndex(max_sort_buffer);
    }
  }
}
