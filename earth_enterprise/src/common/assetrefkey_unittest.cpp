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
#include "khTypes.h"
#include <array>
#include <string>

using namespace std;

TEST(AssetRefKeyTest, empty)
{
    AssetRefKey ref1("key"), ref2;
    EXPECT_TRUE(ref2.empty());
    EXPECT_FALSE(ref1.empty());
}

TEST(AssetRefKeyTest, copy)
{
    AssetRefKey ref1("copy"), ref2(ref1), ref3 = ref1;
    EXPECT_TRUE(ref1 == ref2);
    EXPECT_TRUE(ref1 == ref3);
    EXPECT_TRUE(ref2 == ref3);
}

TEST(AssetRefKeyTest, StringAndConstChar)
{
    std::string val { "StringAndConstChar" };
    AssetRefKey ref1(val), ref2(val.c_str()), ref3 = val, ref4 = val.c_str();

    //compare strings
    EXPECT_TRUE(ref1 == ref3);
    //compare const char*
    EXPECT_TRUE(ref2 == ref4);
    //compare string to const char*
    EXPECT_TRUE(ref1 == ref2 && ref3 == ref4);
}

TEST(AssetRefKeyTest, Moves)
{
    std::string val1 { "Moves1" }, val2 { "Moves2" };
    AssetRefKey ref1(val1), ref2(std::move(ref1));
    EXPECT_TRUE(ref2 == val1);
    EXPECT_TRUE(ref2 == val1.c_str());
    AssetRefKey ref3, ref4;
    ref3 = std::move(ref2);
    ref4 = std::move(val2);
    EXPECT_TRUE(ref3 == std::string("Moves1") && ref3 == "Moves1");
    EXPECT_TRUE(ref4 == std::string("Moves2") && ref4 == "Moves2");
}

TEST(AssetRefKeyTest, Logic)
{
    string str1 { "str1" }, str2 { "str2" };
    AssetRefKey ref1(str1), ref2(str2);
    //comparing like
    EXPECT_FALSE(ref1 == ref2);
    EXPECT_TRUE(ref1 < ref2);
    EXPECT_TRUE(ref2 > ref1);
    //compare to string
    EXPECT_FALSE(ref1 == str2);
    EXPECT_TRUE(ref1 < str2);
    EXPECT_TRUE(ref2 > str1);
    //compare to const char*
    EXPECT_FALSE(ref1 == str2.c_str());
    EXPECT_TRUE(ref1 < str2.c_str());
    EXPECT_TRUE(ref2 > str1.c_str());
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
