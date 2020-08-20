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

#include <thread>
#include <chrono>

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

TEST(FileAccessorTest, validate_getfileinfo) {
    string filename = "/tmp/TestFile.txt";

    time_t timeBeforeCreation = std::time(nullptr);

    // Without a sleep here the difference between file creation time, modification
    // time, and closing time would be too small to measure using time_t
    auto sleepTime = std::chrono::seconds(1);
    std::this_thread::sleep_for(sleepTime);

    unique_ptr<AbstractFileAccessor> aFA = AbstractFileAccessor::getAccessor(filename);

    const size_t fileSize = 200;
    string theData = string(fileSize, 'X');

    aFA->Open(filename, "w");
    aFA->PwriteAll(theData.data(), fileSize, 0);
    aFA->Close();

    // Without a sleep here, the difference between file creation time, modification
    // time, and closing time would be too small to measure
    std::this_thread::sleep_for(sleepTime);

    time_t timeAfterClose = std::time(nullptr);

    uint64_t size = 0;
    time_t mtime = 0;
    
    bool result = aFA->GetFileInfo(filename, size, mtime);
    double creationToModification = std::difftime(mtime, timeBeforeCreation);
    double modificationToClose = std::difftime(timeAfterClose, mtime);
    std::cout << "\n\ncreationToModification = " << creationToModification <<
        " modificationToClose = " << modificationToClose << "\n\n";
    EXPECT_TRUE(result);
    EXPECT_EQ(size, fileSize);

    // Check that the modification time is after the creation time, but before
    // the close time.
    EXPECT_TRUE(creationToModification > 0.0);
    EXPECT_TRUE(modificationToClose > 0.0);
    remove(filename.c_str());
}


int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
