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


#include <cstdint>

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <third_party/rsa_md5/crc32.h>
#include <notify.h>
#include <khThread.h>
#include <khSimpleException.h>
#include <khFileUtils.h>
#include <khEndian.h>
#include "filebundlewriter.h"

const std::string FileBundleWriter::kSegmentFilePrefix("bundle");

// Constructor - creates specified directory if it doesn't already exist.
//
// If a FileBundle already exists at the location(as determined by
// existence of a header file), an exception will be thrown if
// overwrite is false.  Otherwise, any exiting header file will be
// removed (any other files/subdirectories are not touched).  Note
// that even if overwrite is false, bundle.* files will still be
// overwritten (the assumption is that if there is no header, then any
// segment files are left over from a previous unsuccessful attempt).
//
// The constructor creates an empty first segment file.  The header is
// not written until Checkpoint or Close is called.


FileBundleWriter::FileBundleWriter(geFilePool &file_pool,
                                   const std::string &path,
                                   bool overwrite,
                                   mode_t mode,
                                   std::uint64_t segment_break)
    : FileBundle(file_pool, path, segment_break),
      mode_(mode),
      wb_size_(0),
      wb_segment_(NULL),
      wb_buffer_offset_(0),
      wb_length_(0) {
  // Check argument validity
  assert(segment_break != 0);

  // Create the directory (and parents in path) if not already there
  if (overwrite) {
    if (!khMakeCleanDir(abs_path(), mode)) {
      throw khSimpleException("FileBundleWriter: could not create clean dir: ")
        << abs_path();
    }
  } else if (!khMakeDir(abs_path(), mode_)) {
    throw khSimpleException("FileBundleWriter: could not create dir: ")
      << abs_path();
  }

  // If already exists and overwrite not allowed, error
  std::string header_path = HeaderPath();
  bool exists = khExists(header_path);
  if (exists) {
    throw khSimpleException("FileBundleWriter: destination already exists: ")
      << header_path;
  }

  // Create the first segment file
  CreateSegment();
}

// Alternate protected constructor, used only by subclasses.
// Doesn't create new bundle.

FileBundleWriter::FileBundleWriter(geFilePool &file_pool,
                                   const std::string &path,
                                   mode_t mode,
                                   std::uint64_t segment_break)
    : FileBundle(file_pool, path, segment_break),
      mode_(mode),
      wb_size_(0),
      wb_segment_(NULL),
      wb_buffer_offset_(0),
      wb_length_(0) {
}


// FileBundleUpdateWriter - opens existing file bundle, allows adding
// additional segments.  File bundle directory and header must be
// writable.  Only existing segments are readable, only new segments
// are writable.

FileBundleUpdateWriter::FileBundleUpdateWriter(geFilePool &file_pool,
                                               const std::string &path,
                                               mode_t mode)
       : FileBundleWriter(file_pool, path, mode, 0) {
  // Get header of existing file bundle - throws exception if not
  // found.  Existing segments are loaded as read-only.
  LoadHeaderFile();

  // Start a new (empty) writable segment
  CreateSegment();
}

// FileBundlePackImport opens an old-style "pack" file and creates a
// bundle for it.  Writing is not allowed (CreateSegment() is disabled).
// All pack file segment sizes must be <= the specified segment break.
// A bundle header file will be written on Close.

