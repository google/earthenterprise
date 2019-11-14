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

#include "AssetOperation.h"
#include "gee_version.h"
#include "StateUpdater.h"
#include "StorageManager.h"

#include <gtest/gtest.h>

using namespace std;

// Declare the globals that AssetOperation functions use. This is cheating but
// it lets us mock the classes that AssetOperation calls.
extern std::unique_ptr<StateUpdater> stateUpdater;
extern StorageManagerInterface<AssetVersionImpl> * assetOpStorageManager;

const AssetKey REF = "a";

class MockVersion : public AssetVersionImpl {
  public:
    bool stateSetForRefAndDependents;
    AssetDefs::State refAndDependentsState;
    std::function<bool(AssetDefs::State)> updateStateFunc;
    bool setProgress;
    bool setDone;
    bool setFailed;
    bool outFilesReset;
    mutable bool updatedWaiting;
    mutable AssetDefs::State oldState;
    mutable WaitingFor waitingFor;
    mutable bool updatedSucceeded;

    MockVersion() :
        stateSetForRefAndDependents(false),
        refAndDependentsState(AssetDefs::Failed),
        updateStateFunc(nullptr),
        setProgress(false),
        setDone(false),
        setFailed(false),
        outFilesReset(false),
        updatedWaiting(false),
        oldState(AssetDefs::New),
        waitingFor({0, 0}),
        updatedSucceeded(false) {
      type = AssetDefs::Imagery; // Ensures that operator bool can return true
      taskid = 0;
      beginTime = 0;
      progressTime = 0;
      progress = 0.0;
    }
    MockVersion(const AssetKey & ref)
        : MockVersion() {
      name = ref;
    }
    MockVersion(const MockVersion & that) : MockVersion() {
      name = that.name;
    }
    virtual void ResetOutFiles(const vector<string> & newOutfiles) override {
      outFilesReset = true;
      outfiles = newOutfiles;
    }

    // Not used - only included to make MockVersion non-virtual
    string PluginName(void) const override { return string(); }
    void GetOutputFilenames(vector<string> &) const override {}
    string GetOutputFilename(uint) const override { return string(); }
};

class MockStorageManager : public StorageManagerInterface<AssetVersionImpl> {
  private:
    // This storage only holds one version
    shared_ptr<MockVersion> version;
    PointerType GetRef(const AssetKey & ref) {
      assert(ref == REF);
      return version;
    }
  public:
    MockStorageManager() : version(make_shared<MockVersion>(REF)) {}
    virtual AssetHandle<const AssetVersionImpl> Get(const AssetKey &ref) {
      return AssetHandle<const AssetVersionImpl>(GetRef(ref), nullptr);
    }
    virtual AssetHandle<AssetVersionImpl> GetMutable(const AssetKey &ref) {
      return AssetHandle<AssetVersionImpl>(GetRef(ref), nullptr);
    }
    // The two functions below pull the pointer out of the AssetHandle and use it
    // directly, which is really bad. We can get away with it in test code, but we
    // should never do this in production code. Hopefully the production code will
    // never need to convert AssetHandles to different types, but if it does we
    // need to come up with a better way.
    const MockVersion * GetVersion() {
      AssetHandle<const AssetVersionImpl> handle = Get(REF);
      return dynamic_cast<const MockVersion *>(handle.operator->());
    }
    MockVersion * GetMutableVersion() {
      AssetHandle<AssetVersionImpl> handle = GetMutable(REF);
      return dynamic_cast<MockVersion *>(handle.operator->());
    }
};

