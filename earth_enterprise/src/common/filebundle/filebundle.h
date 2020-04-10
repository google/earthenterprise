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


#ifndef GEO_EARTH_ENTERPRISE_SRC_COMMON_FILEBUNDLE_FILEBUNDLE_H_
#define GEO_EARTH_ENTERPRISE_SRC_COMMON_FILEBUNDLE_FILEBUNDLE_H_

#include <vector>
#include <string>
#include "common/base/macros.h"
#include "common/khGuard.h"
#include "common/geFilePool.h"
#include "common/ManifestEntry.h"

class CachedReadAccessor;

// FileBundle is a private class which provides common capabilities to
// the reader and writer.  Don't include this file directly, use
// filebundlewriter.h or filebundlereader.h and the respective
// classes.
//
// FileBundle provides the ability to read, write, and modify a very
// large virtual file made up of multiple smaller files.  The segments
// are each less than a specified limit (default 1GB).  All files
// belonging to a FileBundle are in the same directory.  The directory
// path is used to specify the location of the FileBundle.  The size
// of each bundle segment is limited to 2^31 bytes (2GB) to accomodate
// old style unix file addressing, which uses signed long ints (32
// bits).
//
// A header file (bundle.hdr) contains information about the
// FileBundle, including a list of the files composing the FileBundle,
// and the segment size used to create the file.
//
// Addressing within a FileBundle: data within a FileBundle is
// accessed by a 64-bit signed address within the FileBundle.
// Internally, the data portion of the FileBundle consists of one or
// more separate segment files.  Each segment within a FileBundle may
// be of any length <= the segment limit for the particular file.
// When appending a record to a FileBundle, a new segment file will be
// started when the record would cause the last existing segment to
// become greater in length than the segment limit.
//
// Note that FileBundle has no external concept of a "current
// position".  All reads and rewrites must specify a position, and
// append uses the current end of file.  This allows for an
// implementation which gives reproducible results in a multi-threaded
// environment.

// Header file format (see FileBundleWriter::SaveHeaderFile() and
// FileBundle::LoadHeaderFile() for code to write and read).
// All header file elements stored in little endian format.
//
// Size(bytes)   Description
// -----------   -------------------------------------------------------
// char[16]      File signature: "FileBundleHeader" (no 0 terminator)
// std::uint32_t        File format version
// std::uint32_t        Count of segments
// std::uint64_t        Segment break value
// char[]        Original abs path to header (not incl. file name), 0 terminated
// For each segment:
//   char[]      Original abs path to segment (not incl. file name),
//               0 terminated
//   char[]      Variable length segment file name, 0 terminated
//   std::uint32_t      Segment size (bytes)
// std::uint32_t        CRC-32 of file contents

// FileBundleSegment - information for file segment (private to
// FileBundle).  This is an abstract class which will be subclassed by
// FileBundleReader and FileBundleWriter to provide facilities
// specific to those classes.

// jagadev 02/24/2010; Modified to avoid absolute file names in disk for
// portable globe.
//
// * writting all directories relative to directory where bundle.hdr is.
// * keep file_name simple name always.
// * convert back the directories while reading only if those are not absolute.
// * Take special care to leave empty directories as empty both while writing
// and reading. relative path of current directory is kept as "." rather than
// empty "".
class FileBundleSegment {
 public:
  virtual ~FileBundleSegment() {}
  typedef enum { kAnySegment = 0, kReaderSegment = 1, kWriterSegment = 2 } Type;
  virtual Type SegmentType() const = 0;
  virtual void Close() = 0;
  virtual void Pread(void *buffer, size_t size, off64_t offset) = 0;
  const std::string &name() const { return name_; }
  const std::string &orig_abs_path() const { return orig_abs_path_; }
  std::uint32_t data_size() const { return data_size_; }
  void IncrementDataSize(std::uint32_t increment) { data_size_ += increment; }
  void set_data_size(std::uint32_t data_size) { data_size_ = data_size; }
  void clear_orig_abs_path() { orig_abs_path_.clear(); }
  // Unique segment id within a bundle (only runtime, not persisted).
  std::uint32_t id() const { return id_; }