FileBundlePackImport::FileBundlePackImport(geFilePool &file_pool,
                                           const std::string &path,
                                           mode_t mode,
                                           std::uint64_t segment_break)
    : FileBundleWriter(file_pool, path, mode, segment_break) {
  // Get names of pack files
  std::vector<std::string> pack_names;
  khGetNumericFilenames(abs_path() + "pack", pack_names);
  if (pack_names.size() == 0) {
    throw khSimpleException("FileBundleImportPack: no pack files found at ")
      << abs_path();
  }

  // Sort segments into order
  std::sort(pack_names.begin(), pack_names.end());

  // Create reader for each segment
  ReserveSegments(pack_names.size());
  for (std::vector<std::string>::iterator pack_name = pack_names.begin();
       pack_name != pack_names.end();
       ++pack_name) {
    std::string::size_type slash_pos = pack_name->rfind('/');
    assert(slash_pos != std::string::npos);
    std::string file_name = pack_name->substr(slash_pos+1);

    FileBundleReaderSegment *new_segment =
      new FileBundleReaderSegment(file_pool, abs_path(), file_name, abs_path(),
                                  last_segment_id_++);
    FileBundle::AddSegment(new_segment);
    if (new_segment->Filesize() > segment_break) {
      throw khSimpleException("FileBundleImportPack: ")
        << " file " << abs_path() << file_name
        << ", size " << new_segment->Filesize()
        << " is > segment break " << segment_break;
    }
    IncrementDataSize(new_segment->Filesize());
  }
}

// Destructor - depends on parent class destructor to close all files.
// We deliberately don't try to save a header, since something
// probably has gone wrong if we get here and Close hasn't been called
// already.

FileBundleWriter::~FileBundleWriter() {
  if (!std::uncaught_exception()) {
    if (!SegmentsEmpty()) {
      notify(NFY_WARN,
             "FileBundleWriter not closed properly, header not updated: %s",
             abs_path().c_str());
    }
    if (wb_length_ != 0) {
      notify(NFY_FATAL,
             "FileBundleWriter write buffer not flushed: %s",
             abs_path().c_str());
    }
  }
}

// CreateSegment - create a new segment file named "bundle.nnnn" (nnnn is
// the segment number).  Except in the constructor, the caller must
// have locked the mutex before the call.  Any execute permissions
// are removed from the mode setting.

void FileBundleWriter::CreateSegment() {
  std::string seg_name = khMakeNumericFilename(kSegmentFilePrefix,
                                               SegmentCount(),
                                               kSegmentFileSuffixLength);
  if (SegmentCount() >= kSegmentFileCountMax) {
    throw khSimpleException
    ("FileBundleWriter::CreateSegment: max segment file count exceeded");
  }

  FileBundle::AddSegment(new FileBundleWriterSegment(file_pool(),
                                                     abs_path(),
                                                     seg_name,
                                                     last_segment_id_++,
                                                     mode_ & ~kExecuteMode));

  notify(NFY_DEBUG, "FileBundleWriter::CreateSegment added %llu: %s",
         static_cast<long long unsigned>(SegmentCount() - 1), seg_name.c_str());
}

std::string FileBundleWriter::SegmentFileName(unsigned int segment) {
  return khMakeNumericFilename(kSegmentFilePrefix, segment,
                               kSegmentFileSuffixLength);
}


void FileBundlePackImport::CreateSegment() {
  throw khSimpleException(
      "FileBundlePackImport: cannot add data to imported bundle ")
        << abs_path();
}

// AllocateBlock - thread safe file space allocator.  When writing to
// a file, we always use pwrite.  This routine allocates a block at
// the current end of file, starting a new segment if necessary.
//
// If we just queried the file position and then did a write, there's
// a very slight chance that some other thread might do a write in
// between the query and the write, resulting in a bad position.
// Rather than keeping the mutex locked during the entire write, we
// instead allocate a block while locked, then release the lock to do
// the write.  If another thread allocates and writes to a block
// farther in the file between our allocate and write, everything
// still works OK (Unix just creates a hole in file where our write
// will occur, and the the write fills the hole).
//
// The first argument specifies the size of the block needed.  The
// output arguments receive the segment to use, the position to
// write at within the file, and the position in the bundle.

