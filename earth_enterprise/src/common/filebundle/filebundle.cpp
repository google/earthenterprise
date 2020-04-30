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


#include "common/filebundle/filebundle.h"

#include "common/notify.h"
#include "common/khSimpleException.h"
#include "common/khFileUtils.h"
#include "common/khEndian.h"
#include "common/filebundle/filebundlewriter.h"
#include "common/filebundle/cachedreadaccessor.h"
#include "third_party/rsa_md5/crc32.h"

//-----------------------------------------------------------------------------

// FileBundle

// Constants
const size_t FileBundle::kCRCsize = kCRC32Size;
const std::string FileBundle::kHeaderSignature("FileBundleHeader");
const std::uint32_t FileBundle::kFormatVersion = 0x00000002;
const std::string FileBundle::kHeaderFileName("bundle.hdr");
const std::uint32_t FileBundle::kSegmentFileSuffixLength = 4;
const std::uint32_t FileBundle::kSegmentFileCountMax = 10000;


// FileBundleSegment

// Constructor/destructor

FileBundleSegment::FileBundleSegment(const std::string &filename,
                                     const std::string &orig_abs_path,
                                     std::uint32_t id)
    : name_(filename),
      orig_abs_path_(orig_abs_path),
      data_size_(0), id_(id) {
}

FileBundleReaderSegment::FileBundleReaderSegment(
    geFilePool &file_pool,
    const std::string &abs_path,
    const std::string &filename,
    const std::string &orig_abs_path,
    std::uint32_t id)
    : FileBundleSegment(filename, orig_abs_path, id),
      reader_(TransferOwnership(new geFilePool::Reader(
                                    file_pool, khIsAbspath(filename)
                                    ? filename : abs_path + filename))) {
  set_data_size(reader_->Filesize());
}

FileBundleWriterSegment::FileBundleWriterSegment(
    geFilePool &file_pool,
    const std::string &abs_path,
    const std::string &filename,
    std::uint32_t id,
    mode_t mode)
    : FileBundleSegment(filename, abs_path, id),
      writer_(TransferOwnership(
                  new geFilePool::Writer(geFilePool::Writer::ReadWrite,
                                         geFilePool::Writer::Truncate,
                                         file_pool,
                                         abs_path + filename,
                                         mode))) {
}

FileBundleWriterSegment::~FileBundleWriterSegment() {
}

void FileBundleWriterSegment::Close() {
  writer_->SyncAndClose();

  const std::string segment_abs_path =
      khIsAbspath(name()) ? name() : khComposePath(orig_abs_path(), name());
  // Make sure that in case no bundle file is generated, due to empty Segment,
  // at least make an empty bundle file for following processing step
  // (bundle.xxxx). It is required because we write bundle.xxxx-references
  // into bundle header file (bundle.hdr).
  if (!khExists(segment_abs_path)) {
#if defined(NDEBUG)
    khMakeEmptyFile(segment_abs_path);  // function issues NFY_WARNING
#else
    assert(khMakeEmptyFile(segment_abs_path));
#endif
  }
}

void FileBundleWriterSegment::Pwrite(const void *buffer, size_t size,
                                     off64_t offset) {
  if (size == 0) {
    throw khSimpleException(
        "Internal Error: Attempt to write 0 bytes to offset")
          << offset << " in file " << name();
  }
  writer_->Pwrite(buffer, size, offset);
}


FileBundle::FileBundle(geFilePool & file_pool,
                       const std::string &path,
                       std::uint64_t segment_break)
    : file_pool_(file_pool),
      abs_path_(khNormalizeDir(khAbspath(path))),
      segment_break_(segment_break),
      data_size_(0),
      last_segment_id_(0),
      was_relative_paths_(true),
      prefix_for_abs_paths("") {
  assert(!path.empty());
}


FileBundle::FileBundle(geFilePool & file_pool,
                       const std::string &path,
                       std::uint64_t segment_break,
                       const std::string &prefix)
    : file_pool_(file_pool),
      abs_path_(khNormalizeDir(khAbspath(path))),
      segment_break_(segment_break),
      data_size_(0),
      last_segment_id_(0),
      was_relative_paths_(true),
      prefix_for_abs_paths(prefix) {
  assert(!path.empty());
}

