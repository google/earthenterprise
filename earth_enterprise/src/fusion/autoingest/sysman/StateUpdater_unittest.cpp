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

#include "AssetVersionD.h"
#include "gee_version.h"
#include "StateUpdater.h"

#include <algorithm>
#include <assert.h>
#include <gtest/gtest.h>
#include <map>
#include <string>

using namespace std;

// These tests cover both StateUpdater.cpp/.h and DependentStateTree.cpp/.h.

// All refs must end with "?version=X" or the calls to AssetVersionRef::Bind
// will try to load assets from disk. For simplicity, we force everything to
// be version 1. When you write additional tests you have to remember to call
// this function when working directly with the state updater.
const string SUFFIX = "?version=1";
AssetKey fix(AssetKey ref) {
  return ref.toString() + SUFFIX;
}
AssetKey unfix(AssetKey ref) {
  string str = ref.toString();
  return str.erase(ref.toString().size() - SUFFIX.size());
}

const AssetDefs::State STARTING_STATE = AssetDefs::Blocked;
const AssetDefs::State CALCULATED_STATE = AssetDefs::InProgress;

enum OnStateChangeBehavior {
  NO_ERRORS,
  STATE_CHANGE_EXCEPTION,
  STD_EXCEPTION,
  UNKNOWN_EXCEPTION,
  CHANGE_NUM_CHILDREN,
  RETURN_NEW_STATE
};

class MockVersion : public AssetVersionImpl {
  public:
    bool loadedMutable;
    int onStateChangeCalled;
    int notificationsSent;
    mutable bool fatalLogFileWritten;
    mutable InputAndChildStateData stateData;
    mutable bool stateRecalced;
    OnStateChangeBehavior stateChangeBehavior;
    bool recalcStateReturnVal;
    vector<AssetKey> dependents;

