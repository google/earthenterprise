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

// Class for reading from a Fusion database via the Apache module

#ifndef GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_APACHE_FDB_READER_H_
#define GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_APACHE_FDB_READER_H_

#include <http_request.h>
#include <http_log.h>
#include <http_protocol.h>
#include <ap_compat.h>
#include <apr_date.h>
#include <fstream>  // NOLINT(readability/streams)
#include <string>
#include "fusion/portableglobe/servers/fileunpacker/shared/glc_reader.h"
#include "common/geFilePool.h"
//#include "common/khTypes.h"
#include <cstdint>

class ApacheFdbReader : public GlcReader {
 public:
  ApacheFdbReader(
      const std::string& file_path, geFilePool& file_pool);
  ~ApacheFdbReader();

  /**
   * @return whether reader was able open file to read.
   */
  virtual bool IsOpen() const;

  /**
   * Reads data into string buffer. Resizes the string first.
   * @param buffer Destination for the data.
   * @param offset Starting point for the data in the file.
   * @param size Size of data in bytes to read.
   * @return whether data was read.
   */
  virtual bool Read(std::string* buffer, std::uint64_t offset, std::uint64_t size) const;
  virtual bool Read(
      std::string* buffer, std::uint64_t offset, std::uint64_t size, request_rec* r) const;

  /**
   * Reads data into given memory location from the given
   * position in the file.
   * @param ptr Pointer to memory where data will be stored.
   * @param offset Starting point for the data in the file.
   * @param size Size of data to read.
   * @return whether data was read.
   */
  virtual bool ReadData(void* ptr, std::uint64_t offset, std::uint64_t size) const;

  /**
   * Returns the size of the file or 0 if there is a problem with
   * the file.
   */
  virtual std::uint64_t Size() const;

  /**
   * Returns the 3-char suffix of the file being read.
   */
  virtual std::string Suffix() const { return suffix_; }

 private:
  mutable geFilePool::Reader reader_;
  std::string suffix_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_SERVER_MOD_FDB_APACHE_FDB_READER_H_
