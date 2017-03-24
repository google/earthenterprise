/*
 * Copyright 2017 Google Inc.
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


#ifndef COMMON_GEINDEX_INDEXMANIFEST_H__
#define COMMON_GEINDEX_INDEXMANIFEST_H__

#include <packetfile/packetfilereader.h>
#include <geindex/IndexBundleReader.h>
#include <common/ManifestEntry.h>

// IndexManifest
//
// Provide a complete listing of all files referenced through an index.
// Provide a method to obtain modified versions of header files with
// server path information inserted into headers.

namespace geindex {

class IndexManifest {
 public:

  // Get manifest of current files
  // If tmp_dir is not empty, and index.hdr or bundle.hdr are not relative,
  // (due to old fusion builds),
  // it creates files in tmp_dir making those paths relative.
  static void GetManifest(geFilePool &filePool,
                          const std::string &index_path,
                          std::vector<ManifestEntry> &manifest,
                          const std::string &tmpdir,
                          const std::string& disconnect_prefix,
                          const std::string& publish_prefix);
 private:
  IndexManifest();                      // can't actually create an object
  DISALLOW_COPY_AND_ASSIGN(IndexManifest);
};

} // namespace geindex

#endif  // COMMON_GEINDEX_INDEXMANIFEST_H__
