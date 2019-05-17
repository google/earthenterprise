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

#include <algorithm>
#include <gtest/gtest.h>
#include <sstream>

using namespace std;

const size_t CACHE_SIZE = 5;

class TestItem : public khRefCounter, public StorageManaged {
 public:
  TestItem() : val(nextValue++), saveSucceeds(true) {}
  const int val;
  string savename;
  bool saveSucceeds;
  const string XMLFilename() {
    stringstream filename;
    filename << val;
    return filename.str();
  }
  virtual bool Save(const string &filename) {
    savename = filename;
    return saveSucceeds;
  }
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
    virtual string Filename() const { return "/dev/null"; }
    virtual HandleType Load(const string &) const {
      return khRefGuardFromNew<TestItem>(new TestItem());
    }
    virtual bool Valid(const HandleType &) const { return true; }
    TestHandle(const AssetKey & name) : name(name) {}
    TestHandle() = default;
    AssetKey name;
    HandleType handle;
};

template<class HandleClass>
HandleClass Get(StorageManager<TestItem> & storageManager,
                const AssetKey & name,
                bool checkFileExistenceFirst,
                bool addToCache,
                bool makeMutable) {
  HandleClass handle(name);
  handle.handle = storageManager.Get(&handle, checkFileExistenceFirst, addToCache, makeMutable);
  return handle;
}

class StorageManagerTest : public testing::Test {
 protected:
  StorageManager<TestItem> storageManager;
 public:
  StorageManagerTest() : storageManager(CACHE_SIZE, "test") {}
};

TEST_F(StorageManagerTest, AddAndRetrieve) {
  ASSERT_EQ(storageManager.CacheSize(), 0) << "Storage manager has unexpected item in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  // Create some items
  TestHandle first = Get<TestHandle>(storageManager, "first", false, true, false);
  TestHandle second = Get<TestHandle>(storageManager, "second", false, true, false);
  TestHandle third = Get<TestHandle>(storageManager, "third", false, true, false);
  
  ASSERT_EQ(storageManager.CacheSize(), 3) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  ASSERT_EQ(first.handle->val, 1) << "First item has unexpected value";
  ASSERT_EQ(second.handle->val, 2) << "Second item has unexpected value";
  ASSERT_EQ(third.handle->val, 3) << "Third item has unexpected value";
  
  // Retrieve one of the items
  TestHandle second2 = Get<TestHandle>(storageManager, "second", false, true, false);
  ASSERT_EQ(storageManager.CacheSize(), 3) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  ASSERT_EQ(second.handle->val, second2.handle->val) << "Could not retrieve existing item from storage manager";
}

TEST_F(StorageManagerTest, LoadWithoutCache) {
  TestHandle newItem = Get<TestHandle>(storageManager, "new", false, false, false);
  ASSERT_EQ(storageManager.CacheSize(), 0) << "Storage manager has unexpected item in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  storageManager.AddExisting(newItem.name, newItem.handle);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  TestHandle new2 = Get<TestHandle>(storageManager, "new", false, true, false);
  ASSERT_EQ(newItem.handle->val, new2.handle->val) << "Could not retrieve existing item from storage manager.";
}

TEST_F(StorageManagerTest, AddNew) {
  HandleType newItem(khRefGuardFromNew(new TestItem()));
  ASSERT_EQ(storageManager.CacheSize(), 0) << "Storage manager has unexpected item in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  storageManager.AddNew("newItem", newItem);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";
  
  TestHandle another = Get<TestHandle>(storageManager, "another", false, true, false);
  ASSERT_EQ(storageManager.CacheSize(), 2) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";

  TestHandle retrieved = Get<TestHandle>(storageManager, "newItem", false, true, false);
  ASSERT_EQ(newItem->val, retrieved.handle->val) << "Could not retrieve new item from storage manager.";
}

class TestHandleBadFile : public TestHandle {
  public:
    virtual string Filename() const { return "notafile"; }
    TestHandleBadFile(const AssetKey & name) : TestHandle(name) {}
};

TEST_F(StorageManagerTest, CheckFileExistence) {
  TestHandle goodFile = Get<TestHandle>(storageManager, "good", true, true, false);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  TestHandleBadFile badFile = Get<TestHandleBadFile>(storageManager, "bad", true, true, false);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  ASSERT_FALSE(badFile.handle) << "Should get empty handle from non-existant file";
}

TEST_F(StorageManagerTest, Mutable) {
  TestHandle mut = Get<TestHandle>(storageManager, "mutable", false, true, true);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";

  TestHandle another = Get<TestHandle>(storageManager, "another", false, true, false);
  ASSERT_EQ(storageManager.CacheSize(), 2) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";
}

TEST_F(StorageManagerTest, PurgeCache) {
  // Put items in the cache but don't hold handles so they will be purged
  for(size_t i = 0; i < CACHE_SIZE + 2; ++i) {
    stringstream s;
    s << "asset" << i;
    Get<TestHandle>(storageManager, s.str(), false, true, false);
    ASSERT_EQ(storageManager.CacheSize(), min(i+1, CACHE_SIZE)) << "Unexpected number of items in cache";
    ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  }
}

