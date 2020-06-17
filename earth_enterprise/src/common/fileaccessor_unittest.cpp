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

#include <gtest/gtest.h>
#include "FileAccessor.h"

using namespace std;
using namespace testing;

TEST(FileAccessorTest, validate_lines_from_file) {
    string filename = "/tmp/TestFile.txt";
    string testdata = "This is a test string.\nThis is another test string.\r\n";
    vector<string> lines;
    unique_ptr<AbstractFileAccessor> aFA = AbstractFileAccessor::getAccessor(filename);

    aFA->Open(filename, "w");
    aFA->PwriteAll(testdata.data(), testdata.size(), 0);
    aFA->Close();

    aFA->GetLinesFromFile(lines, filename);
    EXPECT_EQ(lines.size(), 2);
    EXPECT_EQ(lines.at(0), "This is a test string.");
    EXPECT_EQ(lines.at(1), "This is another test string.");

    for (const auto &it : lines) {
        EXPECT_EQ(it.find_first_of("\n\r"), string::npos);
    }

    remove(filename.c_str());
}

TEST(FileAccessorTest, validate_printf) {
    string filename = "/tmp/TestFile.txt";
    string formatstring = "%s%s%d\n";
    vector<string> lines;
    unique_ptr<AbstractFileAccessor> aFA = AbstractFileAccessor::getAccessor(filename);

    aFA->Open(filename, "w");
    aFA->fprintf(formatstring.c_str(), "This is a test string.\n", "This is another test string.\n", 0);
    aFA->Close();

    aFA->GetLinesFromFile(lines, filename);
    EXPECT_EQ(lines.size(), 3);
    EXPECT_EQ(lines.at(0), "This is a test string.");
    EXPECT_EQ(lines.at(1), "This is another test string.");
    EXPECT_EQ(lines.at(2), "0");

    remove(filename.c_str());
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}