/*
 * Copyright 2019 The Open GEE Contributors
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

#include "AssetSerializer.h"

#include <gtest/gtest.h>
#include <string>

using namespace std;

class TestItemStorage {};

class TestItem : public TestItemStorage {
 public:
  using Base = TestItemStorage;
  static int nextValue;
  static string loadFile;
  int val;
  TestItem() : val(nextValue++) {}
  static AssetPointerType<TestItem> Load(const string & filename) {
    loadFile = filename;
    return make_shared<TestItem>();
  }
  string GetName() const {
    return "TestItem";
  }
  void SerializeConfig(khxml::DOMElement*) const {
  }
};

int TestItem::nextValue;
string TestItem::loadFile;

class AssetSerializerTest : public testing::Test {
 protected:
  AssetSerializerLocalXML<TestItem> serializer;
 public:
  AssetSerializerTest() {
    // Reset the static variables in TestItem
    TestItem::nextValue = 1;
    TestItem::loadFile = "";
  }
};

void ToElement(khxml::DOMElement *, const TestItemStorage &) {}

TEST_F(AssetSerializerTest, Load) {
  auto item = serializer.Load("myfile");
  ASSERT_EQ(TestItem::loadFile, "myfile");
  ASSERT_EQ(item->val, 1);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