    MockVersion()
        : loadedMutable(false),
          onStateChangeCalled(0),
          notificationsSent(0),
          fatalLogFileWritten(false),
          // Default the num*WaitingFor values to 999 instead of 0 to catch bugs where
          // they are never set.
          stateData({AssetDefs::New, AssetDefs::New, false, 999, 999}),
          stateRecalced(false),
          stateChangeBehavior(NO_ERRORS),
          recalcStateReturnVal(true) {
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
    virtual void DependentChildren(vector<SharedString> & d) const override {
      for(auto dependent : dependents) {
        d.push_back(dependent);
      }
    }
    virtual AssetDefs::State CalcStateByInputsAndChildren(const InputAndChildStateData & stateData) const override {
      this->stateData = stateData;
      return CALCULATED_STATE;
    }
    virtual AssetDefs::State OnStateChange(AssetDefs::State, AssetDefs::State) override {
      ++onStateChangeCalled;
      switch (stateChangeBehavior) {
        case STATE_CHANGE_EXCEPTION:
          throw StateChangeException("Custom state change error", "test code");
        case STD_EXCEPTION:
          throw std::exception();
        case UNKNOWN_EXCEPTION:
          throw "Unknown exception";
        case CHANGE_NUM_CHILDREN:
          children.push_back(fix("b"));
          break;
        case RETURN_NEW_STATE:
          // Return different states on two iterations
          if (state == CALCULATED_STATE) return AssetDefs::Waiting;
          else if (state == AssetDefs::Waiting) return AssetDefs::Queued;
        case NO_ERRORS:
          break;
      }
      return state;
    }
    virtual void WriteFatalLogfile(const std::string &, const std::string &) const throw() override {
      fatalLogFileWritten = true;
    }
    virtual bool RecalcState() const override {
      stateRecalced = true;
      return recalcStateReturnVal;
    }

    // Not used - only included to make MockVersion non-virtual
    string PluginName(void) const override { return string(); }
    void GetOutputFilenames(vector<string> &) const override {}
    string GetOutputFilename(uint) const override { return string(); }
};

class MockStorageManager : public StorageManagerInterface<AssetVersionImpl> {
  private:
    using VersionMap = map<AssetKey, shared_ptr<MockVersion>>;
    VersionMap versions;
    PointerType GetFromMap(const AssetKey & ref) {
      auto versionIter = versions.find(ref);
      if (versionIter != versions.end()) {
        auto version = dynamic_pointer_cast<AssetVersionImpl>(versionIter->second);
        assert(version);
        return version;
      }
      cout << "Could not find asset version " << ref << endl;
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
};

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
  // This doesn't count as loading the asset mutable. We only care if the
  // state udpater loads it mutable. Thus, we first get it non-mutable so we
  // can record whether it's been loaded mutable and then set it back to
  // whatever it was at the end of this function.
  bool wasLoadedMutable = GetVersion(sm, key)->loadedMutable;
  AssetHandle<AssetVersionImpl> handle = sm.GetMutable(fix(key));
  auto ptr = dynamic_cast<MockVersion *>(handle.operator->());
  ptr->loadedMutable = wasLoadedMutable;
  return ptr;
}

class MockAssetManager : public khAssetManagerInterface {
  MockStorageManager * const sm;
  public:
    MockAssetManager(MockStorageManager * sm) : sm(sm) {}
    virtual void NotifyVersionStateChange(const SharedString &ref,
                                          AssetDefs::State state) {
      ++GetMutableVersion(*sm, unfix(ref))->notificationsSent;
    }
};

class StateUpdaterTest : public testing::Test {
  protected:
   MockStorageManager sm;
   MockAssetManager am;
   StateUpdater updater;
  public:
   StateUpdaterTest() : sm(), am(&sm), updater(&sm, &am) {}
};

void SetVersions(MockStorageManager & sm, vector<MockVersion> versions) {
  for (auto & version: versions) {
    sm.AddVersion(version.name, version);
  }
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

void assertStateSet(MockStorageManager & sm, const SharedString & ref, int stateChanges = 1) {
  ASSERT_TRUE(GetVersion(sm, ref)->loadedMutable) << ref << " was not loaded mutable";
  ASSERT_EQ(GetVersion(sm, ref)->onStateChangeCalled, stateChanges) << "OnStateChange was not called enough times for " << ref;
  ASSERT_EQ(GetVersion(sm, ref)->notificationsSent, 1) << "Wrong number of notifications sent for " << ref;
}

void assertStateNotSet(MockStorageManager & sm, const SharedString & ref) {
  ASSERT_FALSE(GetVersion(sm, ref)->loadedMutable) << ref << " was unexpectedly loaded mutable";
  ASSERT_EQ(GetVersion(sm, ref)->onStateChangeCalled, 0) << "OnStateChange was unexpectedly called for " << ref;
  ASSERT_EQ(GetVersion(sm, ref)->notificationsSent, 0) << "Notifications unexpectedly sent for " << ref;
  ASSERT_FALSE(GetVersion(sm, ref)->fatalLogFileWritten) << "Fatal log file unexpectedly written for " << ref;
}

TEST_F(StateUpdaterTest, SetStateSingleVersion) {
  AssetKey ref1 = "test1";
  AssetKey ref2 = "test2";
  SetVersions(sm, {MockVersion(ref1), MockVersion(ref2)});
  updater.SetStateForRefAndDependents(fix(ref1), AssetDefs::New, [](AssetDefs::State) { return true; });
  assertStateSet(sm, ref1);
  assertStateNotSet(sm, ref2);
}

TEST_F(StateUpdaterTest, SetStateMultipleVersions) {
  GetBigTree(sm);
  updater.SetStateForRefAndDependents(fix("gp"), AssetDefs::New, [](AssetDefs::State) {return true; });
  
  assertStateSet(sm, "gp");
  assertStateSet(sm, "p1");
  assertStateSet(sm, "c1");
  assertStateSet(sm, "c2");
  
  assertStateNotSet(sm, "gpi");
  assertStateNotSet(sm, "pi1");
  assertStateNotSet(sm, "p2");
  assertStateNotSet(sm, "c3");
  assertStateNotSet(sm, "c4");
  assertStateNotSet(sm, "ci1");
  assertStateNotSet(sm, "ci2");
  assertStateNotSet(sm, "ci3");
}

TEST_F(StateUpdaterTest, SetStateMultipleVersionsFromChild) {
  GetBigTree(sm);
  updater.SetStateForRefAndDependents(fix("p1"), AssetDefs::New, [](AssetDefs::State) { return true; });
  
  assertStateSet(sm, "p1");
  assertStateSet(sm, "c1");
  assertStateSet(sm, "gp");

  assertStateNotSet(sm, "gpi");
  assertStateNotSet(sm, "pi1");
  assertStateNotSet(sm, "p2");
  assertStateNotSet(sm, "c2");
  assertStateNotSet(sm, "c3");
  assertStateNotSet(sm, "c4");
  assertStateNotSet(sm, "ci1");
  assertStateNotSet(sm, "ci2");
  assertStateNotSet(sm, "ci3");
}

TEST_F(StateUpdaterTest, StateDoesntChange) {
  SetVersions(sm, {MockVersion("a")});
  GetMutableVersion(sm, "a")->state = CALCULATED_STATE;
  updater.SetStateForRefAndDependents(fix("a"), CALCULATED_STATE, [](AssetDefs::State) { return true; });
  assertStateNotSet(sm, "a");
}

// These tests are set up to make it through all the if statements to verify
// that we don't recompute state for the states tested. We make a's state the
// same as the passed in state so it skips over the first state setting, but b
// (the dependent) has its state changed. Thus, a's state would normally be
// recalculated (because its dependent's state changed) except in cases where
// a's state is one that has to be cleared manually. We also include one case
// where we do compute the state to verify that the tests are set up correctly.
void testNeedComputeState(MockStorageManager & sm, StateUpdater & updater, AssetDefs::State state) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b")});
  SetParentChild(sm, "a", "b");
  SetDependent(sm, "a", "b");
  GetMutableVersion(sm, "a")->state = state;
  updater.SetStateForRefAndDependents(fix("a"), state, [](AssetDefs::State) { return true; });
}

TEST_F(StateUpdaterTest, DontComputeStateBad) {
  testNeedComputeState(sm, updater, AssetDefs::Bad);
  assertStateNotSet(sm, "a");
}

TEST_F(StateUpdaterTest, DontComputeStateOffline) {
  testNeedComputeState(sm, updater, AssetDefs::Offline);
  assertStateNotSet(sm, "a");
}

TEST_F(StateUpdaterTest, DontComputeStateCanceled) {
  testNeedComputeState(sm, updater, AssetDefs::Canceled);
  assertStateNotSet(sm, "a");
}

TEST_F(StateUpdaterTest, ComputeState) {
  testNeedComputeState(sm, updater, AssetDefs::Queued);
  assertStateSet(sm, "a");
}

TEST_F(StateUpdaterTest, NoInputsNoChildren) {
  SetVersions(sm, {MockVersion("a")});
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return true; });
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.stateByInputs, AssetDefs::Queued);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.stateByChildren, AssetDefs::Succeeded);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numInputsWaitingFor, 0);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numChildrenWaitingFor, 0);
}

