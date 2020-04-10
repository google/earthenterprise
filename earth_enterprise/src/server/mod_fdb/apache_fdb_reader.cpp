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


// Class for reading from a Fusion database.

#include "server/mod_fdb/apache_fdb_reader.h"
#include <http_config.h>
#include <string>
#include "common/geFilePool.h"
#include "common/khSimpleException.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/glc_reader.h"

#ifdef APLOG_USE_MODULE
APLOG_USE_MODULE(fdb);
#endif

/**
 * Constructor.
 */
ApacheFdbReader::ApacheFdbReader(
    const std::string& file_path, geFilePool& file_pool)
    : reader_(file_pool, file_path) {
  if (file_path.size() > 3) {
    suffix_ = file_path.substr(file_path.size() - 3, 3);
  } else {
    suffix_ = file_path;
  }
}

ApacheFdbReader::~ApacheFdbReader() { }

/**
 * Always true here because constructor fails if file does not exist.
 */
bool ApacheFdbReader::IsOpen() const {
  return true;
}

/**
 * Reads data into string buffer. Resizes the string first.
 */
bool ApacheFdbReader::Read(
    std::string* buffer, std::uint64_t offset, std::uint64_t size, request_rec* r) const {
  buffer->resize(size);
  return ReadData(&(*buffer)[0], offset, size);
}

/**
 * Reads data into string buffer. Resizes the string first.
 */
bool ApacheFdbReader::Read(
    std::string* buffer, std::uint64_t offset, std::uint64_t size) const {
  buffer->resize(size);
  return ReadData(&(*buffer)[0], offset, size);
}

/**
 * Reads the data from the globe file depending on what was last found.
 * Exceptions propagate to caller.
 */
bool ApacheFdbReader::ReadData(void* buffer, std::uint64_t offset, std::uint64_t size) const {
  reader_.Pread(buffer, size, offset);
  return true;
}

/**
 * Returns the size of the file or 0 if there is a problem with
 * the file.
 */
 std::uint64_t ApacheFdbReader::Size() const {
  return reader_.Filesize();
}
