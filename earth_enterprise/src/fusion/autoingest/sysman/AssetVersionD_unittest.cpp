// Copyright 2019 The Open GEE contributors
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
#include <vector>
#include "gee_version.h"
#include "AssetVersionD.h"

class LeafAssetVersionImplDTest : public LeafAssetVersionImplD, public testing::Test {
 protected:
  mutable size_t stateSyncs = 0;
  // Override state sync so we can record when it's called
  void SyncState(const std::shared_ptr<StateChangeNotifier>) const {
    ++stateSyncs;
  }
 public:
  // Handle multiple input state changes (useful for testing)
  void HandleInputStateChanges(AssetDefs::State myState, uint32 waiting, std::vector<AssetDefs::State> states) {
    state = myState;
    numWaitingFor = waiting;
    //numWaitingFor = waiting;
    for (AssetDefs::State state : states) {
      HandleInputStateChange(state, nullptr);
    }
  }

// Define pure virutal functions
 protected:
  virtual void DoSubmitTask() {}
  virtual bool Save(const std::string &) const { return true; }
 public:
  virtual std::string PluginName() const { return "LeafAssetVersionImplDTest"; }
};

TEST_F(LeafAssetVersionImplDTest, SingleSucceededTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 10, {AssetDefs::Succeeded});
  EXPECT_EQ(numWaitingFor, 9);
  EXPECT_EQ(stateSyncs, 0);
}

TEST_F(LeafAssetVersionImplDTest, MultipleSucceededTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 10,
      {AssetDefs::Succeeded, AssetDefs::Succeeded, AssetDefs::Succeeded});
  EXPECT_EQ(numWaitingFor, 7);
  EXPECT_EQ(stateSyncs, 0);
}

TEST_F(LeafAssetVersionImplDTest, AllSucceededTest) {
  HandleInputStateChanges(AssetDefs::Waiting, 5,
      {AssetDefs::Succeeded, AssetDefs::Succeeded, AssetDefs::Succeeded, AssetDefs::Succeeded, AssetDefs::Succeeded});
  EXPECT_LE(numWaitingFor, 1);
  EXPECT_GT(stateSyncs, 0);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
