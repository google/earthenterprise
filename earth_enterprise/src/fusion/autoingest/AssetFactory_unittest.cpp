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

#include <autoingest/.idl/storage/AssetDefs.h>
#include <common/khException.h>
#include "AssetFactory.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

using namespace std;
using namespace AssetFactory;

class MockAssetStorage 
{
public:
  SharedString name;
  AssetDefs::Type type;
  string subtype;
  vector<SharedString> inputs;
  khMetaData meta;

  static MockAssetStorage MakeStorage(const SharedString &name_,
              AssetDefs::Type type_,
              const std::string &subtype_,
              const std::vector<SharedString> &inputs_,
              const khMetaData &meta_)
              {
                MockAssetStorage storage;
                storage.name = name_;
                storage.type = type_;
                storage.subtype = subtype_;
                storage.inputs = inputs_;
                storage.meta = meta_;
                return storage;
              }
};

// used for comparison of config
static uint8 configID = 0;
class MockAssetConfig
{
private:
    uint8 ID;
public:
    MockAssetConfig() { ID = configID++; }
    bool operator==(const MockAssetConfig& other) const
    {
        return ID == other.ID;
    }
};

class MockAssetImpl: public MockAssetStorage
{
public:
  static AssetDefs::Type EXPECTED_TYPE;
  static string EXPECTED_SUBTYPE;

  using Base = MockAssetStorage;
  MockAssetConfig config;

  void Modify(const khMetaData& _meta, const MockAssetConfig& _config)
  {
      meta = _meta;
      config = _config;
  }

  void Modify(const vector<SharedString>& _inputs,
              const khMetaData& _meta,
              const MockAssetConfig& _config)
  {
      inputs = _inputs;
      Modify(_meta, _config);
  }

  MockAssetImpl(MockAssetStorage storage, 
                const MockAssetConfig &config_)
                : MockAssetStorage(storage), config(config_) {
  }
};

AssetDefs::Type MockAssetImpl::EXPECTED_TYPE;
string MockAssetImpl::EXPECTED_SUBTYPE;

class MockMutableAsset
{
 public:
    using Impl = MockAssetImpl;

    static string testSubTypeToUseForStringConstructor;

    shared_ptr<MockAssetImpl> impl = nullptr;

    MockMutableAsset(){}
    MockMutableAsset(shared_ptr<MockAssetImpl> impl_)
    {
      impl = impl_;
    }

    MockMutableAsset(const string &ref_)
    {
      // Need to be ready to construct a mock from a string reference.
      // Leaving the subtype changeable by the tests via testSubTypeToUseForStringConstructor;
      MockAssetStorage storage;
      storage.name = ref_;
      storage.type = MockAssetImpl::EXPECTED_TYPE;
      storage.subtype = testSubTypeToUseForStringConstructor;
      
      MockAssetConfig config;
      impl = std::make_shared<MockAssetImpl>(storage, config);
    }

    // For the `if (asset)` check in MakeNew
    explicit operator bool(void) const
    {
      return impl != nullptr;
    }

    Impl* operator->(void) const {
      return impl.get();
    }
};

string MockMutableAsset::testSubTypeToUseForStringConstructor;

class AssetFactoryTest : public testing::Test {
 public:
  AssetFactoryTest() 
  {
    MockAssetImpl::EXPECTED_TYPE = AssetDefs::Imagery;
    MockAssetImpl::EXPECTED_SUBTYPE = "mockSubtype";
    MockMutableAsset::testSubTypeToUseForStringConstructor = "someOtherSubtype";
  }
};

// TEST DATA
string testAssetRef = "/gevol/assets/AssetRef",
       testAssetRef1 = "/gevol/assets/AssetRef1",
       testAssetRef2 = "/gevol/assets/AssetRef2";
std::vector<SharedString> testInputs { "Input1", "Input2"},
                          testInputs1 { "Inputs3", "Inputs4", "Inputs5" };
khMetaData testMeta;
MockAssetConfig testConfig0, testConfig1, testConfig2;

TEST_F(AssetFactoryTest, MakeNew) {
  MockMutableAsset handle = MakeNew<MockMutableAsset, MockAssetConfig>(
      testAssetRef, testInputs, testMeta, testConfig0);
  ASSERT_EQ(handle.impl->name, testAssetRef);
  ASSERT_EQ(handle.impl->inputs, testInputs);
  ASSERT_EQ(handle.impl->meta, testMeta);
  ASSERT_EQ(handle.impl->config, testConfig0);
  ASSERT_EQ(handle.impl->type, AssetDefs::Imagery);
}

// ASSERT_THROW seems to trip up on function templates with more than one
// template parameter. Making a function pointer helps it get past that.
MockMutableAsset (*pMakeNew)( const std::string &ref_, 
                                    const std::vector<SharedString>& inputs_,
                                    const khMetaData &meta, 
                                    const MockAssetConfig &config) =
  MakeNew<MockMutableAsset, MockAssetConfig>;
TEST_F(AssetFactoryTest, MakeNewAlreadyExists) {
  // Make sure the std::string constructor for MockMutableAsset will have the same
  // subtype as the one we're trying to create in order to induce an exception.

  MockMutableAsset::testSubTypeToUseForStringConstructor = MockAssetImpl::EXPECTED_SUBTYPE;
  ASSERT_THROW(pMakeNew(testAssetRef, testInputs, testMeta, testConfig0), khException);
}

TEST_F(AssetFactoryTest, FindMake_New)
{
    MockMutableAsset handle = FindMake<MockMutableAsset, MockAssetConfig>
            (testAssetRef1, AssetDefs::Imagery, testInputs1, testMeta, testConfig1);
    ASSERT_EQ(handle.impl->name, testAssetRef1);
    ASSERT_EQ(handle.impl->inputs, testInputs1);
    ASSERT_EQ(handle.impl->meta, testMeta);
    ASSERT_EQ(handle.impl->config, testConfig1);
    ASSERT_EQ(handle.impl->type, AssetDefs::Imagery);
}

TEST_F(AssetFactoryTest, FindMake_Exists)
{
    MockMutableAsset handle = FindMake<MockMutableAsset, MockAssetConfig>
            (testAssetRef, testInputs, testMeta, testConfig2);
    ASSERT_EQ(handle.impl->name, testAssetRef);
    ASSERT_EQ(handle.impl->inputs, testInputs);
    ASSERT_EQ(handle.impl->meta, testMeta);
    ASSERT_EQ(handle.impl->config, testConfig2);
    ASSERT_EQ(handle.impl->type, AssetDefs::Imagery);
    MockMutableAsset handle1 = FindMake<MockMutableAsset, MockAssetConfig>
            (testAssetRef1, AssetDefs::Imagery, testMeta, testConfig2);
    ASSERT_EQ(handle1.impl->name, testAssetRef1);
    ASSERT_EQ(handle1.impl->meta, testMeta);
    ASSERT_EQ(handle.impl->config, testConfig2);
    ASSERT_EQ(handle1.impl->type, AssetDefs::Imagery);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
