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

#include <gtest/gtest.h>
#include "FileAccessorPluginLoader.h"

using namespace std;
using namespace testing;

class DummyAccessor9876: public FileAccessorInterface {
public:
   virtual int GetSomething() override {
      return 9876;
   }
};

class DummyAccessor5432: public FileAccessorInterface {
public:
   virtual int GetSomething() override {
      return 5432;
   }
};

static DummyAccessor9876 accessor9876;
static DummyAccessor5432 accessor5432;

class DummyAccessorFactory9876: public FileAccessorFactoryInterface {
public:
   virtual FileAccessorInterface* GetAccessor(const std::string &fileName) override {
      if (fileName == "9876file")
         return &accessor9876;
      else return nullptr;
   }
};

class DummyAccessorFactory5432: public FileAccessorFactoryInterface {
public:
   virtual FileAccessorInterface* GetAccessor(const std::string &fileName) override {
      if (fileName == "5432file")
         return &accessor5432;
      else return nullptr;
   }
};

static DummyAccessorFactory9876 factory9876;
static DummyAccessorFactory5432 factory5432;

void LoadPluginsTestFunc (std::vector<FileAccessorPluginLoader::PluginFactory> &factories) {
   factories.push_back({(void*)1, &factory9876});
   factories.push_back({(void*)2, &factory5432});
}

void UnloadPluginsTestFunc (FileAccessorPluginLoader::PluginHandle handle) {
   // Do nothing. The default function calls dlclose on the handle.
}

#include <iostream>

TEST(FAPluginLoaderTest, get_accessor){
   FileAccessorPluginLoader pluginLoader(LoadPluginsTestFunc, UnloadPluginsTestFunc);
   FileAccessorInterface *pAccessor = pluginLoader.GetAccessor("9876file");

   EXPECT_TRUE(pAccessor != nullptr);
   ASSERT_EQ(pAccessor->GetSomething(), 9876);
}

TEST(FAPluginLoaderTest, get_multiple_accessors){
   FileAccessorPluginLoader pluginLoader(LoadPluginsTestFunc, UnloadPluginsTestFunc);
   FileAccessorInterface *pAccessor = pluginLoader.GetAccessor("9876file");

   EXPECT_TRUE(pAccessor != nullptr);
   ASSERT_EQ(pAccessor->GetSomething(), 9876);

   pAccessor = pluginLoader.GetAccessor("5432file");

   EXPECT_TRUE(pAccessor != nullptr);
   ASSERT_EQ(pAccessor->GetSomething(), 5432);
}

// TEST(FAPluginLoaderTest, test_boost_filesystem) {
//    std::vector<std::string> files;

//    boost::filesystem::path p("/tmp");
//    boost::filesystem::directory_iterator start(p);
//    boost::filesystem::directory_iterator end;
//    std::transform(start, end, std::back_inserter(files), 
//        [](const boost::filesystem::directory_entry& entry) {
//            return entry.path().leaf().string(); });

//    for (auto & file : files) {
//       cout << file << "\n";
//    }
// }

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
