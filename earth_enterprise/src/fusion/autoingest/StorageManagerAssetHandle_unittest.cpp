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

using namespace std;

class TestItem {
  public:
    TestItem() : type(AssetDefs::Invalid) {}
    AssetDefs::Type type;
};

TEST(AssetHandleTest, EmptyHandle) {
  AssetHandle<TestItem> empty;
  ASSERT_FALSE(empty.operator bool());
  ASSERT_EQ(empty.operator->(), nullptr);
}

TEST(AssetHandleTest, BasicHandle) {
  auto item = make_shared<TestItem>();
  item->type = AssetDefs::Imagery;
  AssetHandle<TestItem> handle(item, nullptr);
  ASSERT_TRUE(handle.operator bool());
  ASSERT_EQ(handle.operator->(), &*item);
}

TEST(AssetHandleTest, InvalidItem) {
  auto item = make_shared<TestItem>();
  AssetHandle<TestItem> handle(item, nullptr);
  ASSERT_FALSE(handle.operator bool());
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