void OnlineInputBlockerTest(MockStorageManager & sm, StateUpdater & updater, AssetDefs::State inputState) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d")});
  SetListenerInput(sm, "a", "b");
  SetListenerInput(sm, "a", "c");
  SetListenerInput(sm, "a", "d");
  GetMutableVersion(sm, "b")->state = inputState;
  GetMutableVersion(sm, "c")->state = AssetDefs::InProgress;
  GetMutableVersion(sm, "d")->state = AssetDefs::Succeeded;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return true; });
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.stateByInputs, AssetDefs::Blocked);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.blockersAreOffline, false);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numInputsWaitingFor, 0);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numChildrenWaitingFor, 0);
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
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return true; });
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.stateByInputs, AssetDefs::Blocked);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.blockersAreOffline, true);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numInputsWaitingFor, 0);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numChildrenWaitingFor, 0);
}

TEST_F(StateUpdaterTest, BlockedAndOfflineInput) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c")});
  SetListenerInput(sm, "a", "b");
  SetListenerInput(sm, "a", "c");
  GetMutableVersion(sm, "b")->state = AssetDefs::Offline;
  GetMutableVersion(sm, "c")->state = AssetDefs::Failed;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return true; });
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.stateByInputs, AssetDefs::Blocked);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.blockersAreOffline, false);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numInputsWaitingFor, 0);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numChildrenWaitingFor, 0);
}

