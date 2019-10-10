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

using AssetVersion = uint8_t;

// forward declare classes
class MockAssetImpl;
class MockAssetConfig;
class MockMutableAsset;
class MockAssetVersionImpl;
class MockMutableAssetVersion;
//

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


// added ID field to aid in comparison
class MockAssetConfig
{
public:
    int8_t ID;
    MockAssetConfig(int8 _ID = -1) : ID(_ID) {}
    bool operator==(const MockAssetConfig& other) const { return ID == other.ID; }
    bool operator==(uint8_t _ID) { return ID == _ID; }
    void operator=(const MockAssetConfig& other) { ID = other.ID; }
    void operator=(uint8_t _ID) { ID = _ID; }
 };

class MockAssetVersionImpl : public MockAssetStorage
{
public:
    using MutableAssetType = MockMutableAsset;
    static AssetDefs::Type EXPECTED_TYPE;
    static string EXPECTED_SUBTYPE;

    using Base = MockAssetStorage;

    MockAssetConfig config;
    bool needed;

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

    MockAssetVersionImpl(MockAssetStorage storage,
                  const MockAssetConfig &config_)
                : MockAssetStorage(storage), config(config_), needed(false) {}
};


class MockMutableAssetVersion : public MockAssetStorage
{
public:
    using Impl = MockAssetVersionImpl;
    static string testSubTypeToUseForStringConstructor;
    shared_ptr<Impl> impl = nullptr;

    MockMutableAssetVersion() = default;
    MockMutableAssetVersion(const string& ref_)
    {
        MockAssetStorage storage;
        storage.name = ref_;
        storage.type = Impl::EXPECTED_TYPE;
        storage.subtype = testSubTypeToUseForStringConstructor;

        MockAssetConfig config;
        impl = make_shared<Impl>(storage, config);
    }

    MockMutableAssetVersion(const MockAssetStorage& storage, const MockAssetConfig& config)
    {
        impl = make_shared<Impl>(storage, config);
    }

    MockMutableAssetVersion(const string& name, AssetDefs::Type type)
    {
        MockAssetStorage storage;
        storage.name = name;
        storage.type = type;
        MockAssetConfig config;
        impl = make_shared<Impl>(storage,config);
    }

    explicit operator bool(void) const
    {
        return impl != nullptr;
    }

    Impl* operator->() const
    {
        return impl.get();
    }

};

class MockAssetImpl: public MockAssetStorage
{
public:
  static AssetDefs::Type EXPECTED_TYPE;
  static string EXPECTED_SUBTYPE;

  using Base = MockAssetStorage;
  using Impl = MockAssetImpl;
  MockAssetConfig config;
  bool needed;

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
              : MockAssetStorage(storage), config(config_), needed(false) {}

  MockAssetImpl* operator->()
  {
      return this;
  }

  MockMutableAssetVersion MyUpdate(bool& _needed, const vector<AssetVersion>& v = {})
  {
      auto retval = MockMutableAssetVersion(this->name, this->type);
      retval->needed = _needed = true;
      return retval;
  }

  MockMutableAssetVersion Update(bool& _needed, const vector<AssetVersion>& v = {})
  {
      return MyUpdate(needed, v);
  }
};

AssetDefs::Type MockAssetImpl::EXPECTED_TYPE;
string MockAssetImpl::EXPECTED_SUBTYPE;

AssetDefs::Type MockAssetVersionImpl::EXPECTED_TYPE;
string MockAssetVersionImpl::EXPECTED_SUBTYPE;

string MockMutableAssetVersion::testSubTypeToUseForStringConstructor;

class MockMutableAsset // pointer type
{
 public:
    using Impl = MockAssetImpl;

    static string testSubTypeToUseForStringConstructor;
    shared_ptr<Impl> impl = nullptr;

    MockMutableAsset() = default;
    MockMutableAsset(shared_ptr<Impl> impl_) : impl(impl_) {}

    MockMutableAsset(const string &ref_)
    {
      // Need to be ready to construct a mock from a string reference.
      // Leaving the subtype changeable by the tests via testSubTypeToUseForStringConstructor;
      MockAssetStorage storage;
      storage.name = ref_;
      storage.type = Impl::EXPECTED_TYPE;
      storage.subtype = testSubTypeToUseForStringConstructor;

      MockAssetConfig config;
      impl = std::make_shared<Impl>(storage, config);
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
    MockAssetImpl::EXPECTED_TYPE = MockAssetVersionImpl::EXPECTED_TYPE = AssetDefs::Imagery;
    MockAssetImpl::EXPECTED_SUBTYPE = MockAssetVersionImpl::EXPECTED_SUBTYPE = "mockSubtype";

    MockMutableAsset::testSubTypeToUseForStringConstructor =
    MockMutableAssetVersion::testSubTypeToUseForStringConstructor = "someOtherSubtype";
  }
};

// TEST DATA
string testAssetRef = "/gevol/assets/AssetRef",
       testAssetRef1 = "/gevol/assets/AssetRef1",
       testAssetRef2 = "/gevol/assets/AssetRef2";
std::vector<SharedString> testInputs { "Input1", "Input2"},
                          testInputs1 { "Inputs3", "Inputs4", "Inputs5" };
khMetaData testMeta;
MockAssetConfig testConfig0(0);

