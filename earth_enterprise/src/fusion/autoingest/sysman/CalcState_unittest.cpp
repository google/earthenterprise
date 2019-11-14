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
#include <gtest/gtest.h>

using namespace std;

vector<AssetDefs::State> states = { AssetDefs::New, AssetDefs::Waiting, AssetDefs::Blocked, AssetDefs::Queued, AssetDefs::InProgress, AssetDefs::Failed, AssetDefs::Succeeded, AssetDefs::Canceled, AssetDefs::Offline, AssetDefs::Bad };

class TestLeafAssetVersionImplD : public LeafAssetVersionImplD {
  public:
      // Not used - only included to make MockVersion non-virtual
    string PluginName(void) const override { return string(); }
    void GetOutputFilenames(vector<string> &) const override {}
    string GetOutputFilename(uint) const override { return string(); }
    virtual void DoSubmitTask() {}
};

template <typename TestAssetVersionD>
void TestCalcState(AssetDefs::State startingState, AssetDefs::State byInputs, AssetDefs::State byChildren, bool blockersOffline, AssetDefs::State expected) {
  InputAndChildStateData data = { byInputs, byChildren, blockersOffline, 0, 0 };
  TestAssetVersionD version;
  version.state = startingState;
  AssetDefs::State actualState = version.CalcStateByInputsAndChildren(data);
  cout << ToString(actualState) << endl;
}

template <typename TestAssetVersionD>
void TestAllStartingStates(AssetDefs::State byInputs, AssetDefs::State byChildren, bool blockersOffline, const vector<AssetDefs::State> expected) {
  ASSERT_EQ(states.size(), expected.size());
  cout << "------------------------" << endl;
  for (uint i = 0; i < states.size(); ++i) {
    TestCalcState<TestAssetVersionD>(states[i], byInputs, byChildren, blockersOffline, expected[i]);
  }
}

TEST(CalcStateTest, LeafAssetVersion) {
  // The number of children and number of good children will always be 0, so
  // state by children will always be Succeeded.
  AssetDefs::State byChildren = AssetDefs::Succeeded;
  // If the stateByInputs is Queued or Waiting, blockersAreOffline will always
  // be true because numblocking and numoffline will both be 0.
  TestAllStartingStates<TestLeafAssetVersionImplD>(AssetDefs::Queued, byChildren, true, 
      { AssetDefs::Queued, AssetDefs::Queued, AssetDefs::Queued, AssetDefs::Queued, AssetDefs::InProgress, AssetDefs::Failed, AssetDefs::Succeeded, AssetDefs::Canceled, AssetDefs::Offline, AssetDefs::Bad });
  TestAllStartingStates<TestLeafAssetVersionImplD>(AssetDefs::Waiting, byChildren, true, 
      { AssetDefs::Waiting, AssetDefs::Waiting, AssetDefs::Waiting, AssetDefs::Waiting, AssetDefs::Waiting, AssetDefs::Waiting, AssetDefs::Waiting, AssetDefs::Waiting, AssetDefs::Waiting, AssetDefs::Waiting });

  TestAllStartingStates<TestLeafAssetVersionImplD>(AssetDefs::Blocked, byChildren, true, 
      { AssetDefs::Blocked, AssetDefs::Blocked, AssetDefs::Blocked, AssetDefs::Blocked, AssetDefs::Blocked, AssetDefs::Failed, AssetDefs::Succeeded, AssetDefs::Canceled, AssetDefs::Offline, AssetDefs::Bad });
  TestAllStartingStates<TestLeafAssetVersionImplD>(AssetDefs::Blocked, byChildren, false, 
      { AssetDefs::Blocked, AssetDefs::Blocked, AssetDefs::Blocked, AssetDefs::Blocked, AssetDefs::Blocked, AssetDefs::Blocked, AssetDefs::Blocked, AssetDefs::Blocked, AssetDefs::Blocked, AssetDefs::Blocked });
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
