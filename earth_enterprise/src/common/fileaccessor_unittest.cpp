#include <gtest/gtest.h>
#include "FileAccessor.h"

using namespace std;
using namespace testing;

TEST(FileAccessorTest, validate_lines_from_file) {
    string filename = "/tmp/TestFile.txt";
    vector<string> lines;
    unique_ptr<AbstractFileAccessor> aFA = AbstractFileAccessor::getAccessor(filename);

    aFA->Open(filename, "w");
    assert (aFA->getFD() != -1);
    aFA->Close();
    assert (aFA->Close() == -1);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}