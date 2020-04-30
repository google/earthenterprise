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


// Methods for packing up all files in a globe into a single
// package file.

#include "fusion/portableglobe/shared/file_packer.h"
#include <notify.h>
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include <vector>
#include "common/khFileUtils.h"
#include "common/khConstants.h"
#include "fusion/portableglobe/shared/file_package.h"
#include "fusion/portableglobe/shared/file_unpacker.h"

namespace fusion_portableglobe {

// This is called in case --make_copy is not used,
// and so we are trying to reuse writings to the first file.
// This method tries to reuse crc of that file.
// Throws exception in case reading of the file fails.
// Returns size of the file in bytes.
size_t GetCrc(std::string file_name, IncrementalCrcCalculator* crc_calculator) {
  const size_t file_size = khGetFileSizeOrThrow(file_name);

  std::string crc_file = file_name + kCrcFileExtension;
  bool crc_found = false;
  std::uint32_t crc = 0;
  // [1] Check if crc file exists, if so then use that.
  if (khExists(crc_file)) {
    std::ifstream fp_in(crc_file.c_str(), std::ios::binary);
    if (!fp_in) {
      notify(NFY_WARN, "Unable to open file: '%s'", crc_file.c_str());
    } else {
      fp_in.read(reinterpret_cast<char*>(&crc), 4);
      crc_found = true;
    }
  }
  // [2] Else calculate crc
  if (!crc_found) {
    crc = Package::CalculateCrc(file_name, 0);
  }
  crc_calculator->SetCrc(crc);
  const size_t remaining = file_size % 4;
  if (remaining != 0) {
    // Since for file size of non-even-multiple of 4 last <4 bytes have not
    // been used for crc, pre-fill crc_calculator with those few bytes.
    std::ifstream fp_in(file_name.c_str(), std::ios::binary);
    // Throw exception if above fails.
    std::streamoff offset = remaining;
    fp_in.seekg(-offset, std::ios_base::end);
    char buff[3];
    fp_in.read(buff, remaining);
    crc_calculator->SetRemainingBytes(std::string(buff, remaining));
  }
  return file_size;
}


/**
 * Constructor. If the file exists move to the end of it.
 * Otherwise create a new file.
 */
FilePacker::FilePacker(const std::string& output_file,
                       const std::string& base_file,
                       size_t prefix_len)
    : addendum_offset_(0), is_addendum_(false),
      fp_out_(&crc_calculator_), file_path_(output_file) {
  if (output_file == base_file) {
    // Get the end of the file position. Calculate the CRC as well.
    write_pos_ = GetCrc(output_file, &crc_calculator_);
    // Write index entry for the base file.
    PackageFileLoc file_loc(0, write_pos_);
    const std::string base_path = output_file.substr(prefix_len + 1);
    PackageIndexEntry entry(base_path, file_loc);
    index_.push_back(entry);

    // Move to the end of the file.
    fp_out_.open(file_path_.c_str(), std::ios::app);
  } else {
    fp_out_.open(file_path_.c_str(), std::ios::out);
    write_pos_ = 0;
    AddFile(base_file, prefix_len);
  }
}


/**
 * Constructor for addendum to an existing parent file.
 */
FilePacker::FilePacker(const std::string& output_file,
                       const std::string& parent_file)
    : is_addendum_(true), fp_out_(&crc_calculator_),
      file_path_(output_file) {
  // If output file exists, it will be cleared.
  assert(output_file != parent_file);
  addendum_offset_ = khGetFileSizeOrThrow(parent_file);

  fp_out_.open(file_path_.c_str(), std::ios::out);
  write_pos_ = 0;

  // Read in parent file's index and copy entries to this index.
  FileUnpacker unpacker(parent_file.c_str());
  const std::map<std::string, PackageFileLoc>&
      index = unpacker.Index();
  std::map<std::string, PackageFileLoc>::const_iterator it;
  for (it = index.begin(); it != index.end(); ++it) {
    PackageFileLoc file_loc(it->second.Offset(), it->second.Size());
    PackageIndexEntry entry(it->first, file_loc);
    index_.push_back(entry);
  }
}


/**
 * Destructor. Add index to the end of the package file. Then add
 * offset to index and crc. Finally, close the file.
 */
FilePacker::~FilePacker() {
  AddIndexToPackage();
  AddVersionToPackage();
  AddCrcToPackage();
}


/**
 * Appends file to the package being created and adds its information
 * to the index.
 */
void FilePacker::AddFile(const std::string& path, size_t prefix_len) {
  // Get length of file.
  std::uint64_t length = khGetFileSizeOrThrow(path);

  std::ifstream fp_in(path.c_str(), std::ios::binary);
  if (!fp_in) {
    notify(NFY_FATAL, "Unable to open file: %s", path.c_str());
  }

  // Append file to end of package file.
  char buffer[4096];
  std::uint64_t bytes_remaining = length;
  size_t buffer_size = 4096;
  while (bytes_remaining > 0) {
    if (bytes_remaining < buffer_size) {
      buffer_size = bytes_remaining;
    }

    fp_in.read(buffer, buffer_size);
    fp_out_.write(buffer, buffer_size);
    bytes_remaining -= buffer_size;
  }

  fp_in.close();

  // Add file to index.
  const std::string base_path = path.substr(prefix_len + 1);
  PackageFileLoc file_loc(addendum_offset_ + write_pos_, length);
  PackageIndexEntry entry(base_path, file_loc);
  index_.push_back(entry);
  write_pos_ += length;
}

/**
 * Adds all files from directory to the package. Only skips
 * the "." and ".." paths in the directory.
 */
void FilePacker::AddAllFiles(const std::string& path, size_t prefix_len) {
  std::vector<std::string> file_list;
  khFindFilesInDir(path, file_list, "");
  for (size_t i = 0; i < file_list.size(); ++i) {
    if (khDirExists(file_list[i])) {
      AddAllFiles(file_list[i], prefix_len);
    } else {
      AddFile(file_list[i], prefix_len);
    }
  }
}

/**
 * Adds the index to the end of the package.
 */
void FilePacker::AddIndexToPackage() {
  std::uint64_t index_offset = addendum_offset_ + write_pos_;

  // Save all of the files in the index.
  std::uint32_t num_files = index_.size();
  fp_out_.write(reinterpret_cast<char*>(&num_files), 4);
  for (size_t i = 0; i < num_files; ++i) {
    std::string path = index_[i].Path();
    std::uint16_t path_len = path.size();
    fp_out_.write(reinterpret_cast<char*>(&path_len),
                  sizeof(path_len));
    fp_out_.write(&path[0], path_len);

    PackageFileLoc file_loc = index_[i].FileLoc();
    fp_out_.write(reinterpret_cast<char*>(&file_loc),
                  sizeof(PackageFileLoc));
  }

  // Save offset to index.
  fp_out_.write(reinterpret_cast<char*>(&index_offset), 8);
}

/**
 * Adds the version to the end of the package.
 */
void FilePacker::AddVersionToPackage() {
  // Save version.
  fp_out_.write(kPortableGlobeVersion.c_str(), 8);
}


/**
 * Adds the crc to the end of the package.
 */
void FilePacker::AddCrcToPackage() {
  // Save crc.
  const std::uint32_t crc = crc_calculator_.GetCrc();
  fp_out_.write(reinterpret_cast<const char*>(&crc), 4);
  fp_out_.close();
}

}  // namespace fusion_portableglobe
