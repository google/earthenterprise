// Copyright 2017 Google Inc.
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

//
// Unit tests for geFileUtils.cpp

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string>
#include "common/geFileUtils.h"
#include "common/khFileUtils.h"
#include "common/UnitTest.h"


/**
 * Helper class for creating files in temporary directories.
 */
class geTmpPath {
  private:
    std::string path_;
    std::string dir_;

  public:
    explicit geTmpPath(std::string fname) {
      char tmpDir[128];
      snprintf(tmpDir, sizeof(tmpDir), "/tmp/khXXXXXX");
      (void) mkdtemp(tmpDir);
      dir_ = tmpDir;

      path_ = dir_ + "/" + fname;
    }

    ~geTmpPath() {
      khPruneFileOrDir(path_);
      khPruneFileOrDir(dir_);
    }

    std::string path() {
      return path_;
    }
};


/**
 * Main testing class for FileUtils.
 */
class geFileUtilsTest: public UnitTest<geFileUtilsTest> {
  public:

  bool geLocalBufferWriteFileTester() {
    const char buff[] = {'H', 'i'};
    const size_t kBuffSize = sizeof(buff);
    const mode_t st_mode = 0660;

    geTmpPath fileName("a/b/c.txt");
    for (unsigned i = 0; i < 2; ++i) {
      {
        geLocalBufferWriteFile proxyFile;
        (void) ::umask(i);
        proxyFile.Open(fileName.path(), O_RDWR|O_CREAT|O_TRUNC, st_mode);
        (void) ::umask(077);
        int fd = proxyFile.FileId();
        ::write(fd, buff, 2);
      }

      struct stat statBuf;
      stat(fileName.path().c_str(), &statBuf);
      EXPECT_EQ(st_mode & ~i, statBuf.st_mode & 07777);
      EXPECT_EQ(kBuffSize, (size_t)statBuf.st_size);
      FILE *fp = fopen(fileName.path().c_str(), "r");
      char expected_buff[kBuffSize];
      size_t num_read = fread(expected_buff, 1, kBuffSize, fp);
      EXPECT_EQ(num_read, kBuffSize);
      EXPECT_EQ(memcmp(expected_buff, buff, kBuffSize), 0);
      fclose(fp);
    }

    return true;
  }

  bool geLocalBufferWriteTesterLargeFile() {
    const size_t kBuffSize = 4096;
    const size_t kTestFileSize = 1024 * 1024;  // this value * 4kB = 4GB
    char buff[kBuffSize];
    const mode_t st_mode = 0660;

    geTmpPath fileName("big.txt");
    // Create 4k buffer
    for (size_t i = 0; i < kBuffSize; i++) {
      // Test all possible characters
      buff[i] = static_cast<char>(i & 0xff);
    }

    {
      geLocalBufferWriteFile proxyFile;
      proxyFile.Open(fileName.path(), O_RDWR|O_CREAT|O_TRUNC, st_mode);
      int fd = proxyFile.FileId();

      // Write 4GB file
      for (size_t i = 0; i < kTestFileSize; i++) {
        ::write(fd, buff, kBuffSize);
      }
    }

    struct stat statBuf;
    stat(fileName.path().c_str(), &statBuf);
    // Check file size
    EXPECT_EQ(kBuffSize * kTestFileSize, (size_t) statBuf.st_size);

    FILE *fp = fopen(fileName.path().c_str(), "r");
    char expected_buff[kBuffSize];

    // Test all 4GB of data itself
    for (size_t i = 0; i < kTestFileSize; i++) {
      size_t num_read = fread(expected_buff, 1, kBuffSize, fp);
      EXPECT_EQ(num_read, kBuffSize);
      EXPECT_EQ(memcmp(expected_buff, buff, kBuffSize), 0);
    }

    fclose(fp);

    return true;
  }

  bool geLocalBufferWriteFileTesterBadDstFile() {
    const mode_t st_mode = 0660;
    geLocalBufferWriteFile proxyFile;
    try {
      proxyFile.Open("", O_RDWR|O_CREAT|O_TRUNC, st_mode);
      return false;
    } catch(const std::exception &e) {
      // Expected
    }

    return true;
  }

  bool geLocalBufferWriteFileTesterDoubleOpen() {
    const mode_t st_mode = 0660;
    geLocalBufferWriteFile proxyFile;

    geTmpPath fileName("a/b/d.txt");
    geTmpPath file2Name("a/b/e.txt");

    proxyFile.Open(fileName.path(), O_RDWR|O_CREAT|O_TRUNC, st_mode);
    try {
      proxyFile.Open(file2Name.path(), O_RDWR|O_CREAT|O_TRUNC, st_mode);
      return false;
    } catch(const std::exception &e) {
      // Expected
    }

    // First file should be created, but second should not be
    struct stat statBuf;
    EXPECT_EQ(stat(fileName.path().c_str(), &statBuf), 0);
    EXPECT_TRUE(stat(file2Name.path().c_str(), &statBuf) != 0);

    return true;
  }

  bool geRelativePathTester() {
    // from, to, relative_path triplets
    const char* golden[8][3] = {
      {"/", "/a/b/c", "a/b/c"},
      {"/a/b/c", "/", "../../.."},
      {"/a", "/a/b/c", "b/c"},
      {"/a/d/", "/a/b/c", "../b/c"},
      {"/a/d", "/a/b/c", "../b/c"},
      {"/a/d/../d", "/a/b/c", "../b/c"},
      {"/a//d", "/a/b/c", "../b/c"},
      {"/a/b/c/d", "/a/b/c/d", ""}
    };
    for (int i = 0; i < 8; ++i) {
      EXPECT_EQ(golden[i][2], khRelativePath(golden[i][0], golden[i][1]));
    }
    return true;
  }

  geFileUtilsTest(void) : BaseClass("geFileUtilsTest") {
    REGISTER(geLocalBufferWriteFileTester);
    REGISTER(geLocalBufferWriteFileTesterBadDstFile);
    REGISTER(geLocalBufferWriteFileTesterDoubleOpen);
    REGISTER(geRelativePathTester);
    REGISTER(geLocalBufferWriteTesterLargeFile);
  }
};

int main(int argc, char *argv[]) {
  geFileUtilsTest tests;

  return tests.Run();
}
