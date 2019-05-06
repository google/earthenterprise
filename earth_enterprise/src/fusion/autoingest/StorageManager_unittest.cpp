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

class TestItem : public khRefCounter, public StorageManaged {
 public:
  TestItem() : val(nextValue++) {}
  const int val;
  static khRefGuard<TestItem> Load(std::string) { return khRefGuardFromNew<TestItem>(new TestItem()); }
 private:
  static int nextValue;
};
int TestItem::nextValue = 1;
template<> const bool StorageManager<TestItem>::check_timestamps = false;

class StorageManagerTest : public testing::Test {
 protected:
  StorageManager<TestItem> storageManager;
 public:
  StorageManagerTest() : storageManager(10, "test") {}
};

//TEST_F(StorageManagerTest, GetNew) {
//  ASSERT_EQ(storageManager.CacheSize(), 0) << "Storage manager has unexpected item in cache";
//  khRefGuard<TestItem> item = storageManager.Get("Item1", "/dev/null", false, true, false);
//}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