class MockStateUpdater : public StateUpdater {
  private:
    MockStorageManager * sm;
  public:
    MockStateUpdater(MockStorageManager * sm) : sm(sm) {}
    virtual void SetStateForRefAndDependents(
        const SharedString & ref,
        AssetDefs::State newState,
        std::function<bool(AssetDefs::State)> updateStatePredicate) {
      assert(ref == REF);
      auto version = sm->GetMutableVersion();
      version->stateSetForRefAndDependents = true;
      version->refAndDependentsState = newState;
      version->updateStateFunc = updateStatePredicate;
    }
    virtual void SetInProgress(AssetHandle<AssetVersionImpl> & version) {
      auto v = dynamic_cast<MockVersion *>(version.operator->());
      v->setProgress = true;
    }
    virtual void SetSucceeded(AssetHandle<AssetVersionImpl> & version) {
      auto v = dynamic_cast<MockVersion *>(version.operator->());
      v->setDone = true;
    }
    virtual void SetFailed(AssetHandle<AssetVersionImpl> & version) {
      auto v = dynamic_cast<MockVersion *>(version.operator->());
      v->setFailed = true;
    }
    virtual void UpdateWaitingAssets(
        AssetHandle<const AssetVersionImpl> version,
        AssetDefs::State oldState,
        const WaitingFor & waitingFor) {
      auto v = dynamic_cast<const MockVersion *>(version.operator->());
      v->updatedWaiting = true;
      v->oldState = oldState;
      v->waitingFor = waitingFor;
    }
    virtual void UpdateSucceeded(AssetHandle<const AssetVersionImpl> version, bool propagate) {
      // AssetOperation should always set propagate to false, so we check that here
      ASSERT_FALSE(propagate);
      auto v = dynamic_cast<const MockVersion *>(version.operator->());
      v->updatedSucceeded = true;
    }
};

class AssetOperationTest : public testing::Test {
  protected:
    MockStorageManager sm;
  public:
    AssetOperationTest() {
      assetOpStorageManager = &sm;
      stateUpdater.reset(new MockStateUpdater(&sm));
    }
};

TEST_F(AssetOperationTest, Rebuild) {
  sm.GetMutableVersion()->state = AssetDefs::Canceled;
  RebuildVersion(REF, true);
  ASSERT_TRUE(sm.GetVersion()->stateSetForRefAndDependents);
  ASSERT_EQ(sm.GetVersion()->refAndDependentsState, AssetDefs::New);

  // Make sure the function that determines which states to update returns the
  // correct values.
  ASSERT_TRUE(sm.GetVersion()->updateStateFunc(AssetDefs::Canceled));
  ASSERT_TRUE(sm.GetVersion()->updateStateFunc(AssetDefs::Failed));
  ASSERT_FALSE(sm.GetVersion()->updateStateFunc(AssetDefs::New));
  ASSERT_FALSE(sm.GetVersion()->updateStateFunc(AssetDefs::Waiting));
  ASSERT_FALSE(sm.GetVersion()->updateStateFunc(AssetDefs::Blocked));
  ASSERT_FALSE(sm.GetVersion()->updateStateFunc(AssetDefs::Queued));
  ASSERT_FALSE(sm.GetVersion()->updateStateFunc(AssetDefs::InProgress));
  ASSERT_FALSE(sm.GetVersion()->updateStateFunc(AssetDefs::Succeeded));
  ASSERT_FALSE(sm.GetVersion()->updateStateFunc(AssetDefs::Offline));
  ASSERT_FALSE(sm.GetVersion()->updateStateFunc(AssetDefs::Bad));
}

void RebuildWrongState(MockStorageManager & sm, AssetDefs::State state) {
  sm.GetMutableVersion()->state = state;
  ASSERT_THROW(RebuildVersion(REF, true), khException);
}

TEST_F(AssetOperationTest, RebuildWrongStateSucceeded) {
  RebuildWrongState(sm, AssetDefs::Succeeded);
}

TEST_F(AssetOperationTest, RebuildWrongStateOffline) {
  RebuildWrongState(sm, AssetDefs::Offline);
}

TEST_F(AssetOperationTest, RebuildWrongStateBad) {
  RebuildWrongState(sm, AssetDefs::Bad);
}

TEST_F(AssetOperationTest, RebuildBadVersion) {
  sm.GetMutableVersion()->type = AssetDefs::Invalid;
  RebuildVersion(REF, true);
  ASSERT_FALSE(sm.GetVersion()->stateSetForRefAndDependents);
}

TEST_F(AssetOperationTest, Progress) {
  const uint32 TASKID = 123;
  const time_t BEGIN_TIME = 456;
  const time_t PROGRESS_TIME = 789;
  const double PROGRESS = 10.1112;

  auto msg = TaskProgressMsg(REF, TASKID, BEGIN_TIME, PROGRESS_TIME, PROGRESS);
  sm.GetMutableVersion()->taskid = TASKID;
  HandleTaskProgress(msg, true);
  ASSERT_EQ(sm.GetVersion()->beginTime, BEGIN_TIME);
  ASSERT_EQ(sm.GetVersion()->progressTime, PROGRESS_TIME);
  ASSERT_EQ(sm.GetVersion()->progress, PROGRESS);
  ASSERT_TRUE(sm.GetVersion()->setProgress);
}

