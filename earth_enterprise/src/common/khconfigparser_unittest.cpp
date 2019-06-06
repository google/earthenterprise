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
#include "khConfigFileParser.h"
#include <array>

using namespace std;
using namespace testing;

std::array<string, 3> options
{{
    "OPTION1",
    "OPTION2",
    "OPTION3"
}};

TEST(ConfigParserTest, no_value_present)
{
    stringstream ss;
    ss << "OPTION1=";
    khConfigFileParser testParser;
    testParser.addOption("OPTION1");
    try
    {
        testParser.parse(ss);
        FAIL() << "Expected ValueNotPresentException";
    }
    catch (const ValueNotPresentException& e) {}
}

TEST(ConfigParserTest, key_not_present)
{
    stringstream ss;
    ss << "OPTION4=4\n";
    khConfigFileParser testParser;
    testParser.addOption("OPTION1");
    try
    {
        testParser.parse(ss);
    }
    catch (...) {
        FAIL() << "Should ignore invalid options";
    }
}

TEST(ConfigParserTest, check_data)
{
    khConfigFileParser testParser;
    stringstream ss;
    testParser.addOption("OPTION1");
    testParser.addOption("OPTION2");
    testParser.addOption("OPTION3");
    ss << "OPTION1=1" << endl
       << "OPTION2=2" << endl
       << "OPTION3=3" << endl;
    testParser.parse(ss);
    try
    {
        EXPECT_EQ("1", testParser.at("OPTION1"));
        EXPECT_EQ("2", testParser["OPTION2"]);
        EXPECT_EQ("3", testParser["OPTION3"]);
    }
    catch (const KeyNotPresentException& e)
    {
        FAIL() << "Exception thrown: " << e.what();
    }
    try
    {
        auto temp = testParser.at("NOT_PRESENT");
        FAIL() << "Expected KeyNotPresentException";
    }
    catch (const KeyNotPresentException& e){}
}

TEST(ConfigParserTest, validate_integer_data)
{
    khConfigFileParser testParser;
    stringstream ss1, ss2;
    for (const auto& a : options)
    {
        testParser.addOption(a);
        ss1 << a.c_str() << "=1" << endl;
        ss2 << a.c_str() << "=text" << endl;
    }
    try
    {
        testParser.parse(ss1);
        EXPECT_EQ(3, testParser.size());
    }
    catch (const khConfigFileParserException& e)
    {
        FAIL() << "Exception thrown: " << e.what();
    }
    try
    {
        testParser.parse(ss2);
        FAIL() << "Expected ValidateIntegersException";
    }
    catch (const ValidateIntegersException& e) {}
}

TEST(ConfigParserTest, ignore_comments_and_empty_lines)
{
    khConfigFileParser testParser;
    stringstream ss;
    for (const auto& a : options)
        testParser.addOption(a);
    ss << "# commented out line" << endl
       << endl
       << "\n\t\t# another commented out line with whitespace"
       << endl << "OPTION1=2 # comment after valid data, will still accept"
       << endl;
    try
    {
        testParser.parse(ss);
        EXPECT_EQ(1, testParser.size());
    }
    catch (const khConfigFileParserException& e)
    {
        FAIL() << "Exception thrown: " << e.what();
    }
}


TEST(ConfigParserTest, no_file_no_options)
{
    khConfigFileParser testParser;
    string fn {"absentFile.cfg"};
    try
    {
        testParser.parse(fn);
        FAIL() << "Expected OptionsEmptyException";
    }
    catch (const OptionsEmptyException& e) {}
    testParser.addOption("OPTION1");
    try
    {
        testParser.parse(fn);
        FAIL() << "Expected FileNotPresentException";
    }
    catch (const FileNotPresentException& e) {}
}



int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
