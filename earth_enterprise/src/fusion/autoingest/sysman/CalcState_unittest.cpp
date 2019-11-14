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

class ExpectedStates {
  private:
    using MapType = map<AssetDefs::State, // starting state
                    map<AssetDefs::State, // state by inputs
                    map<AssetDefs::State, // state by children
                    map<bool,             // blockers are offline
                    map<bool,             // offline inputs break me
                    map<bool,             // has children
                    map<bool,             // composites care about inputs
                    AssetDefs::State      // the expected state based on the above
                    >>>>>>>;
    MapType expectedStates;
  public:
    AssetDefs::State Get(AssetDefs::State startingState,
        AssetDefs::State byInputs,
        AssetDefs::State byChildren,
        bool blockersOffline,
        bool offlineBreaks,
        bool hasChildren,
        bool caresAboutInputs) const {
      try {
        return expectedStates.at(startingState)
            .at(byInputs)
            .at(byChildren)
            .at(blockersOffline)
            .at(offlineBreaks)
            .at(hasChildren)
            .at(caresAboutInputs);
      }
      catch (out_of_range) {
        // There's some mismatch between the states saved in the file and the
        // states checked by the test.
        assert(false);
      }
    }
    void Set(
        AssetDefs::State startingState,
        AssetDefs::State byInputs,
        AssetDefs::State byChildren,
        bool blockersOffline,
        bool offlineBreaks,
        bool hasChildren,
        bool caresAboutInputs,
        AssetDefs::State expected) {
      expectedStates[startingState]
          [byInputs]
          [byChildren]
          [blockersOffline]
          [offlineBreaks]
          [hasChildren]
          [caresAboutInputs]
          = expected;
    }

};

class TestLeafAssetVersionImplD : public LeafAssetVersionImplD {
  public:
    static bool offlineBreaks;
    virtual bool OfflineInputsBreakMe() const override {
      return offlineBreaks;
    }

    // Only used for composites but included so we can use the same code for both
    static bool hasChildren;
    static bool caresAboutInputs;

      // Not used - only included to make MockVersion non-virtual
    string PluginName(void) const override { return string(); }
    void GetOutputFilenames(vector<string> &) const override {}
    string GetOutputFilename(uint) const override { return string(); }
    virtual void DoSubmitTask() {}
};

bool TestLeafAssetVersionImplD::offlineBreaks = false;
bool TestLeafAssetVersionImplD::hasChildren = false;
bool TestLeafAssetVersionImplD::caresAboutInputs = false;

class TestCompositeAssetVersionImplD : public CompositeAssetVersionImplD {
  public:
    static bool hasChildren;
    TestCompositeAssetVersionImplD() {
      if (hasChildren) {
        children.push_back("child");
      }
    }

    static bool offlineBreaks;
    virtual bool OfflineInputsBreakMe() const override {
      return offlineBreaks;
    }

    static bool caresAboutInputs;
    virtual bool CompositeStateCaresAboutInputsToo() const override {
      return caresAboutInputs;
    }

      // Not used - only included to make MockVersion non-virtual
    string PluginName(void) const override { return string(); }
    void GetOutputFilenames(vector<string> &) const override {}
    string GetOutputFilename(uint) const override { return string(); }
};

bool TestCompositeAssetVersionImplD::hasChildren = false;
bool TestCompositeAssetVersionImplD::offlineBreaks = false;
bool TestCompositeAssetVersionImplD::caresAboutInputs = false;

template <typename TestAssetVersionD>
void TestCalcState(
    AssetDefs::State startingState,
    AssetDefs::State byInputs,
    AssetDefs::State byChildren,
    bool blockersOffline,
    bool offlineBreaks,
    bool hasChildren,
    bool caresAboutInputs,
    ExpectedStates & expected) {
  InputAndChildStateData data = { byInputs, byChildren, blockersOffline, 0, 0 };
  TestAssetVersionD::offlineBreaks = offlineBreaks;
  TestAssetVersionD::hasChildren = hasChildren;
  TestAssetVersionD::caresAboutInputs = caresAboutInputs;
  TestAssetVersionD version;
  version.state = startingState;
  AssetDefs::State actualState = version.CalcStateByInputsAndChildren(data);
  expected.Set(startingState, byInputs, byChildren, blockersOffline, offlineBreaks,
              hasChildren, caresAboutInputs, actualState);
}

// This allows us to test all possible combinations of inputs to state
// calculation functions. We make some effort to skip invalid combinations, but
// this may test some cases that don't happen in real life.
template <typename TestAssetVersionD>
void TestAll(
    bool hasChildren,
    vector<bool> caresAboutInputsOptions) {
  ExpectedStates expectedStates;
  vector<AssetDefs::State> byChildrenStates;
  if (hasChildren) {
    byChildrenStates = {AssetDefs::Succeeded, AssetDefs::Blocked, AssetDefs::InProgress, AssetDefs::Queued };
  }
  else {
    // If there are no chilren then numkids and numgood will both be 0, so the
    // state by children will be succeeded.
    byChildrenStates = {AssetDefs::Succeeded};
  }
  for (auto startingState : states) { // All states are valid starting states
    for (auto byInputs : {AssetDefs::Queued, AssetDefs::Blocked, AssetDefs::Waiting}) {
      for (auto byChildren : byChildrenStates) {
        for (auto blockersOffline : {false, true}) {
          for (auto caresAboutInputs : caresAboutInputsOptions) {
            for (auto offlineBreaks: {false, true}) {
              TestCalcState<TestLeafAssetVersionImplD>(startingState,
                  byInputs, byChildren, blockersOffline, offlineBreaks,
                  hasChildren, caresAboutInputs, expectedStates);
            }
          }
        }
      }
    }
  }
}

TEST(CalcStateTest, LeafAssetVersion) {
  auto hasChildren = false;  // Leaf assets have no children
  auto caresAboutInputs = {true};  // Leaf assets always care about inputs
  TestAll<TestLeafAssetVersionImplD>(hasChildren, caresAboutInputs);
}

TEST(CalcStateTest, CompositeAssetVersion) {
  auto caresAboutInputs = {false, true};
  for (auto hasChildren : {false, true}) {
    TestAll<TestCompositeAssetVersionImplD>(hasChildren, caresAboutInputs);
  }
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
