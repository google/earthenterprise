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

const size_t CACHE_SIZE = 5;

class TestItem : public khRefCounter, public StorageManaged {
 public:
  TestItem() : val(nextValue++) {}
  const int val;
 private:
  static int nextValue;
};
int TestItem::nextValue = 1;
template<> const bool StorageManager<TestItem>::check_timestamps = false;

using HandleType = typename StorageManager<TestItem>::HandleType;
using AssetKey = typename StorageManager<TestItem>::AssetKey;

class TestHandle : public AssetHandleInterface<TestItem> {
  public:
    virtual const AssetKey Key() const { return name; }
    virtual std::string Filename() const { return "/dev/null"; }
    virtual HandleType Load(const std::string &) const {
      return khRefGuardFromNew<TestItem>(new TestItem());
    }
    virtual bool Valid(const HandleType &) const { return true; }
    TestHandle(StorageManager<TestItem> & storageManager,
               const AssetKey & name,
               bool checkFileExistenceFirst,
               bool addToCache,
               bool makeMutable) :
        name(name),
        handle(storageManager.Get(this, checkFileExistenceFirst, addToCache, makeMutable)) {}
    const AssetKey name;
    const HandleType handle;
};

class StorageManagerTest : public testing::Test {
 protected:
  StorageManager<TestItem> storageManager;
 public:
  StorageManagerTest() : storageManager(CACHE_SIZE, "test") {}
};

TEST_F(StorageManagerTest, AddAndRemove) {
  ASSERT_EQ(storageManager.CacheSize(), 0) << "Storage manager has unexpected item in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  // Create some items
  TestHandle first(storageManager, "first", false, true, false);
  TestHandle second(storageManager, "second", false, true, false);
  TestHandle third(storageManager, "third", false, true, false);
  
  ASSERT_EQ(storageManager.CacheSize(), 3) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  ASSERT_EQ(first.handle->val, 1) << "First item has unexpected value";
  ASSERT_EQ(second.handle->val, 2) << "Second item has unexpected value";
  ASSERT_EQ(third.handle->val, 3) << "Third item has unexpected value";
  
  // Retrieve one of the items
  TestHandle second2(storageManager, "second", false, true, false);
  ASSERT_EQ(storageManager.CacheSize(), 3) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  ASSERT_EQ(second.handle->val, second2.handle->val) << "Could not retrieve existing item from storage manager";
  
  // Remove one of the items
  storageManager.NoLongerNeeded(first.name);
  ASSERT_EQ(storageManager.CacheSize(), 2) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  storageManager.NoLongerNeeded(first.name);
  ASSERT_EQ(storageManager.CacheSize(), 2) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
