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

// Shared classes for structures internal to the file package.

#include <notify.h>
#include <string.h>
#include <fstream>  // NOLINT(readability/streams)
#include <string>
#include "common/khFileUtils.h"
#include "fusion/portableglobe/shared/file_package.h"

namespace fusion_portableglobe {

const int Package::kCrcSize = 4;
const int Package::kCrcOffset = 4;
const int Package::kVersionSize = 8;
const int Package::kVersionOffset = 12;
const int Package::kIndexOffsetSize = 8;
const int Package::kIndexOffsetOffset = 20;

IncrementalCrcCalculator::IncrementalCrcCalculator() : crc_(0) {
  remaining_bytes_.reserve(Package::kCrcSize);
}


void IncrementalCrcCalculator::CalculateCrc(const char* buffer,
                                            size_t buffer_size) {
  std::uint32_t next_word;
  if (remaining_bytes_.size() != 0) {
    int i = remaining_bytes_.size();
    for (; i < Package::kCrcSize && buffer_size != 0;
         ++i, --buffer_size, ++buffer) {
      remaining_bytes_.push_back(*buffer);
    }
    if (i < Package::kCrcSize) {
      return;
    }
    char *ptr = reinterpret_cast<char*>(&next_word);
    ptr[0] = remaining_bytes_[0];
    ptr[1] = remaining_bytes_[1];
    ptr[2] = remaining_bytes_[2];
    ptr[3] = remaining_bytes_[3];
    crc_ ^= next_word;
    remaining_bytes_.clear();
  }
  for (; buffer_size >= static_cast<size_t>(Package::kCrcSize);
       buffer_size -= Package::kCrcSize, buffer += Package::kCrcSize) {
    char *ptr = reinterpret_cast<char*>(&next_word);
    ptr[0] = buffer[0];
    ptr[1] = buffer[1];
    ptr[2] = buffer[2];
    ptr[3] = buffer[3];
    crc_ ^= next_word;
  }
  for (;buffer_size != 0; --buffer_size, ++buffer) {
    remaining_bytes_.push_back(*buffer);
  }
}


/**
 * Calculate 4-byte crc for the file.
 */
 std::uint32_t Package::CalculateCrc(std::string file_path,
                             std::uint64_t bytes_at_end_to_discard) {
  // Get length of file.
  std::uint64_t length = FileSize(file_path);
  if (!length) {
    return 0;
  }

  // Don't read crc or (length % 4) last bytes.
  // The last few bytes are part of the version and will be checked separately.
  std::uint64_t num_of_reads = (length - bytes_at_end_to_discard) / kCrcSize;
  std::uint32_t crc = 0;
  std::uint32_t next_word;
  std::ifstream fp_in(file_path.c_str(), std::ios::binary);
  for (std::uint64_t i = 0; i < num_of_reads; ++i) {
    fp_in.read(reinterpret_cast<char*>(&next_word), kCrcSize);
    crc ^= next_word;
  }
  fp_in.close();
  return crc;
}

/**
 * Read 4-byte crc (last 4 bytes) from the file.
 */
 std::uint32_t Package::ReadCrc(std::string file_path) {
  // Get length of file.
  std::uint64_t length = FileSize(file_path);
  if (!length) {
    return 0;
  }

  // Crc is last 4 bytes.
  std::uint32_t crc = 0;
  std::ifstream fp_in(file_path.c_str(), std::ios::binary);
  fp_in.seekg(length - kCrcOffset);
  fp_in.read(reinterpret_cast<char*>(&crc), kCrcSize);
  return crc;
}

/**
 * Read 8-byte version from the file and return as string.
 */
std::string Package::ReadVersion(std::string file_path) {
  // Get length of file.
  std::uint64_t length = FileSize(file_path);
  if (!length) {
    return "no file";
  }

  // Version is last 8 bytes before crc.
  std::string version;
  version.resize(8);
  std::ifstream fp_in(file_path.c_str(), std::ios::binary);
  fp_in.seekg(length - kVersionOffset);
  fp_in.read(&version[0], kVersionSize);
  return version;
}

/**
 * Get file length.
 */
 std::uint64_t Package::FileSize(std::string file_path) {
  // Get length of file.
  std::ifstream fp_in(file_path.c_str());
  std::uint64_t length = 0;
  if (fp_in) {
    fp_in.seekg(0L, std::ios::end);
    length = fp_in.tellg();
    fp_in.close();
  }
  return length;
}

}  // namespace fusion_portableglobe