FileBundle::~FileBundle() {
  CleanupCachedReadAccessor();
}

void FileBundle::CleanupCachedReadAccessor() {
  cached_read_accessor_.clear();
}

void FileBundle::EnableReadCache(std::uint32_t max_blocks, std::uint32_t block_size) {
  CleanupCachedReadAccessor();
  // Don't enable read cache for less than 2 blocks.
  if (max_blocks >= 2) {
    cached_read_accessor_ = TransferOwnership(
        new CachedReadAccessor(max_blocks, block_size));
  }
}

// FileBundleAddr

void FileBundleAddr::Push(EndianWriteBuffer &buf) const {
  buf << offset << size;
}

void FileBundleAddr::Pull(EndianReadBuffer &buf) {
  buf >> offset >> size;
}

// Load and verify header file

void FileBundle::LoadHeaderFile() {
  LittleEndianReadBuffer buffer;
  file_pool().ReadStringFile(HeaderPath(), &buffer);
  header_size_ = buffer.size();

  try {
    // Validate header "signature"
    const char *loaded_signature = buffer.PullRaw(kHeaderSignature.size());
    if (0 != memcmp(loaded_signature,
                    kHeaderSignature.data(),
                    kHeaderSignature.size())) {
      throw khSimpleException(
        "FileBundleReader: corrupt header file, bad signature: ")
          << HeaderPath();
    }

    // Check CRC of header
    std::uint32_t computed_crc = Crc32(buffer.data(), buffer.size() - kCRCsize);
    std::string::size_type after_sig = buffer.Seek(buffer.size() - kCRCsize);
    std::uint32_t loaded_crc;
    buffer.Pull(&loaded_crc);
    if (computed_crc != loaded_crc) {
      throw khSimpleException("FileBundleReader: corrupt header, bad CRC: ")
        << HeaderPath();
    }
    buffer.resize(buffer.size() - kCRCsize);  // remove CRC from buffer end
    buffer.Seek(after_sig);                   // restore position

    // Extract numeric data values
    std::uint32_t format_version;
    buffer.Pull(&format_version);
    if (format_version != kFormatVersion) {
      throw khSimpleException("FileBundleReader: unknown header version ")
        << format_version << " in " << HeaderPath();
    }

    std::uint32_t segment_count;
    buffer.Pull(&segment_count);
    if (segment_count == 0  ||  segment_count > kSegmentFileCountMax) {
      throw khSimpleException("FileBundleReader: corrupt header segment count ")
        << segment_count << " in " << HeaderPath();
    }

    std::uint64_t break_value;
    buffer.Pull(&break_value);
    if (break_value == 0) {
      throw khSimpleException(
          "FileBundleReader: corrupt header, segment break is 0")
            << " in " << HeaderPath();
    }
    *const_cast<std::uint64_t *>(&segment_break_) = break_value;

    // Extract orig abs path for header file

    buffer.Pull(&hdr_orig_abs_path_);
    if (!hdr_orig_abs_path_.empty()) {
      if (!khIsAbspath(hdr_orig_abs_path_)) {
        hdr_orig_abs_path_ = (hdr_orig_abs_path_ == "." ? abs_path_ :
                              khNormalizeDir(abs_path_ + hdr_orig_abs_path_));
      } else {
        if (!prefix_for_abs_paths.empty()) {
          hdr_orig_abs_path_ = prefix_for_abs_paths + hdr_orig_abs_path_;
        }
        was_relative_paths_ = false;
      }
      assert(khExists(hdr_orig_abs_path_));
    }

    // Extract segment info: original abs path, segment file name, and size

    ReserveSegments(segment_count);

    for (std::uint32_t i = 0; i < segment_count; ++i) {
      // Copy the zero-terminated file name to a string
      std::string orig_abs_path;
      buffer.Pull(&orig_abs_path);
      if (!orig_abs_path.empty()) {
        if (!khIsAbspath(orig_abs_path)) {
          orig_abs_path = (orig_abs_path == "." ? abs_path_ :
                           khNormalizeDir(abs_path_ + orig_abs_path));
        } else {
          if (!prefix_for_abs_paths.empty()) {
            orig_abs_path = prefix_for_abs_paths + orig_abs_path;
          }
          was_relative_paths_ = false;
        }
        assert(khExists(orig_abs_path));
      }

      std::string file_name;
      buffer.Pull(&file_name);
      if (khIsAbspath(file_name)) {
        if (!prefix_for_abs_paths.empty()) {
          file_name = prefix_for_abs_paths + file_name;
        }
        assert(khExists(file_name));
        file_name = khRelativePath(abs_path_, file_name);
        was_relative_paths_ = false;
      }
      std::uint32_t segment_size;
      buffer.Pull(&segment_size);

      // Add to segment table
      FileBundleReaderSegment *new_segment =
        new FileBundleReaderSegment(file_pool(),
                                    abs_path(),
                                    file_name,
                                    orig_abs_path,
                                    last_segment_id_++);
      AddSegment(new_segment);
      if (new_segment->Filesize() > segment_break()) {
        throw khSimpleException("FileBundleReader: corrupt header or segment, "
                                "file size ")
          << new_segment->Filesize() << " for segment " << i
          << " > segment break " << segment_break()
          << " in " << HeaderPath();
      }
      if (new_segment->Filesize() != segment_size) {
        throw khSimpleException("FileBundleReader: corrupt header or segment, "
                                "file size ")
          << new_segment->Filesize() << " for segment " << i
          << " does not match stored file size " << segment_size
          << " in " << HeaderPath();
      }
      IncrementDataSize(new_segment->Filesize());
    }

    if (!buffer.AtEnd()) {
      throw khSimpleException(
          "FileBundleReader: corrupt header, garbage after segment list")
            << " in " << HeaderPath();
    }
  } catch(...) {
    notify(NFY_WARN,
           "FileBundleReader:: failed to open FileBundle %s",
           HeaderPath().c_str());
    throw;
  }
}

