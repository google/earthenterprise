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

#include <gtest/gtest.h>
#include <string>

using namespace std;

// This file tests the template functions in StorageManager.h that are not
// directly a part of the storage manager class. These tests are separate from
// the other storage manager tests to keep the test size manageable and for
// easier compliation.

class MockAsset;

class MockAssetImpl {
  public:
    using AssetType = MockAsset;
    static AssetDefs::Type EXPECTED_TYPE;
    static string EXPECTED_SUBTYPE;
};

AssetDefs::Type MockAssetImpl::EXPECTED_TYPE;
string MockAssetImpl::EXPECTED_SUBTYPE;

class MockAsset {
  public:
    using Impl = MockAssetImpl;
    static bool BOOL_VALUE;
    string ref;
    AssetDefs::Type type;
    string subtype;

    MockAsset() = default;
    MockAsset(const std::string & ref) : ref(ref), type(Impl::EXPECTED_TYPE), subtype(Impl::EXPECTED_SUBTYPE) {}
    explicit operator bool() { return BOOL_VALUE; }
    // The functions we are testing expect an object that acts like a pointer,
    // so we provide a pointer to ourselves.
    MockAsset * operator->() { return this; }
};

bool MockAsset::BOOL_VALUE;

class AssetFactoryTest : public testing::Test {
  public:
    AssetFactoryTest() {
      // Reset the static variables
      MockAssetImpl::EXPECTED_TYPE = AssetDefs::Terrain;
      MockAssetImpl::EXPECTED_SUBTYPE = "Product";
      MockAsset::BOOL_VALUE = true;
    }
};

TEST_F(AssetFactoryTest, FindNoTypePass) {
  string testRef = "test_ref";
  auto asset = Find<MockAsset>(testRef);
  ASSERT_EQ(asset.ref, testRef);
}

TEST_F(AssetFactoryTest, FindNoTypeInvalid) {
  MockAssetImpl::EXPECTED_TYPE = AssetDefs::Invalid;
  ASSERT_DEATH(Find<MockAsset>("blank"), ".*");
}

TEST_F(AssetFactoryTest, FindAssetFalse) {
  MockAsset::BOOL_VALUE = false;
  auto asset = Find<MockAsset>("test_ref");
  ASSERT_EQ(asset.ref, "");
}

TEST_F(AssetFactoryTest, FindInvalidType) {
  ASSERT_DEATH(Find<MockAsset>("blank", AssetDefs::Invalid), ".*");
}

TEST_F(AssetFactoryTest, ValidateNoTypeInvalid) {
  MockAssetImpl::EXPECTED_TYPE = AssetDefs::Invalid;
  ASSERT_DEATH(ValidateRefForInput<MockAsset>("blank"), ".*");
}

TEST_F(AssetFactoryTest, ValidateInvalidType) {
  ASSERT_DEATH(ValidateRefForInput<MockAsset>("blank", AssetDefs::Invalid), ".*");
}

// TODO: new tests
// Find with type, type doesn't match
// Find with type, subtype doesn't match
// ValdiateRefForInput no type
// ValidateRefForInput type invalid
// ValidateRefForInput version current, find passes
// ValidateRefForInput version current, find fails
// ValidateRefForInput version not current, find passes
// ValidateRefForInput version not current, find fails
// Test that catches the load error we got from the bad cast

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
