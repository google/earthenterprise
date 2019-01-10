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
#include "gee_version.h"
#include "AssetVersionD.h"

class LeafAssetVersionImplDTest : public LeafAssetVersionImpl, public testing::Test {
    
// Define pure virutal functions
 protected:
  virtual void DoSubmitTask() {}
 public:
  virtual std::string PluginName() const { return "LeafAssetVersionImplDTest"; }
};

TEST_F(LeafAssetVersionImplDTest, TrueTest) {
  EXPECT_TRUE(true);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}
