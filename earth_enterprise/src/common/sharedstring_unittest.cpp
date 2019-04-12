// Copyright 2019 The Open GEE Contributors
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
#include "SharedString.h"
#include <sstream>
#include <string>
#include <cstring>
using namespace testing;
using namespace std;

const string test_string1 {"/this/path/points/to/an_asset"};
const string test_string2 {"/this/path/points/to/another_asset"};
const string test_string3 {"/this/path/points/to/yet_another_asset"};

SharedString tsst0,
             tsst1(test_string1),
             tsst2(test_string2.c_str()),
             tsst3(test_string3);

// simple class to pull out the key
class TestSharedString : public SharedString
{
public:
    TestSharedString(const SharedString& other) : SharedString(other) {}
    inline const uint32_t operator()() noexcept { return key; }
};

             
TEST(SharedStringTest, accessors)
{
    // test operator=, string(), toString(), and c_str()
    SharedString tsst4;
    tsst4 = test_string2;
    EXPECT_TRUE(string(tsst4) == test_string2);
    EXPECT_TRUE(tsst4.toString() == test_string2);
    EXPECT_TRUE(string(tsst4) == tsst4.toString());

    // check to make sure that operator<< contains the correct string
    stringstream ss;
    ss << tsst3;
    EXPECT_TRUE(ss.str() == test_string3); 
}

TEST(SharedStringTest, booleans)
{
    // make sure that underlying reference key is different
    TestSharedString tsst1_key(tsst1), tsst2_key(tsst2);
    EXPECT_FALSE(tsst1_key() == tsst2_key());

    // make sure that operator< gives the correct result
    EXPECT_TRUE(tsst1 < tsst2);

    // make sure that comparisons to strings work
    EXPECT_TRUE(tsst1 == test_string1);
    EXPECT_TRUE(tsst1 != test_string2);
}

TEST(SharedStringTest, construction_assignment)
{
    // make sure that default constructor works
    EXPECT_TRUE(tsst0.empty());

    // make sure that std::string/c-string constructor works
    EXPECT_FALSE(tsst1.empty());
   
    // make sure that copy constructor works
    SharedString tsst4(tsst2);
    EXPECT_FALSE(tsst4.empty());

    // make sure that copy assignment works
    SharedString tsst5;
    tsst5 = tsst4;
    EXPECT_FALSE(tsst5.empty()); 
}

int main(int argc, char** argv)
{
    InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}
