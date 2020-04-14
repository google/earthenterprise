// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


// First unittest in fusion/autoingest
// for AssetDefsExtras

#include <string>
#include <gtest/gtest.h>
#include "autoingest/.idl/storage/AssetDefs.h"

namespace {

class AssetDefsExtraTest : public testing::Test { };

TEST_F(AssetDefsExtraTest, ValidateAssetName) {
  std::string good_name = "abcdefghijklmnopqrstuvwxyz.0123456789@#$^()-";
  EXPECT_TRUE(AssetDefs::ValidateAssetName(good_name));

  std::string invalid_characters("&%'\" \\*=+~`?<>;:");
  for(unsigned int i = 0; i < invalid_characters.size(); ++i) {
    std::string bad_name = "whatswronghere";
    bad_name.push_back(invalid_characters[i]);  // now something's wrong
    EXPECT_FALSE(AssetDefs::ValidateAssetName(bad_name));
  }

  good_name = "abcdefghijklmnopqrstuvwxyz.0123456789?version=111";
  EXPECT_TRUE(AssetDefs::ValidateAssetName(good_name));
  // If a version identifier is present, it needs to end in a numeral.
  std::string bad_name = "abcdefghijklmnopqrstuvwxyz.0123456789?version=111x";
  EXPECT_FALSE(AssetDefs::ValidateAssetName(bad_name));

  // Version identifier needs a numeral.
  bad_name = "abcdefghijklmnopqrstuvwxyz.0123456789?version=";
  EXPECT_FALSE(AssetDefs::ValidateAssetName(bad_name));

  // Version identifier needs a numeral.
  bad_name = "abcdefghijklmnopqrstuvwxyz.0123456789?version";
  EXPECT_FALSE(AssetDefs::ValidateAssetName(bad_name));

  // Version identifier needs a numeral.
  bad_name = "abcdefghijklmnopqrstuvwxyz.0123456789version=111";
  EXPECT_FALSE(AssetDefs::ValidateAssetName(bad_name));
}

}  // namespace

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

