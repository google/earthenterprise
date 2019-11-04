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

#include "StorageManagerAssetHandle.h"
#include "autoingest/.idl/storage/AssetDefs.h"

#include <gtest/gtest.h>
#include <type_traits>

using namespace std;

class TestItem {
  public:
    TestItem() : type(AssetDefs::Imagery) {}
    virtual ~TestItem() = default;
    AssetDefs::Type type;
};

class DerivedItem1 : public TestItem {};
class DerivedItem2 : public TestItem {};

TEST(AssetHandleTest, EmptyHandle) {
  AssetHandle<TestItem> empty;
  ASSERT_FALSE(empty.operator bool());
  ASSERT_EQ(empty.operator->(), nullptr);
}

TEST(AssetHandleTest, BasicHandle) {
  auto item = make_shared<TestItem>();
  AssetHandle<TestItem> handle(item, nullptr);
  ASSERT_TRUE(handle.operator bool());
  ASSERT_EQ(handle.operator->(), &*item);
}

TEST(AssetHandleTest, InvalidItem) {
  auto item = make_shared<TestItem>();
  item->type = AssetDefs::Invalid;
  AssetHandle<TestItem> handle(item, nullptr);
  ASSERT_FALSE(handle.operator bool());
}

TEST(AssetHandleTest, Finalize) {
  bool finalized = false;
  {
    auto item = make_shared<TestItem>();
    AssetHandle<TestItem> handle(item, [&]() {
      finalized = true;
    });
    ASSERT_FALSE(finalized);
  }
  ASSERT_TRUE(finalized);
}

TEST(AssetHandleTest, ConvertMutableToConst) {
  AssetHandle<TestItem> mutableHandle;
  AssetHandle<const TestItem> constHandle;
  constHandle = mutableHandle;
  ASSERT_EQ(mutableHandle.operator->(), constHandle.operator->());
}

TEST(AssetHandleTest, ConvertConstToMutable) {
  // We should not be able to convert a const handle to a mutable handle.
  if(std::is_assignable<AssetHandle<TestItem>, AssetHandle<const TestItem>>::value) {
    // If the above statement is wrapped in ASSERT_FALSE it will fail to compile
    // for some reason. This is our simple workaround.
    FAIL();
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