// BuildHeaderWriteBuffer
//
// Format the header for writing to a file.
//
// jagadev 02/24/2010; Modified to avoid absolute file names in disk for
// portable globe.
//
// * writing all directories relative to directory where bundle.hdr is.
// * keep file_name simple name always.
// * convert back the directories while reading only if those are not absolute.
// * Take special care to leave empty directories as empty both while writing
// and reading. relative path of current directory is kept as "." rather than
// empty "".
void FileBundle::BuildHeaderWriteBuffer(EndianWriteBuffer &buffer) const {
  if (SegmentsEmpty()) {
    throw khSimpleException("FileBundle::BuildHeaderWriteBuffer: not open: ")
      << abs_path();
  }

  buffer << FixedLengthString(kHeaderSignature, kHeaderSignature.size());

  buffer.push(kFormatVersion);
  buffer.push(static_cast<std::uint32_t>(SegmentCount()));
  buffer.push(segment_break());
  // Convert all absolute paths to relative paths w.r.t abs_path_
  buffer.push(hdr_orig_abs_path_.empty() ? hdr_orig_abs_path_
              : (abs_path_ == hdr_orig_abs_path_ ? std::string(".")
                 : khRelativePath(abs_path_, hdr_orig_abs_path_)));

  // For each segment, append original abs path, file name, and size
  for (size_t i = 0; i < SegmentCount(); ++i) {
    const std::string& orig_abs_path = Segment(i)->orig_abs_path();
    buffer.push(orig_abs_path.empty() ? orig_abs_path
                :(abs_path_ == orig_abs_path ? std::string(".")
                  : khRelativePath(abs_path_, orig_abs_path)));
    const std::string& segment_name = Segment(i)->name();
    buffer.push(khIsAbspath(segment_name) ?
                khRelativePath(abs_path_, segment_name) : segment_name);
    buffer.push(Segment(i)->data_size());
  }
}

