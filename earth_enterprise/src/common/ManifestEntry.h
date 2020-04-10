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


// Manifest Entry extracted from filebundle.h

#ifndef COMMON_MANIFESTENTRY_H__
#define COMMON_MANIFESTENTRY_H__

#include <string>
#include <vector>
//#include "common/khTypes.h"
#include <cstdint>
#include "common/khFileUtils.h"

struct ManifestEntry;

struct ManifestEntry {
  std::string orig_path;
  std::string current_path;
  std::uint64_t data_size;
  std::vector<ManifestEntry> dependents;

  ManifestEntry() : data_size(0) {}

  explicit ManifestEntry(const std::string& orig_path_) :
      orig_path(orig_path_),
      current_path(orig_path_),
      data_size(khDiskUsage(orig_path_)) {
  }

  ManifestEntry(const std::string& orig_path_,
                const std::string& current_path_) :
      orig_path(orig_path_),
      current_path(current_path_),
      data_size(khDiskUsage(current_path_)) {
  }

  ManifestEntry(const std::string& orig_path_,
                const std::string& current_path_,
                std::uint64_t data_size_)
      : orig_path(orig_path_),
        current_path(current_path_),
        data_size(data_size_) {
  }

  bool operator==(const ManifestEntry &other) const {
    return orig_path == other.orig_path
    && current_path == other.current_path
        && data_size == other.data_size
        && dependents == other.dependents;
  }
};

#endif /* COMMON_MANIFESTENTRY_H__ */
