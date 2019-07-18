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

#include <assert.h>
#include <gtest/gtest.h>
#include <map>
#include <string>

using namespace std;

class MockVersion : public AssetVersionImpl {
  public:
    bool stateSet;

    MockVersion() : stateSet(false) {
      type = AssetDefs::Imagery;
      state = AssetDefs::New;
    }
    MockVersion(const AssetKey & ref) : MockVersion() {
      // All refs must end with "?version=X" or the calls to AssetVersionRef::Bind
      // will try to load assets from disk. For simplicity, we force everything to
      // be version 1.
      string suffix = "?version=1";
      string refStr = ref.toString();
      assert(name.empty() || equal(suffix.rbegin(), suffix.rend(), refStr.rbegin()));
      name = ref;
    }
    MockVersion(const MockVersion & that) : MockVersion(that.name) {}
    void DependentChildren(vector<SharedString> & dependents) const {}
    bool NeedComputeState() const { return true; }
    AssetDefs::State CalcStateByInputsAndChildren(
        AssetDefs::State stateByInputs,
        AssetDefs::State stateByChildren,
        bool blockersAreOffline,
        uint32 numWaitingFor) const
    { return AssetDefs::New; }
    void SetMyStateOnly(AssetDefs::State newState, bool sendNotifications) {
      stateSet = true;
    }
    
    // Not used - only included to make MockVersion non-virtual
    string PluginName(void) const { return string(); }
    void GetOutputFilenames(vector<string> &) const {}
    string GetOutputFilename(uint) const { return string(); }
};

using VersionMap = map<AssetKey, shared_ptr<MockVersion>>;

class MockStorageManager : public StorageManagerInterface<AssetVersionImpl> {
  private:
    VersionMap versions;
    PointerType GetFromMap(const AssetKey & ref) {
      auto versionIter = versions.find(ref);
      if (versionIter != versions.end()) {
        auto version = dynamic_pointer_cast<AssetVersionImpl>(versionIter->second);
        assert(version);
        return version;
      }
      assert(false);
      return nullptr;
    }
  public:
    void AddVersion(const AssetKey & key, const MockVersion & v) {
      versions[key] = make_shared<MockVersion>(v);
    }
    virtual AssetHandle<const AssetVersionImpl> Get(const AssetKey &ref) {
      return AssetHandle<const AssetVersionImpl>(GetFromMap(ref), nullptr);
    }
    virtual AssetHandle<AssetVersionImpl> GetMutable(const AssetKey &ref) {
      return AssetHandle<AssetVersionImpl>(GetFromMap(ref), nullptr);
    }
};

class StateUpdaterTest : public testing::Test {
  protected:
   MockStorageManager sm;
   StateUpdater updater;
  public:
   StateUpdaterTest() : sm(), updater(&sm) {}
};

MockVersion MakeVersion(const AssetKey & ref) {
  return MockVersion(ref);
}

void SetVersions(MockStorageManager & sm, vector<MockVersion> versions) {
  for (auto & version: versions) {
    sm.AddVersion(version.name, version);
  }
}

const MockVersion * GetVersion(MockStorageManager & sm, const AssetKey & key) {
  auto version = sm.Get(key);
  return dynamic_cast<const MockVersion*>(version.operator->());
}

TEST_F(StateUpdaterTest, SetStateSingleVersion) {
  AssetKey ref1 = "test1?version=1";
  AssetKey ref2 = "test2?version=1";
  AssetKey ref3 = "test3?version=1";
  SetVersions(sm, {MakeVersion(ref1), MakeVersion(ref2), MakeVersion(ref3)});
  updater.SetStateForRefAndDependents(ref1, AssetDefs::Bad, [](AssetDefs::State) { return true; });
  ASSERT_TRUE(GetVersion(sm, ref1)->stateSet);
  ASSERT_FALSE(GetVersion(sm, ref2)->stateSet);
  ASSERT_FALSE(GetVersion(sm, ref3)->stateSet);
  updater.SetStateForRefAndDependents(ref2, AssetDefs::Bad, [](AssetDefs::State) { return false; });
  ASSERT_FALSE(GetVersion(sm, ref2)->stateSet);
  ASSERT_FALSE(GetVersion(sm, ref3)->stateSet);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
