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

/*
* The purpose of this file is to produce, as part of the build, a shared library
* that can be loaded by `dlopen` and that implements the function that the
* FileAccessorPluginLoader requires. 
* The resulting shared libary is used only by the unit tests.
*/
#include "FileAccessor.h"

class TestFileAccessor: public AbstractFileAccessor {
public:
public:
  virtual bool isValid() { return false; }
  virtual void invalidate() {}
  virtual void Open(const std::string &fname, const char *mode = nullptr, int flags = 00, mode_t createMask = 0666) {}
  virtual int FsyncAndClose() {return 0;}
  virtual int Close() {return 0;}
  virtual bool PreadAll(void* buffer, size_t size, off64_t offset) {return false;}
  virtual bool PwriteAll(const void* buffer, size_t size, off64_t offset) {return false;}
  virtual bool ReadStringFromFile(const std::string &filename, std::string &str, std::uint64_t limit = 0) {return false;}
  virtual bool Exists(const std::string &filename) {return false;}
  virtual bool GetLinesFromFile(std::vector<std::string> &lines, const std::string &filename) {return false;}
  virtual void fprintf(const char *format, ...) {}

  // getFD is used by the unit tests to verify which accessor they have
  virtual int getFD() {return 9999;}
};

class TestFileAccessorFactory: public FileAccessorFactory {
public:
  virtual AbstractFileAccessor* GetAccessor(const std::string &fileName) override {
   if (fileName == "9999file")
    return new TestFileAccessor();
   else return nullptr;
  }
};

static TestFileAccessorFactory factory;

extern "C" {
  FileAccessorFactory* get_factory_v1 () {return (FileAccessorFactory*)&factory;}
}
