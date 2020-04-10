/*
 * Copyright 2020 The Open GEE Contributors
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
using namespace khxml;

// This test covers the error conditions in AssetSerializerLocalXML. It does
// not include test cases for the "happy path" because that involves creating
// a file, and we don't want to do that in unit tests. We'll cover the happy
// path plenty of times in integration and other tests.

class TestItemStorage {};

class TestItem : public TestItemStorage {
  public:
    using Base = TestItemStorage;
    static int nextValue;
    static string loadFile;
    int val;
    time_t timestamp; 
    std::uint64_t filesize;
    string name;
    TestItem() : val(nextValue++), name("TestItem") {}
    static AssetPointerType<TestItem> Load(const string & filename) {
      loadFile = filename;
      return make_shared<TestItem>();
    }
    string GetName() const {
      return name;
    }
    void SerializeConfig(khxml::DOMElement*) const {}
};

int TestItem::nextValue;
string TestItem::loadFile;

class TestXMLException : public XMLException {
  public:
    virtual const XMLCh * getType () const {
      return ToXMLStr("");
    }
};

// Define a TestItem-specific version of GetFileInfo for testing
static std::uint64_t getFileInfoSize;
static time_t getFileInfoTime;
static bool getFileInfoReturnValue;
template<>
bool GetFileInfo<TestItem>(const std::string &fname, std::uint64_t &size, time_t &mtime) {
  size = getFileInfoSize;
  mtime = getFileInfoTime;
  return getFileInfoReturnValue;
}

// Define a TestItem-specific version of ReadXMLDocument for testing
static std::unique_ptr<GEDocument> geDocPtr = nullptr;
template<>
std::unique_ptr<GEDocument> ReadXMLDocument<TestItem>(const std::string &filename) {
  return std::move(geDocPtr);
}

// We throw various exceptions from this function to test exception handling.
enum ExceptionType {XML, DOM, STD, OTHER, NONE};
static ExceptionType exceptionToThrow;
void ToElement(khxml::DOMElement *, const TestItemStorage &) {
  switch (exceptionToThrow) {
    case XML:
      throw TestXMLException();
    case DOM:
      throw DOMException();
    case STD:
      throw exception();
    case OTHER:
      throw "This is a string, not an exception object";
    case NONE:
      break;
  }
}

class AssetSerializerTest : public testing::Test {
 protected:
  AssetSerializerLocalXML<TestItem> serializer;
 public:
  AssetSerializerTest() {
    // Reset the static variables in TestItem
    TestItem::nextValue = 1;
    TestItem::loadFile = "";
    exceptionToThrow = NONE;
  }
};

TEST_F(AssetSerializerTest, Load) {
  auto item = serializer.Load("myfile");
  ASSERT_EQ(TestItem::loadFile, "myfile");
  ASSERT_EQ(item->val, 1);
}

void ExceptionTest(AssetSerializerLocalXML<TestItem> & serializer, ExceptionType type) {
  exceptionToThrow = type;
  AssetPointerType<TestItem> item = make_shared<TestItem>();
  bool status = serializer.Save(item, "myitem");
  ASSERT_FALSE(status);
}

TEST_F(AssetSerializerTest, XMLException) {
  ExceptionTest(serializer, XML);
}

TEST_F(AssetSerializerTest, DOMException) {
  ExceptionTest(serializer, DOM);
}

TEST_F(AssetSerializerTest, STDException) {
  ExceptionTest(serializer, STD);
}

TEST_F(AssetSerializerTest, OtherException) {
  ExceptionTest(serializer, OTHER);
}

TEST_F(AssetSerializerTest, BadName) {
  AssetPointerType<TestItem> item = make_shared<TestItem>();
  item->name = "<";
  bool status = serializer.Save(item, "myitem");
  ASSERT_FALSE(status);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
