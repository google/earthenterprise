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

#include "POSIXFileAccessor.h"


// ****************************************************************************
// ***  POSIXFileAccessor
// ****************************************************************************
void POSIXFileAccessor::Open(const std::string &fname, const char *mode, int flags, mode_t createMask) {
  int result;
  if (mode) {
    if (strcmp(mode, "w") == 0) {
      result = khOpenForWrite(fname, createMask);
    }
    else {
      result = khOpenForRead(fname);
    }
  }
  else {
    result = khOpen(fname, flags, createMask);
  }
  fileDescriptor = result;
}

int POSIXFileAccessor::FsyncAndClose() {
  int result = khFsyncAndClose(fileDescriptor);
  this->invalidate();
  return result;
}

int POSIXFileAccessor::Close() {
  int result = khClose(fileDescriptor);
  this->invalidate();
  return result;
}

bool POSIXFileAccessor::PreadAll(void* buffer, size_t size, off64_t offset) {
  return khPreadAll(fileDescriptor, buffer, size, offset);
}

bool POSIXFileAccessor::PwriteAll(const void* buffer, size_t size, off64_t offset) {
  return khPwriteAll(fileDescriptor, buffer, size, offset);
}

bool POSIXFileAccessor::ReadStringFromFile(const std::string &filename, std::string &str, std::uint64_t limit) {
  return khReadStringFromFile(filename, str, limit);
}

bool POSIXFileAccessor::Exists(const std::string &filename) {
  return khExists(filename);
}

bool POSIXFileAccessor::GetLinesFromFile(std::vector<std::string> &lines, const std::string &filename) {
  std::string content;

  try {
    this->ReadStringFromFile(filename, content);

    TokenizeString(content, lines, "\n");

    for (auto &it : lines) {
      CleanString(&it, "\r\n");
    }

    return true;
  }

  catch (...) {
    return false;
  }
}

void POSIXFileAccessor::fprintf(const char *format, ...) {
  va_list arguments;
  va_start(arguments, format);
  ::vdprintf(fileDescriptor, format, arguments);
  va_end(arguments);
}
