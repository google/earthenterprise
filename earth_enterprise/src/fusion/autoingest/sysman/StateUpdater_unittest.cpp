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

#include <algorithm>
#include <assert.h>
#include <gtest/gtest.h>
#include <map>
#include <string>

using namespace std;

// All refs must end with "?version=X" or the calls to AssetVersionRef::Bind
// will try to load assets from disk. For simplicity, we force everything to
// be version 1. When you write additional tests you have to remember to call
// this function when working directly with the state updater.
const string SUFFIX = "?version=1";
AssetKey fix(AssetKey ref) {
  return ref.toString() + SUFFIX;
}

const AssetDefs::State STARTING_STATE = AssetDefs::Blocked;
const AssetDefs::State CALCULATED_STATE = AssetDefs::InProgress;

class MockVersion : public AssetVersionImpl {
  public:
    bool stateSet;
    bool loadedMutable;
    bool notificationsSent;
    bool needComputeState;
    mutable AssetDefs::State stateByInputsVal;
    mutable AssetDefs::State stateByChildrenVal;
    mutable bool blockersAreOfflineVal;
    mutable uint32 numWaitingForVal;
    vector<AssetKey> dependents;

    MockVersion()
        : stateSet(false),
          loadedMutable(false),
          notificationsSent(false),
          needComputeState(true),
          stateByInputsVal(AssetDefs::Bad),
          stateByChildrenVal(AssetDefs::Bad),
          blockersAreOfflineVal(false),
          numWaitingForVal(-1) {
      type = AssetDefs::Imagery;
      state = STARTING_STATE;
    }
    MockVersion(const AssetKey & ref)
        : MockVersion() {
      name = fix(ref);
    }
    MockVersion(const MockVersion & that) : MockVersion() {
      name = that.name; // Don't add the suffix - the other MockVersion already did
    }
    void DependentChildren(vector<SharedString> & d) const {
      for(auto dependent : dependents) {
        d.push_back(dependent);
      }
    }
    bool NeedComputeState() const { return needComputeState; }
    AssetDefs::State CalcStateByInputsAndChildren(
        AssetDefs::State stateByInputs,
        AssetDefs::State stateByChildren,
        bool blockersAreOffline,
        uint32 numWaitingFor) const {
      stateByInputsVal = stateByInputs;
      stateByChildrenVal = stateByChildren;
      blockersAreOfflineVal = blockersAreOffline;
      numWaitingForVal = numWaitingFor;
      return CALCULATED_STATE;
    }
    void SetMyStateOnly(AssetDefs::State newState, bool sendNotifications) {
      stateSet = true;
      notificationsSent = sendNotifications;
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
      auto ptr = GetFromMap(ref);
      dynamic_pointer_cast<MockVersion>(ptr)->loadedMutable = true;
      return AssetHandle<AssetVersionImpl>(ptr, nullptr);
    }
    void ResetLoadedMutable() {
      for (auto & v : versions) {
        v.second->loadedMutable = false;
      }
    }
};

class StateUpdaterTest : public testing::Test {
  protected:
   MockStorageManager sm;
   StateUpdater updater;
  public:
   StateUpdaterTest() : sm(), updater(&sm) {}
};

void SetVersions(MockStorageManager & sm, vector<MockVersion> versions) {
  for (auto & version: versions) {
    sm.AddVersion(version.name, version);
  }
}

// The two functions below pull the pointer out of the AssetHandle and use it
// directly, which is really bad. We can get away with it in test code, but we
// should never do this in production code. Hopefully the production code will
// never need to convert AssetHandles to different types, but if it does we
// need to come up with a better way.
const MockVersion * GetVersion(MockStorageManager & sm, const AssetKey & key) {
  AssetHandle<const AssetVersionImpl> handle = sm.Get(fix(key));
  return dynamic_cast<const MockVersion *>(handle.operator->());
}
MockVersion * GetMutableVersion(MockStorageManager & sm, const AssetKey & key) {
  AssetHandle<AssetVersionImpl> handle = sm.GetMutable(fix(key));
  return dynamic_cast<MockVersion *>(handle.operator->());
}

void SetParentChild(MockStorageManager & sm, AssetKey parent, AssetKey child) {
  GetMutableVersion(sm, parent)->children.push_back(fix(child));
  GetMutableVersion(sm, child)->parents.push_back(fix(parent));
}

void SetListenerInput(MockStorageManager & sm, AssetKey listener, AssetKey input) {
  GetMutableVersion(sm, listener)->inputs.push_back(fix(input));
  GetMutableVersion(sm, input)->listeners.push_back(fix(listener));
}

