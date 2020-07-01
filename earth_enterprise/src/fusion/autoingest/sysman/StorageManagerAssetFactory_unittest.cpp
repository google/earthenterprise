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

#include "StorageManager.h"
#include "gee_version.h"
#include "AssetFactory.h"

#include <gtest/gtest.h>
#include <string>

using namespace std;
using namespace AssetFactory;

// This tests some of the simpler functions in AssetFactory using a simpler
// setup than AssetFactory_unittest.cpp, but the tests are more thorough.

class MockAsset {
  public:
    using BBase = MockAsset;

    static bool BOOL_VALUE;
    static AssetDefs::Type USE_TYPE;
    static string USE_SUBTYPE;
    
    string ref;
    AssetDefs::Type type;
    string subtype;

    MockAsset() = default;
    MockAsset(const std::string & ref) : ref(ref), type(USE_TYPE), subtype(USE_SUBTYPE) {}
    explicit operator bool() { return BOOL_VALUE; }
    // The functions we are testing expect an object that acts like a pointer,
    // so we provide a pointer to ourselves.
    MockAsset * operator->() { return this; }
};

class MockVersion : public MockAsset {
  public:
    using BBase = MockAsset;

    MockVersion() = default;
    MockVersion(const std::string & ref) : MockAsset(ref) {}
};

bool MockAsset::BOOL_VALUE;
AssetDefs::Type MockAsset::USE_TYPE;
string MockAsset::USE_SUBTYPE;

struct MockType {
  using Asset = MockAsset;
  using AssetD = MockAsset;
  using VersionD = MockVersion;

  static AssetDefs::Type TYPE;
  static string SUBTYPE;
};

AssetDefs::Type MockType::TYPE;
string MockType::SUBTYPE;

class AssetFactoryTest : public testing::Test {
  public:
    AssetFactoryTest() {
      // Reset the static variables
      MockType::TYPE = AssetDefs::Terrain;
      MockType::SUBTYPE = "Product";
      MockAsset::BOOL_VALUE = true;
      MockAsset::USE_TYPE = MockType::TYPE;
      MockAsset::USE_SUBTYPE = MockType::SUBTYPE;
    }
};

void testFindFailure() {
  auto asset = Find<MockType>("test_ref");
  ASSERT_EQ(asset.ref, "");
}

void testFindSuccess() {
  string testRef = "test_ref";
  auto asset = Find<MockType>(testRef);
  ASSERT_EQ(asset.ref, testRef);
}

TEST_F(AssetFactoryTest, FindNoTypePass) {
  testFindSuccess();
}

// Invalid types are only checked in asserts, so these tests will fail to die
// in release builds.
#ifndef NDEBUG
TEST_F(AssetFactoryTest, FindNoTypeInvalid) {
  MockType::TYPE = AssetDefs::Invalid;
  ASSERT_DEATH(Find<MockType>("blank"), ".*");
}
#endif

TEST_F(AssetFactoryTest, FindAssetFalse) {
  MockAsset::BOOL_VALUE = false;
  testFindFailure();
}

#ifndef NDEBUG
TEST_F(AssetFactoryTest, FindInvalidType) {
  ASSERT_DEATH(Find<MockType>("blank", AssetDefs::Invalid), ".*");
}
#endif

TEST_F(AssetFactoryTest, FindWrongType) {
  MockAsset::USE_TYPE = AssetDefs::Map;
  testFindFailure();
}

TEST_F(AssetFactoryTest, FindWrongTypePassedIn) {
  auto asset = Find<MockType>("test_ref", AssetDefs::Map);
  ASSERT_EQ(asset.ref, "");
}

TEST_F(AssetFactoryTest, FindWrongSubtype) {
  MockAsset::USE_SUBTYPE = "Map";
  testFindFailure();
}

// For the version tests the ref must include a version string to prevent
// AssetVersionRef from trying to load assets from disk.

#ifndef NDEBUG
TEST_F(AssetFactoryTest, ValidateNoTypeInvalid) {
  MockType::TYPE = AssetDefs::Invalid;
  ASSERT_DEATH(ValidateRefForInput<MockType>("blank?version=1"), ".*");
}
#endif

#ifndef NDEBUG
TEST_F(AssetFactoryTest, ValidateInvalidType) {
  ASSERT_DEATH(ValidateRefForInput<MockType>("blank?version=1", AssetDefs::Invalid), ".*");
}
#endif

TEST_F(AssetFactoryTest, ValidateCurrentVersionPass) {
  ValidateRefForInput<MockType>("blank?version=current");
}

TEST_F(AssetFactoryTest, ValidateCurrentVersionFail) {
  MockAsset::BOOL_VALUE = false;
  ASSERT_THROW(ValidateRefForInput<MockType>("blank?version=current"), std::invalid_argument);
}

TEST_F(AssetFactoryTest, ValidateNonCurrentVersionPass) {
  ValidateRefForInput<MockType>("blank?version=1");
}

TEST_F(AssetFactoryTest, ValidateNonCurrentVersionFail) {
  MockAsset::BOOL_VALUE = false;
  ASSERT_THROW(ValidateRefForInput<MockType>("blank?version=1"), std::invalid_argument);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