TEST_F(StateUpdaterTest, WaitingOnInput) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d")});
  SetListenerInput(sm, "a", "b");
  SetListenerInput(sm, "a", "c");
  SetListenerInput(sm, "a", "d");
  GetMutableVersion(sm, "b")->state = AssetDefs::Queued;
  GetMutableVersion(sm, "c")->state = AssetDefs::InProgress;
  GetMutableVersion(sm, "d")->state = AssetDefs::Succeeded;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return true; });
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.stateByInputs, AssetDefs::Waiting);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numInputsWaitingFor, 2);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numChildrenWaitingFor, 0);
}

TEST_F(StateUpdaterTest, SucceededInputs) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d")});
  SetListenerInput(sm, "a", "b");
  SetListenerInput(sm, "a", "c");
  SetListenerInput(sm, "a", "d");
  GetMutableVersion(sm, "b")->state = AssetDefs::Succeeded;
  GetMutableVersion(sm, "c")->state = AssetDefs::Succeeded;
  GetMutableVersion(sm, "d")->state = AssetDefs::Succeeded;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return true; });
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.stateByInputs, AssetDefs::Queued);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numInputsWaitingFor, 0);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numChildrenWaitingFor, 0);
}

void ChildBlockerTest(MockStorageManager & sm, StateUpdater & updater, AssetDefs::State inputState) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d")});
  SetParentChild(sm, "a", "b");
  SetParentChild(sm, "a", "c");
  SetParentChild(sm, "a", "d");
  GetMutableVersion(sm, "b")->state = inputState;
  GetMutableVersion(sm, "c")->state = AssetDefs::InProgress;
  GetMutableVersion(sm, "d")->state = AssetDefs::Succeeded;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return true; });
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.stateByChildren, AssetDefs::Blocked);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numChildrenWaitingFor, 0);
}

TEST_F(StateUpdaterTest, FailedChildBlocker) {
  ChildBlockerTest(sm, updater, AssetDefs::Failed);
}

TEST_F(StateUpdaterTest, BlockedChildBlocker) {
  ChildBlockerTest(sm, updater, AssetDefs::Blocked);
}

TEST_F(StateUpdaterTest, CanceledChildBlocker) {
  ChildBlockerTest(sm, updater, AssetDefs::Canceled);
}

TEST_F(StateUpdaterTest, OfflineChildBlocker) {
  ChildBlockerTest(sm, updater, AssetDefs::Offline);
}

TEST_F(StateUpdaterTest, BadChildBlocker) {
  ChildBlockerTest(sm, updater, AssetDefs::Bad);
}

TEST_F(StateUpdaterTest, ChildInProgress) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d")});
  SetParentChild(sm, "a", "b");
  SetParentChild(sm, "a", "c");
  SetParentChild(sm, "a", "d");
  GetMutableVersion(sm, "b")->state = AssetDefs::Queued;
  GetMutableVersion(sm, "c")->state = AssetDefs::InProgress;
  GetMutableVersion(sm, "d")->state = AssetDefs::Succeeded;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return true; });
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.stateByChildren, AssetDefs::InProgress);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numChildrenWaitingFor, 2);
}

