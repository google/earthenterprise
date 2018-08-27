// Copyright 2017 Google Inc.
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


// Class for reading the glc file.
// Simple reader for use on Portable Server.
//
// Switched to C-based file reading to accommodate
// Windows problems with big (>2G) files.

#include "./portable_glc_reader.h"
#include <stdio.h>
#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include "./khTypes.h"

/**
 * Constructor.
 */
PortableGlcReader::PortableGlcReader(const char* path)
  : path_(path) {
  glc_file_ = fopen(path_.c_str(), "rb");

  auto last_sep = path_.find_last_of('/');
  if (last_sep == std::string::npos) {
    // No forward slash.  Look for backward slash in case we're on Windows.
    last_sep = path_.find_last_of('\\');
  }

  std::string full_filename;
  if (last_sep != std::string::npos) {
    full_filename = path_.substr(last_sep + 1);
  } else {
    full_filename = path_;
  }

  auto last_dot = full_filename.find_last_of('.');
  if (last_dot != std::string::npos) {
    filename_nosuffix_ = full_filename.substr(0, last_dot);

    last_dot++;
    if (last_dot < full_filename.length()) {
      suffix_ = full_filename.substr(last_dot);
    }
  } else {
    filename_nosuffix_ = full_filename;
  }

  // Set file size (important for negative offsets).
  glc_file_size_ = 0;
  if (glc_file_) {
    fseek(glc_file_, 0, SEEK_END);
    glc_file_size_ = ftell(glc_file_);
  } else {
    std::cerr << "GlcReader stream is not open." << std::endl;
  }
}

PortableGlcReader::~PortableGlcReader() {
  if (glc_file_) {
    fclose(glc_file_);
  }
}

/**
 * @return whether reader was able open file to read.
 */
bool PortableGlcReader::IsOpen() const {
  return glc_file_ != 0;
}

/**
 * Reads data into string buffer. Resizes the string first.
 */
bool PortableGlcReader::Read(
    std::string* buffer, uint64 offset, uint64 size) const {
  buffer->resize(size);
  return ReadData(&(*buffer)[0], offset, size);
}

/**
 * Reads the data from the globe file depending on what was last found.
 * Not thread safe.
 */
bool PortableGlcReader::ReadData(
    void* buffer, uint64 offset, uint64 size) const {
  if (!glc_file_) {
    return false;
  }

  try {
    fseek(glc_file_, offset, SEEK_SET);
    fread(reinterpret_cast<char*>(buffer), size, 1, glc_file_);
    return true;
  } catch(...) {
    return false;
  }
}

/**
 * Returns the size of the file or 0 if there is a problem with
 * the file.
 */
uint64 PortableGlcReader::Size() const {
  return glc_file_size_;
}

