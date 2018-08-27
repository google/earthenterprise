// Copyright 2018 the Open GEE Contributors
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


// Unit tests for Json Utilities 

#include <string>
#include <gtest/gtest.h>
#include "fusion/autoingest/JsonUtils.h"

namespace {

class JsonUnitTest : public testing::Test { };

TEST_F(JsonUnitTest, singleFieldNospaces) {
    const std::string js = R"js_({name:"fred"})js_";
    const std::string expectedJson = R"js_({"name":"fred"})js_";
    std::string result = JsonUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

}  // namespace

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

