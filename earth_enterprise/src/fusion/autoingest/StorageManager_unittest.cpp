/*
 * Copyright 2020 The Open GEE Contributors
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
#include "CacheSizeCalculations.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <sstream>
#include <memory>
#include <type_traits>

using namespace std;

const size_t CACHE_SIZE = 5;

class TestItemStorage {
  public:
    TestItemStorage() {}
};

class TestItem : public StorageManaged, public TestItemStorage {
 public:
  using Base = TestItemStorage;

  static int nextValue;
  static string fileName;
  static bool isValidRef;

  const int val;
  AssetDefs::Type type;
  string savename;
  string loadname;
  bool saveSucceeds;
  TestItem() : val(nextValue++), type(AssetDefs::Imagery), saveSucceeds(true) {}
  const string XMLFilename() {
    return TestItem::Filename(SharedString());
  }

  std::string GetName() const {
    return "TestItem";
  }

  // determine amount of memory used by TestItem
  std::uint64_t GetHeapUsage() const{
    return ::GetHeapUsage(nextValue)
    + ::GetHeapUsage(fileName)
    + ::GetHeapUsage(isValidRef)
    + ::GetHeapUsage(val)
    + ::GetHeapUsage(type)
    + ::GetHeapUsage(savename)
    + ::GetHeapUsage(loadname)
    + ::GetHeapUsage(saveSucceeds);
  }

  static string Filename(const std::string ref) {
    return fileName;
  }
  static SharedString Key(const SharedString & ref) {
    return ref;
  }
  static bool ValidRef(const SharedString & ref) {
    return isValidRef;
  }
};

inline std::uint64_t GetHeapUsage(const TestItem &obj) {
  return obj.GetHeapUsage();
}

class TestSerializer : public AssetSerializerInterface<TestItem> {
  public:
    virtual AssetPointerType<TestItem> Load(const std::string & filename) {
      auto asset = std::make_shared<TestItem>();
      asset->loadname = filename;
      return asset;
    }
    virtual bool Save(AssetPointerType<TestItem> asset, std::string filename) {
      asset->savename = filename;
      return asset->saveSucceeds;
    }
};

int TestItem::nextValue;
string TestItem::fileName;
bool TestItem::isValidRef;
template<> const bool StorageManager<TestItem>::check_timestamps = false;

using PointerType = AssetPointerType<TestItem>;

class TestHandle : public AssetHandleInterface<TestItem> {
  public:
    virtual bool Valid(const PointerType &) const { return true; }
    TestHandle(const AssetKey & name) : name(name) {}
    TestHandle() = default;
    AssetKey name;
    PointerType handle;
};

template<class HandleClass>
HandleClass Get(StorageManager<TestItem> & storageManager,
                const AssetKey & name,
                bool checkFileExistenceFirst,
                bool addToCache,
                bool makeMutable) {
  HandleClass handle(name);
  handle.handle = storageManager.Get(&handle, name, checkFileExistenceFirst, addToCache, makeMutable);
  return handle;
}

class StorageManagerTest : public testing::Test {
 protected:
  StorageManager<TestItem> storageManager;
 public:
  StorageManagerTest() : storageManager(CACHE_SIZE, false, 0, 100, "test", unique_ptr<TestSerializer>(new TestSerializer())) {
    // Reset the static variables in TestItem
    TestItem::nextValue = 1;
    TestItem::fileName = "/dev/null"; // A file that exists
    TestItem::isValidRef = true;
  }
};

// Tests using the legacy Get method
TEST_F(StorageManagerTest, AddAndRetrieveLegacy) {
  int startValue = TestItem::nextValue;
  ASSERT_EQ(storageManager.CacheSize(), 0) << "Storage manager has unexpected item in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  // Create some items
  TestHandle first = Get<TestHandle>(storageManager, "first", false, true, false);
  TestHandle second = Get<TestHandle>(storageManager, "second", false, true, false);
  TestHandle third = Get<TestHandle>(storageManager, "third", false, true, false);
  
  ASSERT_EQ(storageManager.CacheSize(), 3) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  ASSERT_EQ(first.handle->val, startValue) << "First item has unexpected value";
  ASSERT_EQ(second.handle->val, startValue + 1) << "Second item has unexpected value";
  ASSERT_EQ(third.handle->val, startValue + 2) << "Third item has unexpected value";
  ASSERT_EQ(first.handle->loadname, "first") << "Asset loaded from wrong file";
  
  // Retrieve one of the items
  TestHandle second2 = Get<TestHandle>(storageManager, "second", false, true, false);
  ASSERT_EQ(storageManager.CacheSize(), 3) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  ASSERT_EQ(second.handle->val, second2.handle->val) << "Could not retrieve existing item from storage manager";
}

TEST_F(StorageManagerTest, LoadWithoutCacheLegacy) {
  TestHandle newItem = Get<TestHandle>(storageManager, "new", false, false, false);
  ASSERT_EQ(storageManager.CacheSize(), 0) << "Storage manager has unexpected item in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  storageManager.AddExisting(newItem.name, newItem.handle);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  TestHandle new2 = Get<TestHandle>(storageManager, "new", false, true, false);
  ASSERT_EQ(newItem.handle->val, new2.handle->val) << "Could not retrieve existing item from storage manager.";
}

TEST_F(StorageManagerTest, AddNewLegacy) {
  PointerType newItem(new TestItem());
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

TEST_F(StorageManagerTest, CheckFileExistenceLegacy) {
  TestHandle goodFile = Get<TestHandle>(storageManager, "good", true, true, false);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  TestItem::fileName = "notafile"; // Try to read an invalid file
  TestHandle badFile = Get<TestHandle>(storageManager, "bad", true, true, false);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  ASSERT_FALSE(badFile.handle) << "Should get empty handle from non-existant file";
}

TEST_F(StorageManagerTest, MutableLegacy) {
  TestHandle mut = Get<TestHandle>(storageManager, "mutable", false, true, true);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";

  TestHandle another = Get<TestHandle>(storageManager, "another", false, true, false);
  ASSERT_EQ(storageManager.CacheSize(), 2) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";
}

TEST_F(StorageManagerTest, PurgeCacheBasedOnNumberOfObjectsLegacy) {
  // Put items in the cache but don't hold handles so they will be purged
  for(size_t i = 0; i < CACHE_SIZE + 2; ++i) {
    stringstream s;
    s << "asset" << i;
    Get<TestHandle>(storageManager, s.str(), false, true, false);
    ASSERT_EQ(storageManager.CacheSize(), min(i+1, CACHE_SIZE)) << "Unexpected number of items in cache";
    ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  }
}

TEST_F(StorageManagerTest, PurgeCacheBasedOnMemoryUtilizationLegacy) {
  // Purges items from cache when the memory utilization exceeds
  // the limit and determines if the cache memory usage reflects the size
  // of the items in cache.
  size_t i;
  std::uint64_t cacheItemSize = 0;
  std::uint64_t memoryLimit = 0;
  for(i = 0; i < CACHE_SIZE + 2; ++i) {
    stringstream s;
    s << "asset" << i;
    Get<TestHandle>(storageManager, s.str(), false, true, false);
    if (cacheItemSize == 0) {
      cacheItemSize = storageManager.GetCacheItemSize(s.str());
      memoryLimit = cacheItemSize * (CACHE_SIZE - 1);
      storageManager.SetCacheMemoryLimit(true, memoryLimit);
    }
    ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  }
  ASSERT_EQ(storageManager.CacheSize(), (CACHE_SIZE - 1)) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.CacheMemoryUse(), memoryLimit) << "Unexpected memory usage";
}

TEST_F(StorageManagerTest, PurgeCacheWithHandlesLegacy) {
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
  // Add another asset and make sure the cache is purged.
  Get<TestHandle>(storageManager, "asset999", false, true, false);
  ASSERT_EQ(storageManager.CacheSize(), CACHE_SIZE) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";

  // Add another item and make sure the cache is purged again.
  Get<TestHandle>(storageManager, "asset998", false, true, false);
  ASSERT_EQ(storageManager.CacheSize(), CACHE_SIZE) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
}

class TestHandleInvalidLegacy : public TestHandle {
  public:
    virtual bool Valid(const PointerType &) const { return false; }
    TestHandleInvalidLegacy(const AssetKey & name) : TestHandle(name) {}
};

TEST_F(StorageManagerTest, InvalidItemInCacheLegacy) {
  TestHandle invalid = Get<TestHandle>(storageManager, "invalid", false, true, false);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  // Ensure that if we get it again it is reloaded since it's invalid
  TestHandle invalid2 = Get<TestHandleInvalidLegacy>(storageManager, "invalid", false, true, false);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  ASSERT_NE(invalid.handle->val, invalid2.handle->val) << "Did not reload invalid item";
}

TEST_F(StorageManagerTest, AbortLegacy) {
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

void getAssetsForDirtyTestLegacy(StorageManager<TestItem> & storageManager, TestHandle handles[5]) {
  handles[0] = Get<TestHandle>(storageManager, "asset0", false, true, false);
  handles[1] = Get<TestHandle>(storageManager, "asset1", false, true, false);
  handles[2] = Get<TestHandle>(storageManager, "mutable2", false, true, true);
  handles[3] = Get<TestHandle>(storageManager, "asset3", false, true, false);
  handles[4] = Get<TestHandle>(storageManager, "mutable4", false, true, true);
  ASSERT_EQ(storageManager.CacheSize(), 5) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 2) << "Storage manager has wrong number of items in dirty map";
}

TEST_F(StorageManagerTest, SaveDirtyLegacy) {
  TestHandle handles[5];
  getAssetsForDirtyTestLegacy(storageManager, handles);
  
  khFilesTransaction trans;
  bool result = storageManager.SaveDirtyToDotNew(trans, nullptr);
  ASSERT_TRUE(result) << "SaveDirtyToDotNew should return true when there are no issues";
  ASSERT_EQ(storageManager.CacheSize(), 5) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has wrong number of items in dirty map";
  ASSERT_EQ(trans.NumNew(), 2) << "Wrong number of new items in file transaction";
  ASSERT_EQ(trans.NumDeleted(), 0) << "Wrong number of deleted items in file transaction";
  ASSERT_EQ(handles[0].handle->savename, string()) << "Non-dirty files should not be saved";
  ASSERT_EQ(handles[4].handle->savename, "/dev/null.new") << "Asset saved to wrong file";
}

TEST_F(StorageManagerTest, SaveDirtyToVectorLegacy) {
  TestHandle handles[5];
  getAssetsForDirtyTestLegacy(storageManager, handles);
  
  khFilesTransaction trans;
  vector<SharedString> saved;
  storageManager.SaveDirtyToDotNew(trans, &saved);
  ASSERT_EQ(saved.size(), 2) << "Wrong number of items in saved vector";
  ASSERT_TRUE(find(saved.begin(), saved.end(), "mutable2") != saved.end()) << "Dirty item missing from saved vector";
  ASSERT_TRUE(find(saved.begin(), saved.end(), "mutable4") != saved.end()) << "Dirty item missing from saved vector";
}

TEST_F(StorageManagerTest, FailedSaveLegacy) {
  TestHandle item = Get<TestHandle>(storageManager, "item", false, true, true);
  item.handle->saveSucceeds = false;
  khFilesTransaction trans;
  bool result = storageManager.SaveDirtyToDotNew(trans, nullptr);
  ASSERT_FALSE(result) << "SaveDirtyToDotNew should return false when a save fails";
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";
  ASSERT_EQ(trans.NumNew(), 0) << "Transaction should be empty after failed save";
  ASSERT_EQ(trans.NumDeleted(), 0) << "Wrong number of deleted items in file transaction";
}

// Tests using the new Get methods
TEST_F(StorageManagerTest, AddAndRetrieve) {
  int startValue = TestItem::nextValue;
  ASSERT_EQ(storageManager.CacheSize(), 0) << "Storage manager has unexpected item in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  // Create some items
  auto first = storageManager.Get("first");
  auto second = storageManager.Get("second");
  auto third = storageManager.Get("third");
  
  ASSERT_EQ(storageManager.CacheSize(), 3) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  ASSERT_EQ(first->val, startValue) << "First item has unexpected value";
  ASSERT_EQ(second->val, startValue + 1) << "Second item has unexpected value";
  ASSERT_EQ(third->val, startValue + 2) << "Third item has unexpected value";
  ASSERT_EQ(first->loadname, "first") << "Asset loaded from wrong file";

  // Retrieve one of the items
  auto second2 = storageManager.Get("second");
  ASSERT_EQ(storageManager.CacheSize(), 3) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  ASSERT_EQ(second->val, second2->val) << "Could not retrieve existing item from storage manager";
}

TEST_F(StorageManagerTest, AddNew) {
  PointerType newItem = make_shared<TestItem>();
  ASSERT_EQ(storageManager.CacheSize(), 0) << "Storage manager has unexpected item in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  storageManager.AddNew("newItem", newItem);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";
  
  auto another = storageManager.Get("another");
  ASSERT_EQ(storageManager.CacheSize(), 2) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";

  auto retrieved = storageManager.Get("newItem");
  ASSERT_EQ(newItem->val, retrieved->val) << "Could not retrieve new item from storage manager.";
}

TEST_F(StorageManagerTest, CheckFileExistence) {
  auto goodFile = storageManager.Get("good");
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  
  TestItem::fileName = "notafile"; // Try to read an invalid file
  auto badFile = storageManager.Get("bad");
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  ASSERT_FALSE(badFile) << "Should get empty handle from non-existant file";
}

TEST_F(StorageManagerTest, Mutable) {
  auto mut = storageManager.GetMutable("mutable");
  // Make sure we get a pointer to a non-const
  ASSERT_FALSE(is_const<remove_pointer<decltype(mut.operator->())>::type>());
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";

  auto another = storageManager.Get("another");
  // Make sure we get a pointer to a const
  ASSERT_TRUE(is_const<remove_pointer<decltype(another.operator->())>::type>());
  ASSERT_EQ(storageManager.CacheSize(), 2) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";
}

TEST_F(StorageManagerTest, PurgeCacheBasedOnNumberOfObjects) {
  // Put items in the cache but don't hold handles so they will be purged
  for(size_t i = 0; i < CACHE_SIZE + 2; ++i) {
    stringstream s;
    s << "asset" << i;
    storageManager.Get(s.str());
    ASSERT_EQ(storageManager.CacheSize(), min(i+1, CACHE_SIZE)) << "Unexpected number of items in cache";
    ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  }
}

TEST_F(StorageManagerTest, PurgeCacheBasedOnMemoryUtilization) {
  // Purges items from cache when the memory utilization exceeds
  // the limit and determines if the cache memory usage reflects the size
  // of the items in cache.
  size_t i;
  std::uint64_t cacheItemSize = 0;
  std::uint64_t memoryLimit = 0;
  for(i = 0; i < CACHE_SIZE + 2; ++i) {
    stringstream s;
    s << "asset" << i;
    storageManager.Get(s.str());
    if (cacheItemSize == 0) {
      cacheItemSize = storageManager.GetCacheItemSize(s.str());
      memoryLimit = cacheItemSize * (CACHE_SIZE - 1);
      storageManager.SetCacheMemoryLimit(true, memoryLimit);
    }
    ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
  }
  ASSERT_EQ(storageManager.CacheSize(), (CACHE_SIZE - 1)) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.CacheMemoryUse(), memoryLimit) << "Unexpected memory usage";
}

TEST_F(StorageManagerTest, PurgeCacheWithHandles) {
  {
    // Put items in the cache and hold handles so they won't be purged
    AssetHandle<const TestItem> handles[CACHE_SIZE+3];
    for(size_t i = 0; i < CACHE_SIZE + 3; ++i) {
      stringstream s;
      s << "asset" << i;
      handles[i] = storageManager.Get(s.str());
      ASSERT_EQ(storageManager.CacheSize(), i+1) << "Unexpected number of items in cache";
      ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
    }
  }

  // Now that we no longer have handles items can be removed from the cache.
  // Add another asset and make sure the cache is purged.
  storageManager.Get("asset999");
  ASSERT_EQ(storageManager.CacheSize(), CACHE_SIZE) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";

// Add another item and make sure the cache is purged again.
  storageManager.Get("asset998");
  ASSERT_EQ(storageManager.CacheSize(), CACHE_SIZE) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has unexpected item in dirty map";
}

TEST_F(StorageManagerTest, Abort) {
  AssetHandle<const TestItem> handles[3];
  AssetHandle<TestItem> mutHandles[2];
  handles[0] = storageManager.Get("asset0");
  handles[1] = storageManager.Get("asset1");
  mutHandles[0] = storageManager.GetMutable("mutable2");
  handles[2] = storageManager.Get("asset3");
  mutHandles[1] = storageManager.GetMutable("mutable4");
  ASSERT_EQ(storageManager.CacheSize(), 5) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 2) << "Storage manager has wrong number of items in dirty map";
  
  // Abort should remove the mutable items but nothing else
  storageManager.Abort();
  ASSERT_EQ(storageManager.CacheSize(), 3) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has wrong number of items in dirty map";
  
  // Try to reload one of the non-dirty items
  auto notdirty = storageManager.Get("asset3");
  ASSERT_EQ(handles[2]->val, notdirty->val) << "Non-dirty item was removed from cache on abort";
  
  // Try to reload one of the dirty items
  auto dirty = storageManager.GetMutable("mutable2");
  ASSERT_NE(handles[0]->val, dirty->val) << "Dirty item was not removed from cache on abort";
  ASSERT_EQ(storageManager.CacheSize(), 4) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";
}

void getAssetsForDirtyTest(
    StorageManager<TestItem> & storageManager,
    AssetHandle<const TestItem> handles[3],
    AssetHandle<TestItem> mutHandles[2]) {
  handles[0] = storageManager.Get("asset0");
  handles[1] = storageManager.Get("asset1");
  mutHandles[0] = storageManager.GetMutable("mutable2");
  handles[2] = storageManager.Get("asset3");
  mutHandles[1] = storageManager.GetMutable("mutable4");
  ASSERT_EQ(storageManager.CacheSize(), 5) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 2) << "Storage manager has wrong number of items in dirty map";
}

TEST_F(StorageManagerTest, SaveDirty) {
  AssetHandle<const TestItem> handles[3];
  AssetHandle<TestItem> mutHandles[2];
  getAssetsForDirtyTest(storageManager, handles, mutHandles);
  
  khFilesTransaction trans;
  bool result = storageManager.SaveDirtyToDotNew(trans, nullptr);
  ASSERT_TRUE(result) << "SaveDirtyToDotNew should return true when there are no issues";
  ASSERT_EQ(storageManager.CacheSize(), 5) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 0) << "Storage manager has wrong number of items in dirty map";
  ASSERT_EQ(trans.NumNew(), 2) << "Wrong number of new items in file transaction";
  ASSERT_EQ(trans.NumDeleted(), 0) << "Wrong number of deleted items in file transaction";
  ASSERT_EQ(handles[0]->savename, string()) << "Non-dirty files should not be saved";
  ASSERT_EQ(mutHandles[1]->savename, "/dev/null.new") << "Asset saved to wrong file";
}

TEST_F(StorageManagerTest, SaveDirtyToVector) {
  AssetHandle<const TestItem> handles[3];
  AssetHandle<TestItem> mutHandles[2];
  getAssetsForDirtyTest(storageManager, handles, mutHandles);
  
  khFilesTransaction trans;
  vector<SharedString> saved;
  storageManager.SaveDirtyToDotNew(trans, &saved);
  ASSERT_EQ(saved.size(), 2) << "Wrong number of items in saved vector";
  ASSERT_TRUE(find(saved.begin(), saved.end(), "mutable2") != saved.end()) << "Dirty item missing from saved vector";
  ASSERT_TRUE(find(saved.begin(), saved.end(), "mutable4") != saved.end()) << "Dirty item missing from saved vector";
}

TEST_F(StorageManagerTest, FailedSave) {
  auto item = storageManager.GetMutable("item");
  item->saveSucceeds = false;
  khFilesTransaction trans;
  bool result = storageManager.SaveDirtyToDotNew(trans, nullptr);
  ASSERT_FALSE(result) << "SaveDirtyToDotNew should return false when a save fails";
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";
  ASSERT_EQ(trans.NumNew(), 0) << "Transaction should be empty after failed save";
  ASSERT_EQ(trans.NumDeleted(), 0) << "Wrong number of deleted items in file transaction";
}

TEST_F(StorageManagerTest, InvalidRef) {
  TestItem::isValidRef = false;
  auto asset = storageManager.Get("ref");
  ASSERT_FALSE(asset) << "Invalid refs should return empty handles";
}

TEST_F(StorageManagerTest, PruneLimiting) {
  storageManager.SetPrunePercent(50);

  PointerType newItem = make_shared<TestItem>();
  storageManager.AddNew("newItem", newItem);
  ASSERT_EQ(storageManager.CacheSize(), 1) << "Storage manager has wrong number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has wrong number of items in dirty map";
  // Make sure DetermineIfPrune() returns false since the size of the dirty map is not less than or equal to half the size of the cache.
  ASSERT_EQ(storageManager.DetermineIfPrune(), false) << "Storage manager has determined wrong choice for pruning cache";
  // Add items to the cache so the number of items exceeds the limit by two.
  {
    AssetHandle<const TestItem> handles[CACHE_SIZE+1];
    for (size_t i = 0; i < CACHE_SIZE+1; ++i) {
      stringstream s;
      s << "anotherItem" << i;
      handles[i] = storageManager.Get(s.str());
      ASSERT_EQ(storageManager.CacheSize(), i+2) << "Unexpected number of items in cache";
      ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has unexpected item in dirty map";
    }
  }
  // Make sure DetermineIfPrune() returns true since the size of the dirty map is now less than half the size of the cache.
  ASSERT_EQ(storageManager.DetermineIfPrune(), true) << "Storage manager has determined wrong choice for pruning cache";

  // Add an item to the cache and make sure cache is pruned as well.
  storageManager.Get("anotherItem999");
  ASSERT_EQ(storageManager.CacheSize(), CACHE_SIZE) << "Unexpected number of items in cache";
  ASSERT_EQ(storageManager.DirtySize(), 1) << "Storage manager has unexpected item in dirty map";
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
