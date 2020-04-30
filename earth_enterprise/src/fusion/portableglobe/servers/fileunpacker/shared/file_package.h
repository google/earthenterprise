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


// Shared classes for structures internal to the file package.
// Used by file packer and unpacker.
//
// Package structure:
//   File0 data
//   File1 data
//   ...
//   FileN data
//   Index info
//     Relative path, offset, and size  of each file in package
//   8 byte offset to index
//   8 byte version
//   4 byte crc
//
// TODO: Generate these files that need to work on multiple OS's
// TODO: from a template.
//
// TODO: Consider implementing a crc that can be generated more
// TODO: easily on the file. E.g. add crc to packages, padding
// TODO: at boundaries, so that crc's can be combined easily.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_FILE_PACKAGE_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_FILE_PACKAGE_H_

#include <string>
#include "./glc_reader.h"

/**
 * Class for holding information about file's location and size in the
 * package.
 */
class PackageFileLoc {
 public:
  PackageFileLoc() { }
  PackageFileLoc(std::int64_t offset, std::uint64_t size)
      : offset_(offset), size_(size) { }

  // Normalize negative offset by the filesize. I.e. use it as a negative
  // offset from the end of the file. Useful for adding a gla to the end
  // of a glc without having to know the history of updates it has been
  // through. E.g. positive offsets might reference core (large) glbs or
  // glms in a glc, and then small updates could be added repeatedly using
  // negative offsets to the updated data, without having to worry how many
  // updates might have been added to the glc previously.
  void NormalizeNegativeOffset(std::uint64_t file_size) {
    if (offset_ < 0) {
      offset_ += file_size;
    }
  }

  // Add base offset to offset. Useful for access files within files,
  // such as a glb stored within a glc.
  void AddBaseToOffset(std::uint64_t base) { offset_ += base; }
  void Set(std::int64_t offset, std::uint64_t size) {
     offset_ = offset;
     size_ = size;
  }

  std::int64_t Offset() const { return offset_; }
  std::uint64_t Size() const { return size_; }

  // For Swig and accessing values via Python.
  int LowOffset() { return (offset_ & 0x000000000ffffffff); }
  int HighOffset() { return ((offset_ >> 32) & 0x000000000ffffffff); }
  int LowSize() { return (size_ & 0x000000000ffffffff); }
  int HighSize() { return ((size_ >> 32) & 0x000000000ffffffff); }

 private:
  std::int64_t offset_;
  std::uint64_t size_;
};

/**
 * Class for holding index information for each file in package.
 */
class PackageIndexEntry {
 public:
  PackageIndexEntry() { }
  PackageIndexEntry(
      const std::string& path, const PackageFileLoc& file_loc)
      : path_(path), file_loc_(file_loc) { }
  std::string Path() const { return path_; }
  PackageFileLoc FileLoc() const { return file_loc_; }
 private:
  std::string path_;
  PackageFileLoc file_loc_;
};

/**
 * Class for static package methods
 */
class Package {
 public:
  // Use int so swig translates to python nicely.
  // Size of crc in bytes.
  static const int kCrcSize;
  // Offset to crc from end of file in bytes.
  static const int kCrcOffset;
  // Size of version in bytes.
  static const int kVersionSize;
  // Offset to version from end of file in bytes.
  static const int kVersionOffset;
  // Size of index offset in bytes.
  static const int kIndexOffsetSize;
  // Offset to index offset from end of file in bytes.
  static const int kIndexOffsetOffset;

  /**
   * Calculate crc for the given file.
   */
  static std::uint32_t CalculateCrc(const GlcReader& glc_reader);

  /**
   * Read crc for given file from the file.
   */
  static std::uint32_t ReadCrc(const GlcReader& glc_reader);

  /**
   * Read version string from the file.
   */
  static std::string ReadVersion(const GlcReader& glc_reader);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_FILE_PACKAGE_H_