void SetDependent(MockStorageManager & sm, AssetKey dependee, AssetKey dependent) {
  GetMutableVersion(sm, dependee)->dependents.push_back(fix(dependent));
}

void GetBigTree(MockStorageManager & sm) {
  SetVersions(sm,
              {
                MockVersion("gp"),
                MockVersion("p1"),
                MockVersion("p2"),
                MockVersion("gpi"),
                MockVersion("pi1"),
                MockVersion("c1"),
                MockVersion("c2"),
                MockVersion("c3"),
                MockVersion("c4"),
                MockVersion("ci1"),
                MockVersion("ci2"),
                MockVersion("ci3")
              });
  SetParentChild(sm, "gp", "p1");
  SetParentChild(sm, "gp", "p2");
  SetListenerInput(sm, "gp", "gpi");
  SetListenerInput(sm, "p1", "pi1");
  SetParentChild(sm, "p1", "c1");
  SetParentChild(sm, "p1", "c2");
  SetParentChild(sm, "p2", "c3");
  SetParentChild(sm, "p2", "c4");
  SetListenerInput(sm, "c2", "ci3");
  SetListenerInput(sm, "c4", "ci1");
  SetListenerInput(sm, "c4", "ci2");
  SetDependent(sm, "gp", "p1");
  SetDependent(sm, "gp", "c2");
  SetDependent(sm, "p1", "c1");
}

TEST_F(StateUpdaterTest, SetStateSingleVersion) {
  AssetKey ref1 = "test1";
  AssetKey ref2 = "test2";
  AssetKey ref3 = "test3";
  SetVersions(sm, {MockVersion(ref1), MockVersion(ref2), MockVersion(ref3)});
  updater.SetStateForRefAndDependents(fix(ref1), AssetDefs::Bad, [](AssetDefs::State) { return true; });
  ASSERT_TRUE(GetVersion(sm, ref1)->stateSet);
  ASSERT_FALSE(GetVersion(sm, ref2)->stateSet);
  ASSERT_FALSE(GetVersion(sm, ref3)->stateSet);
  updater.SetStateForRefAndDependents(fix(ref2), AssetDefs::Bad, [](AssetDefs::State) { return false; });
  ASSERT_FALSE(GetVersion(sm, ref2)->stateSet);
  ASSERT_FALSE(GetVersion(sm, ref3)->stateSet);
}

TEST_F(StateUpdaterTest, SetStateMultipleVersions) {
  GetBigTree(sm);
  updater.SetStateForRefAndDependents(fix("gp"), AssetDefs::Canceled, [](AssetDefs::State) {return true; });
  
  ASSERT_TRUE(GetVersion(sm, "gp")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "p1")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "c1")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "c2")->stateSet);
  
  ASSERT_FALSE(GetVersion(sm, "gpi")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "pi1")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "p2")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "c3")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "c4")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "ci1")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "ci2")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "ci3")->stateSet);

  ASSERT_FALSE(GetVersion(sm, "gp")->notificationsSent);
  ASSERT_FALSE(GetVersion(sm, "p1")->notificationsSent);
  ASSERT_FALSE(GetVersion(sm, "c1")->notificationsSent);
  ASSERT_FALSE(GetVersion(sm, "c2")->notificationsSent);
}

TEST_F(StateUpdaterTest, SetStateMultipleVersionsFromChild) {
  GetBigTree(sm);
  sm.ResetLoadedMutable();
  updater.SetStateForRefAndDependents(fix("p1"), AssetDefs::Canceled, [](AssetDefs::State) { return true; });
  
  ASSERT_TRUE(GetVersion(sm, "p1")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "c1")->stateSet);
  
  ASSERT_FALSE(GetVersion(sm, "gp")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "gpi")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "pi1")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "p2")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "c2")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "c3")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "c4")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "ci1")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "ci2")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "ci3")->stateSet);

  ASSERT_TRUE(GetVersion(sm, "p1")->loadedMutable);
  ASSERT_TRUE(GetVersion(sm, "c1")->loadedMutable);
  
  ASSERT_FALSE(GetVersion(sm, "gp")->loadedMutable);
  ASSERT_FALSE(GetVersion(sm, "gpi")->loadedMutable);
  ASSERT_FALSE(GetVersion(sm, "pi1")->loadedMutable);
  ASSERT_FALSE(GetVersion(sm, "p2")->loadedMutable);
  ASSERT_FALSE(GetVersion(sm, "c2")->loadedMutable);
  ASSERT_FALSE(GetVersion(sm, "c3")->loadedMutable);
  ASSERT_FALSE(GetVersion(sm, "c4")->loadedMutable);
  ASSERT_FALSE(GetVersion(sm, "ci1")->loadedMutable);
  ASSERT_FALSE(GetVersion(sm, "ci2")->loadedMutable);
  ASSERT_FALSE(GetVersion(sm, "ci3")->loadedMutable);
}

