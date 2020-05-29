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
#include "Asset.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

using namespace std;
using namespace AssetFactory;

const AssetDefs::Type EXPECTED_TYPE = AssetDefs::Imagery;
const string EXPECTED_SUBTYPE = "mockSubtype";

class MockExtraArgs {};

class MockAssetConfig {
  public:
    int8_t ID;
    bool upToDate;
    MockAssetConfig(std::int8_t _ID = -1) : ID(_ID), upToDate(true) {}
    bool operator==(const MockAssetConfig& other) const { return ID == other.ID; }
    bool operator==(uint8_t _ID) { return ID == _ID; }
    void operator=(const MockAssetConfig& other) { ID = other.ID; }
    void operator=(uint8_t _ID) { ID = _ID; }
    bool IsUpToDate(MockAssetConfig& other) const
    {
        return upToDate == other.upToDate;
    }
};

class MockAssetVersionImpl : public AssetVersionStorage {
  private:
    void SetUp() {
      state = mockState;
    }
  public:
    static string testSubTypeToUseForStringConstructor;
    static AssetDefs::State mockState;
    static bool throwInConstructor;

    bool updated;
    MockAssetConfig config;
    bool permanent;

    MockAssetVersionImpl(const SharedString & ref) {
      if (throwInConstructor) {
        throw std::runtime_error("Throwing because throwInConstructor==true");
      }
      name = ref;
      type = EXPECTED_TYPE;
      subtype = testSubTypeToUseForStringConstructor;
      updated = false;
      SetUp();
    }
    MockAssetVersionImpl(const AssetVersionStorage & storage) :
            AssetVersionStorage(storage) {
        SetUp();
    }
};

string MockAssetVersionImpl::testSubTypeToUseForStringConstructor;
AssetDefs::State MockAssetVersionImpl::mockState;
bool MockAssetVersionImpl::throwInConstructor;

class MockAssetVersionBase {
  public:
    SharedString ref;
    shared_ptr<MockAssetVersionImpl> impl;

    MockAssetVersionBase() = default;
    MockAssetVersionBase(const SharedString & ref) : ref(ref) {}
    MockAssetVersionBase(const AssetVersionStorage & storage) :
        ref(storage.name), impl(make_shared<MockAssetVersionImpl>(storage)) {}
    MockAssetVersionBase(const shared_ptr<MockAssetVersionImpl> impl) : impl(impl) {}
    explicit operator bool(void) const { return impl != nullptr; }
    MockAssetVersionImpl * operator->() const { return impl.get(); }
};

class MockAssetVersion : public MockAssetVersionBase {
  public:
    using Base = MockAssetVersionBase;

    MockAssetVersion() = default;
    MockAssetVersion(const SharedString & ref) : MockAssetVersionBase(ref) {}
    MockAssetVersion(const AssetVersionStorage & storage) : MockAssetVersionBase(storage) {}
    MockAssetVersion(const shared_ptr<MockAssetVersionImpl> impl) : MockAssetVersionBase(impl) {}
};

class MockAssetVersionD : public MockAssetVersion {
  public:
    MockAssetVersionD() = default;
    MockAssetVersionD(const SharedString & ref) : MockAssetVersion(ref) {}
    MockAssetVersionD(const AssetVersionStorage & storage) :
        MockAssetVersion(storage) {}
    MockAssetVersionD(const shared_ptr<MockAssetVersionImpl> impl) :
        MockAssetVersion(impl) {}

  void LoadAsTemporary() {
    impl = std::make_shared<MockAssetVersionImpl>(ref);
  }
  void MakePermanent() {
    impl->permanent = true;
  }
};

class MutableMockAssetVersionD : public MockAssetVersionD {
  public:
    MutableMockAssetVersionD() = default;
    MutableMockAssetVersionD(const AssetVersionStorage & storage) : MockAssetVersionD(storage) {}
    MutableMockAssetVersionD(const MockAssetVersionD & other) : MockAssetVersionD(other.impl) {}
};

class MockAssetImpl : public AssetStorage {
  private:
    void SetUp() {
      for (const auto & v : mockVersions) {
        versions.insert(versions.begin(), v);
      }
    }
  public:
    static string testSubTypeToUseForStringConstructor;
    static bool throwInConstructor;
    static vector<SharedString> mockVersions;