 protected:
  // Initialize a FileBundleSegment with the filename, the absolute path and
  // a unique ID. The "id" must be unique within a FileBundle at runtime and
  // is not persisted across runs.
  FileBundleSegment(const std::string &filename,
                    const std::string &orig_abs_path,
                    std::uint32_t id);

 private:
  // Note name_ can be absolute path and in such a case orig_abs_path_ is not
  // used.
  const std::string name_;     // file name (without path)
  std::string orig_abs_path_;  // original path (without file name)
  std::uint32_t data_size_;
  std::uint32_t id_;                  // Unique segment id within the file bundle.
  DISALLOW_COPY_AND_ASSIGN(FileBundleSegment);
};

// FileBundleReaderSegment - instantiable class for file segments for
// Reader (internal use only).
//
// Filename is normally a relative path.  The server publish code may
// replace this with an absolute path, in which case ignore the
// current abs_path and just use the path stored in the filename.

class FileBundleReaderSegment : public FileBundleSegment {
 public:
  // Initialize a FileBundleReaderSegment with the file pool, filename,
  // the absolute path, original absolute path and a unique ID.
  // The "id" must be unique within a FileBundle at runtime and
  // is not persisted across runs.
  FileBundleReaderSegment(geFilePool &file_pool,
                          const std::string &abs_path,
                          const std::string &filename,
                          const std::string &orig_abs_path,
                          std::uint32_t id);
  virtual Type SegmentType() const { return kReaderSegment; }
  virtual ~FileBundleReaderSegment() {}
  virtual void Pread(void *buffer, size_t size, off64_t offset) {
    reader_->Pread(buffer, size, offset);
  }
  std::uint64_t Filesize() const { return reader_->Filesize(); }
  virtual void Close() {}
 private:
  // Helper object for caching reads.
  khDeleteGuard<geFilePool::Reader> reader_;
  DISALLOW_COPY_AND_ASSIGN(FileBundleReaderSegment);
};

// FileBundleWriterSegment - instantiable class for file segments for
// Writer (internal use only)

class FileBundleWriterSegment : public FileBundleSegment {
 public:
  // Initialize a FileBundleWriterSegment with the file pool, filename,
  // the absolute path, original absolute path, the file mode and a unique ID.
  // The "id" must be unique within a FileBundle at runtime and
  // is not persisted across runs.
  FileBundleWriterSegment(geFilePool &file_pool,
                          const std::string &abs_path,
                          const std::string &filename,
                          std::uint32_t id,
                          mode_t mode);
  virtual ~FileBundleWriterSegment();

  virtual Type SegmentType() const { return kWriterSegment; }

  virtual void Close();
  void Pwrite(const void *buffer, size_t size, off64_t offset);
  virtual void Pread(void *buffer, size_t size, off64_t offset) {
    writer_->Pread(buffer, size, offset);
  }
 private:
  khDeleteGuard<geFilePool::Writer> writer_;
  DISALLOW_COPY_AND_ASSIGN(FileBundleWriterSegment);
};

// FileBundle abstract class for common definitions

class EndianReadBuffer;
class EndianWriteBuffer;

// Convenience class to store record address (offset and size in
// bundle).
class FileBundleAddr {
  std::uint64_t offset;
  std::uint32_t size;
 public:
  inline FileBundleAddr(std::uint64_t offset_, std::uint32_t size_) :
      offset(offset_), size(size_) { }
  inline FileBundleAddr(void) : offset(0), size(0) { }

