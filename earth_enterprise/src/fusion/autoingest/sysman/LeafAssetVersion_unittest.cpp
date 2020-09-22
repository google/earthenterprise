// Copyright 2020 The Open GEE contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>
#include <algorithm>
#include <vector>
#include "gee_version.h"
#include "AssetVersionD.h"

class LeafAssetVersionImplDTest : public LeafAssetVersionImplD, public testing::Test {
 protected:
  mutable size_t stateSyncs = 0;
  // Override state sync so we can record when it's called
  bool SyncState(const std::shared_ptr<StateChangeNotifier>) const {
    ++stateSyncs;
    return true;
  }
 public:
  // Handle multiple input state changes (useful for testing)
  void HandleInputStateChanges(AssetDefs::State myState, std::uint32_t waiting, std::vector<AssetDefs::State> states) {
    state = myState;
    numInputsWaitingFor = waiting;
    NotifyStates inputStates;
    inputStates.numSucceeded = std::count_if(states.begin(), states.end(), [](AssetDefs::State s) {
      return s == AssetDefs::Succeeded;
    });
    inputStates.allWorkingOrSucceeded = std::all_of(states.begin(), states.end(), [](AssetDefs::State s) {
      return AssetDefs::Working(s) || s == AssetDefs::Succeeded;
    });

    HandleInputStateChange(inputStates, nullptr);
  }

// Define pure virutal functions so this class can be instantiated
 protected:
  virtual void DoSubmitTask() {}
  virtual bool Save(const std::string &) const { return true; }
 public:
  virtual std::string PluginName() const { return "LeafAssetVersionImplDTest"; }
};

// In any case below in which stateSyncs ends up greater than zero, the value
// of numInputsWaitingFor at the end of the test does not matter. If stateSyncs is
// greater than zero, that means we called SyncState, which will set
// numInputsWaitingFor to the correct value in the actual code.

TEST_F(LeafAssetVersionImplDTest, NewTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 10, {AssetDefs::New});
  EXPECT_GT(stateSyncs, 0);
}

TEST_F(LeafAssetVersionImplDTest, WaitingTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 10, {AssetDefs::Waiting});
  EXPECT_GT(stateSyncs, 0);
}

TEST_F(LeafAssetVersionImplDTest, BlockedTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 10, {AssetDefs::Blocked});
  EXPECT_GT(stateSyncs, 0);
}

TEST_F(LeafAssetVersionImplDTest, QueuedTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 10, {AssetDefs::Queued});
  EXPECT_EQ(numInputsWaitingFor, 10);
  EXPECT_EQ(stateSyncs, 0);
}

TEST_F(LeafAssetVersionImplDTest, InProgressTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 10, {AssetDefs::InProgress});
  EXPECT_EQ(numInputsWaitingFor, 10);
  EXPECT_EQ(stateSyncs, 0);
}

TEST_F(LeafAssetVersionImplDTest, FailedTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 10, {AssetDefs::Failed});
  EXPECT_GT(stateSyncs, 0);
}

TEST_F(LeafAssetVersionImplDTest, SucceededTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 10, {AssetDefs::Succeeded});
  EXPECT_EQ(numInputsWaitingFor, 9);
  EXPECT_EQ(stateSyncs, 0);
}

TEST_F(LeafAssetVersionImplDTest, CanceledTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 10, {AssetDefs::Canceled});
  EXPECT_GT(stateSyncs, 0);
}

TEST_F(LeafAssetVersionImplDTest, OfflineTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 10, {AssetDefs::Offline});
  EXPECT_GT(stateSyncs, 0);
}

TEST_F(LeafAssetVersionImplDTest, BadTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 10, {AssetDefs::Bad});
  EXPECT_GT(stateSyncs, 0);
}

TEST_F(LeafAssetVersionImplDTest, MultipleSucceededTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 10,
      {AssetDefs::Succeeded, AssetDefs::Succeeded, AssetDefs::Succeeded});
  EXPECT_EQ(numInputsWaitingFor, 7);
  EXPECT_EQ(stateSyncs, 0);
}

TEST_F(LeafAssetVersionImplDTest, AllSucceededTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 5,
      {AssetDefs::Succeeded, AssetDefs::Succeeded, AssetDefs::Succeeded, AssetDefs::Succeeded, AssetDefs::Succeeded});
  EXPECT_GT(stateSyncs, 0);
}

TEST_F(LeafAssetVersionImplDTest, MultipleStateTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 10,
      {AssetDefs::Failed, AssetDefs::Succeeded, AssetDefs::Succeeded, AssetDefs::Canceled, AssetDefs::Succeeded, AssetDefs::Blocked});
  EXPECT_GT(stateSyncs, 0);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