TEST_F(StateUpdaterTest, ChildQueued) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d")});
  SetParentChild(sm, "a", "b");
  SetParentChild(sm, "a", "c");
  SetParentChild(sm, "a", "d");
  GetMutableVersion(sm, "b")->state = AssetDefs::Queued;
  GetMutableVersion(sm, "c")->state = AssetDefs::Queued;
  GetMutableVersion(sm, "d")->state = AssetDefs::Queued;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return true; });
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.stateByChildren, AssetDefs::Queued);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numChildrenWaitingFor, 0);
}

TEST_F(StateUpdaterTest, SucceededChildren) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"), MockVersion("d")});
  SetParentChild(sm, "a", "b");
  SetParentChild(sm, "a", "c");
  SetParentChild(sm, "a", "d");
  GetMutableVersion(sm, "b")->state = AssetDefs::Succeeded;
  GetMutableVersion(sm, "c")->state = AssetDefs::Succeeded;
  GetMutableVersion(sm, "d")->state = AssetDefs::Succeeded;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return true; });
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.stateByChildren, AssetDefs::Succeeded);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numChildrenWaitingFor, 0);
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
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State state) {
    // Don't change the state of any children, just recalculate the parent's state
    return !(state == AssetDefs::Succeeded || state == AssetDefs::Queued || state == AssetDefs::Failed);
  });
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.stateByChildren, AssetDefs::InProgress);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numChildrenWaitingFor, 1);
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
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return true; });
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.stateByChildren, AssetDefs::InProgress);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.stateByInputs, AssetDefs::Waiting);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numInputsWaitingFor, 2);
  ASSERT_EQ(GetMutableVersion(sm, "a")->stateData.numChildrenWaitingFor, 2);
}

// This tests a tricky corner case. d and e are inputs/children of a, but they
// are also dependent's of a's dependent's. Thus, d and e don't seem to be in
// the dependent tree from a's perspective, even though they actually are. This
// test ensures that the code handles that case.
TEST_F(StateUpdaterTest, ChildDepIsInput) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"),
                   MockVersion("d"), MockVersion("e")});
  SetDependent(sm, "a", "b");
  SetDependent(sm, "b", "c");
  SetDependent(sm, "c", "d");
  SetDependent(sm, "c", "e");
  SetListenerInput(sm, "a", "d");
  SetParentChild(sm, "a", "e");
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return true; });
  assertStateSet(sm, "a");
  assertStateSet(sm, "b");
  assertStateSet(sm, "c");
  assertStateSet(sm, "d");
  assertStateSet(sm, "e");
}

// This tests another corner case where a's input/child is the parent/listener
// of another asset version in the dependency tree. d and e need to be updated
// but it doesn't look that way from a's perspective.
TEST_F(StateUpdaterTest, InputIsParent) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"),
                   MockVersion("d"), MockVersion("e")});
  SetDependent(sm, "a", "b");
  SetDependent(sm, "b", "c");
  SetListenerInput(sm, "a", "d");
  SetParentChild(sm, "a", "e");
  SetParentChild(sm, "d", "c");
  SetListenerInput(sm, "e", "c");
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State) { return true; });
  assertStateSet(sm, "a");
  assertStateSet(sm, "b");
  assertStateSet(sm, "c");
  assertStateSet(sm, "d");
  assertStateSet(sm, "e");
}

