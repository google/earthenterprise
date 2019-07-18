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

#include "AssetVersion.h"
#include "gee_version.h"
#include "StateUpdater.h"

#include <gtest/gtest.h>
#include <string>

using namespace std;

class MockVersion : public AssetVersionImpl {
  private:
    bool needComputeState;
  public:
    MockVersion(bool computeState) : needComputeState(computeState) {}
    void DependentChildren(vector<SharedString> & dependents) const {}
    bool NeedComputeState() const { return needComputeState; }
    AssetDefs::State CalcStateByInputsAndChildren(
        AssetDefs::State stateByInputs,
        AssetDefs::State stateByChildren,
        bool blockersAreOffline,
        uint32 numWaitingFor) const
    { return AssetDefs::New; }
    void SetMyStateOnly(AssetDefs::State newState, bool sendNotifications) {}
    
    // Not used - only included to make MockVersion non-virtual
    string PluginName(void) const { return string(); }
    void GetOutputFilenames(vector<string> &) const {}
    string GetOutputFilename(uint) const { return string(); }
};

class MockStorageManager : public StorageManagerInterface<AssetVersionImpl> {
  private:
    AssetPointerType<AssetVersionImpl> Get() {
      auto mockPtr = make_shared<MockVersion>(false);
      return dynamic_pointer_cast<AssetVersionImpl>(mockPtr);
    }
  public:
    virtual AssetHandle<const AssetVersionImpl> Get(const AssetKey &) { return AssetHandle<const AssetVersionImpl>(Get(), nullptr); }
    virtual AssetHandle<AssetVersionImpl> GetMutable(const AssetKey &) { return AssetHandle<AssetVersionImpl>(Get(), nullptr); }
};

class StateUpdaterTest : public testing::Test {
  protected:
   MockStorageManager storageManager;
   StateUpdater updater;
  public:
   StateUpdaterTest()
     : storageManager(),
       updater(&storageManager) {}
};

TEST_F(StateUpdaterTest, NullTest) {
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
