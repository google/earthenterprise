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


// Unit Tests for portable map builder.

#include <string>
#include <gtest/gtest.h>
#include "fusion/portableglobe/portablemapbuilder.h"

namespace fusion_portableglobe {

// Test class for portable globe builder.
class PortableMapBuilderTest : public testing::Test {
 protected:
  PortableMapBuilder map_builder;

  virtual void SetUp() {
  }
};

// Tests our ability to extract values from js associative arrays.
// Although this is a private function, it merits testing as we now
// have pseudo json (geeServerDefs) from multiple sources, such as
// GME.
TEST_F(PortableMapBuilderTest, TestJsExtract) {
  std::string js = " ... stuff to ignore ..."
      "   {      "
      "      key1:  \"value 1\","
      "      \"key 2\" :\"value2\"  ,"
      "      key3 :  03,"
      "      key4 :  4  ,\n"
      "      key5:-31,\n"
      "      \"key-6\": -3-1\n"
      "      bad1:  ,"
      "      bad2:  asdf ,"
      "   }      "
      " ... stuff to ignore ...";

  // Try legal keys first:
  EXPECT_EQ("value 1", map_builder.ExtractValue(js, "key1"));
  EXPECT_EQ("value2", map_builder.ExtractValue(js, "key 2"));
  EXPECT_EQ("03", map_builder.ExtractValue(js, "key3"));
  EXPECT_EQ("4", map_builder.ExtractValue(js, "key4"));
  EXPECT_EQ("-31", map_builder.ExtractValue(js, "key5"));
  EXPECT_EQ("-3", map_builder.ExtractValue(js, "key-6"));

  // Try non-existent key:
  EXPECT_EQ("", map_builder.ExtractValue(js, "bad3"));

  // Try bad keys next:
  EXPECT_EQ("", map_builder.ExtractValue(js, "bad1"));
  EXPECT_EQ("", map_builder.ExtractValue(js, "bad2"));
  EXPECT_EQ("", map_builder.ExtractValue(js, "value2"));

  // Try non-existent values:
  EXPECT_EQ("", map_builder.ExtractValue("bad3", "bad3"));
  EXPECT_EQ("", map_builder.ExtractValue("bad3:", "bad3"));
  EXPECT_EQ("", map_builder.ExtractValue("bad3:\"", "bad3"));

  // Try abrupt end:
  EXPECT_EQ("31", map_builder.ExtractValue("noclip: 31", "noclip"));
  EXPECT_EQ("hi", map_builder.ExtractValue("noclip: \"hi\"", "noclip"));
  EXPECT_EQ("hi", map_builder.ExtractValue("noclip: \"hi", "noclip"));
}

}  // namespace fusion_portableglobe

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
