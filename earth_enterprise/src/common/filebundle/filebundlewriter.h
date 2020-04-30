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


#ifndef COMMON_FILEBUNDLE_FILEBUNDLEWRITER_H__
#define COMMON_FILEBUNDLE_FILEBUNDLEWRITER_H__

#include <sys/types.h>
#include <sys/stat.h>
#include "filebundle.h"

// FileBundleWriter
//
// Write methods for file bundles. See filebundle.h for an overview of
// FileBundle capabilities and common data and methods.
//
// The constructor for FileBundleWriter will create a new, empty
// bundle.  WriteAppend can be used to write new records, returning
// the record location.  WriteAt can be used to rewrite data already
// written.  CRC version of both these functions are supplied.
//
// FileBundleUpdateWriter will open an existing bundle.  Existing
// segments will be read-only, WriteAppend will write to new segments.
// WriteAt may be used only in new segments.  Attempts to write to
// pre-existing segments will cause an exception.
//
// FileBundleWriter supports optional bundle-level write buffering. This is a
// tradeoff, so it is off by default. There is a single mutex lock for the
// buffering, so multi-threaded write performance can suffer with buffering. On
// the other hand, buffering writes can increase write speed for small
// sequential writes/appends, in particular for sync-mounted remote filesystems
// like NAS's.

class FileBundleWriterSegment;

// FileBundleWriter creates a new file bundle, optionally overwriting
// any existing bundle at the destination.

class FileBundleWriter : public FileBundle {
 public:
  static const std::uint64_t kDefaultSegmentBreak = 1024 * 1024 * 1024;
  static const mode_t kDefaultMode = S_IRUSR | S_IWUSR | S_IXUSR
    | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH;
  static const mode_t kExecuteMode = S_IXUSR | S_IXGRP | S_IXOTH;

  FileBundleWriter(geFilePool &file_pool,
                   const std::string &path,
                   bool overwrite = true,
                   mode_t mode = kDefaultMode,
                   std::uint64_t segment_break = kDefaultSegmentBreak);
  virtual ~FileBundleWriter();

  // allocates space for a later call to WriteAt
  std::uint64_t AllocateAppend(size_t write_len);

  std::uint64_t WriteAppend(const void *buffer, size_t write_len);
  virtual void WriteAt(std::uint64_t write_pos, const void *buffer, size_t write_len);
  void WriteAt(const FileBundleAddr &address, const void *buffer) {
    WriteAt(address.Offset(), buffer, address.Size());
  }
  // CRC versions of Write and WriteAt will overwrite last 4
  // (kCRCsize) bytes in buffer with CRC of previous bytes.  Buffer
  // should be 4 bytes larger than actual data.
  virtual std::uint64_t WriteAppendCRC(void *buffer, size_t write_len);
  virtual void WriteAtCRC(std::uint64_t write_pos, void *buffer, size_t write_len);
  void WriteAtCRC(const FileBundleAddr &address, void *buffer) {
    WriteAtCRC(address.Offset(), buffer, address.Size());
  }
  virtual void Checkpoint();
  virtual void Close();
  mode_t mode() const { return mode_; }

  // Buffer writes with the given size buffer. write_buffer_size is in bytes. 0
  // turns off buffering (default). Concurrent reading/writing results are
  // undefined.
  void BufferWrites(size_t write_buffer_size);
  // Write any contents in the write buffer to disk.
  void FlushWriteBuffer();

  // Returns a filename like bundle.abcd, abcd being digits
  static std::string SegmentFileName(unsigned int segment);

 protected:
  FileBundleWriter(geFilePool &file_pool, // for use by update writer only
                   const std::string &path,
                   mode_t mode,
                   std::uint64_t segment_break);
  virtual void CreateSegment();
  khMutex &modify_lock() { return modify_lock_; }
 private:
  static const std::string kSegmentFilePrefix;
  mode_t mode_;
  khMutex modify_lock_;                 // lock when modifying

  // Variables to support buffering continuous writes within a single segment.
  size_t wb_size_;                       // Size of total write buffer
  khDeleteGuard<char, ArrayDeleter> write_buffer_;  // The buffer
  FileBundleWriterSegment *wb_segment_;  // The segment being buffered
  off_t wb_buffer_offset_;               // Buffer offset in segment
  size_t wb_length_;                     // Number of bytes currently in buffer

  // Write the buffer to the given offset in the segment. Optionally buffers
  // continuous writes within a single segment.
  void WriteToSegment(FileBundleWriterSegment *segment,
                      off_t segment_offset,
                      const void *buffer,
                      size_t write_len);
  // Write any contents in the write buffer to disk. Assumes the modify_lock_
  // has been locked.
  void FlushWriteBufferLocked();
  void BufferCRC(void *buffer, size_t buffer_len);
  void AllocateBlock(size_t block_size,
                     FileBundleWriterSegment **segment,
                     off_t *seg_position,
                     std::uint64_t *bundle_position);
  void SaveHeaderFile();
  void SaveHeaderFileWithLock();
  DISALLOW_COPY_AND_ASSIGN(FileBundleWriter);
};

// FileBundleUpdateWriter opens an existing file bundle with read-only
// access to existing segments, and appends new segments.  The
// "WriteAt" functions are allowed only on newly created segments.

class FileBundleUpdateWriter : public FileBundleWriter {
 public:
  FileBundleUpdateWriter(geFilePool &file_pool,
                         const std::string &path,
                         mode_t mode = kDefaultMode);
  virtual ~FileBundleUpdateWriter() {}
 private:
  DISALLOW_COPY_AND_ASSIGN(FileBundleUpdateWriter);
};

// FileBundlePackImport opens an old-style "pack" file and creates a
// bundle header for it.  The data segments are read-only.
//
// WARNING: FileBundle uses a different addressing scheme than
// pack.idx, so use the supplied address translation function.  The
// old-style addressing treated the pack file segments as a continuous
// address space.  FileBundle segments may have unused address space
// between the end of a segment and the next segment break.
// This class has unbuffered read functionality of FileBundleReader also.
class FileBundlePackImport : public FileBundleWriter {
 public:
  FileBundlePackImport(geFilePool &file_pool,
                       const std::string &path,
                       mode_t mode = kDefaultMode,
                       std::uint64_t segment_break = 1024 * 1024 * 1024);
  virtual void CreateSegment();
  virtual void CloseNoUpdate();         // don't write header file
 private:
  DISALLOW_COPY_AND_ASSIGN(FileBundlePackImport);
};

#endif  // COMMON_FILEBUNDLE_FILEBUNDLEWRITER_H__
