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

#include <gtest/gtest.h>
#include <string>

using namespace std;

class MockAssetImpl {
  public:
    static const AssetDefs::Type EXPECTED_TYPE;
    static const string EXPECTED_SUBTYPE;
};

const AssetDefs::Type MockAssetImpl::EXPECTED_TYPE = AssetDefs::Terrain;
const string MockAssetImpl::EXPECTED_SUBTYPE = "Product";

class MockAsset {
  public:
    using Impl = MockAssetImpl;
    string ref;
    AssetDefs::Type type;
    string subtype;

    MockAsset() = default;
    MockAsset(const std::string & ref) : ref(ref), type(Impl::EXPECTED_TYPE), subtype(Impl::EXPECTED_SUBTYPE) {}
    explicit operator bool() { return true; }
    MockAsset * operator->() { return this; }
};

class AssetFactoryTest : public testing::Test {
  public:
    AssetFactoryTest() {
      // Reset the static variables
    }
};

TEST_F(AssetFactoryTest, FindNoTypePass) {
  string testRef = "test_ref";
  auto asset = Find<MockAsset>(testRef);
  ASSERT_EQ(asset.ref, testRef);
}


// This file tests the template functions in StorageManager.h that are not
// directly a part of the storage manager class. These tests are separate from
// the other storage manager tests to keep the test size manageable.

// TODO: new tests
// Find with type invalid
// Find with type, Asset returns false
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
