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


#ifndef COMMON_FILEBUNDLE_FILEBUNDLEREADER_H__
#define COMMON_FILEBUNDLE_FILEBUNDLEREADER_H__

#include "filebundle.h"
#include <common/khGuard.h>

// FileBundleReader
//
// Read methods for file bundles. See filebundle.h for an overview of
// FileBundle capabilities and common data and methods.

class FileBundleReader : public FileBundle {
 public:
  FileBundleReader(geFilePool &file_pool, const std::string &path);
  FileBundleReader(geFilePool &file_pool, const std::string &path,
                   const std::string &prefix);
  virtual ~FileBundleReader();
  // Read from specified position in bundle.  Throws exception if
  // error.  The CRC versions expect the last 4 (kCRCsize) bytes to
  // contain the CRC-32 for the rest of the data.  An exception is
  // thrown if the CRC is incorrect.
  void Close();

  static bool IsFileBundle(geFilePool* file_pool, const std::string &path,
                           const std::string &prefix);


 private:

  khMutex modify_lock_;                 // lock when modifying

  DISALLOW_COPY_AND_ASSIGN(FileBundleReader);
};

#endif  // COMMON_FILEBUNDLE_FILEBUNDLEREADER_H__
