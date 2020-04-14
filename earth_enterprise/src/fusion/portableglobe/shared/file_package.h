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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_FILE_PACKAGE_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_FILE_PACKAGE_H_

#include <string>

namespace fusion_portableglobe {

/**
 * Class for holding information about file's location and size in the
 * package.
 */
class PackageFileLoc {
 public:
  PackageFileLoc() { }
  PackageFileLoc(std::uint64_t offset, std::uint64_t size)
      : offset_(offset), size_(size) { }
  std::uint64_t Offset() const { return offset_; }
  std::uint64_t Size() const { return size_; }
 private:
  std::uint64_t offset_;
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
  static std::uint32_t CalculateCrc(std::string file_path,
                             std::uint64_t bytes_at_end_to_discard = kCrcSize);

  /**
   * Read crc for given file from the file.
   */
  static std::uint32_t ReadCrc(std::string file_path);

  /**
   * Read version string from the file.
   */
  static std::string ReadVersion(std::string file_path);

  /**
   * Returns size of file.
   */
  static std::uint64_t FileSize(std::string file_path);
};

// We know that CRC calculator calculates in chunks of 4 bytes and ignores the
// last < 4 odd bytes. But data may come incrementally in chunks of non-even
// multiples of 4. This class helps in that case.
// Usage
// IncrementalCrcCalculator calculator;
// calculator.CalculateCrc(buffer1, buffer1_size)
// calculator.CalculateCrc(buffer2, buffer2_size)
// ..............................................
// calculator.CalculateCrc(buffern, buffern_size)
// calculator.GetCrc()
// calculator.GetRemainingBytes()
class IncrementalCrcCalculator {
 public:
  IncrementalCrcCalculator();
  void CalculateCrc(const char* buffer, size_t buffer_size);
  std::uint32_t GetCrc() const { return crc_; }
  std::string GetRemainingBytes() const { return remaining_bytes_; }
  void SetCrc(std::uint32_t crc) { crc_ = crc; }
  void SetRemainingBytes(const std::string str) { remaining_bytes_ = str; }
 private:
  std::string remaining_bytes_;
  std::uint32_t crc_;
};

}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_FILE_PACKAGE_H_
