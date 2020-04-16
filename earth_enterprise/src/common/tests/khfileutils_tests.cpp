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


#include <gtest/gtest.h>
#include "common/khFileUtils.h"
#include "common/khGuard.h"
#include "common/khSimpleException.h"
#include "common/khStringUtils.h"

class Test_khFilesEqual : public testing::Test {
 protected:
  std::string work_dir_;
  std::string big_file_1_;
  std::string big_file_2_;

  virtual void SetUp() {
    // create one master tmp dir for all our tests to use for their output
    work_dir_ = khCreateTmpDir("filesequal_test");
    // khCreateTmpDir reports failure by returning an empty string
    EXPECT_FALSE(work_dir_.empty());

    big_file_1_ = khTmpFilename(khComposePath(work_dir_, "test"));
    big_file_2_ = khTmpFilename(khComposePath(work_dir_, "test"));


    khWriteFileCloser file_1(::open(big_file_1_.c_str(),
                                    O_WRONLY | O_CREAT | O_LARGEFILE | O_TRUNC,
                                    0600));
    if (!file_1.valid()) {
      throw khSimpleErrnoException("Unable to open ")
          << big_file_1_ << " for writing";
    }

    khWriteFileCloser file_2(::open(big_file_2_.c_str(),
                                    O_WRONLY | O_CREAT | O_LARGEFILE | O_TRUNC,
                                    0600));
    if (!file_2.valid()) {
      throw khSimpleErrnoException("Unable to open ")
          << big_file_2_ << " for writing";
    }

    // just a large number that's not an even multiple of the 1024 * 512
    // sized buffer used by khFilesEqual
    const std::uint64_t common_size = 1024 * 612 + 34;
    EXPECT_TRUE(khFillFile(file_1.fd(), 'w', common_size));
    EXPECT_TRUE(khFillFile(file_2.fd(), 'w', common_size));
    EXPECT_TRUE(khFillFile(file_1.fd(), '1', 10));
    EXPECT_TRUE(khFillFile(file_2.fd(), '2', 10));
  }

  virtual void TearDown() {
    khPruneFileOrDir(work_dir_);
  }


  void TestWithContent(const std::string &content_1,
                       const std::string &content_2,
                       bool expected) {
    khTmpFileGuard file_1(work_dir_);
    khTmpFileGuard file_2(work_dir_);
    khWriteStringToFileOrThrow(file_1.name(), content_1);
    khWriteStringToFileOrThrow(file_2.name(), content_2);

    if (expected) {
      EXPECT_TRUE(khFilesEqual(file_1.name(), file_2.name()));
    } else {
      EXPECT_FALSE(khFilesEqual(file_1.name(), file_2.name()));
    }
  }
};


TEST_F(Test_khFilesEqual, FirstMissing) {
  EXPECT_FALSE(khFilesEqual("/missing-file.txt", "/bin/true"));
}

TEST_F(Test_khFilesEqual, SecondMissing) {
  EXPECT_FALSE(khFilesEqual("/bin/true", "/missing-file.txt"));
}

TEST_F(Test_khFilesEqual, FirstNoAccess) {
  EXPECT_FALSE(khFilesEqual("/proc/kcore", "/bin/true"));
}

TEST_F(Test_khFilesEqual, SecondNoAccess) {
  EXPECT_FALSE(khFilesEqual("/bin/true", "/proc/kcore"));
}

TEST_F(Test_khFilesEqual, DifferentLengths) {
  TestWithContent("1234567", "12345678", false);
}

TEST_F(Test_khFilesEqual, DifferentContent) {
  TestWithContent("1234567", "abcdefg", false);
}

TEST_F(Test_khFilesEqual, SameContent) {
  TestWithContent("1234567", "1234567", true);
}

TEST_F(Test_khFilesEqual, FirstEmpty) {
  TestWithContent("", "1234567", false);
}

TEST_F(Test_khFilesEqual, SecondEmpty) {
  TestWithContent("1234567", "", false);
}

TEST_F(Test_khFilesEqual, BothEmpty) {
  TestWithContent("", "", true);
}

TEST_F(Test_khFilesEqual, BigEqual) {
  EXPECT_TRUE(khFilesEqual(big_file_1_, big_file_1_));
}

TEST_F(Test_khFilesEqual, BigNotEqual) {
  EXPECT_FALSE(khFilesEqual(big_file_1_, big_file_2_));
}

TEST_F(Test_khFilesEqual, UTCPath) {
  std::string input = "1970-02-01T03:12:36Z";
  struct tm ts;
  ParseUTCTime(input, &ts);

  std::string timePath = khComposeTimePath(ts);
  std::string expected_timePath("1970/02/01/11556000");
  EXPECT_EQ(expected_timePath, timePath);
}

int main(int argc, char *argv[]) {
  testing::FLAGS_gtest_catch_exceptions = true;

  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