    MockAssetConfig config;
    bool modified;

    MockAssetImpl(const SharedString & ref) :
        AssetStorage(AssetStorage::MakeStorage(ref, EXPECTED_TYPE,
                     testSubTypeToUseForStringConstructor,
                     std::vector<SharedString>(), khMetaData())) {
        if (throwInConstructor) {
            throw std::runtime_error("Throwing because throwInConstrutor==true");
        }
        SetUp();
    }
    MockAssetImpl(const AssetStorage & storage, const MockAssetConfig & config) :
            AssetStorage(storage), config(config) {
        SetUp();
    }

    void Modify(const khMetaData& _meta, const MockAssetConfig& _config)
    {
        meta = _meta;
        config = _config;
        modified = true;
    }

    void Modify(const vector<SharedString>& _inputs,
                const khMetaData& _meta,
                const MockAssetConfig& _config)
    {
        inputs = _inputs;
        Modify(_meta, _config);
    }

    MutableMockAssetVersionD MyUpdate(bool& _needed,
                                      const vector<MockAssetVersionBase>& v = {})
    {
        auto retval = MutableMockAssetVersionD(
            AssetVersionStorage::MakeStorageFromAsset(*this));
        retval->updated = true;
        retval->meta = meta;
        return retval;
    }

    MutableMockAssetVersionD MyUpdate(bool& _needed,
                                      const MockExtraArgs& extras)
    {
        auto retval = MutableMockAssetVersionD(
            AssetVersionStorage::MakeStorageFromAsset(*this));
        retval->updated = true;
        retval->meta = meta;
        return retval;
    }

};

string MockAssetImpl::testSubTypeToUseForStringConstructor;
bool MockAssetImpl::throwInConstructor;
vector<SharedString> MockAssetImpl::mockVersions;

class MockAssetImplD : public MockAssetImpl {
  public:
    MockAssetImplD(const AssetStorage & storage, const MockAssetConfig & config) :
        MockAssetImpl(storage, config) {}
};

class MockAssetBase {
  public:
    shared_ptr<MockAssetImpl> impl;
    MockAssetBase() = default;
    MockAssetBase(const SharedString & ref) : impl(make_shared<MockAssetImpl>(ref)) {}
    MockAssetBase(shared_ptr<MockAssetImpl> impl) : impl(impl) {}
    explicit operator bool(void) const { return impl != nullptr; }
    MockAssetImpl * operator->() const { return impl.get(); }
};

class MockAsset : public MockAssetBase {
  public:
    MockAsset() = default;
    MockAsset(const SharedString & ref) : MockAssetBase(ref) {}
    MockAsset(shared_ptr<MockAssetImpl> impl) : MockAssetBase(impl) {}
};

class MockAssetD : public MockAsset {
  public:
    using BBase = MockAssetBase;
    MockAssetD() = default;
    MockAssetD(const SharedString & ref) : MockAsset(ref) {}
    MockAssetD(shared_ptr<MockAssetImpl> impl) : MockAsset(impl) {}
};

class MutableMockAssetD : public MockAssetD {
  public:
    MutableMockAssetD(shared_ptr<MockAssetImpl> impl) : MockAssetD(impl) {}
    MutableMockAssetD(const MockAssetD & other) : MockAssetD(other.impl) {}
};

struct MockType {
  using Asset = MockAsset;
  using Version = MockAssetVersion;
  using AssetD = MockAssetD;
  using VersionD = MockAssetVersionD;
  using MutableAssetD = MutableMockAssetD;
  using MutableVersionD = MutableMockAssetVersionD;
  using AssetImplD = MockAssetImplD;
  using Config = MockAssetConfig;

  static AssetDefs::Type TYPE;
  static string SUBTYPE;
};

AssetDefs::Type MockType::TYPE;
string MockType::SUBTYPE;

