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

#include "./file_package.h"

#include <string>
#include "./portable_glc_reader.h"

const int Package::kCrcSize = 4;
const int Package::kCrcOffset = 4;
const int Package::kVersionSize = 8;
const int Package::kVersionOffset = 12;
const int Package::kIndexOffsetSize = 8;
const int Package::kIndexOffsetOffset = 20;

/**
 * Calculate 4-byte crc for the file.
 */
 std::uint32_t Package::CalculateCrc(const GlcReader& glc_reader) {
  // Get length of file.
  std::uint64_t length = glc_reader.Size();
  if (!length) {
    return 0;
  }

  // Don't read crc or (length % 4) last bytes.
  // The last few bytes are part of the version and will be checked separately.
  std::uint64_t num_of_reads = (length - kCrcSize) / kCrcSize;
  std::uint32_t crc = 0;
  std::uint32_t next_word;
  std::uint64_t offset = 0;
  for (std::uint64_t i = 0; i < num_of_reads; ++i) {
    // Maybe a little inefficient because of implicit seek for each read.
    glc_reader.ReadData(&next_word, offset, kCrcSize);
    crc ^= next_word;
    offset += kCrcSize;
  }
  return crc;
}

/**
 * Read 4-byte crc (last 4 bytes) from the file.
 */
 std::uint32_t Package::ReadCrc(const GlcReader& glc_reader) {
  // Get length of file.
  std::uint64_t length = glc_reader.Size();
  if (!length) {
    return 0;
  }

  // Crc is last 4 bytes.
  std::uint32_t crc;
  glc_reader.ReadData(&crc, length - kCrcOffset, kCrcSize);
  return crc;
}

/**
 * Read 8-byte version from the file and return as string.
 */
std::string Package::ReadVersion(const GlcReader& glc_reader) {
  // Get length of file.
  std::uint64_t length = glc_reader.Size();
  if (!length) {
    return "none";
  }

  // Version is last 8 bytes before crc.
  std::string version;
  glc_reader.Read(&version, length - kVersionOffset, kVersionSize);
  return version;
}

