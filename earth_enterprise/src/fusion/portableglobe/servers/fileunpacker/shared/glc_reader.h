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


// Virtual class for reading the glc file.
// This class wraps the implementation which may be different for
// the Portable than it is for the Apache plugin.
#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_GLC_READER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_GLC_READER_H_

#include <fstream>  // NOLINT(readability/streams)
#include <string>

class GlcReader {
 public:

  virtual ~GlcReader() {};

  /**
   * @return whether reader was able open file to read.
   */
  virtual bool IsOpen() const = 0;

  /**
   * Reads data into string buffer. Resizes the string first.
   * @param buffer Destination for the data.
   * @param offset Starting point for the data in the file.
   * @param size Size of data in bytes to read.
   * @return whether data was read.
   */
  virtual bool Read(std::string* buffer, std::uint64_t offset, std::uint64_t size) const = 0;

  /**
   * Reads data into given memory location from the given
   * position in the file.
   * @param ptr Pointer to memory where data will be stored.
   * @param offset Starting point for the data in the file.
   * @param size Size of data to read.
   * @return whether data was read.
   */
  virtual bool ReadData(void* ptr, std::uint64_t offset, std::uint64_t size) const = 0;

  /**
   * Returns the size of the file or 0 if there is a problem with
   * the file.
   */
  virtual std::uint64_t Size() const = 0;

  /**
   * Returns the 3-char suffix of the file being read.
   */
  virtual std::string Suffix() const = 0;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_GLC_READER_H_