  inline bool operator==(const FileBundleAddr &o) const {
    return ((offset == o.offset) &&
            (size == o.size));
  }
  inline bool operator!=(const FileBundleAddr &o) const {
    return !operator==(o);
  }
  inline operator bool(void) const { return size != 0; }
  inline std::uint64_t Offset(void) const { return offset; }
  inline std::uint32_t Size(void)   const { return size; }

  void Push(EndianWriteBuffer &buf) const;
  void Pull(EndianReadBuffer &buf);
};

class FileBundle {
 public:
  // Constants
  static const size_t kCRCsize;  // bytes needed for CRC
  static const std::string kHeaderSignature;
  static const std::uint32_t kFormatVersion;
  static const std::string kHeaderFileName;

  // Data retrieval
  // Enable cached reading, with "max_blocks" number of cache blocks each
  // of size "block_size".
  // Suggested usage is max_blocks=2, block_size=30MB for a linear read through
  // a FileBundle.
  // For a quadtree traversal, perhaps use max_blocks=10, block_size=5MB.
  // max_blocks must be >= 2.
  void EnableReadCache(std::uint32_t max_blocks, std::uint32_t block_size);

  void ReadAt(std::uint64_t read_pos, void *buffer, size_t read_len) const;
  void ReadAt(const FileBundleAddr &address, void *buffer) const {
    ReadAt(address.Offset(), buffer, address.Size());
  }
  // Assumption: buffer must be large enough to receive data.
  void ReadAt(std::uint64_t read_pos, std::string* buffer, const size_t offset,
              const size_t read_len) const {
    assert((offset + read_len) <= buffer->size());
    ReadAt(read_pos, &((*buffer)[offset]), read_len);
  }

  // Read read_len bytes data to buffer.
  void ReadAt(std::uint64_t read_pos, std::string* buffer,
              const size_t read_len) const {
    if (buffer->size() < read_len) {
      // This is done so that while resizing the characters are not copied.
      buffer->clear();
    }
    buffer->resize(read_len);
    ReadAt(read_pos, &((*buffer)[0]), read_len);
  }

  void ReadAtCRC(std::uint64_t read_pos, void *buffer, size_t read_len) const;
  void ReadAtCRC(const FileBundleAddr &address, void *buffer) const {
    ReadAtCRC(address.Offset(), buffer, address.Size());
  }

  // Read at the buffer (*buffer)[offset]; Returns the number of bytes read,
  // excluding the number of bytes for CRC.
  // Assumption: buffer must be large enough to receive data.
  size_t ReadAtCRC(std::uint64_t read_pos, std::string* buffer, const size_t offset,
                   const size_t read_len) const {
    assert((offset + read_len) <= buffer->size());
    ReadAtCRC(read_pos, &((*buffer)[offset]), read_len);
    return read_len - FileBundle::kCRCsize;
  }

  // Read read_len - crc_bytes data to buffer. Return read_len - crc_bytes.
  size_t ReadAtCRC(std::uint64_t read_pos, std::string* buffer,
                   const size_t read_len) const {
    if (buffer->size() < read_len) {
      buffer->clear();
      buffer->resize(read_len);
    }
    ReadAtCRC(read_pos, &((*buffer)[0]), read_len);
    const size_t data_len = read_len - FileBundle::kCRCsize;
    buffer->resize(data_len);
    return data_len;
  }

  std::uint64_t segment_break() const { return segment_break_; }
  std::uint64_t data_size() const { return data_size_; }
  const std::string &abs_path() const { return abs_path_; }
  void GetSegmentList(std::vector<std::string> *segment_list) const;
  void GetSegmentOrigPaths(std::vector<std::string> *orig_list) const;
  virtual void Close() = 0;

  // Translate old pack file linear position (addressing continuous
  // across segment boundary) to FileBundle position (segment start
  // addresses are multiples of segment break).  Throws exception if
  // invalid position.
  std::uint64_t LinearToBundlePosition(std::uint64_t linear_position) const;

  // Determine if specified position is within an existing, writeable
  // segment
  bool IsWriteable(std::uint64_t bundle_pos) const;