void FileBundleWriter::AllocateBlock(size_t block_size,
                                     FileBundleWriterSegment **segment,
                                     off_t *seg_position,
                                     std::uint64_t *bundle_position) {
  assert(block_size <= segment_break());

  khLockGuard lock(modify_lock_);

  // Check for adequate room in current segment; if not, create a new
  // segment
  FileBundleSegment *cur_seg = LastSegment();
  if (cur_seg->data_size() + block_size > segment_break()) {
    CreateSegment();
    cur_seg = LastSegment();
  }

  assert(cur_seg->SegmentType() == FileBundleSegment::kWriterSegment);
  *segment = reinterpret_cast<FileBundleWriterSegment*>(cur_seg);
  *seg_position = cur_seg->data_size();
  cur_seg->IncrementDataSize(block_size);
  IncrementDataSize(block_size);
  *bundle_position = (SegmentCount() - 1) * segment_break() + *seg_position;
}

 std::uint64_t FileBundleWriter::AllocateAppend(size_t write_len) {
  FileBundleWriterSegment *segment;     // segment to write into
  off_t seg_position;                   // position in segment file
  std::uint64_t bundle_position;
  AllocateBlock(write_len, &segment, &seg_position, &bundle_position);

  return bundle_position;
}


void FileBundleWriter::BufferWrites(size_t write_buffer_size) {
  khLockGuard lock(modify_lock_);
  FlushWriteBufferLocked();
  wb_size_ = write_buffer_size;
  wb_segment_ = NULL;
  wb_buffer_offset_ = 0;
  wb_length_ = 0;
  if (wb_size_ > 0) {
    write_buffer_ = TransferOwnership(new char[wb_size_]);
  } else {
    write_buffer_.clear();
  }
}

// Externally-visible routine that locks the buffer.
void FileBundleWriter::FlushWriteBuffer() {
  khLockGuard lock(modify_lock_);
  FlushWriteBufferLocked();
}

// Internal-only routine assuming buffer is locked already.
void FileBundleWriter::FlushWriteBufferLocked() {
  if (write_buffer_ && wb_segment_ != NULL && wb_length_ != 0) {
    wb_segment_->Pwrite(&write_buffer_[0], wb_length_, wb_buffer_offset_);
  }
  wb_length_ = 0;
}

void FileBundleWriter::WriteToSegment(FileBundleWriterSegment *segment,
                                      off_t segment_offset,
                                      const void *buffer,
                                      size_t write_len) {
  // If the new write will not fit in the write buffer, then just write to disk,
  // ignoring the buffer.  This includes the unbuffered case.
  if (write_len > wb_size_) {
    segment->Pwrite(buffer, write_len, segment_offset);
  } else {
    // Buffer the writes.
    // Protect against multiple threads accessing the write buffer at once.
    khLockGuard lock(modify_lock_);

    // If we can't add the new write to the currently-buffered segment, then
    // flush the buffer.  To add to the buffer, the new write must be:
    //   - in the buffered segment,
    //   - appending to the end of the currently-buffered data, and
    //   - must fit in the buffer size.
    if (segment != wb_segment_ ||
        segment_offset != static_cast<off_t>(wb_buffer_offset_ + wb_length_) ||
        wb_length_ + write_len > wb_size_) {
      FlushWriteBufferLocked();
      wb_segment_ = segment;
      wb_buffer_offset_ = segment_offset;
      wb_length_ = 0;
    }

    // Now append the new data to the write buffer.
    assert(write_buffer_);
    assert(wb_length_ + write_len <= wb_size_);
    memcpy(&write_buffer_[0] + wb_length_, buffer, write_len);
    wb_length_ += write_len;
  }
}

// WriteAppend - write specified data at end of bundle.  Return
// position of start of data in bundle.

 std::uint64_t FileBundleWriter::WriteAppend(const void *buffer, size_t write_len) {
  if (SegmentsEmpty()) {               // if bundle closed
    throw khSimpleException
    ("FileBundleWriter::WriteAppend: bundle already closed");
  }

  FileBundleWriterSegment *segment;     // segment to write into
  off_t seg_position;                   // position in segment file
  std::uint64_t bundle_position;
  AllocateBlock(write_len, &segment, &seg_position, &bundle_position);
  WriteToSegment(segment, seg_position, buffer, write_len);
  return bundle_position;
}