TEST_F(StateUpdaterTest, SetState_StateDoesAndDoesntChange) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b")});
  GetMutableVersion(sm, "a")->state = CALCULATED_STATE;
  GetMutableVersion(sm, "b")->state = STARTING_STATE;
  
  updater.SetStateForRefAndDependents(fix("a"), CALCULATED_STATE, [](AssetDefs::State) { return true; });
  ASSERT_FALSE(GetVersion(sm, "a")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "b")->stateSet);

  updater.SetStateForRefAndDependents(fix("b"), CALCULATED_STATE, [](AssetDefs::State) { return true; });
  ASSERT_FALSE(GetVersion(sm, "a")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "b")->stateSet);
}

TEST_F(StateUpdaterTest, SetStatePredicate) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b")});
  GetMutableVersion(sm, "a")->state = AssetDefs::Bad;
  GetMutableVersion(sm, "b")->state = AssetDefs::Canceled;
  SetDependent(sm, "a", "b");
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::Succeeded, [](AssetDefs::State state) {
    return state != AssetDefs::Canceled;
  });
  ASSERT_TRUE(GetVersion(sm, "a")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "b")->stateSet);
}

// All tests that call RecalculateAndSaveStates must first call a different
// function on the state updater. RecalculateAndSaveStates doesn't build the
// asset tree, it just updates the existing asset tree to handle operations
// that were performed previously. Thus, to have a correct test, some operation
// needs to be performed before calling RecalculateAndSaveStates.

TEST_F(StateUpdaterTest, NeedComputeStateFalse) {
  SetVersions(sm, {MockVersion("a")});
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::Canceled, [](AssetDefs::State) { return true; });
  GetMutableVersion(sm, "a")->needComputeState = false;
  GetMutableVersion(sm, "a")->stateSet = false;
  updater.RecalculateAndSaveStates();
  ASSERT_FALSE(GetVersion(sm, "a")->stateSet);
  GetMutableVersion(sm, "a")->needComputeState = true;
  updater.RecalculateAndSaveStates();
  ASSERT_TRUE(GetVersion(sm, "a")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "a")->notificationsSent);
}

TEST_F(StateUpdaterTest, RecalculateState_StateDoesAndDoesntChange) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b")});
  GetMutableVersion(sm, "a")->state = CALCULATED_STATE;
  GetMutableVersion(sm, "b")->state = STARTING_STATE;
  SetListenerInput(sm, "a", "b");
  GetMutableVersion(sm, "a")->loadedMutable = false;
  updater.SetStateForRefAndDependents(fix("a"), CALCULATED_STATE, [](AssetDefs::State) { return false; });

  // Verify that the setup is correct
  ASSERT_FALSE(GetVersion(sm, "a")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "b")->stateSet);
  
  updater.RecalculateAndSaveStates();
  ASSERT_FALSE(GetVersion(sm, "a")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "b")->stateSet);
  ASSERT_FALSE(GetVersion(sm, "a")->loadedMutable);
}

TEST_F(StateUpdaterTest, NoInputsNoChildren) {
  SetVersions(sm, {MockVersion("a")});
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return false; });
  updater.RecalculateAndSaveStates();
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateByInputsVal, AssetDefs::Queued);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateByChildrenVal, AssetDefs::Succeeded);
  ASSERT_EQ(GetMutableVersion(sm, "a")->numWaitingForVal, 0);
}

void OnlineInputBlockerTest(MockStorageManager & sm, StateUpdater & updater, AssetDefs::State inputState) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d")});
  SetListenerInput(sm, "a", "b");
  SetListenerInput(sm, "a", "c");
  SetListenerInput(sm, "a", "d");
  GetMutableVersion(sm, "b")->state = inputState;
  GetMutableVersion(sm, "c")->state = AssetDefs::InProgress;
  GetMutableVersion(sm, "d")->state = AssetDefs::Succeeded;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return false; });
  updater.RecalculateAndSaveStates();
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateByInputsVal, AssetDefs::Blocked);
  ASSERT_EQ(GetMutableVersion(sm, "a")->blockersAreOfflineVal, false);
  ASSERT_EQ(GetMutableVersion(sm, "a")->numWaitingForVal, 0);
}