// PositionToSegment - translate position and length in bundle to
// position in segment.  Validate position and length (cannot cross
// segment boundary).  Output is pointer to segment, and offset within
// segment.

void FileBundle::PositionToSegment(std::uint64_t bundle_pos,
                                   size_t buffer_len,
                                   FileBundleSegment::Type seg_type,
                                   FileBundleSegment **segment,
                                   std::int32_t *segment_offset) const {
  std::uint64_t seg_number = bundle_pos / segment_break();
  std::uint64_t seg_offset = bundle_pos % segment_break();

  // Make sure position is completely within a valid segment
  if (seg_number >= SegmentCount()
      ||  seg_offset + buffer_len > Segment(seg_number)->data_size()) {
    throw khSimpleException("Internal Error: FileBundle::PositionToSegment: ")
      << "position " << bundle_pos << " not within segment";
  }

  *segment = Segment(seg_number);
  *segment_offset = seg_offset;
  if (seg_type != FileBundleSegment::kAnySegment
      && (*segment)->SegmentType() != seg_type) {
    throw khSimpleException("Internal Error: FileBundle::PositionToSegment: ")
      << "operation type " << seg_type << " does not match segment type "
      << (*segment)->SegmentType();
  }
}


// IsWriteable - determine if specified position is within an
// existing, writeable segment
bool FileBundle::IsWriteable(std::uint64_t bundle_pos) const {
  std::uint64_t seg_number = bundle_pos / segment_break();
  std::uint64_t seg_offset = bundle_pos % segment_break();
  return seg_number < SegmentCount()
    && Segment(seg_number)->SegmentType() == FileBundleSegment::kWriterSegment
    && seg_offset < Segment(seg_number)->data_size();
}

// Translate old pack file linear position (addressing continuous
// across segment boundary) to FileBundle position (segment start
// addresses are multiples of segment break).  Throws exception if
// invalid position.

 std::uint64_t FileBundle::LinearToBundlePosition(std::uint64_t linear_position) const {
  std::uint64_t remainder = linear_position;
  size_t seg = 0;
  while (seg < segment_.size()  &&  remainder >= Segment(seg)->data_size()) {
    remainder -= Segment(seg)->data_size();
    ++seg;
  }
  if (seg < segment_.size()) {
    return seg * segment_break_ + remainder;
  } else {
    throw khSimpleException("FileBundle::LinearToBundlePosition: ")
      << "linear position " << linear_position << " is beyond end of bundle";
  }
}

// ReadAt - read data at specified position in segment.  Throws
// exception if error.

void FileBundle::ReadAt(std::uint64_t read_pos, void *buffer, size_t read_len) const {
  // Make sure position is completely within a valid segment
  FileBundleSegment *segment;
  std::int32_t seg_offset;
  PositionToSegment(read_pos,
                    read_len,
                    FileBundleSegment::kAnySegment,
                    &segment,
                    &seg_offset);

  // Perform the read
  if (segment == NULL) {
    notify(NFY_DEBUG, "FileBundle::ReadAt: NULL Segment\n");
  } else if (seg_offset + read_len > segment->data_size()) {
    notify(NFY_DEBUG, "FileBundle::ReadAt: bad read_len or seg_offset\n");
  }
  if (cached_read_accessor_) {  // Use the cached reader when available.
    cached_read_accessor_->Pread(*segment, buffer, read_len, seg_offset);
  } else {  // Use the non-cached reader otherwise.
    segment->Pread(buffer, read_len, seg_offset);
  }
}

