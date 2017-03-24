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


#include "filebundlereader.h"
#include <third_party/rsa_md5/crc32.h>
#include "cachedreadaccessor.h"
#include <khFileUtils.h>
#include <khSimpleException.h>
#include <khEndian.h>

// Constructor - opens and loads header file with bundle properties
// and list of segment files.  Path parameter is path to directory.

FileBundleReader::FileBundleReader(
    geFilePool &file_pool, const std::string &path)
    : FileBundle(file_pool, path, 0) {
  LoadHeaderFile();
}


FileBundleReader::FileBundleReader(
    geFilePool &file_pool, const std::string &path, const std::string &prefix)
    : FileBundle(file_pool, path, 0, prefix) {
  LoadHeaderFile();
}

FileBundleReader::~FileBundleReader() {
}

// Close
void FileBundleReader::Close() {
  khLockGuard lock(modify_lock_);
  CleanupCachedReadAccessor();

  while (!SegmentsEmpty()) {
    delete PopBackSegment();
  }
}


bool FileBundleReader::IsFileBundle(
    geFilePool* file_pool, const std::string &path, const std::string &prefix) {
  try {
    FileBundleReader(*file_pool, path, prefix).LoadHeaderFile();
  } catch(const std::exception& e) {
    notify(NFY_WARN, "FileBundleReader: \"%s\"\n.", e.what());
    return false;
  }
  return true;
}