  void AppendManifest(std::vector<ManifestEntry> &manifest,
                      const std::string& tmp_dir) const;
  void BuildHeaderWriteBuffer(EndianWriteBuffer &buffer) const;

  // used only by geextractdbs
  void ClearOrigPaths(void);
  std::string hdr_orig_abs_path() const { return hdr_orig_abs_path_; }
  std::string HeaderPath() const { return abs_path_ + kHeaderFileName; }
  geFilePool &file_pool() const { return file_pool_; }

 protected:
  // Utility to delete the existing cached_read_accessor_ if any.
  void CleanupCachedReadAccessor();
  // These two constants should be consistent:
  // kSegmentFileCountMax <= 10^kSegmentFileSuffixLength
  static const std::uint32_t kSegmentFileSuffixLength;
  static const std::uint32_t kSegmentFileCountMax;

  // Translate position and length in bundle to position in segment.
  // Validate position and length (cannot cross segment boundary).
  // Output is pointer to segment, and offset within segment.
  void PositionToSegment(std::uint64_t bundle_pos,
                         size_t buffer_len,
                         FileBundleSegment::Type seg_type,
                         FileBundleSegment **segment,
                         std::int32_t *segment_offset) const;

  // Methods for dealing with header file
  // As a side effect set was_relative_paths_ to false if some paths were abs.
  void LoadHeaderFile();                // load data fields from file

  // Data member accessors
  std::uint32_t header_size() const { return header_size_; }
  void set_header_size(size_t header_size) { header_size_ = header_size; }
  void IncrementDataSize(std::uint64_t increment) { data_size_ += increment; }
  void set_hdr_orig_abs_path() {
    hdr_orig_abs_path_ = abs_path();
  }

  void ReserveSegments(size_t segment_count) {
    segment_.reserve(segment_count);
  }
  void AddSegment(FileBundleSegment *new_segment) {
    segment_.push_back(new_segment);
  }
  FileBundleSegment *PopBackSegment() {
    FileBundleSegment *back_segment = segment_.back();
    segment_.pop_back();
    return back_segment;
  }
  FileBundleSegment *Segment(size_t i) const { return segment_.at(i); }
  FileBundleSegment *LastSegment() const { return segment_.back(); }
  size_t SegmentCount() const { return segment_.size(); }
  bool SegmentsEmpty() const { return segment_.empty(); }

  FileBundle(geFilePool &file_pool,
             const std::string &path,
             std::uint64_t segment_break);

  // To be used by Manifest Generating readers
  FileBundle(geFilePool &file_pool,
             const std::string &path,
             std::uint64_t segment_break,
             const std::string &prefix);

 public:
  virtual ~FileBundle();

 private:
  friend class FileBundleUnitTest;
  friend class CachedReadAccessorUnitTest;
  friend class PacketFileUnitTest;
  // Instance data
  geFilePool &file_pool_;       // pool to use for readers/writers
  const std::string abs_path_;          // absolute path to directory
  const std::uint64_t segment_break_;          // max segment size
  // segments will usually be
  // strictly smaller than the break size (but never greater).
  std::uint64_t data_size_;
  size_t header_size_;          // size of header file
  std::string hdr_orig_abs_path_;       // orig path to which hdr written

  khDeletingVector<FileBundleSegment> segment_;

 protected:
  // Assign unique segment id's within the filebundle, only needed for runtime
  // segment identification, these id's are not persistent.
  int last_segment_id_;
  bool was_relative_paths_;
  // In case file bundle is not kept in original asset_root
  // location, prefix_ needs to be added to absolute paths
  // to get current path.
  const std::string prefix_for_abs_paths;
  // Helper object for caching reads.
  mutable khDeleteGuard<CachedReadAccessor> cached_read_accessor_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FileBundle);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_COMMON_FILEBUNDLE_FILEBUNDLE_H_
