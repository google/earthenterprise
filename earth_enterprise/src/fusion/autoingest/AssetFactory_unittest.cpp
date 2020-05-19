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
#include "AssetVersion.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

using namespace std;
using namespace AssetFactory;

// forward declare classes
class MockAssetImpl;
class MockAssetConfig;
class MockMutableAsset;
class MockAssetVersionImpl;
class MockMutableAssetVersion;
class MockExtras;

class MockExtraArgs{};

class MockAssetStorage
{
public:
  SharedString name;
  AssetDefs::Type type;
  string subtype;
  vector<SharedString> inputs;
  khMetaData meta;
  vector<AssetVersion> versions;
  static vector<AssetVersion> MOCK_VERSIONS;

  static MockAssetStorage MakeStorage(const SharedString &name_,
                                      AssetDefs::Type type_,
                                      const std::string &subtype_,
                                      const std::vector<SharedString> &inputs_,
                                      const khMetaData &meta_) {
        MockAssetStorage storage;
        storage.name = name_;
        storage.type = type_;
        storage.subtype = subtype_;
        storage.inputs = inputs_;
        storage.meta = meta_;
        storage.versions = MockAssetStorage::MOCK_VERSIONS;
        return storage;
    }
};

vector<AssetVersion> MockAssetStorage::MOCK_VERSIONS = {};

// added ID field to aid in comparison
class MockAssetConfig
{
public:
    int8_t ID;
    bool upToDate;
    MockAssetConfig(int8 _ID = -1) : ID(_ID), upToDate(true) {}
    bool operator==(const MockAssetConfig& other) const { return ID == other.ID; }
    bool operator==(uint8_t _ID) { return ID == _ID; }
    void operator=(const MockAssetConfig& other) { ID = other.ID; }
    void operator=(uint8_t _ID) { ID = _ID; }
    bool IsUpToDate(MockAssetConfig& other) const
    {
        return upToDate == other.upToDate;
    }
 };

class MockAssetVersionImpl : public MockAssetStorage
{
public:
    using MutableAssetType = MockMutableAsset;
    static AssetDefs::Type EXPECTED_TYPE;
    static string EXPECTED_SUBTYPE;
    static AssetDefs::State EXPECTED_STATE;

    using Base = MockAssetStorage;

    MockAssetConfig config;
    bool needed;
    AssetDefs::State state;

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

    bool IsUpToDate(const MockAssetVersionImpl& other) const
    {
        return (this->name == other.name) &&
               (this->subtype == other.subtype) &&
               (this->meta == other.meta) &&
               (this->inputs == other.inputs) &&
               (this->config == other.config) &&
               (this->state == other.state) &&
               (this->type == other.type);
    }

    MockAssetVersionImpl(MockAssetStorage storage,
                  const MockAssetConfig &config_)
        : MockAssetStorage(storage), config(config_), needed(false),
          state(EXPECTED_STATE) {}
};

AssetDefs::State MockAssetVersionImpl::EXPECTED_STATE = AssetDefs::New;


class MockMutableAssetVersion
{
public:
    using Impl = MockAssetVersionImpl;
    static string testSubTypeToUseForStringConstructor;
    shared_ptr<Impl> impl = nullptr;

    MockMutableAssetVersion() = default;
    MockMutableAssetVersion(const string& ref_)
    {
        MockAssetStorage storage =
            MockAssetStorage::MakeStorage(ref_,
                                          Impl::EXPECTED_TYPE,
                                          testSubTypeToUseForStringConstructor,
                                          {},
                                          khMetaData());
        MockAssetConfig config;
        impl = make_shared<Impl>(storage, config);
    }

    MockMutableAssetVersion(const MockAssetStorage& storage, const MockAssetConfig& config)
    {
        impl = make_shared<Impl>(storage, config);
    }