// This test creates a long chain of parents and dependents and makes sure
// state propagation stops at appropriate points.
TEST_F(StateUpdaterTest, MultipleLevelsParentsDependents) {
  const AssetDefs::State NO_CHANGE_STATE = AssetDefs::Canceled;
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c"),
                   MockVersion("d"), MockVersion("e"), MockVersion("f"),
                   MockVersion("g"), MockVersion("h"), MockVersion("i")});
  SetParentChild(sm, "a", "b");
  SetParentChild(sm, "b", "c");
  SetParentChild(sm, "c", "d");
  SetParentChild(sm, "d", "e");
  SetDependent(sm, "e", "f");
  SetDependent(sm, "f", "g");
  SetDependent(sm, "g", "h");
  SetDependent(sm, "h", "i");
  GetMutableVersion(sm, "b")->state = CALCULATED_STATE;
  GetMutableVersion(sm, "h")->state = NO_CHANGE_STATE;
  updater.SetStateForRefAndDependents(fix("e"), AssetDefs::New, [](AssetDefs::State state) {
    return state != NO_CHANGE_STATE;
  });
  assertStateNotSet(sm, "a");
  assertStateNotSet(sm, "b");
  assertStateSet(sm, "c");
  assertStateSet(sm, "d");
  assertStateSet(sm, "e");
  assertStateSet(sm, "f");
  assertStateSet(sm, "g");
  assertStateNotSet(sm, "h");
  assertStateNotSet(sm, "i");
}

// Verify that an asset's state doesn't change if its non-child dependent's
// state changes.
TEST_F(StateUpdaterTest, NonChildDependent) {
  const AssetDefs::State NO_CHANGE_STATE = AssetDefs::Waiting;
  SetVersions(sm, {MockVersion("a"), MockVersion("b")});
  SetDependent(sm, "a", "b");
  GetMutableVersion(sm, "a")->state = NO_CHANGE_STATE;
  updater.SetStateForRefAndDependents(fix("a"), NO_CHANGE_STATE, [](AssetDefs::State) { return true; });
  assertStateNotSet(sm, "a");
  assertStateSet(sm, "b");
}

void StateChangeErrorTest(MockStorageManager & sm, StateUpdater & updater, OnStateChangeBehavior behavior) {
  SetVersions(sm, {MockVersion("a")});
  GetMutableVersion(sm, "a")->stateChangeBehavior = behavior;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State state) { return true; });
  // We will call OnStateChange twice - once after setting it to the requested
  // state and again after setting it to Failed.
  assertStateSet(sm, "a", 2);
  ASSERT_EQ(GetVersion(sm, "a")->state, AssetDefs::Failed);
}

TEST_F(StateUpdaterTest, StateChangeExceptionTest) {
  StateChangeErrorTest(sm, updater, STATE_CHANGE_EXCEPTION);
  ASSERT_TRUE(GetVersion(sm, "a")->fatalLogFileWritten);
}

TEST_F(StateUpdaterTest, StdExceptionTest) {
  StateChangeErrorTest(sm, updater, STD_EXCEPTION);
}

TEST_F(StateUpdaterTest, UnknownExceptionTest) {
  StateChangeErrorTest(sm, updater, UNKNOWN_EXCEPTION);
}

// If an asset calls DelayedBuildChildren as part of a state change the code
// should detect that and revert to legacy state propagation. This will change
// in the future as we handle more cases in the state updater.
TEST_F(StateUpdaterTest, DelayedBuildChildrenTest) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b")});
  GetMutableVersion(sm, "a")->stateChangeBehavior = CHANGE_NUM_CHILDREN;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State state) { return true; });
  auto * version = GetMutableVersion(sm, "a");
  // Make sure OnStateChange behaved as expected
  ASSERT_NE(find(version->children.begin(), version->children.end(), fix("b")), version->children.end());
  // We should get partway through setting the state of "a" (load it mutable
  // and call OnStateChange but not send notifications).
  ASSERT_TRUE(GetVersion(sm, "a")->loadedMutable);
  ASSERT_EQ(GetVersion(sm, "a")->onStateChangeCalled, 1);
  ASSERT_EQ(GetVersion(sm, "a")->notificationsSent, 0);
  assertStateNotSet(sm, "b");
}

TEST_F(StateUpdaterTest, OnStateChangeReturnsNewState) {
  SetVersions(sm, {MockVersion("a")});
  GetMutableVersion(sm, "a")->stateChangeBehavior = RETURN_NEW_STATE;
  updater.SetStateForRefAndDependents(fix("a"), AssetDefs::New, [](AssetDefs::State state) { return true; });
  assertStateSet(sm, "a", 3);
}