void resetStaticMockVariables() {
  // Reset most of the static variables in various mock classes back to
  // reasonable default behavior (no exceptions, etc.)
  MockType::TYPE = EXPECTED_TYPE;
  MockType::SUBTYPE = EXPECTED_SUBTYPE;

  MockAssetImpl::testSubTypeToUseForStringConstructor = "someOtherSubtype";
  MockAssetVersionImpl::testSubTypeToUseForStringConstructor = "someOtherSubtype";

  MockAssetImpl::throwInConstructor = false;
  MockAssetVersionImpl::throwInConstructor = false;

  MockAssetImpl::mockVersions = {};

  MockAssetVersionImpl::mockState = AssetDefs::New;
}

class AssetFactoryTest : public testing::Test {
 public:
  AssetFactoryTest()
  {
    resetStaticMockVariables();
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
  MutableMockAssetD handle = MakeNew<MockType>(
      testAssetRef, testInputs, testMeta, testConfig0);
  ASSERT_EQ(handle.impl->name, testAssetRef);
  ASSERT_EQ(handle.impl->inputs, testInputs);
  ASSERT_EQ(handle.impl->meta, testMeta);
  ASSERT_EQ(handle.impl->type, AssetDefs::Imagery);
}

TEST_F(AssetFactoryTest, MakeNewAlreadyExists) {
  // Make sure the std::string constructor for MockMutableAsset will have the same
  // subtype as the one we're trying to create in order to induce an exception.
  MockAssetImpl::testSubTypeToUseForStringConstructor = EXPECTED_SUBTYPE;
  ASSERT_THROW(MakeNew<MockType>(testAssetRef, testInputs, testMeta, testConfig0), khException);
}

TEST_F(AssetFactoryTest, FindMake_New)
{
    MutableMockAssetD handle = FindMake<MockType>
            (testAssetRef1, AssetDefs::Imagery, testInputs1, testMeta, MockAssetConfig(1));
    ASSERT_EQ(handle.impl->name, testAssetRef1);
    ASSERT_EQ(handle.impl->inputs, testInputs1);
    ASSERT_EQ(handle.impl->meta, testMeta);
    ASSERT_EQ(handle.impl->type, AssetDefs::Imagery);
    ASSERT_EQ(handle.impl->config, 1);
    ASSERT_FALSE(handle.impl->modified);
}

TEST_F(AssetFactoryTest, FindMake_Exists)
{
    MockAssetImpl::testSubTypeToUseForStringConstructor = EXPECTED_SUBTYPE;
    MutableMockAssetD handle = FindMake<MockType>
            (testAssetRef, testInputs, testMeta, testConfig0);
    ASSERT_EQ(handle.impl->name, testAssetRef);
    ASSERT_EQ(handle.impl->inputs, testInputs);
    ASSERT_EQ(handle.impl->meta, testMeta);
    ASSERT_EQ(handle.impl->type, AssetDefs::Imagery);
    ASSERT_EQ(handle.impl->config, 0);
    ASSERT_TRUE(handle.impl->modified);
    MutableMockAssetD handle2 = FindMake<MockType>
            (testAssetRef, AssetDefs::Imagery, testInputs, testMeta, testConfig0);
    ASSERT_EQ(handle.impl->name, testAssetRef);
    ASSERT_EQ(handle.impl->inputs, testInputs);
    ASSERT_EQ(handle.impl->meta, testMeta);
    ASSERT_EQ(handle.impl->type, AssetDefs::Imagery);
    ASSERT_EQ(handle.impl->config, 0);
    ASSERT_TRUE(handle.impl->modified);
}

TEST_F(AssetFactoryTest, FindAndModifyPresent)
{
    MockAssetImpl::testSubTypeToUseForStringConstructor = EXPECTED_SUBTYPE;
    MutableMockAssetD handle = FindAndModify<MockType>(
                testAssetRef, testInputs, testMeta, testConfig0);
    ASSERT_EQ(handle.impl->name, testAssetRef);
}

TEST_F(AssetFactoryTest, FindAndModifyAbsent)
{
    try {
        MutableMockAssetD handle = FindAndModify<MockType>(
                    string("not present"), testInputs, testMeta, testConfig0);
        FAIL() << "FindAndModify should have thrown an exception";
    } catch (...) {
      // should throw an exception because ref isn't present
    }
}

TEST_F(AssetFactoryTest, FindMakeAndUpdateAssets)
{
    vector<MockAssetVersionBase> v;
    MutableMockAssetVersionD
            handle6_5 = FindMakeAndUpdateSubAsset //tests 6 paramater FMAUS, which calls FMAU 5 params
                        <MockType>
                        ("parent", "base", testInputs, testMeta, testConfig0, v),
            handle7_6 = FindMakeAndUpdateSubAsset // tests 7 parameter FMAUs, FMAU 6 params
                        <MockType>
                        ("parent1", AssetDefs::Imagery, "base1", testInputs1,
                         testMeta, testConfig0, vector<MockAssetVersionBase>());
    // want to just make sure paths reach MyUpdate method
    ASSERT_TRUE(handle6_5->updated);
    ASSERT_TRUE(handle7_6->updated);
}

TEST_F(AssetFactoryTest, FiveParameterReuseOrMakeAndUpdateSubAsset)
{
  // ReuseOrMakeAndUpdateSubAsset
  // 417: parentName, type_, baseName, meta_, config_
  // calls ReuseOrMakeAndUpdate, 262: ref_, type_, meta_, config_

  {
    // Simplest call, no previous assets, no exceptions

    resetStaticMockVariables();
    MockAssetImpl::testSubTypeToUseForStringConstructor = EXPECTED_SUBTYPE;

    MutableMockAssetVersionD
      handle =
      ReuseOrMakeAndUpdateSubAsset<MockType>
      (
       "reuse-parent",    // const std::string& parentName,
       AssetDefs::Imagery, // AssetDefs::Type type_,
       "base",      // const std::string& baseName ,
       testMeta,    // const khMetaData& meta_,
       testConfig0 // const ConfigType& config_)
       );
    EXPECT_TRUE(handle->updated);
    EXPECT_EQ("reuse-parent/base.kia?version=1", handle.impl->name.toString());
    EXPECT_EQ(testMeta, handle.impl->meta);
    EXPECT_EQ(AssetDefs::Imagery, handle.impl->type);
    EXPECT_EQ(AssetDefs::New, handle.impl->state);
  }

  {
    // Test with 2 offline versions and make sure neither is used

    resetStaticMockVariables();
    MockAssetImpl::testSubTypeToUseForStringConstructor = EXPECTED_SUBTYPE;
    MockAssetImpl::mockVersions = {"reuse-parent/base.kia?version=1", "reuse-parent/base.kia?version=2"};
    MockAssetVersionImpl::mockState = AssetDefs::Offline;

    MutableMockAssetVersionD
      handle =
      ReuseOrMakeAndUpdateSubAsset<MockType>
      (
       "reuse-parent",    // const std::string& parentName,
       AssetDefs::Imagery, // AssetDefs::Type type_,
       "base",      // const std::string& baseName ,
       testMeta,    // const khMetaData& meta_,
       testConfig0 // const ConfigType& config_)
       );
    EXPECT_TRUE(handle->updated);
    EXPECT_EQ("reuse-parent/base.kia?version=3", handle.impl->name.toString());
    EXPECT_EQ(testMeta, handle.impl->meta);
    EXPECT_EQ(AssetDefs::Imagery, handle.impl->type);
    EXPECT_EQ(AssetDefs::Offline, handle.impl->state);
    // Since this version was just created it shouldn't be marked permanent
    EXPECT_FALSE(handle.impl->permanent);
  }

  {
    // Two previous versions, and version constructor throws

    resetStaticMockVariables();
    MockAssetImpl::testSubTypeToUseForStringConstructor = EXPECTED_SUBTYPE;
    MockAssetImpl::mockVersions = {"reuse-parent/base.kia?version=1", "reuse-parent/base.kia?version=2"};
    MockAssetVersionImpl::mockState = AssetDefs::Offline;
    MockAssetVersionImpl::throwInConstructor = true;

    MutableMockAssetVersionD
      handle =
      ReuseOrMakeAndUpdateSubAsset<MockType>
      (
       "reuse-parent",    // const std::string& parentName,
       AssetDefs::Imagery, // AssetDefs::Type type_,
       "base",      // const std::string& baseName ,
       testMeta,    // const khMetaData& meta_,
       testConfig0 // const ConfigType& config_)
       );
    EXPECT_TRUE(handle->updated);
    EXPECT_EQ("reuse-parent/base.kia?version=3", handle.impl->name.toString());
    EXPECT_EQ(testMeta, handle.impl->meta);
    EXPECT_EQ(AssetDefs::Imagery, handle.impl->type);
    EXPECT_EQ(AssetDefs::Offline, handle.impl->state);
    // Since this version was just created it shouldn't be marked permanent
    EXPECT_FALSE(handle.impl->permanent);
  }

  {
    // One previous version that matches

    resetStaticMockVariables();
    MockAssetImpl::testSubTypeToUseForStringConstructor = EXPECTED_SUBTYPE;
    MockAssetImpl::mockVersions = {"reuse-parent/base.kia?version=1"};

    MutableMockAssetVersionD
      handle =
      ReuseOrMakeAndUpdateSubAsset<MockType>
      (
       "reuse-parent",    // const std::string& parentName,
       AssetDefs::Imagery, // AssetDefs::Type type_,
       "base",      // const std::string& baseName ,
       testMeta,    // const khMetaData& meta_,
       testConfig0 // const ConfigType& config_)
       );
    EXPECT_FALSE(handle->updated);
    EXPECT_EQ("reuse-parent/base.kia?version=1", handle.impl->name.toString());
    EXPECT_EQ(testMeta, handle.impl->meta);
    EXPECT_EQ(AssetDefs::Imagery, handle.impl->type);
    EXPECT_EQ(AssetDefs::New, handle.impl->state);
    EXPECT_TRUE(handle.impl->permanent);
  }
}

TEST_F(AssetFactoryTest, SevenParameterReuseOrMakeAndUpdateSubAsset)
{
  // ReuseOrMakeAndUpdateSubAsset
  // 526: parentName, type_, baseName, inputs_, meta_, config_, cachedinputs_
  // calls ReuseOrMakeAndUpdate
  // 311: ref_, type, inputs_, meta_, config_, cachedinputs_

  vector<MockAssetVersionBase> cachedInputs;

  vector<SharedString> versionedInputs
    { "Input1?version=1", "Input2?version=4"};

  {
    // Simplest call, no previous assets, no exceptions

    resetStaticMockVariables();
    MockAssetImpl::testSubTypeToUseForStringConstructor = EXPECTED_SUBTYPE;

    MutableMockAssetVersionD
      handle =
      ReuseOrMakeAndUpdateSubAsset
      <MockType>
      (
       "reuse-parent",    // const std::string& parentName,
       AssetDefs::Imagery, // AssetDefs::Type type_,
       "base",      // const std::string& baseName ,
       versionedInputs,
       testMeta,    // const khMetaData& meta_,
       testConfig0, // const ConfigType& config_)
       cachedInputs
       );

    EXPECT_TRUE(handle->updated);
    EXPECT_EQ("reuse-parent/base.kia?version=1", handle.impl->name.toString());
    EXPECT_EQ(testMeta, handle.impl->meta);
    EXPECT_EQ(AssetDefs::Imagery, handle.impl->type);
    EXPECT_EQ(AssetDefs::New, handle.impl->state);
  }

    {
    // Test with 2 offline versions and make sure neither is used

    resetStaticMockVariables();
    MockAssetImpl::testSubTypeToUseForStringConstructor = EXPECTED_SUBTYPE;
    MockAssetImpl::mockVersions = {"reuse-parent/base.kia?version=1", "reuse-parent/base.kia?version=2"};
    MockAssetVersionImpl::mockState = AssetDefs::Offline;

    MutableMockAssetVersionD
      handle =
      ReuseOrMakeAndUpdateSubAsset
      <MockType>
      (
       "reuse-parent",    // const std::string& parentName,
       AssetDefs::Imagery, // AssetDefs::Type type_,
       "base",      // const std::string& baseName ,
       versionedInputs,
       testMeta,    // const khMetaData& meta_,
       testConfig0, // const ConfigType& config_)
       cachedInputs
       );
    EXPECT_TRUE(handle->updated);
    EXPECT_EQ("reuse-parent/base.kia?version=3", handle.impl->name.toString());
    EXPECT_EQ(testMeta, handle.impl->meta);
    EXPECT_EQ(AssetDefs::Imagery, handle.impl->type);
    EXPECT_EQ(AssetDefs::Offline, handle.impl->state);
    // Since this version was just created it shouldn't be marked permanent
    EXPECT_FALSE(handle.impl->permanent);
  }

  {
    // Two previous versions, and version constructor throws

    resetStaticMockVariables();
    MockAssetImpl::testSubTypeToUseForStringConstructor = EXPECTED_SUBTYPE;
    MockAssetImpl::mockVersions = {"reuse-parent/base.kia?version=1", "reuse-parent/base.kia?version=2"};
    MockAssetVersionImpl::mockState = AssetDefs::Offline;
    MockAssetVersionImpl::throwInConstructor = true;

    MutableMockAssetVersionD
      handle =
      ReuseOrMakeAndUpdateSubAsset
      <MockType>
      (
       "reuse-parent",    // const std::string& parentName,
       AssetDefs::Imagery, // AssetDefs::Type type_,
       "base",      // const std::string& baseName ,
       versionedInputs,
       testMeta,    // const khMetaData& meta_,
       testConfig0, // const ConfigType& config_)
       cachedInputs
       );
    EXPECT_TRUE(handle->updated);
    EXPECT_EQ("reuse-parent/base.kia?version=3", handle.impl->name.toString());
    EXPECT_EQ(testMeta, handle.impl->meta);
    EXPECT_EQ(AssetDefs::Imagery, handle.impl->type);
    EXPECT_EQ(AssetDefs::Offline, handle.impl->state);
    // Since this version was just created it shouldn't be marked permanent
    EXPECT_FALSE(handle.impl->permanent);
  }

  {
    // One previous version that matches

    resetStaticMockVariables();
    MockAssetImpl::testSubTypeToUseForStringConstructor = EXPECTED_SUBTYPE;
    MockAssetImpl::mockVersions = {"reuse-parent/base.kia?version=1"};

    MutableMockAssetVersionD
      handle =
      ReuseOrMakeAndUpdateSubAsset
      <MockType>
      (
       "reuse-parent",    // const std::string& parentName,
       AssetDefs::Imagery, // AssetDefs::Type type_,
       "base",      // const std::string& baseName ,
       vector<SharedString>(),
       testMeta,    // const khMetaData& meta_,
       testConfig0, // const ConfigType& config_)
       cachedInputs
       );
    EXPECT_FALSE(handle->updated);
    EXPECT_EQ("reuse-parent/base.kia?version=1", handle.impl->name.toString());
    EXPECT_EQ(testMeta, handle.impl->meta);
    EXPECT_EQ(AssetDefs::Imagery, handle.impl->type);
    EXPECT_EQ(AssetDefs::New, handle.impl->state);
    EXPECT_TRUE(handle.impl->permanent);
  }
}

TEST_F(AssetFactoryTest, ReuseFourParamsNoPrevious) {
  // No previous version
  MutableMockAssetVersionD handle =
    ReuseOrMakeAndUpdate
    <MockType>
    ("reuse-parent/base.kia",
      AssetDefs::Imagery,
      testMeta,
      testConfig0);

  EXPECT_TRUE(handle->updated);
  EXPECT_EQ("reuse-parent/base.kia?version=1", handle.impl->name.toString());
  EXPECT_EQ(testMeta, handle.impl->meta);
  EXPECT_EQ(AssetDefs::Imagery, handle.impl->type);
  EXPECT_EQ(AssetDefs::New, handle.impl->state);
}

TEST_F(AssetFactoryTest, ReuseFiveParamsNoVersions) {
  // No previous versions
  MockAssetImpl::testSubTypeToUseForStringConstructor = EXPECTED_SUBTYPE;

  MutableMockAssetVersionD handle =
    ReuseOrMakeAndUpdate
    <MockType>
    ("reuse-parent/base.kia",
      AssetDefs::Imagery,
      testMeta,
      testConfig0,
      MockExtraArgs());

  EXPECT_TRUE(handle->updated);
  EXPECT_EQ("reuse-parent/base.kia?version=1", handle.impl->name.toString());
  EXPECT_EQ(testMeta, handle.impl->meta);
  EXPECT_EQ(AssetDefs::Imagery, handle.impl->type);
  EXPECT_EQ(AssetDefs::New, handle.impl->state);
}

TEST_F(AssetFactoryTest, ReuseFiveParamsNoPrevious) {
  // Asset doesn't exist
  MutableMockAssetVersionD handle =
    ReuseOrMakeAndUpdate
    <MockType>
    ("reuse-parent/base.kia",
      AssetDefs::Imagery,
      testMeta,
      testConfig0,
      MockExtraArgs());

  EXPECT_TRUE(handle->updated);
  EXPECT_EQ("reuse-parent/base.kia?version=1", handle.impl->name.toString());
  EXPECT_EQ(testMeta, handle.impl->meta);
  EXPECT_EQ(AssetDefs::Imagery, handle.impl->type);
  EXPECT_EQ(AssetDefs::New, handle.impl->state);
}

TEST_F(AssetFactoryTest, ReuseFiveParamsVersionMatches) {
  // Previous version matches
  MockAssetImpl::testSubTypeToUseForStringConstructor = EXPECTED_SUBTYPE;
  MockAssetImpl::mockVersions = {"reuse-parent/base.kia?version=2"};

  MutableMockAssetVersionD handle =
    ReuseOrMakeAndUpdate
    <MockType>
    ("reuse-parent/base.kia",
      AssetDefs::Imagery,
      testMeta,
      testConfig0,
      MockExtraArgs());

  EXPECT_FALSE(handle->updated);
  EXPECT_TRUE(handle.impl->permanent);
  EXPECT_EQ("reuse-parent/base.kia?version=2", handle.impl->name.toString());
  EXPECT_EQ(testMeta, handle.impl->meta);
  EXPECT_EQ(AssetDefs::Imagery, handle.impl->type);
  EXPECT_EQ(AssetDefs::New, handle.impl->state);
}

TEST_F(AssetFactoryTest, ReuseFiveParamsVersionThrows) {
  // Version constructor throws
  MockAssetImpl::testSubTypeToUseForStringConstructor = EXPECTED_SUBTYPE;
  MockAssetImpl::mockVersions = {"reuse-parent/base.kia?version=1"};
  MockAssetVersionImpl::throwInConstructor = true;

  MutableMockAssetVersionD handle =
    ReuseOrMakeAndUpdate
    <MockType>
    ("reuse-parent/base.kia",
      AssetDefs::Imagery,
      testMeta,
      testConfig0,
      MockExtraArgs());

  EXPECT_TRUE(handle->updated);
  EXPECT_FALSE(handle.impl->permanent);
  EXPECT_EQ("reuse-parent/base.kia?version=2", handle.impl->name.toString());
  EXPECT_EQ(testMeta, handle.impl->meta);
  EXPECT_EQ(AssetDefs::Imagery, handle.impl->type);
  EXPECT_EQ(AssetDefs::New, handle.impl->state);
}

TEST_F(AssetFactoryTest, ReuseSixParams) {
  // Asset doesn't exist
  vector<SharedString> inputs = {"myref?version=1"};
  vector<MockAssetVersionBase> cachedInputs;
  for (auto inputRef : inputs) {
    cachedInputs.emplace_back(inputRef);
  }

  MutableMockAssetVersionD handle =
    ReuseOrMakeAndUpdate
    <MockType>
    ("reuse-parent/base.kia",
      AssetDefs::Imagery,
      inputs,
      testMeta,
      testConfig0,
      cachedInputs);

    EXPECT_TRUE(handle->updated);
    EXPECT_EQ("reuse-parent/base.kia?version=1", handle.impl->name.toString());
    EXPECT_EQ(testMeta, handle.impl->meta);
    EXPECT_EQ(AssetDefs::Imagery, handle.impl->type);
    EXPECT_EQ(AssetDefs::New, handle.impl->state);
}

TEST_F(AssetFactoryTest, FindThrow) {
  MockAssetImpl::throwInConstructor = true;
  MockAssetD handle = Find<MockType>("testref");
  EXPECT_FALSE(handle);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