    MockMutableAssetVersion(const string& name, AssetDefs::Type type)
    {
        MockAssetStorage storage =
            MockAssetStorage::MakeStorage(name, type, "", {}, khMetaData());
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

class MockVersionDType : public MockMutableAssetVersion
{
public:
  // These are the parameters used to call MakeStorage() inside LoadAsTemporary()
  // In regular code they're figured out automatically when the version is loaded
  // in the constructor.  Since the mocks don't really load a version, these values
  // are used instead.
  static string EXPECTED_REF;
  static AssetDefs::Type EXPECTED_TYPE;
  static string EXPECTED_SUBTYPE;
  static bool THROW_IN_CONSTRUCTOR;

  vector<AssetVersion> versions;
  bool permanent;

  MockVersionDType(const AssetVersion& v)
  {
    if (MockVersionDType::THROW_IN_CONSTRUCTOR) {
      MockVersionDType::THROW_IN_CONSTRUCTOR = false;
      throw std::runtime_error("Throwing because THROW_IN_CONSTRUCTOR==true");
    }
    permanent = false;
    versions.push_back(v);
  }
  void NoLongerNeeded() {
    impl->needed = false;
    ++MockVersionDType::NOT_NEEDED_COUNT;
  }
  void LoadAsTemporary() {
    MockAssetStorage storage = MockAssetStorage::MakeStorage(EXPECTED_REF,
                                                             EXPECTED_TYPE,
                                                             EXPECTED_SUBTYPE,
                                                             {},
                                                             khMetaData());
    MockAssetConfig config;
    permanent = false;
    impl = std::make_shared<Impl>(storage, config);
  }
  void MakePermanent() {
    permanent = true;
    impl->needed = true;
  }

  static size_t NOT_NEEDED_COUNT;
};

string MockVersionDType::EXPECTED_REF;
AssetDefs::Type MockVersionDType::EXPECTED_TYPE;
string MockVersionDType::EXPECTED_SUBTYPE;
size_t MockVersionDType::NOT_NEEDED_COUNT = 0;
bool MockVersionDType::THROW_IN_CONSTRUCTOR = false;


class MockAssetImpl: public MockAssetStorage
{
public:
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

  MockAssetImpl(MockAssetStorage storage,
                const MockAssetConfig &config_)
              : MockAssetStorage(storage), config(config_), needed(false) {}

  MockMutableAssetVersion MyUpdate(bool& _needed,
                                   const vector<AssetVersion>& v = {})
  {
      auto retval = MockMutableAssetVersion(this->name, this->type);
      retval->needed = _needed = true;
      return retval;
  }

  MockMutableAssetVersion MyUpdate(bool& _needed,
                                   const MockExtraArgs& extras)
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
    static bool THROW_IN_CONSTRUCTOR;
    shared_ptr<Impl> impl = nullptr;

    MockMutableAsset() = default;
    MockMutableAsset(shared_ptr<Impl> impl_) : impl(impl_) {}

    MockMutableAsset(const string &ref_)
    {
      if (THROW_IN_CONSTRUCTOR) {
          THROW_IN_CONSTRUCTOR=false;
          throw std::runtime_error("MockMustableAsset::THROW_IN_CONSTRUCTOR == true");
      }
      // Need to be ready to construct a mock from a string reference.
      // Leaving the subtype changeable by the tests via testSubTypeToUseForStringConstructor;
      MockAssetStorage storage = MockAssetStorage::MakeStorage(ref_,
                                                               Impl::EXPECTED_TYPE,
                                                               testSubTypeToUseForStringConstructor,
                                                               {},
                                                               khMetaData());
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

bool MockMutableAsset::THROW_IN_CONSTRUCTOR = false;
string MockMutableAsset::testSubTypeToUseForStringConstructor;


void resetStaticMockVariables() {
  // Reset most of the static variables in various mock classes back to
  // reasonable default behavior (no exceptions, etc.)
  MockMutableAsset::testSubTypeToUseForStringConstructor="";

  MockVersionDType::EXPECTED_REF = "";
  MockVersionDType::EXPECTED_TYPE = AssetDefs::Imagery;
  MockVersionDType::EXPECTED_SUBTYPE = "";

  MockAssetStorage::MOCK_VERSIONS={};
  MockAssetVersionImpl::EXPECTED_STATE = AssetDefs::New;

  MockVersionDType::NOT_NEEDED_COUNT = 0;
  MockVersionDType::THROW_IN_CONSTRUCTOR = false;
  MockMutableAsset::THROW_IN_CONSTRUCTOR = false;
  MockAssetImpl::EXPECTED_TYPE = MockAssetVersionImpl::EXPECTED_TYPE = AssetDefs::Imagery;

  MockAssetImpl::EXPECTED_SUBTYPE = MockAssetVersionImpl::EXPECTED_SUBTYPE = "mockSubtype";

  MockMutableAsset::testSubTypeToUseForStringConstructor =
    MockMutableAssetVersion::testSubTypeToUseForStringConstructor = "someOtherSubtype";
}


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
