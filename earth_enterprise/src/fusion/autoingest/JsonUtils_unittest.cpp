// Copyright 2021 the Open GEE Contributors
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


// Unit tests for Js Utilities 

#include <string>
#include <gtest/gtest.h>
#include "fusion/autoingest/JsonUtils.h"

namespace {

class JsonUnitTest : public testing::Test, public JsonUtils {
};

// Testing the JsonBool function
TEST_F(JsonUnitTest, JsonBoolTrue) {
    const bool trueBool = true;
    const std::string expectedTrue= "true";
    std::string result = JsonBool(trueBool);

    EXPECT_EQ(expectedTrue, result);
}

TEST_F(JsonUnitTest, JsonBoolFalse) {
    const bool falseBool = false;
    const std::string expectedFalse= "false";
    std::string result = JsonBool(falseBool);

    EXPECT_EQ(expectedFalse, result);
}

TEST_F(JsonUnitTest, LookAtValid) {
    const std::string lookAtString = "-93.02119299429238|38.88758732203867|4963567.527932385|0.4781091614682265|6.862074527538373";
    const std::string expectedJson= "{\naltitude : 4.96357e+06,\nlat : 38.88758732203867,\nlng : -93.02119299429238,\nzoom : 3\n}\n";
    std::string result = LookAtJson(lookAtString);

    EXPECT_EQ(expectedJson, result);
}

TEST_F(JsonUnitTest, LookAtInvalid) {
    const std::string lookAtString = "-93.02119299429238|38.88758732203867";
    const std::string expectedJson= "{\naltitude : 0,\nlat : 0,\nlng : 0,\nzoom : 1\n}\n";
    std::string result = LookAtJson(lookAtString);

    EXPECT_EQ(expectedJson, result);
}

TEST_F(JsonUnitTest, BasicJsonObject) {
    std::map<std::string, std::string> field_map;
    field_map["foo"] = "bar";
    field_map["bar"] = "baz";
    const std::string expectedJson= "{\nbar : baz,\nfoo : bar\n}\n";
    std::string result = JsonObject(field_map);

    EXPECT_EQ(expectedJson, result);
}

TEST_F(JsonUnitTest, BasicJsonArray) {
    std::vector<std::string> array_values;
    array_values.push_back("foo");
    array_values.push_back("bar");
    array_values.push_back("baz");
    const std::string expectedJson="\n[\nfoo,\nbar,\nbaz\n]\n";
    std::string result = JsonArray(array_values);

    EXPECT_EQ(expectedJson, result);
}


}  // namespace

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