TEST_F(AssetFactoryTest, MakeNew) {
  MockMutableAsset handle = MakeNew<MockMutableAsset, MockAssetConfig>(
      testAssetRef, testInputs, testMeta, testConfig0);
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
  ASSERT_THROW(pMakeNew(testAssetRef, testInputs, testMeta, testConfig0), khException);
}

TEST_F(AssetFactoryTest, FindMake_New)
{
    MockMutableAsset handle = FindMake<MockMutableAsset, MockAssetConfig>
            (testAssetRef1, AssetDefs::Imagery, testInputs1, testMeta, MockAssetConfig(1));
    ASSERT_EQ(handle.impl->name, testAssetRef1);
    ASSERT_EQ(handle.impl->inputs, testInputs1);
    ASSERT_EQ(handle.impl->meta, testMeta);
    ASSERT_EQ(handle.impl->type, AssetDefs::Imagery);
    ASSERT_EQ(handle.impl->config, 1);
}

TEST_F(AssetFactoryTest, FindMake_Exists)
{
    MockMutableAsset handle = FindMake<MockMutableAsset, MockAssetConfig>
            (testAssetRef, testInputs, testMeta, testConfig0);
    ASSERT_EQ(handle.impl->name, testAssetRef);
    ASSERT_EQ(handle.impl->inputs, testInputs);
    ASSERT_EQ(handle.impl->meta, testMeta);
    ASSERT_EQ(handle.impl->type, AssetDefs::Imagery);
    ASSERT_EQ(handle.impl->config, 0);
    MockMutableAsset handle1 = FindMake<MockMutableAsset, MockAssetConfig>
            (testAssetRef1, AssetDefs::Imagery, testMeta, testConfig0);
    ASSERT_EQ(handle1.impl->name, testAssetRef1);
    ASSERT_EQ(handle1.impl->meta, testMeta);
    ASSERT_EQ(handle1.impl->type, AssetDefs::Imagery);
    ASSERT_EQ(handle.impl->config, 0);
    MockMutableAsset handle2 = FindMake<MockMutableAsset, MockAssetConfig>
            (testAssetRef, AssetDefs::Imagery, testInputs, testMeta, testConfig0);
    ASSERT_EQ(handle.impl->name, testAssetRef);
    ASSERT_EQ(handle.impl->inputs, testInputs);
    ASSERT_EQ(handle.impl->meta, testMeta);
    ASSERT_EQ(handle.impl->type, AssetDefs::Imagery);
    ASSERT_EQ(handle.impl->config, 0);
}

TEST_F(AssetFactoryTest, FindAndModifyPresent)
{
    MockMutableAsset::testSubTypeToUseForStringConstructor = MockAssetImpl::EXPECTED_SUBTYPE;
    MockMutableAsset handle = FindAndModify<MockMutableAsset, MockAssetConfig>(
                testAssetRef, testInputs, testMeta, testConfig0);
    ASSERT_EQ(handle.impl->name, testAssetRef);
    handle = FindAndModify<MockMutableAsset, MockAssetConfig>
            (testAssetRef, AssetDefs::Imagery, testInputs, testMeta, testConfig0);
    ASSERT_EQ(handle.impl->config, testConfig0);
    handle = FindAndModify<MockMutableAsset, MockAssetConfig>
            (testAssetRef, AssetDefs::Imagery, testMeta, testConfig0);
    ASSERT_EQ(handle.impl->meta, testMeta);
    ASSERT_EQ(handle.impl->type, AssetDefs::Imagery);
}

MockMutableAsset (*pFAMAbsent)( const std::string &ref_,
                                const std::vector<SharedString>& inputs_,
                                const khMetaData &meta,
                                const MockAssetConfig &config) =
  FindAndModify<MockMutableAsset, MockAssetConfig>;

TEST_F(AssetFactoryTest, FindAndModifyAbsent)
{
    try {
        MockMutableAsset handle = FindAndModify<MockMutableAsset, MockAssetConfig>(
                    string("not present"), testInputs, testMeta, testConfig0);
        FAIL() << "FindAndModify should have thrown an exception";
    } catch (...) {/* should throw an exception because ref isn't present*/}
}

TEST_F(AssetFactoryTest, FindMakeAndUpdateAssets)
{
    vector<AssetVersion> v;
    MockMutableAssetVersion
            handle6_5 = FindMakeAndUpdateSubAsset //tests 6 paramater FMAUS, which calls FMAU 5 params
                        <MockMutableAssetVersion, AssetVersion, MockAssetConfig>
                        ("parent", "base", testInputs, testMeta, testConfig0, v),
            handle7_6 = FindMakeAndUpdateSubAsset // tests 7 parameter FMAUs, FMAU 6 params
                        <MockMutableAssetVersion, AssetVersion, MockAssetConfig>
                        ("parent1", AssetDefs::Imagery, "base1", testInputs1,
                         testMeta, testConfig0, vector<AssetVersion>()), // tests remaining FMAU 4 params
            handle_4  = FindMakeAndUpdate<MockMutableAssetVersion, AssetVersion, MockAssetConfig>
                        ("parent1", AssetDefs::Imagery, testMeta, testConfig0);
    // want to just make sure paths reach MyUpdate method
    ASSERT_EQ(handle6_5->needed, true);
    ASSERT_EQ(handle7_6->needed, true);
    ASSERT_EQ(handle_4->needed, true);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
