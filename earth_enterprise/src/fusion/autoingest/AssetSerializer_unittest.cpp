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
#include "Asset.h"

#include <gtest/gtest.h>
#include <string>
using namespace std;
using namespace khxml;

// This test covers the error conditions in AssetSerializerLocalXML. It does
// not include test cases for the "happy path" because that involves creating
// a file, and we don't want to do that in unit tests. We'll cover the happy
// path plenty of times in integration and other tests.

bool AssetThrowPolicy::allow_throw(true);  // Force Load to throw exceptions

class TestItemStorage {};

class TestItem : public TestItemStorage {
  public:
    using Base = TestItemStorage;
    static int nextValue;
    static string loadFile;
    static string xmlFilename;
    static bool newInvalidCalled;
    int val;
    time_t timestamp; 
    std::uint64_t filesize;
    string name;
    TestItem() : val(nextValue++), name("TestItem") {}
    string GetName() const {
      return name;
    }
    void SerializeConfig(khxml::DOMElement*) const {}
    static std::string Filename(const std::string &boundref) {
      return xmlFilename;
    }
    static std::shared_ptr<TestItem> NewFromDOM(void *e) {
      return std::make_shared<TestItem>();
    }
    static std::shared_ptr<TestItem> NewInvalid(const std::string &ref) {
      newInvalidCalled = true;
      return std::make_shared<TestItem>();
    }
    static std::string GetPlaceholderAssetRegistryKey() { return "TestItemAsset"; }
};

int TestItem::nextValue;
string TestItem::loadFile;
string TestItem::xmlFilename;
bool TestItem::newInvalidCalled;

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
    TestItem::xmlFilename = "filename.xml";
    TestItem::newInvalidCalled = false;
    exceptionToThrow = NONE;
    getFileInfoReturnValue = true;
    getFileInfoSize = 1000;
    getFileInfoTime = time(0);
    geDocPtr = nullptr;
  }
};

std::string RunLoadForException(AssetSerializerLocalXML<TestItem> &serializer){
  string errorMsg = "";
  try {
    string boundref = "doesnt_matter";
    serializer.Load(boundref);
  }
  catch (khException e) {
    errorMsg = string(e.what());
  }

  return errorMsg;
}

TEST_F(AssetSerializerTest, Load_GetFileInfoFails) {
  getFileInfoReturnValue = false;
  string errorMsg = RunLoadForException(serializer);
  ASSERT_EQ(errorMsg, "No such file: filename.xml");
}

TEST_F(AssetSerializerTest, Load_FileSizeZero) {
  getFileInfoSize = 0;
  string errorMsg = RunLoadForException(serializer);
  ASSERT_EQ(errorMsg, "No such file: filename.xml");
}

TEST_F(AssetSerializerTest, Load_InvalidXMLDoc) {
  string errorMsg = RunLoadForException(serializer);
  ASSERT_EQ(errorMsg, "Unable to read filename.xml");
}

TEST_F(AssetSerializerTest, Load_NoDocElement) {
  geDocPtr = std::unique_ptr<GEDocument>(new GECreatedDocument(""));
  string errorMsg = RunLoadForException(serializer);
  ASSERT_EQ(errorMsg, "Error loading filename.xml: No document element");
}

TEST_F(AssetSerializerTest, Load_UnknownAssetType) {
  string docElement = "DummyAsset";
  geDocPtr = CreateEmptyDocument(docElement);
  string errorMsg = RunLoadForException(serializer);
  ASSERT_EQ(errorMsg, "Error loading filename.xml: Unknown asset type 'DummyAsset' while parsing filename.xml");
}

class TestItemRegistry: public AssetRegistry<TestItem> {
public:
  static void UnregisterPlugin(
    std::string const & assetTypeName)
  {
    auto it = PluginRegistry().find(assetTypeName);
    if (it != PluginRegistry().end()) {
      it->second.reset();
      PluginRegistry().erase(it);
    }
  }
};

class PluginRegisterHandler {
  /*
  * Simple class for registering and unregistering an asset plugin.
  */
  public:
    PluginRegisterHandler(
      std::function<std::shared_ptr<TestItem>(void*)> newFromDOM = TestItem::NewFromDOM,
      std::function<std::shared_ptr<TestItem>(const std::string &ref)> newInvalid = TestItem::NewInvalid) {
      auto assetPlugin =
        std::unique_ptr<TestItemRegistry::AssetPluginInterface>(
          new TestItemRegistry::AssetPluginInterface(
            newFromDOM,
            newInvalid
          )
        );
      TestItemRegistry::PluginRegistrar assetPluginRegistrar(
        "TestItemAsset", std::move(assetPlugin));
    }
    ~PluginRegisterHandler() {
      TestItemRegistry::UnregisterPlugin("TestItemAsset");
    }
};

TEST_F(AssetSerializerTest, Load_HappyPath) {
  PluginRegisterHandler prh;

  // Set the timestamp and file size
  struct tm nov_11; 
  nov_11.tm_mday = 11;
  nov_11.tm_mon = 11; 
  nov_11.tm_year = 2011;
  getFileInfoTime = mktime(&nov_11);
  getFileInfoSize = 100000;
  
  string docElement = "TestItemAsset";
  geDocPtr = CreateEmptyDocument(docElement);
  string boundref = "doesnt_matter";
  auto testItem = serializer.Load(boundref);
  ASSERT_NE(testItem, nullptr);
  ASSERT_EQ(testItem->timestamp, getFileInfoTime);
  ASSERT_EQ(testItem->filesize, getFileInfoSize);
}

TEST_F(AssetSerializerTest, Load_NewInvalid) {
  /* This tests the case that an asset failed to load when the "throw policy"
  * is not to throw. In this case, it should create a new invalid item. In
  * our case, the type of the placeholder item will be the same as the
  * valid item to make testing easier. The check for TestItem::newInvalidCalled
  * verifies that the asset was not created by NewFromDOM.
  */

  PluginRegisterHandler prh;
  bool originalThrowPolicy = AssetThrowPolicy::allow_throw;
  AssetThrowPolicy::allow_throw = false;

  // Set the timestamp and file size
  struct tm nov_11; 
  nov_11.tm_mday = 11;
  nov_11.tm_mon = 11; 
  nov_11.tm_year = 2011;
  getFileInfoTime = mktime(&nov_11);
  getFileInfoSize = 100000;

  // Cause a Load failure
  getFileInfoReturnValue = false;  

  string boundref = "doesnt_matter";
  auto testItem = serializer.Load(boundref);

  AssetThrowPolicy::allow_throw = originalThrowPolicy;

  ASSERT_NE(testItem, nullptr);
  ASSERT_TRUE(TestItem::newInvalidCalled);
  ASSERT_EQ(testItem->timestamp, getFileInfoTime);
  ASSERT_EQ(testItem->filesize, getFileInfoSize);
}

TEST_F(AssetSerializerTest, Load_NewInvalidNoInvalidType) {
  // Passing nullptr as the second parameter will result in
  // AssetFactory::CreateNewInvalid returning nullptr.
  PluginRegisterHandler prh(TestItem::NewFromDOM, nullptr);
  bool originalThrowPolicy = AssetThrowPolicy::allow_throw;
  AssetThrowPolicy::allow_throw = false;

  // Cause a Load failure so serializer attemps to create an invalid placeholder
  getFileInfoReturnValue = false;  

  string boundref = "doesnt_matter";
  auto testItem = serializer.Load(boundref);

  AssetThrowPolicy::allow_throw = originalThrowPolicy;

  ASSERT_EQ(testItem, nullptr);
  ASSERT_FALSE(TestItem::newInvalidCalled);
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
