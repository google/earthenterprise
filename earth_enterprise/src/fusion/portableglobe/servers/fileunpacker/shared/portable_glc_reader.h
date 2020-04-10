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


// Sub class for reading the glc file from the Portable Server.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_PORTABLE_GLC_READER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_PORTABLE_GLC_READER_H_

#include <stdio.h>
#include <string>
#include "./glc_reader.h"

// Work around for problems in Windows with files >2G.
#ifdef __MINGW32__
#define ftell ftello64
#define fseek fseeko64
#endif

class PortableGlcReader : public GlcReader {
 public:
  explicit PortableGlcReader(const char* path);
  ~PortableGlcReader();

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

  /**
   * Return the name of the file without the suffix.
   */
  virtual std::string Filename() const { return filename_nosuffix_; }

 private:
  mutable int glc_file_;
  std::uint64_t glc_file_size_;
  const std::string path_;
  std::string filename_nosuffix_;
  std::string suffix_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SERVERS_FILEUNPACKER_SHARED_PORTABLE_GLC_READER_H_