TEST_F(AssetOperationTest, ProgressWrongTaskId) {
  const uint32 TASKID = 123;
  const time_t BEGIN_TIME = 456;
  const time_t PROGRESS_TIME = 789;
  const double PROGRESS = 10.1112;

  auto msg = TaskProgressMsg(REF, TASKID, BEGIN_TIME, PROGRESS_TIME, PROGRESS);
  sm.GetMutableVersion()->taskid = TASKID + 1;
  HandleTaskProgress(msg, true);
  ASSERT_NE(sm.GetVersion()->beginTime, BEGIN_TIME);
  ASSERT_NE(sm.GetVersion()->progressTime, PROGRESS_TIME);
  ASSERT_NE(sm.GetVersion()->progress, PROGRESS);
  ASSERT_FALSE(sm.GetVersion()->setProgress);
}

TEST_F(AssetOperationTest, ProgressBadVersion) {
  const uint32 TASKID = 123;
  const time_t BEGIN_TIME = 456;
  const time_t PROGRESS_TIME = 789;
  const double PROGRESS = 10.1112;

  sm.GetMutableVersion()->type = AssetDefs::Invalid;
  auto msg = TaskProgressMsg(REF, TASKID, BEGIN_TIME, PROGRESS_TIME, PROGRESS);
  sm.GetMutableVersion()->taskid = TASKID + 1;
  HandleTaskProgress(msg, true);
  ASSERT_NE(sm.GetVersion()->beginTime, BEGIN_TIME);
  ASSERT_NE(sm.GetVersion()->progressTime, PROGRESS_TIME);
  ASSERT_NE(sm.GetVersion()->progress, PROGRESS);
  ASSERT_FALSE(sm.GetVersion()->setProgress);
}

TEST_F(AssetOperationTest, Done) {
  const uint32 TASKID = 123;
  const time_t BEGIN_TIME = 456;
  const time_t END_TIME = 789;
  const vector<string> OUTFILES = {"x", "y", "z"};

  auto msg = TaskDoneMsg(REF, TASKID, true, BEGIN_TIME, END_TIME, OUTFILES);
  sm.GetMutableVersion()->taskid = TASKID;
  HandleTaskDone(msg, true);
  ASSERT_EQ(sm.GetVersion()->beginTime, BEGIN_TIME);
  ASSERT_EQ(sm.GetVersion()->progressTime, END_TIME);
  ASSERT_EQ(sm.GetVersion()->endTime, END_TIME);
  ASSERT_EQ(sm.GetVersion()->outfiles, OUTFILES);
  ASSERT_TRUE(sm.GetVersion()->setDone);
  ASSERT_FALSE(sm.GetVersion()->setFailed);
}

TEST_F(AssetOperationTest, DoneWrongTaskId) {
  const uint32 TASKID = 123;
  const time_t BEGIN_TIME = 456;
  const time_t END_TIME = 789;
  const vector<string> OUTFILES = {"x", "y", "z"};

  auto msg = TaskDoneMsg(REF, TASKID, true, BEGIN_TIME, END_TIME, OUTFILES);
  sm.GetMutableVersion()->taskid = TASKID + 1;
  HandleTaskDone(msg, true);
  ASSERT_NE(sm.GetVersion()->beginTime, BEGIN_TIME);
  ASSERT_NE(sm.GetVersion()->progressTime, END_TIME);
  ASSERT_NE(sm.GetVersion()->endTime, END_TIME);
  ASSERT_NE(sm.GetVersion()->outfiles, OUTFILES);
  ASSERT_FALSE(sm.GetVersion()->setDone);
  ASSERT_FALSE(sm.GetVersion()->setFailed);
}

