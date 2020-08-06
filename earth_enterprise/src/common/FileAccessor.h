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

#ifndef COMMON_FILEACCESSOR_H
#define COMMON_FILEACCESSOR_H

#include <string>
#include <cstring>
#include <memory>
#include <khFileUtils.h>
#include "khStringUtils.h"

class AbstractFileAccessor {
public:
  virtual ~AbstractFileAccessor() = default;
  static std::unique_ptr<AbstractFileAccessor> getAccessor(const std::string &fname);
  virtual bool isValid() = 0;
  virtual void invalidate() = 0;
  virtual int getFD() = 0;
  virtual void Open(const std::string &fname, const char *mode = nullptr, int flags = 00, mode_t createMask = 0666) = 0;
  virtual int FsyncAndClose() = 0;
  virtual int Close() = 0;
  virtual bool PreadAll(void* buffer, size_t size, off64_t offset) = 0;
  virtual bool PwriteAll(const void* buffer, size_t size, off64_t offset) = 0;
  virtual bool ReadStringFromFile(const std::string &filename, std::string &str, std::uint64_t limit = 0) = 0;
  virtual bool Exists(const std::string &filename) = 0;
  virtual bool GetLinesFromFile(std::vector<std::string> &lines, const std::string &filename) = 0;
  virtual void fprintf(const char *format, ...) = 0;
};

class FileAccessorFactory {
  friend class FileAccessorPluginLoader;
protected:
  // This returns a raw pointer and should only ever be called by
  // FileAccessorPluginLoader who will then own it.
  virtual AbstractFileAccessor* GetAccessor(const std::string &fileName) = 0;
};

#endif //COMMON_FILEACCESSOR_H