TEST_F(StorageManagerTest, PurgeCacheWithHandles) {
  {
    // Put items in the cache and hold handles so they won't be purged
    TestHandle handles[CACHE_SIZE+3];
    for(size_t i = 0; i < CACHE_SIZE + 3; ++i) {
      stringstream s;
      s << "asset" << i;
      handles[i] = Get<TestHandle>(storageManager, s.str(), false, true, false);
      ASSERT_EQ(storageManager.CacheSize(), i+1) << "Unexpected number of items in cache";
      ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
    }
  }
  
  // Now that we no longer have handles items can be removed from the cache.
  // Remove an item but don't purge the cache
  storageManager.NoLongerNeeded("asset0", false);
  ASSERT_EQ(storageManager.CacheSize(), CACHE_SIZE+2) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  // Remove an item and do purge the cache
  storageManager.NoLongerNeeded("asset1");
  ASSERT_EQ(storageManager.CacheSize(), CACHE_SIZE) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  storageManager.NoLongerNeeded("asset1");
  ASSERT_EQ(storageManager.CacheSize(), CACHE_SIZE) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
}

class TestHandleInvalid : public TestHandle {
  public:
    virtual bool Valid(const HandleType &) const { return false; }
    TestHandleInvalid(const AssetKey & name) : TestHandle(name) {}
};

TEST_F(StorageManagerTest, InvalidItemInCache) {
  TestHandle invalid = Get<TestHandle>(storageManager, "invalid", false, true, false);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  // Ensure that if we get it again it is reloaded since it's invalid
  TestHandle invalid2 = Get<TestHandleInvalid>(storageManager, "invalid", false, true, false);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  ASSERT_NE(invalid.handle->val, invalid2.handle->val) << "Did not reload invalid item";
}

TEST_F(StorageManagerTest, Abort) {
  TestHandle handles[5];
  handles[0] = Get<TestHandle>(storageManager, "asset0", false, true, false);
  handles[1] = Get<TestHandle>(storageManager, "asset1", false, true, false);
  handles[2] = Get<TestHandle>(storageManager, "mutable2", false, true, true);
  handles[3] = Get<TestHandle>(storageManager, "asset3", false, true, false);
  handles[4] = Get<TestHandle>(storageManager, "mutable4", false, true, true);
  ASSERT_EQ(storageManager.CacheSize(), 5) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 2) << "Storage manager has wrong number of items in dirty map";
  
  // Abort should remove the mutable items but nothing else
  storageManager.Abort();
  ASSERT_EQ(storageManager.CacheSize(), 3) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has wrong number of items in dirty map";
  
  // Try to reload one of the non-dirty items
  TestHandle notdirty = Get<TestHandle>(storageManager, "asset3", false, true, false);
  ASSERT_EQ(handles[3].handle->val, notdirty.handle->val) << "Non-dirty item was removed from cache on abort";
  
  // Try to reload one of the dirty items
  TestHandle dirty = Get<TestHandle>(storageManager, "mutable2", false, true, true);
  ASSERT_NE(handles[2].handle->val, dirty.handle->val) << "Dirty item was not removed from cache on abort";
  ASSERT_EQ(storageManager.CacheSize(), 4) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";
}

void getAssetsForDirtyTest(StorageManager<TestItem> & storageManager, TestHandle handles[5]) {
  handles[0] = Get<TestHandle>(storageManager, "asset0", false, true, false);
  handles[1] = Get<TestHandle>(storageManager, "asset1", false, true, false);
  handles[2] = Get<TestHandle>(storageManager, "mutable2", false, true, true);
  handles[3] = Get<TestHandle>(storageManager, "asset3", false, true, false);
  handles[4] = Get<TestHandle>(storageManager, "mutable4", false, true, true);
  ASSERT_EQ(storageManager.CacheSize(), 5) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 2) << "Storage manager has wrong number of items in dirty map";
}

TEST_F(StorageManagerTest, SaveDirty) {
  TestHandle handles[5];
  getAssetsForDirtyTest(storageManager, handles);
  
  khFilesTransaction trans;
  storageManager.SaveDirtyToDotNew(trans, nullptr);
  ASSERT_EQ(storageManager.CacheSize(), 5) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has wrong number of items in dirty map";
  ASSERT_EQ(trans.NumNew(), 2) << "Wrong number of new items in file transaction";
  ASSERT_EQ(trans.NumDeleted(), 0) << "Wrong number of deleted items in file transaction";
  ASSERT_EQ(handles[0].handle->savename, string()) << "Non-dirty files should not be saved";
  ASSERT_NE(handles[4].handle->savename, string()) << "Dirty files should be saved";
  string savename = handles[4].handle->savename;
  string ext = ".new";
  ASSERT_TRUE(equal(ext.rbegin(), ext.rend(), savename.rbegin())) << "Saved file name must end in .new";
}

TEST_F(StorageManagerTest, SaveDirtyToVector) {
  TestHandle handles[5];
  getAssetsForDirtyTest(storageManager, handles);
  
  khFilesTransaction trans;
  vector<string> saved;
  storageManager.SaveDirtyToDotNew(trans, &saved);
  ASSERT_EQ(saved.size(), 2) << "Wrong number of items in saved vector";
  ASSERT_TRUE(find(saved.begin(), saved.end(), "mutable2") != saved.end()) << "Dirty item missing from saved vector";
  ASSERT_TRUE(find(saved.begin(), saved.end(), "mutable4") != saved.end()) << "Dirty item missing from saved vector";
}

TEST_F(StorageManagerTest, FailedSave) {
  TestHandle item = Get<TestHandle>(storageManager, "item", false, true, true);
  item.handle->saveSucceeds = false;
  khFilesTransaction trans;
  storageManager.SaveDirtyToDotNew(trans, nullptr);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";
  ASSERT_EQ(trans.NumNew(), 0) << "Transaction should be empty after failed save";
  ASSERT_EQ(trans.NumDeleted(), 0) << "Wrong number of deleted items in file transaction";
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
