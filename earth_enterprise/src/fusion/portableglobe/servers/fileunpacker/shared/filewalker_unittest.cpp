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

// Unit Tests for walking globe files.

#include <algorithm>
#include <fstream>
#include <limits.h>
#include <map>
#include <string>
#include <utility>
#include <gtest/gtest.h>
#include "common/khFileUtils.h"

#include "fusion/portableglobe/servers/fileunpacker/shared/glc_unpacker.h"
#include "fusion/portableglobe/servers/fileunpacker/shared/portable_glc_reader.h"

// Test classes for file walker package.
class FileWalkerTest : public testing::Test {
 protected:
  std::string glb_file;
  std::string glc_file;

  virtual void SetUp() {
    glb_file = "portable/test_data/test.glb";
    glc_file = "portable/test_data/test.glc";
  }

  // Helper function to find values in lists
  template<typename T>
  bool Contains(std::list<T> & element_list, const T & element)
  {
	  auto it = std::find(element_list.begin(), element_list.end(), element);
	  return it != element_list.end();
  }

  // Helper function to check if a file exists
  bool FileExists(const char *filename)
  {
    std::ifstream ifile(filename);
    return (bool)ifile;
  }

  /**
    * Test that the contents of the test GLB have expected size and values
    */
  void TestFilesGlobe() {
    EXPECT_TRUE(FileExists(glb_file.c_str()));

    if (!FileExists(glb_file.c_str())) {
      return;
    }

    PortableGlcReader reader(glb_file.c_str());
    GlcUnpacker unpacker(reader, false, false);
    std::list<std::string> files;
    EXPECT_FALSE(unpacker.MapFileWalker(0, [&] (int layer, const char* file_name) {
      files.push_back(file_name);
      return true;
    }));

    EXPECT_FALSE(files.empty());

    EXPECT_EQ(files.size(), 59);

    EXPECT_FALSE(Contains(files, std::string("data")));

    EXPECT_TRUE(Contains(files, std::string("data/index")));

    EXPECT_TRUE(Contains(files, std::string("icons/773_l.png")));
  }

  /**
    * Test that the contents of the test GLC layers have expected size and values
    */
  void TestFilesComposite() {
    EXPECT_TRUE(FileExists(glc_file.c_str()));

    if (!FileExists(glc_file.c_str())) {
      return;
    }

    PortableGlcReader reader(glc_file.c_str());
    GlcUnpacker unpacker(reader, true, false);
    std::map<int, std::list<std::string>> file_map;
    EXPECT_FALSE(unpacker.MapFileWalker([&] (int layer, const char* file_name) {
      file_map[layer].push_back(file_name);
      return true;
    }));

    std::list<int> layer_ids;
    for(auto const& pair: file_map) {
      layer_ids.push_back(pair.first);
    }

    EXPECT_TRUE(Contains(layer_ids, 1));

    EXPECT_TRUE(Contains(layer_ids, 2));

    EXPECT_EQ(layer_ids.size(), 2);
    
    EXPECT_FALSE(Contains(file_map[1], std::string("data")));

    EXPECT_FALSE(Contains(file_map[2], std::string("data")));

    EXPECT_TRUE(Contains(file_map[1], std::string("icons/773_l.png")));

    EXPECT_TRUE(Contains(file_map[2], std::string("icons/773_l.png")));

    EXPECT_EQ(file_map[1].size(), 59);

    EXPECT_EQ(file_map[2].size(), 52);
  }

  /**
    * Test that exiting early returns the expected size
    */
  void TestFilesCompositeEarlyExit() {
    PortableGlcReader reader(glc_file.c_str());
    GlcUnpacker unpacker(reader, true, false);
    std::list<std::string> files;
    EXPECT_TRUE(unpacker.MapFileWalker([&] (int layer, const char* file_name) {
      files.push_back(file_name);
      return false;
    }));

    EXPECT_EQ(files.size(), 1);
  }
};

// Tests GLB and GLC directory contents
TEST_F(FileWalkerTest, TestFiles) {
  TestFilesGlobe();
  TestFilesComposite();
  TestFilesCompositeEarlyExit();
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