TEST_F(StateUpdaterTest, BlockedInputBlocker) {
  OnlineInputBlockerTest(sm, updater, AssetDefs::Blocked);
}

TEST_F(StateUpdaterTest, FailedInputBlocker) {
  OnlineInputBlockerTest(sm, updater, AssetDefs::Failed);
}

TEST_F(StateUpdaterTest, CanceledInputBlocker) {
  OnlineInputBlockerTest(sm, updater, AssetDefs::Canceled);
}

TEST_F(StateUpdaterTest, BadInputBlocker) {
  OnlineInputBlockerTest(sm, updater, AssetDefs::Bad);
}

TEST_F(StateUpdaterTest, OfflineInputBlocker) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b")});
  SetListenerInput(sm, "a", "b");
  GetMutableVersion(sm, "b")->state = AssetDefs::Offline;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return false; });
  updater.RecalculateAndSaveStates();
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateByInputsVal, AssetDefs::Blocked);
  ASSERT_EQ(GetMutableVersion(sm, "a")->blockersAreOfflineVal, true);
  ASSERT_EQ(GetMutableVersion(sm, "a")->numWaitingForVal, 0);
}

TEST_F(StateUpdaterTest, BlockedAndOfflineInput) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c")});
  SetListenerInput(sm, "a", "b");
  SetListenerInput(sm, "a", "c");
  GetMutableVersion(sm, "b")->state = AssetDefs::Offline;
  GetMutableVersion(sm, "c")->state = AssetDefs::Failed;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return false; });
  updater.RecalculateAndSaveStates();
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateByInputsVal, AssetDefs::Blocked);
  ASSERT_EQ(GetMutableVersion(sm, "a")->blockersAreOfflineVal, false);
  ASSERT_EQ(GetMutableVersion(sm, "a")->numWaitingForVal, 0);
}

TEST_F(StateUpdaterTest, WaitingOnInput) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d")});
  SetListenerInput(sm, "a", "b");
  SetListenerInput(sm, "a", "c");
  SetListenerInput(sm, "a", "d");
  GetMutableVersion(sm, "b")->state = AssetDefs::Queued;
  GetMutableVersion(sm, "c")->state = AssetDefs::InProgress;
  GetMutableVersion(sm, "d")->state = AssetDefs::Succeeded;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return false; });
  updater.RecalculateAndSaveStates();
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateByInputsVal, AssetDefs::Waiting);
  ASSERT_EQ(GetMutableVersion(sm, "a")->numWaitingForVal, 2);
}

TEST_F(StateUpdaterTest, SucceededInputs) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d")});
  SetListenerInput(sm, "a", "b");
  SetListenerInput(sm, "a", "c");
  SetListenerInput(sm, "a", "d");
  GetMutableVersion(sm, "b")->state = AssetDefs::Succeeded;
  GetMutableVersion(sm, "c")->state = AssetDefs::Succeeded;
  GetMutableVersion(sm, "d")->state = AssetDefs::Succeeded;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return false; });
  updater.RecalculateAndSaveStates();
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateByInputsVal, AssetDefs::Queued);
  ASSERT_EQ(GetMutableVersion(sm, "a")->numWaitingForVal, 0);
}

void OnlineChildBlockerTest(MockStorageManager & sm, StateUpdater & updater, AssetDefs::State inputState) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d")});
  SetParentChild(sm, "a", "b");
  SetParentChild(sm, "a", "c");
  SetParentChild(sm, "a", "d");
  GetMutableVersion(sm, "b")->state = inputState;
  GetMutableVersion(sm, "c")->state = AssetDefs::InProgress;
  GetMutableVersion(sm, "d")->state = AssetDefs::Succeeded;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return false; });
  updater.RecalculateAndSaveStates();
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateByChildrenVal, AssetDefs::Blocked);
}

TEST_F(StateUpdaterTest, FailedChildBlocker) {
  OnlineChildBlockerTest(sm, updater, AssetDefs::Failed);
}

TEST_F(StateUpdaterTest, BlockedChildBlocker) {
  OnlineChildBlockerTest(sm, updater, AssetDefs::Blocked);
}

TEST_F(StateUpdaterTest, CanceledChildBlocker) {
  OnlineChildBlockerTest(sm, updater, AssetDefs::Canceled);
}

TEST_F(StateUpdaterTest, OfflineChildBlocker) {
  OnlineChildBlockerTest(sm, updater, AssetDefs::Offline);
}

