// Copyright 2020 The Open GEE Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>
#include "FileAccessorPluginLoader.h"

using namespace std;
using namespace testing;

/* 
  Testing components:
    - TestAccessorBase and children: 
        TestAccessorBase derives from AbstractFileAccessor and provides default 
        overrides for functions that aren't needed for testing. The classes
        that derive from TestAccessorBase override the needed functions.
    - FileAccessorFactory implementations:
        There is one of these for each test accessor derived from TestAccessorBase.
    - Load and unload functions:
        Custom functions, passed into the plugin loader, that handle
        populating/depopulating the plugin loader's vector of file accessor
        factories.
    - Test plugin loader:
        FileAccessorPluginLoader is normally a singleton created by a call to
        FileAccessorPluginLoader::Get(). As such, the constructor is protected.
        In order to pass custom values into the constructor, the test plugin
        loader inherits from FileAccessorPluginLoader.
*/


//
// TestAccessorBase and children
//
class TestAccessorBase: public AbstractFileAccessor {
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
};

class TestAccessor9876: public TestAccessorBase {
public:
  virtual int getFD() {return 9876;}
};

class TestAccessor5432: public TestAccessorBase {
public:
  virtual int getFD() {return 5432;}
};


//
// FileAccessorFactory implementations and instances
//
class TestAccessorFactory9876: public FileAccessorFactory {
protected:
  virtual AbstractFileAccessor* GetAccessor(const std::string &fileName) override {
   if (fileName == "9876file")
    return new TestAccessor9876;
   else return nullptr;
  }
};

class TestAccessorFactory5432: public FileAccessorFactory {
protected:
  virtual AbstractFileAccessor* GetAccessor(const std::string &fileName) override {
   if (fileName == "5432file")
    return new TestAccessor5432;
   else return nullptr;
  }
};

static TestAccessorFactory9876 factory9876;
static TestAccessorFactory5432 factory5432;


//
// Load and unload functions
//
void LoadPluginsTestFunc (const std::string &pluginDirectory, std::vector<FileAccessorPluginLoader::PluginFactory> &factories) {
  factories.push_back({(void*)1, &factory9876});
  factories.push_back({(void*)2, &factory5432});
}

void Load_NO_PluginsTestFunc (const std::string &pluginDirectory, std::vector<FileAccessorPluginLoader::PluginFactory> &factories) {
}

void UnloadPluginsTestFunc (FileAccessorPluginLoader::PluginHandle handle) {
  // Do nothing. The default function calls dlclose on the handle.
}


//
// Test plugin loader
//
class TestPluginLoader: public FileAccessorPluginLoader {
public:
  TestPluginLoader(FileAccessorPluginLoader::LoadPluginFunc LoadImpl = nullptr,
                   FileAccessorPluginLoader::UnloadPluginFunc UnloadImpl = nullptr,
                   std::string PluginFolderLocation = "/opt/google/plugin/fileaccessor/")
    : FileAccessorPluginLoader(LoadImpl, UnloadImpl, PluginFolderLocation) {}
};

//
// Tests
//
TEST(FAPluginLoaderTest, get_multiple_accessors){
  TestPluginLoader pluginLoader(LoadPluginsTestFunc, UnloadPluginsTestFunc);
  std::unique_ptr<AbstractFileAccessor> pAccessor = pluginLoader.GetAccessor("9876file");

  EXPECT_TRUE(pAccessor != nullptr);
  ASSERT_EQ(pAccessor->getFD(), 9876);

  pAccessor = pluginLoader.GetAccessor("5432file");

  EXPECT_TRUE(pAccessor != nullptr);
  ASSERT_EQ(pAccessor->getFD(), 5432);
}

TEST(FAPluginLoaderTest, get_default_accessor){
  TestPluginLoader pluginLoader(Load_NO_PluginsTestFunc, nullptr);
  std::unique_ptr<AbstractFileAccessor> pAccessor = pluginLoader.GetAccessor("9876file");


  EXPECT_TRUE(pAccessor != nullptr);
  ASSERT_EQ(pAccessor->getFD(), -1);
}

TEST(FAPluginLoaderTest, nonexistent_plugin_path){
  // Passing in nullptr for the load/unload functions ensures that it will attempt
  // to scan the directory for plugins.
  TestPluginLoader pluginLoader(nullptr, nullptr, "/zzzz/thisisnot/areal/path/zzzz");
  std::unique_ptr<AbstractFileAccessor> pAccessor = pluginLoader.GetAccessor("notarealfilename.txt");

  EXPECT_TRUE(pAccessor != nullptr);
  ASSERT_EQ(pAccessor->getFD(), -1);
}

TEST(FAPluginLoaderTest, path_with_invalid_plugin){
  TestPluginLoader pluginLoader(nullptr, nullptr, "./plugins/file_accessor/invalid/");
  std::unique_ptr<AbstractFileAccessor> pAccessor = pluginLoader.GetAccessor("notarealfilename.txt");

  EXPECT_TRUE(pAccessor != nullptr);
  ASSERT_EQ(pAccessor->getFD(), -1);
}

TEST(FAPluginLoaderTest, valid_test_plugin){
  TestPluginLoader pluginLoader(nullptr, nullptr, "./plugins/file_accessor/valid/");
  std::unique_ptr<AbstractFileAccessor> pAccessor = pluginLoader.GetAccessor("9999file");

  EXPECT_TRUE(pAccessor != nullptr);
  ASSERT_EQ(pAccessor->getFD(), 9999);
}

TEST(FAPluginLoaderTest, load_executable_not_shared_library){
  TestPluginLoader pluginLoader(nullptr, nullptr, "./plugins/file_accessor/executable/");
  std::unique_ptr<AbstractFileAccessor> pAccessor = pluginLoader.GetAccessor("notarealfilename.txt");

  EXPECT_TRUE(pAccessor != nullptr);
  ASSERT_EQ(pAccessor->getFD(), -1);
}

int main(int argc, char** argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