TEST_F(AssetOperationTest, DoneFailed) {
  const uint32 TASKID = 123;
  const time_t BEGIN_TIME = 456;
  const time_t END_TIME = 789;
  const vector<string> OUTFILES = {"x", "y", "z"};

  auto msg = TaskDoneMsg(REF, TASKID, false, BEGIN_TIME, END_TIME, OUTFILES);
  sm.GetMutableVersion()->taskid = TASKID;
  HandleTaskDone(msg, true);
  ASSERT_EQ(sm.GetVersion()->beginTime, BEGIN_TIME);
  ASSERT_EQ(sm.GetVersion()->progressTime, END_TIME);
  ASSERT_EQ(sm.GetVersion()->endTime, END_TIME);
  ASSERT_NE(sm.GetVersion()->outfiles, OUTFILES);
  ASSERT_FALSE(sm.GetVersion()->setDone);
  ASSERT_TRUE(sm.GetVersion()->setFailed);
}

TEST_F(AssetOperationTest, DoneBadVersion) {
  const uint32 TASKID = 123;
  const time_t BEGIN_TIME = 456;
  const time_t END_TIME = 789;
  const vector<string> OUTFILES = {"x", "y", "z"};

  sm.GetMutableVersion()->type = AssetDefs::Invalid;
  auto msg = TaskDoneMsg(REF, TASKID, true, BEGIN_TIME, END_TIME, OUTFILES);
  sm.GetMutableVersion()->taskid = TASKID;
  HandleTaskDone(msg, true);
  ASSERT_NE(sm.GetVersion()->beginTime, BEGIN_TIME);
  ASSERT_NE(sm.GetVersion()->progressTime, END_TIME);
  ASSERT_NE(sm.GetVersion()->endTime, END_TIME);
  ASSERT_NE(sm.GetVersion()->outfiles, OUTFILES);
  ASSERT_FALSE(sm.GetVersion()->setDone);
  ASSERT_FALSE(sm.GetVersion()->setFailed);
}

TEST_F(AssetOperationTest, HandleExternalStateChangeWaiting) {
  const AssetDefs::State OLD_STATE = AssetDefs::Failed;
  const WaitingFor WAITING_FOR = {123, 456};

  sm.GetMutableVersion()->state = AssetDefs::Queued;
  HandleExternalStateChange(REF, OLD_STATE, WAITING_FOR.inputs, WAITING_FOR.children, true);
  ASSERT_TRUE(sm.GetVersion()->updatedWaiting);
  ASSERT_EQ(sm.GetVersion()->oldState, OLD_STATE);
  ASSERT_EQ(sm.GetVersion()->waitingFor.inputs, WAITING_FOR.inputs);
  ASSERT_EQ(sm.GetVersion()->waitingFor.children, WAITING_FOR.children);
  ASSERT_FALSE(sm.GetVersion()->updatedSucceeded);
}

TEST_F(AssetOperationTest, HandleExternalStateChangeSucceeded) {
  const AssetDefs::State OLD_STATE = AssetDefs::Failed;
  const WaitingFor WAITING_FOR = {123, 456};

  sm.GetMutableVersion()->state = AssetDefs::Succeeded;
  HandleExternalStateChange(REF, OLD_STATE, WAITING_FOR.inputs, WAITING_FOR.children, true);
  ASSERT_TRUE(sm.GetVersion()->updatedWaiting);
  ASSERT_EQ(sm.GetVersion()->oldState, OLD_STATE);
  ASSERT_EQ(sm.GetVersion()->waitingFor.inputs, WAITING_FOR.inputs);
  ASSERT_EQ(sm.GetVersion()->waitingFor.children, WAITING_FOR.children);
  ASSERT_TRUE(sm.GetVersion()->updatedSucceeded);
}

TEST_F(AssetOperationTest, HandleExternalStateChangeBadVersion) {
  const AssetDefs::State OLD_STATE = AssetDefs::Failed;
  const WaitingFor WAITING_FOR = {123, 456};

  sm.GetMutableVersion()->state = AssetDefs::Succeeded;
  sm.GetMutableVersion()->type = AssetDefs::Invalid;
  HandleExternalStateChange(REF, OLD_STATE, WAITING_FOR.inputs, WAITING_FOR.children, true);
  ASSERT_FALSE(sm.GetVersion()->updatedWaiting);
  ASSERT_NE(sm.GetVersion()->oldState, OLD_STATE);
  ASSERT_NE(sm.GetVersion()->waitingFor.inputs, WAITING_FOR.inputs);
  ASSERT_NE(sm.GetVersion()->waitingFor.children, WAITING_FOR.children);
  ASSERT_FALSE(sm.GetVersion()->updatedSucceeded);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