TEST_F(StateUpdaterTest, BadChildBlocker) {
  OnlineChildBlockerTest(sm, updater, AssetDefs::Bad);
}

TEST_F(StateUpdaterTest, ChildInProgress) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d")});
  SetParentChild(sm, "a", "b");
  SetParentChild(sm, "a", "c");
  SetParentChild(sm, "a", "d");
  GetMutableVersion(sm, "b")->state = AssetDefs::Queued;
  GetMutableVersion(sm, "c")->state = AssetDefs::InProgress;
  GetMutableVersion(sm, "d")->state = AssetDefs::Succeeded;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return false; });
  updater.RecalculateAndSaveStates();
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateByChildrenVal, AssetDefs::InProgress);
}

TEST_F(StateUpdaterTest, ChildQueued) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d")});
  SetParentChild(sm, "a", "b");
  SetParentChild(sm, "a", "c");
  SetParentChild(sm, "a", "d");
  GetMutableVersion(sm, "b")->state = AssetDefs::Queued;
  GetMutableVersion(sm, "c")->state = AssetDefs::Queued;
  GetMutableVersion(sm, "d")->state = AssetDefs::Queued;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return false; });
  updater.RecalculateAndSaveStates();
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateByChildrenVal, AssetDefs::Queued);
}

TEST_F(StateUpdaterTest, SucceededChildren) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d")});
  SetParentChild(sm, "a", "b");
  SetParentChild(sm, "a", "c");
  SetParentChild(sm, "a", "d");
  GetMutableVersion(sm, "b")->state = AssetDefs::Succeeded;
  GetMutableVersion(sm, "c")->state = AssetDefs::Succeeded;
  GetMutableVersion(sm, "d")->state = AssetDefs::Succeeded;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return false; });
  updater.RecalculateAndSaveStates();
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateByChildrenVal, AssetDefs::Succeeded);
}

TEST_F(StateUpdaterTest, ChildrenAndDependents) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d")});
  SetParentChild(sm, "a", "b");
  SetParentChild(sm, "a", "c");
  SetDependent(sm, "a", "b");
  SetDependent(sm, "a", "d");
  // These states are carefully chosen. b is a dependent and a child, and d is
  // a dependent but not a child. If b is not recognized as a child, the state
  // will be Queued. If d is treated as a child, the state will be blocked.
  // The state will only be correct if both are handled correctly.
  GetMutableVersion(sm, "b")->state = AssetDefs::Succeeded;
  GetMutableVersion(sm, "c")->state = AssetDefs::Queued;
  GetMutableVersion(sm, "d")->state = AssetDefs::Failed;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return false; });
  updater.RecalculateAndSaveStates();
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateByChildrenVal, AssetDefs::InProgress);
}

TEST_F(StateUpdaterTest, ChildrenAndInputs) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d"),
                   MockVersion("e"), MockVersion("f"), MockVersion("g")});
  SetParentChild(sm, "a", "b");
  SetParentChild(sm, "a", "c");
  SetParentChild(sm, "a", "d");
  SetListenerInput(sm, "a", "e");
  SetListenerInput(sm, "a", "f");
  SetListenerInput(sm, "a", "g");
  GetMutableVersion(sm, "b")->state = AssetDefs::Queued;
  GetMutableVersion(sm, "c")->state = AssetDefs::InProgress;
  GetMutableVersion(sm, "d")->state = AssetDefs::Succeeded;
  GetMutableVersion(sm, "e")->state = AssetDefs::Queued;
  GetMutableVersion(sm, "f")->state = AssetDefs::InProgress;
  GetMutableVersion(sm, "g")->state = AssetDefs::Succeeded;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return false; });
  updater.RecalculateAndSaveStates();
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateByChildrenVal, AssetDefs::InProgress);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateByInputsVal, AssetDefs::Waiting);
  ASSERT_EQ(GetMutableVersion(sm, "a")->numWaitingForVal, 2);
}

TEST_F(StateUpdaterTest, RecalculateMultipleStates) {
  GetBigTree(sm);
  updater.SetStateForRefAndDependents(fix("gp"), AssetDefs::New, [](AssetDefs::State) { return false; });
  updater.RecalculateAndSaveStates();
  ASSERT_TRUE(GetVersion(sm, "gp")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "gpi")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "p1")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "pi1")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "c1")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "p2")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "c2")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "c3")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "c4")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "ci1")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "ci2")->stateSet);
  ASSERT_TRUE(GetVersion(sm, "ci3")->stateSet);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
