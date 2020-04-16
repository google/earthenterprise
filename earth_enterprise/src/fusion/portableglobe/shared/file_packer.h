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

// Class for packing up a globe into a single file.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_FILE_PACKER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_FILE_PACKER_H_

#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include <vector>
#include "common/khConstants.h"
#include "fusion/portableglobe/shared/file_package.h"

namespace fusion_portableglobe {

class CrcWrappingOfstream : public std::ofstream {
 public:
  explicit CrcWrappingOfstream(IncrementalCrcCalculator* crc_calculator) :
      crc_calculator_(*crc_calculator) {}

  void open(const char * file_name, ios_base::openmode mode = ios_base::in) {
    file_name_ = file_name;
    std::ofstream::open(file_name, mode);
  }

  std::ostream& write(const char* s, std::streamsize n) {
    crc_calculator_.CalculateCrc(s, n);
    return std::ofstream::write(s, n);
  }

  void WriteCrcSideFile() const {
    std::ofstream fp_out;
    std::string side_file_for_crc = file_name_ + kCrcFileExtension;
    fp_out.open(side_file_for_crc.c_str(), std::ios::out);
    std::uint32_t crc = crc_calculator_.GetCrc();
    fp_out.write(reinterpret_cast<const char*>(&crc), sizeof(crc));
    fp_out.close();
  }

 private:
  IncrementalCrcCalculator& crc_calculator_;
  std::string file_name_;
};

/**
 * Class for packing files into a single package file.
 */
class FilePacker {
 public:
  /**
   * Constructor for standalone packed file.
   */
  FilePacker(const std::string& output_file,
             const std::string& base_file, size_t prefix_len);
  /**
   * Constructor for addendum to an existing parent file, rather than
   * to a standalone packed file. It should be possible to concatenate
   * the addendum to the parent file, making both the files in the
   * parent file and the addendum available. If a file in the
   * addendum has the same path as a file in the parent file, then
   * the file in the addendum should always be returned for
   * that path.
   */
  FilePacker(const std::string& output_file, const std::string& parent_file);
  ~FilePacker();

  /**
   * Adds file to the package.
   */
  void AddFile(const std::string& path, size_t prefix_len);

  /**
   * Add all files from directory to the package. Only skips
   * the "." and ".." paths in the directory.
   */
  void AddAllFiles(const std::string& path, size_t prefix_len);

 private:
  // Offset to add to all index entries. 0 for standalone file.
  std::uint64_t addendum_offset_;
  // Whether file is an addendum (rather than standalone)
  bool is_addendum_;
  std::uint64_t write_pos_;
  CrcWrappingOfstream fp_out_;
  std::string file_path_;
  IncrementalCrcCalculator crc_calculator_;

  std::vector<PackageIndexEntry> index_;

  void AddIndexToPackage();

  void AddVersionToPackage();

  void AddCrcToPackage();
};

}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_SHARED_FILE_PACKER_H_
