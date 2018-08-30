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


// Unit tests for Js Utilities 

#include <string>
#include <gtest/gtest.h>
#include "fusion/autoingest/JsUtils.h"

namespace {

class JsUnitTest : public testing::Test { };

// Testing when the Javascript is simply null
// The conversion method should handle it correctly
TEST_F(JsUnitTest, nullFields) {
    const std::string js = R"js_()js_";
    const std::string expectedJson = R"js_()js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// Testing the Javascript could be simply empty
TEST_F(JsUnitTest, emptyFields) {
    const std::string js = R"js_({})js_";
    const std::string expectedJson = R"js_({})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// Testing the value can be empty
TEST_F(JsUnitTest, unsetValue) {
    const std::string js = R"js_({name:""})js_";
    const std::string expectedJson = R"js_({"name":""})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// this checks the quotes are added to the key when string
TEST_F(JsUnitTest, noKeyQuotes) {
    const std::string js = R"js_({name:"fred"})js_";
    const std::string expectedJson = R"js_({"name":"fred"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// this checks the conversion detects they are strings and quotes them
TEST_F(JsUnitTest, noValueQuotes) {
    const std::string js = R"js_({name:fred})js_";
    const std::string expectedJson = R"js_({"name":"fred"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// this is the most "normal" string case
TEST_F(JsUnitTest, doubleKeyQuotes) {
    const std::string js = R"js_({"name":"fred"})js_";
    const std::string expectedJson = R"js_({"name":"fred"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// Ok so apparently using single quotes is somewhat common
// Surely the conversion utility shouldn't just "accept" them
// and put double quotes INSIDE of them!
//TEST_F(JsUnitTest, singleQuotes) {
//    const std::string js = R"js_({'name':'fred'})js_";
//    const std::string expectedJson = R"js_({"name":"fred"})js_";
//    std::string result = JsUtils::JsToJson(js);
//
//    EXPECT_EQ(expectedJson, result);
//}

// single quotes are just another character in keyname
TEST_F(JsUnitTest, singleQuoteInKey) {
    const std::string js = R"js_({"fred's":"name"})js_";
    const std::string expectedJson = R"js_({"fred's":"name"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// While unlikely and dangerous, this seems legal in Javascript
// Perhaps a bogus test/unlikely edge case
//TEST_F(JsUnitTest, doubleQuoteInKey) {
//    const std::string js = R"js_({"freds\"car\"":"christine"})js_";
//    const std::string expectedJson = R"js_({"freds\"car\"":"christine"})js_";
//    std::string result = JsUtils::JsToJson(js);
//
//    EXPECT_EQ(expectedJson, result);
//}

// single quotes are just another character
TEST_F(JsUnitTest, singleQuoteInValue) {
    const std::string js = R"js_({"car":"fred's"})js_";
    const std::string expectedJson = R"js_({"car":"fred's"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// Can we escape quotes?  This is legit for Javascript
//TEST_F(JsUnitTest, doubleQuoteInValue) {
//    const std::string js = R"js_({"carname":"\"christine\""})js_";
//    const std::string expectedJson = R"js_({"carname":"\"christine\""})js_";
//    std::string result = JsUtils::JsToJson(js);
//
//    EXPECT_EQ(expectedJson, result);
//}

// this is legal in json
TEST_F(JsUnitTest, spaceInKey) {
    const std::string js = R"js_({"my name is":"fred"})js_";
    const std::string expectedJson = R"js_({"my name is":"fred"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// this is legal in json
TEST_F(JsUnitTest, spaceInValue) {
    const std::string js = R"js_({name:"fred flintstone"})js_";
    const std::string expectedJson = R"js_({"name":"fred flintstone"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// this is legal in json
TEST_F(JsUnitTest, underscoreInKey) {
    const std::string js = R"js_({"my_name_is":"fred"})js_";
    const std::string expectedJson = R"js_({"my_name_is":"fred"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// this is legal in json
TEST_F(JsUnitTest, underscoreInValue) {
    const std::string js = R"js_({name:"fred_flintstone"})js_";
    const std::string expectedJson = R"js_({"name":"fred_flintstone"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// this is legal in json
TEST_F(JsUnitTest, dotInKey) {
    const std::string js = R"js_({"my.name.is":"fred"})js_";
    const std::string expectedJson = R"js_({"my.name.is":"fred"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// this is legal in json
TEST_F(JsUnitTest, dotInValue) {
    const std::string js = R"js_({name:"fred.flintstone"})js_";
    const std::string expectedJson = R"js_({"name":"fred.flintstone"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// this is legal in json
TEST_F(JsUnitTest, dashInKey) {
    const std::string js = R"js_({"my-name-is":"fred"})js_";
    const std::string expectedJson = R"js_({"my-name-is":"fred"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// this is legal in json
TEST_F(JsUnitTest, dashInValue) {
    const std::string js = R"js_({name:"fred-flintstone"})js_";
    const std::string expectedJson = R"js_({"name":"fred-flintstone"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// this is legal in json
TEST_F(JsUnitTest, spaceStartValue) {
    const std::string js = R"js_({" name":"fred"})js_";
    const std::string expectedJson = R"js_({" name":"fred"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// This is legal in json
TEST_F(JsUnitTest, underscoreStartValue) {
    const std::string js = R"js_({"_name":"fred"})js_";
    const std::string expectedJson = R"js_({"_name":"fred"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// this is legal in json
TEST_F(JsUnitTest, dotStartValue) {
    const std::string js = R"js_({".name":"fred"})js_";
    const std::string expectedJson = R"js_({".name":"fred"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// this is legal in json
TEST_F(JsUnitTest, dashStartValue) {
    const std::string js = R"js_({"-name":"fred"})js_";
    const std::string expectedJson = R"js_({"-name":"fred"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// While illegal in json with . reference it is ok otherwise
TEST_F(JsUnitTest, numericStartValue) {
    const std::string js = R"js_({"123name":"fred"})js_";
    const std::string expectedJson = R"js_({"123name":"fred"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// Simple multiple pair case
TEST_F(JsUnitTest, multiplePairs) {
    const std::string js = R"js_({name:"fred",car:"christine"})js_";
    const std::string expectedJson = 
        R"js_({"name":"fred","car":"christine"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// Should it eat white space? I would think the conversion would collapse it.
// This may not be a bug.  Yes the test is ugly on purpose to contrast output
//TEST_F(JsUnitTest, spansMultipleLines) {
//    const std::string js = R"js_({name:"fred", 
//         car:"christine"
//        ,color:"red"})js_";
//    const std::string expectedJson = 
//        R"js_({"name":"fred","car":"christine","color":"red"})js_";
//    std::string result = JsUtils::JsToJson(js);
//
//    EXPECT_EQ(expectedJson, result);
//}

// Return/NewLine should be... eaten? Replaced with a space?
// Surely not just have extra escape chars thrown at it
//TEST_F(JsUnitTest, newlineInValue) {
//    const std::string js = R"js_({"name":"fred\r\nis\r\ncool\r\n"})js_";
//    const std::string expectedJson = R"js_({"name":"frediscool"})js_";
//    std::string result = JsUtils::JsToJson(js);
//
//    EXPECT_EQ(expectedJson, result);
//}

// should allow for a numeric key, and not convert to string
TEST_F(JsUnitTest, numericKey) {
    const std::string js = R"js_({0:"fred"})js_";
    const std::string expectedJson = R"js_({0:"fred"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// should allow for numeric and not convert to string
TEST_F(JsUnitTest, numericValue) {
    const std::string js = R"js_({number:5})js_";
    const std::string expectedJson = R"js_({"number":5})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// should allow for boolean and not convert to string
TEST_F(JsUnitTest, booleanValue) {
    const std::string js = R"js_({isBugFree:false})js_";
    const std::string expectedJson = R"js_({"isBugFree":false})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// the least complex array case, all strings
TEST_F(JsUnitTest, stringArrayValue) {
    const std::string js = 
        R"js_({name:["tom","dick","harry"],me:"fred"})js_";
    const std::string expectedJson = 
        R"js_({"name":["tom","dick","harry"],"me":"fred"})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// should allow for boolean array and not convert to strings
TEST_F(JsUnitTest, booleanArrayValue) {
    const std::string js = 
        R"js_({today:true,isBugFree:[false,false,false]})js_";
    const std::string expectedJson = 
        R"js_({"today":true,"isBugFree":[false,false,false]})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// should allow for numeric array and not convert to strings
TEST_F(JsUnitTest, numericArrayValue) {
    const std::string js = 
        R"js_({0:"true",salary:[1000,2000,1990]})js_";
    const std::string expectedJson = 
        R"js_({0:"true","salary":[1000,2000,1990]})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// should not convert either key or value to string
TEST_F(JsUnitTest, numericBoolean) {
    const std::string js = R"js_({0:false,1:true})js_";
    const std::string expectedJson = R"js_({0:false,1:true})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// should not convert boolean or numeric subobjects to strings
TEST_F(JsUnitTest, subObjectTest) {
    const std::string js = 
        R"js_({names:{first:fred,new:false,house:10}})js_";
    const std::string expectedJson = 
        R"js_({"names":{"first":"fred","new":false,"house":10}})js_";
    std::string result = JsUtils::JsToJson(js);

    EXPECT_EQ(expectedJson, result);
}

// possible additional tests
// 1. Arrays that span multiple lines
// 2. Subobjects that span multiple lines

}  // namespace

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