void SetInProgress(MockStorageManager & sm, StateUpdater & updater, AssetKey ref) {
  auto version = sm.GetMutable(fix(ref));
  updater.SetInProgress(version);
  ASSERT_EQ(GetVersion(sm, ref)->state, AssetDefs::InProgress);
  if (GetVersion(sm, ref)->stateChangeBehavior != CHANGE_NUM_CHILDREN) {
    assertStateSet(sm, ref);
  }
}

TEST_F(StateUpdaterTest, BasicSetInProgress) {
  SetVersions(sm, {MockVersion("a")});
  SetInProgress(sm, updater, "a");
}

TEST_F(StateUpdaterTest, SetInProgressNoWaiting) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c")});
  SetParentChild(sm, "a", "c");
  SetListenerInput(sm, "b", "c");
  SetInProgress(sm, updater, "c");
  // Since nothing is marked waiting, both the parent and listener should have
  // their states recalculated.
  ASSERT_TRUE(GetVersion(sm, "a")->stateRecalced);
  ASSERT_TRUE(GetVersion(sm, "b")->stateRecalced);
}

void UpdateWaiting(MockStorageManager & sm, StateUpdater & updater, AssetKey ref, AssetDefs::State oldState) {
  auto version = sm.GetMutable(fix(ref));
  updater.UpdateWaitingAssets(version, oldState);
}

TEST_F(StateUpdaterTest, SetInProgressTestWaiting) {
  // Mark the parent and listener as waiting and make sure their states are
  // not recalculated.
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c")});
  SetParentChild(sm, "a", "c");
  SetListenerInput(sm, "b", "c");

  GetMutableVersion(sm, "a")->state = AssetDefs::InProgress;
  UpdateWaiting(sm, updater, "a", AssetDefs::Queued);
  GetMutableVersion(sm, "b")->state = AssetDefs::Waiting;
  UpdateWaiting(sm, updater, "b", AssetDefs::Queued);
  SetInProgress(sm, updater, "c");
  ASSERT_FALSE(GetVersion(sm, "a")->stateRecalced);
  ASSERT_FALSE(GetVersion(sm, "b")->stateRecalced);
  
  // Now mark the parent and listener as not waiting again and make sure the
  // updater goes back to calculating their states
  GetMutableVersion(sm, "c")->state = AssetDefs::Queued;
  GetMutableVersion(sm, "c")->onStateChangeCalled = 0;
  GetMutableVersion(sm, "c")->notificationsSent = 0;
  GetMutableVersion(sm, "a")->state = AssetDefs::Blocked;
  UpdateWaiting(sm, updater, "a", AssetDefs::InProgress);
  GetMutableVersion(sm, "b")->state = AssetDefs::Blocked;
  UpdateWaiting(sm, updater, "b", AssetDefs::Waiting);
  SetInProgress(sm, updater, "c");
  ASSERT_TRUE(GetVersion(sm, "a")->stateRecalced);
  ASSERT_TRUE(GetVersion(sm, "b")->stateRecalced);
}

TEST_F(StateUpdaterTest, SetInProgressUnsupportedException) {
  SetVersions(sm, {MockVersion("a"), MockVersion("b"), MockVersion("c")});
  SetParentChild(sm, "a", "c");
  GetMutableVersion(sm, "c")->stateChangeBehavior = CHANGE_NUM_CHILDREN;
  SetInProgress(sm, updater, "c");
  // We should get partway through setting c's state
  ASSERT_TRUE(GetVersion(sm, "c")->loadedMutable);
  ASSERT_EQ(GetVersion(sm, "c")->onStateChangeCalled, 1);
  ASSERT_EQ(GetVersion(sm, "c")->notificationsSent, 0);
  // We should not recalculate a's state
  ASSERT_FALSE(GetVersion(sm, "a")->stateRecalced);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

// TODO: SetInProgress tests
// Recalc state when state doesn't change (make sure UpdateWaitingAssets is called)