void FileBundle::ReadAtCRC(std::uint64_t read_pos, void *buffer,
                           size_t read_len) const {
  if (read_len <= kCRCsize) {
    throw khSimpleException
      ("FileBundleReader::ReadAtCRC: buffer size <= CRC size");
  }

  ReadAt(read_pos, buffer, read_len);

  size_t data_len = read_len - kCRCsize;
  std::uint32_t computed_crc = Crc32(buffer, data_len);
  std::uint32_t file_crc;
  memcpy(&file_crc,
         reinterpret_cast<char*>(buffer) + data_len,
         sizeof(file_crc));
  file_crc = LittleEndianToHost(file_crc);
  if (computed_crc != file_crc) {
    // Get file name from segment for error report
    FileBundleSegment *segment;
    std::int32_t seg_offset;
    PositionToSegment(read_pos,
                      read_len,
                      FileBundleSegment::kAnySegment,
                      &segment,
                      &seg_offset);
    throw khSimpleException("FileBundleReader::ReadAtCRC: CRC mismatch, ")
        << "offset=" << seg_offset << ", len=" << read_len
        << ", file: " << khComposePath(abs_path(), segment->name());
  }
}

// Get list of file names of segments

void FileBundle::GetSegmentList(std::vector<std::string> *segment_list) const {
  segment_list->clear();
  segment_list->reserve(segment_.size());

  for (khDeletingVector<FileBundleSegment>::const_iterator cur_seg
         = segment_.begin();
       cur_seg != segment_.end();
       ++cur_seg) {
    segment_list->push_back((*cur_seg)->name());
  }
}

void FileBundle::GetSegmentOrigPaths(
    std::vector<std::string> *orig_list) const {
  orig_list->clear();
  orig_list->reserve(segment_.size());

  for (khDeletingVector<FileBundleSegment>::const_iterator cur_seg
           = segment_.begin();
       cur_seg != segment_.end();
       ++cur_seg) {
    orig_list->push_back((*cur_seg)->orig_abs_path());
  }
}

// Get manifest list (includes header)
void FileBundle::AppendManifest(std::vector<ManifestEntry> &manifest,
                                const std::string& tmp_dir) const {
  size_t segment_count = SegmentCount();
  size_t next = manifest.size();
  manifest.resize(next + segment_count + 1);
  ManifestEntry* entry = &manifest[next];

  // Add header to manifest - IndexManifest requires this to be first
  // so that it will be second after the index header in the overall
  // manifest for the index.
  const std::string header_path = HeaderPath();
  entry->orig_path = hdr_orig_abs_path().empty() ? header_path :
      hdr_orig_abs_path() + kHeaderFileName;
  std::string* curr_header_path = &entry->current_path;
  std::uint64_t* curr_size = &entry->data_size;
  // Note: We need to give the size exactly as it will look like when we finally
  // transfer, not what it is now. This is required as we match file sizes along
  // with file name while doing the delta transfer.
  if (was_relative_paths_ == false) {
    LittleEndianWriteBuffer buf;
    BuildHeaderWriteBuffer(buf);
    *curr_size = buf.size() + kCRC32Size;
    if (!tmp_dir.empty()) {
      *curr_header_path = khComposePath(tmp_dir, header_path);
      khEnsureParentDir(*curr_header_path);
      FileBundle::file_pool().WriteSimpleFileWithCRC(*curr_header_path, buf);
      assert(*curr_size == khGetFileSizeOrThrow(*curr_header_path));
    } else {
      *curr_header_path = header_path;
    }
  } else {
    *curr_size = header_size();
    *curr_header_path = header_path;
    assert(*curr_size == khGetFileSizeOrThrow(*curr_header_path));
  }

  // Add segments to manifest
  size_t i;
  for (i = 0; i < segment_count; ++i) {
    ++next;
    FileBundleSegment *segment = Segment(i);
    manifest[next].current_path = abs_path() + segment->name();
    if (!segment->orig_abs_path().empty()) {
      manifest[next].orig_path = segment->orig_abs_path() + segment->name();
    } else {
      manifest[next].orig_path = manifest[next].current_path;
    }
    manifest[next].data_size = segment->data_size();
  }
}

void FileBundle::ClearOrigPaths(void) {
  hdr_orig_abs_path_.clear();
  unsigned int segment_count = SegmentCount();
  for (unsigned int i = 0; i < segment_count; ++i) {
    Segment(i)->clear_orig_abs_path();
  }
}
