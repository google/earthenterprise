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

class MockAssetConfig {};
class MockAssetType{};

class MockAssetImpl: public MockAssetStorage
{
public:
  static AssetDefs::Type EXPECTED_TYPE;
  static string EXPECTED_SUBTYPE;

  using Base = MockAssetStorage;
  MockAssetConfig config;

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
std::vector<SharedString> testInputs { "Input1", "Input2"};
khMetaData testMeta;
MockAssetConfig testConfig;

TEST_F(AssetFactoryTest, MakeNew) {
  MockMutableAsset handle = MakeNew<MockMutableAsset, MockAssetConfig>(
      testAssetRef, testInputs, testMeta, testConfig);
  ASSERT_EQ(handle.impl->name, testAssetRef);
  ASSERT_EQ(handle.impl->inputs, testInputs);
  ASSERT_EQ(handle.impl->meta, testMeta);
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
  ASSERT_THROW(pMakeNew(testAssetRef, testInputs, testMeta, testConfig), khException);
}

TEST_F(AssetFactoryTest, FindMake_Exists)
{
    MockMutableAsset handle = FindMake<MockMutableAsset, AssetDefs, MockAssetConfig>
            (testAssetRef, AssetDefs::Imagery, testInputs, testMeta, testConfig);
}

TEST_F(AssetFactoryTest, FindMake_New)
{}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