// WriteAt - write data into specified position.  FileBundleWriter
// does no management of records; it is the callers responsibility to
// manage space within the bundle.  WriteAt does verify that the span
// of the new record is within the space already allocated, and does
// not span a segment boundary.

void FileBundleWriter::WriteAt(std::uint64_t write_pos,
                               const void *buffer,
                               size_t write_len) {
  // Make sure position is completely within a valid segment
  FileBundleSegment *segment;
  std::int32_t seg_offset;
  PositionToSegment(write_pos,
                    write_len,
                    FileBundleSegment::kWriterSegment,
                    &segment,
                    &seg_offset);

  // Write the data block
  FileBundleWriterSegment *writer =
    reinterpret_cast<FileBundleWriterSegment*>(segment);
  WriteToSegment(writer, seg_offset, buffer, write_len);
}

// BufferCRC - store CRC32 in last 4 bytes of buffer in little endian
// format.

void FileBundleWriter::BufferCRC(void *buffer, size_t buffer_len) {
  assert(buffer_len > kCRCsize);
  size_t data_len = buffer_len - kCRCsize;
  std::uint32_t crc = HostToLittleEndian(Crc32(buffer, data_len));
  memcpy(static_cast<char*>(buffer) + data_len, &crc, kCRCsize);
}

// WriteAppendCRC - store CRC32 of data in last 4 bytes of buffer in
// little endian format, then write data as in WriteAppend

 std::uint64_t FileBundleWriter::WriteAppendCRC(void *buffer, size_t write_len) {
  BufferCRC(buffer, write_len);
  return WriteAppend(buffer, write_len);
}

// WriteAtCRC - store CRC32 of data in last 4 bytes of buffer in
// little endian format, then write data as in WriteAt

void FileBundleWriter::WriteAtCRC(std::uint64_t write_pos,
                                  void *buffer,
                                  size_t write_len) {
  BufferCRC(buffer, write_len);
  WriteAt(write_pos, buffer, write_len);
}

// Checkpoint - write header for current state of file

void FileBundleWriter::Checkpoint() {
  FlushWriteBuffer();
  SaveHeaderFileWithLock();
}

// Close - write header, flush and close all member files

void FileBundleWriter::Close() {
  khLockGuard lock(modify_lock_);
  FlushWriteBufferLocked();
  SaveHeaderFile();
  while (!SegmentsEmpty()) {
    FileBundleSegment *close_seg = PopBackSegment();
    close_seg->Close();
    delete close_seg;
  }
}

// CloseNoUpdate - delete read-only member files, don't write bundle
// header to disc

void FileBundlePackImport::CloseNoUpdate() {
  FlushWriteBuffer();
  khLockGuard lock(modify_lock());
  while (!SegmentsEmpty()) {
    FileBundleSegment *close_seg = PopBackSegment();
    delete close_seg;
  }
}

// SaveHeaderFile - write data in packed format to header file.  See
// filebundle.h for header description.  NOTE: the mutex should be
// locked to prevent other access before calling this function (or use
// SaveFileHeaderWithLock).

void FileBundleWriter::SaveHeaderFile() {
  if (SegmentsEmpty()) {
    return;                             // already closed
  }

  // Build the header in a buffer in memory

  LittleEndianWriteBuffer buffer;
  BuildHeaderWriteBuffer(buffer);

  file_pool().WriteSimpleFileWithCRC(HeaderPath(),
                                     buffer,
                                     mode_ & ~kExecuteMode);
  set_header_size(buffer.size());
  set_hdr_orig_abs_path();
}

void FileBundleWriter::SaveHeaderFileWithLock() {
  khLockGuard lock(modify_lock_);
  SaveHeaderFile();
}
