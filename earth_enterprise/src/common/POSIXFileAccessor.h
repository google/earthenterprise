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

#ifndef COMMON_POSIXFILEACCESSOR_H
#define COMMON_POSIXFILEACCESSOR_H

#include "FileAccessor.h"

class POSIXFileAccessor: public AbstractFileAccessor {
  int fileDescriptor;
public:
  POSIXFileAccessor(int i = -1) : fileDescriptor{ i }{}
  ~POSIXFileAccessor(void) {
    if (this->isValid()) {
      this->Close();
    }
  }

  bool isValid() override { return fileDescriptor != -1; }
  void invalidate() override { fileDescriptor = -1; }
  int getFD() override { return fileDescriptor; }
  void Open(const std::string &fname, const char *mode = nullptr, int flags = 00, mode_t createMask = 0666) override;
  int FsyncAndClose() override;
  int Close() override;
  bool PreadAll(void* buffer, size_t size, off64_t offset) override;
  bool PwriteAll(const void* buffer, size_t size, off64_t offset) override;
  bool ReadStringFromFile(const std::string &filename, std::string &str, std::uint64_t limit = 0) override;
  bool Exists(const std::string &filename) override;
  void fprintf(const char *format, ...) override;
};

#endif //COMMON_POSIXFILEACCESSOR_H
