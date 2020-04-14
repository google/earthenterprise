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

// Unit tests for geFilePool

#include <sys/resource.h>
#include <functional>
#include <UnitTest.h>
#include <geFilePool.h>
#include <khFileUtils.h>
#include <khThread.h>
#include <khSimpleException.h>
#include <notify.h>

class geFilePoolUnitTest : public UnitTest<geFilePoolUnitTest> {
 public:
  static const geFilePool::Writer::WriteStyle kWriteStyle =
    geFilePool::Writer::WriteOnly;
  static const geFilePool::Writer::TruncateStyle kTruncateStyle =
    geFilePool::Writer::Truncate;

  class RLimitGuard {                   // set system limit, restore on destruct
   public:
    RLimitGuard(int resource, rlim_t new_limit_value)
        : resource_(resource) {
      int status = getrlimit(resource_, &restore_val_);
      if (status != 0)
        throw khSimpleErrnoException("RLimitGuard: error from getrlimit");
      rlimit new_val;
      new_val = restore_val_;
      new_val.rlim_cur = new_limit_value;
      status = setrlimit(resource_, &new_val);
      if (status != 0)
        throw khSimpleErrnoException("RLimitGuard: error from setrlimit");
    }
    ~RLimitGuard() {
      setrlimit(resource_, &restore_val_);
    }
   private:
    int resource_;                      // set/get rlimit resource ID
    struct rlimit restore_val_;         // value to restore on destruct
    DISALLOW_COPY_AND_ASSIGN(RLimitGuard);
  };

  bool TestWrite() {
    khTmpFileGuard tmpfile;// makes a tmpfile and deletes it when we leave
    std::string testdata("01234567890");
    geFilePool theFilePool(1);

    try {
      geFilePool::Writer writer(kWriteStyle,
                                kTruncateStyle,
                                theFilePool,
                                tmpfile.name());
      writer.Pwrite(testdata.data(), testdata.size(), 0);
      writer.SyncAndClose();
    } catch (const std::exception &e) {
      fprintf(stderr, "Exception during TestWrite: %s\n", e.what());
      return false;
    }

    std::string readString;
    if (!khReadStringFromFile(tmpfile.name(), readString)) {
      fprintf(stderr, "Unable to re-read data during TestWrite\n");
      return false;
    }
    if (testdata != readString) {
      fprintf(stderr, "Re-read data differs during TestWrite: '%s' != '%s'\n",
              readString.c_str(), testdata.c_str());
      return false;
    }
    return true;
  }

  bool TestRead() {
    khTmpFileGuard tmpfile;// makes a tmpfile and deletes it when we leave
    std::string testdata("01234567890");
    geFilePool theFilePool(1);

    if (!khWriteStringToFile(tmpfile.name(), testdata)) {
      fprintf(stderr, "Unable to seed data during TestRead\n");
      return false;
    }

    std::string readString;
    readString.resize(testdata.size());
    try {
      geFilePool::Reader reader(theFilePool, tmpfile.name());
      reader.Pread(&readString[0], testdata.size(), 0);
    } catch (const std::exception &e) {
      fprintf(stderr, "Exception during TestRead: %s\n", e.what());
      return false;
    }

    if (testdata != readString) {
      fprintf(stderr, "Read data differs during TestRead: '%s' != '%s'\n",
              readString.c_str(), testdata.c_str());
      return false;
    }
    return true;
  }

  bool TestWriteRead() {
    khTmpFileGuard tmpfile;// makes a tmpfile and deletes it when we leave
    std::string testdata("01234567890");
    geFilePool theFilePool(1);

    std::string readString;
    readString.resize(testdata.size());
    try {
      {
        geFilePool::Writer writer(kWriteStyle,
                                  kTruncateStyle,
                                  theFilePool,
                                  tmpfile.name());
        writer.Pwrite(testdata.data(), testdata.size(), 0);
        writer.SyncAndClose();
      }

      geFilePool::Reader reader(theFilePool, tmpfile.name());
      reader.Pread(&readString[0], testdata.size(), 0);
    } catch (const std::exception &e) {
      fprintf(stderr, "Exception during TestWriteRead: %s\n", e.what());
      return false;
    }

    if (testdata != readString) {
      fprintf(stderr, "Read data differs during TestWriteRead: '%s' != '%s'\n",
              readString.c_str(), testdata.c_str());
      return false;
    }
    return true;
  }

  bool TestOffsetWriteRead() {
    khTmpFileGuard tmpfile;// makes a tmpfile and deletes it when we leave
    std::string testdata("01234567890");
    geFilePool theFilePool(1);
    off64_t offset = 100;

    std::string readString;
    readString.resize(testdata.size());
    try {
      {
        geFilePool::Writer writer(kWriteStyle,
                                  kTruncateStyle,
                                  theFilePool,
                                  tmpfile.name());
        writer.Pwrite(testdata.data(), testdata.size(), offset);
        writer.SyncAndClose();
      }

      {
        std::uint64_t size;
        time_t mtime;
        if (!khGetFileInfo(tmpfile.name(), size, mtime)) {
          fprintf(stderr,
                  "Unable to get filesize during TestOffsetWriteRead\n");
          return false;
        }
        if (std::int64_t(size) != std::int64_t(offset + testdata.size())) {
          fprintf(stderr,
                  "File is wrong size during TestOffsetWriteRead: "
                  "%lld != %lld\n",
                  (long long int)size,
                  (long long int)(offset + testdata.size()));
          return false;
        }
      }

      geFilePool::Reader reader(theFilePool, tmpfile.name());
      reader.Pread(&readString[0], testdata.size(), offset);
    } catch (const std::exception &e) {
      fprintf(stderr, "Exception during TestOffsetWriteRead: %s\n", e.what());
      return false;
    }

    if (testdata != readString) {
      fprintf(stderr,
              "Read data differs during TestOffsetWriteRead: '%s' != '%s'\n",
              readString.c_str(), testdata.c_str());
      return false;
    }
    return true;

  }

  bool TestWriteReadSimple() {
    khTmpFileGuard tmpfile;// makes a tmpfile and deletes it when we leave
    std::string testdata("01234567890");
    geFilePool theFilePool(1);

    std::string readString;
    readString.resize(testdata.size());
    try {
      theFilePool.WriteSimpleFile(tmpfile.name(), testdata.data(),
                                  testdata.size());
      theFilePool.ReadSimpleFile(tmpfile.name(), &readString[0],
                                 testdata.size());
    } catch (const std::exception &e) {
      fprintf(stderr, "Exception during TestWriteReadSimple: %s\n", e.what());
      return false;
    }

    if (testdata != readString) {
      fprintf(stderr,
              "Read data differs during TestWriteReadSimple: '%s' != '%s'\n",
              readString.c_str(), testdata.c_str());
      return false;
    }
    return true;

  }

  bool TestFdSharing() {
    khTmpFileGuard tmpfile1;// makes a tmpfile and deletes it when we leave
    khTmpFileGuard tmpfile2;// makes a tmpfile and deletes it when we leave
    khTmpFileGuard tmpfile3;// makes a tmpfile and deletes it when we leave
    std::string testdata1("01234567890");
    std::string testdata2("3333333333333333333");
    std::string testdata3("76876876786876");
    unsigned int fdlimit = 2;
    geFilePool theFilePool(fdlimit);

    if (!khWriteStringToFile(tmpfile1.name(), testdata1)) {
      fprintf(stderr, "Unable to seed data during TestFdSharing\n");
      return false;
    }
    if (!khWriteStringToFile(tmpfile2.name(), testdata2)) {
      fprintf(stderr, "Unable to seed data during TestFdSharing\n");
      return false;
    }
    if (!khWriteStringToFile(tmpfile3.name(), testdata3)) {
      fprintf(stderr, "Unable to seed data during TestFdSharing\n");
      return false;
    }

    geFilePool::Reader reader1(theFilePool, tmpfile1.name());
    geFilePool::Reader reader2(theFilePool, tmpfile2.name());
    geFilePool::Reader reader3(theFilePool, tmpfile3.name());
    geFilePool::Reader reader4(theFilePool, tmpfile1.name());

    std::string readString1;
    readString1.resize(testdata1.size());
    std::string readString2;
    readString2.resize(testdata2.size());
    std::string readString3;
    readString3.resize(testdata3.size());
    std::string readString4;
    readString4.resize(testdata1.size());
    try {
      for (unsigned int i = 0 ; i < 100 ; ++i) {
        reader1.Pread(&readString1[0], testdata1.size(), 0);
        reader2.Pread(&readString2[0], testdata2.size(), 0);
        reader3.Pread(&readString3[0], testdata3.size(), 0);
        reader4.Pread(&readString4[0], testdata1.size(), 0);

        if ((testdata1 != readString1) ||
            (testdata2 != readString2) ||
            (testdata3 != readString3) ||
            (testdata1 != readString4)) {
          fprintf(stderr, "Read data differs during TestFdSharing\n");
          return false;
        }
      }
    } catch (const std::exception &e) {
      fprintf(stderr, "Exception during TestFdSharing: %s\n", e.what());
      return false;
    }

    if (theFilePool.MaxFdsUsed() > fdlimit) {
      fprintf(stderr, "TestFdSharing: FilePool used too many fds: %d > %d\n",
              (int)theFilePool.MaxFdsUsed(), fdlimit);
      return false;
    }
    if (theFilePool.MaxFdsUsed() < fdlimit) {
      fprintf(stderr, "TestFdSharing: FilePool used too few fds: %d < %d\n",
              (int)theFilePool.MaxFdsUsed(), fdlimit);
      return false;
    }

    return true;

  }

  bool TestFdSharing2() {
    khTmpFileGuard tmpfile1;// makes a tmpfile and deletes it when we leave
    khTmpFileGuard tmpfile2;// makes a tmpfile and deletes it when we leave
    std::string testdata1("01234567890");
    std::string testdata2("3333333333333333333");
    unsigned int fdlimit = 1;
    geFilePool theFilePool(fdlimit);

    if (!khWriteStringToFile(tmpfile1.name(), testdata1)) {
      fprintf(stderr, "Unable to seed data during TestFdSharing2\n");
      return false;
    }
    if (!khWriteStringToFile(tmpfile2.name(), testdata2)) {
      fprintf(stderr, "Unable to seed data during TestFdSharing2\n");
      return false;
    }


    std::string readString1;
    readString1.resize(testdata1.size());
    std::string readString2;
    readString2.resize(testdata2.size());
    std::string readString3;
    readString3.resize(testdata1.size());
    try {
      for (unsigned int i = 0 ; i < 100 ; ++i) {
        geFilePool::Reader reader1(theFilePool, tmpfile1.name());
        reader1.Pread(&readString1[0], testdata1.size(), 0);
        geFilePool::Reader reader2(theFilePool, tmpfile2.name());
        reader2.Pread(&readString2[0], testdata2.size(), 0);
        geFilePool::Reader reader3(theFilePool, tmpfile1.name());
        reader3.Pread(&readString3[0], testdata1.size(), 0);

        if ((testdata1 != readString1) ||
            (testdata2 != readString2) ||
            (testdata1 != readString3)) {
          fprintf(stderr, "Read data differs during TestFdSharing2\n");
          return false;
        }
      }
    } catch (const std::exception &e) {
      fprintf(stderr, "Exception during TestFdSharing2: %s\n", e.what());
      return false;
    }

    if (theFilePool.MaxFdsUsed() > fdlimit) {
      fprintf(stderr, "TestFdSharing2: FilePool used too many fds: %d > %d\n",
              (int)theFilePool.MaxFdsUsed(), fdlimit);
      return false;
    }

    return true;

  }

  static void WriteAndDelayedClose(geFilePool *pool,
                                   const std::string &fname) {
    std::string data("7698769767");
    try {
      fprintf(stderr, "\tWriteAndDelayedClose: Creating writer ...\n");
      geFilePool::Writer writer(kWriteStyle, kTruncateStyle,*pool, fname);
      fprintf(stderr, "\tWriteAndDelayedClose: Create done, Calling Pwrite\n");
      writer.Pwrite(data.data(), data.size(), 0);
      fprintf(stderr, "\tWriteAndDelayedClose: Pwrite done, sleeping ...\n");
      sleep(10);
      fprintf(stderr, "\tWriteAndDelayedClose: Sleep done, closing ...\n");
      writer.SyncAndClose();
      fprintf(stderr, "\tWriteAndDelayedClose: Close done\n");
    } catch (std::exception &e) {
      fprintf(stderr, "Exception in thread, can't propagate: %s\n", e.what());
    }
  }
  static void DelayedWriteAndClose(geFilePool *pool,
                                   const std::string &fname) {
    std::string data("ssdfdgfdgfgfdgf");
    try {
      fprintf(stderr, "\tDelayedWriteAndClose: Sleeping...\n");
      sleep(5);
      fprintf(stderr,"\tDelayedWriteAndClose: Sleep done, creating ...\n");
      geFilePool::Writer writer(kWriteStyle, kTruncateStyle, *pool, fname);
      fprintf(stderr,"\tDelayedWriteAndClose: Create done, calling Pwrite\n");
      writer.Pwrite(data.data(), data.size(), 0);
      fprintf(stderr, "\tDelayedWriteAndClose: Pwrite done, closing ...\n");
      writer.SyncAndClose();
      fprintf(stderr, "\tDelayedWriteAndClose: Close done\n");
    } catch (std::exception &e) {
      fprintf(stderr, "Exception in thread, can't propagate: %s\n", e.what());
    }
  }

  bool TestWriteFdLimit() {
    khTmpFileGuard tmpfile1;// makes a tmpfile and deletes it when we leave
    khTmpFileGuard tmpfile2;// makes a tmpfile and deletes it when we leave
    unsigned int fdlimit = 1;
    geFilePool theFilePool(fdlimit);

    try {
      khThread thread1(khFunctor<void>(std::ptr_fun(&WriteAndDelayedClose),
                                       &theFilePool, tmpfile1.name()));
      khThread thread2(khFunctor<void>(std::ptr_fun(&DelayedWriteAndClose),
                                       &theFilePool, tmpfile2.name()));
      thread1.run();
      thread2.run();

      // leaving scope will join with threads
    } catch (const std::exception &e) {
      fprintf(stderr, "Exception during TestWriteFdLimit: %s\n", e.what());
      return false;
    }

    if (theFilePool.MaxFdsUsed() > fdlimit) {
      fprintf(stderr,
              "TestWriteFdLimit: FilePool used too many fds: %d > %d\n",
              (int)theFilePool.MaxFdsUsed(), fdlimit);
      return false;
    }

    return true;

  }


  static void ThreadRead1(geFilePool *pool,
                         const std::string &fname) {
    try {
      std::uint32_t size = 1024UL*1024UL*100UL;
      std::vector<char> buf(size);

      fprintf(stderr, "\tThreadRead1: Creating reader ...\n");
      geFilePool::Reader reader(*pool, fname);
      fprintf(stderr, "\tThreadRead1: Create done, Calling Pread\n");
      reader.Pread(&buf[0], size, 0);
      fprintf(stderr, "\tThreadRead1: Pread done\n");
    } catch (std::exception &e) {
      fprintf(stderr, "Exception in thread, can't propagate: %s\n", e.what());
    }
  }
  static void ThreadRead2(geFilePool *pool,
                         const std::string &fname) {
    try {
      std::uint32_t size = 1024UL*1024UL*100UL;
      std::vector<char> buf(size);

      fprintf(stderr, "\tThreadRead2: Creating reader ...\n");
      geFilePool::Reader reader(*pool, fname);
      fprintf(stderr, "\tThreadRead2: Create done, Calling Pread\n");
      reader.Pread(&buf[0], size, 0);
      fprintf(stderr, "\tThreadRead2: Pread done\n");
    } catch (std::exception &e) {
      fprintf(stderr, "Exception in thread, can't propagate: %s\n", e.what());
    }
  }

  bool TestThreadedReadFdLimit() {
    unsigned int fdlimit = 1;
    geFilePool theFilePool(fdlimit);
    std::uint32_t size = 1024UL*1024UL*100UL;
    khTmpFileGuard tmpfile1;// makes a tmpfile and deletes it when we leave
    khTmpFileGuard tmpfile2;// makes a tmpfile and deletes it when we leave
    {
      geFilePool::Writer writer(kWriteStyle,
                                kTruncateStyle,
                                theFilePool,
                                tmpfile1.name());
      writer.Pwrite("1", 1, size);
      writer.SyncAndClose();
    }
    {
      geFilePool::Writer writer(kWriteStyle,
                                kTruncateStyle,
                                theFilePool,
                                tmpfile2.name());
      writer.Pwrite("1", 1, size);
      writer.SyncAndClose();
    }

    try {
      khThread thread1(khFunctor<void>(std::ptr_fun(&ThreadRead1),
                                       &theFilePool, tmpfile1.name()));
      khThread thread2(khFunctor<void>(std::ptr_fun(&ThreadRead2),
                                       &theFilePool, tmpfile2.name()));
      thread1.run();
      thread2.run();

      // leaving scope will join with threads
    } catch (const std::exception &e) {
      fprintf(stderr, "Exception during TestWriteFdLimit: %s\n", e.what());
      return false;
    }

    if (theFilePool.MaxFdsUsed() > fdlimit) {
      fprintf(stderr,
              "TestWriteFdLimit: FilePool used too many fds: %d > %d\n",
              (int)theFilePool.MaxFdsUsed(), fdlimit);
      return false;
    }

    return true;

  }


  bool TestWriteAndReadFromWriter() {
    khTmpFileGuard tmpfile;// makes a tmpfile and deletes it when we leave
    std::string testdata("01234567890");
    geFilePool theFilePool(1);

    std::string readString;
    readString.resize(testdata.size());
    try {
      geFilePool::Writer writer(geFilePool::Writer::ReadWrite,
                                geFilePool::Writer::Truncate,
                                theFilePool, tmpfile.name());
      writer.Pwrite(testdata.data(), testdata.size(), 0);

      writer.Pread(&readString[0], testdata.size(), 0);

      writer.SyncAndClose();
    } catch (const std::exception &e) {
      fprintf(stderr, "Exception during TestWriteAndReadFromWriter: %s\n",
              e.what());
      return false;
    }

    if (testdata != readString) {
      fprintf(stderr, "Read data differs during TestWriteAndReadFromWriter"
              ": '%s' != '%s'\n", readString.c_str(), testdata.c_str());
      return false;
    }
    return true;
  }

  bool TestClearTrunc() {
    khTmpFileGuard tmpfile;// makes a tmpfile and deletes it when we leave
    khTmpFileGuard tmpfile2;// makes a tmpfile and deletes it when we leave
    std::string testdata("01234567890");
    std::string result = testdata + testdata;
    geFilePool theFilePool(1);

    try {
      geFilePool::Writer writer(kWriteStyle,
                                kTruncateStyle,
                                theFilePool,
                                tmpfile.name());
      writer.Pwrite(testdata.data(), testdata.size(), 0);
      geFilePool::Writer writer2(kWriteStyle,
                                 kTruncateStyle,
                                 theFilePool,
                                 tmpfile2.name());
      writer2.Pwrite(testdata.data(), testdata.size(), 0);
      writer.Pwrite(testdata.data(), testdata.size(), testdata.size());


      writer.SyncAndClose();
      writer2.SyncAndClose();
    } catch (const std::exception &e) {
      fprintf(stderr, "Exception during TestClearTrunc: %s\n", e.what());
      return false;
    }

    if (theFilePool.MaxFdsUsed() > 1) {
      fprintf(stderr,
              "TestClearTrunc: FilePool used too many fds: %d > %d\n",
              (int)theFilePool.MaxFdsUsed(), 1);
      return false;
    }

    std::string readString;
    if (!khReadStringFromFile(tmpfile.name(), readString)) {
      fprintf(stderr, "Unable to re-read data during TestClearTrunc\n");
      return false;
    }
    if (result != readString) {
      fprintf(stderr, "Re-read data bad during TestClearTrunc: '%s' != '%s'\n",
              readString.c_str(), result.c_str());
      return false;
    }
    return true;
  }

  bool TestOSFdLimit() {
    // This test sets the operating system file limit to a low value,
    // and then opens more files than the limit to make sure we're
    // observing it.
    std::uint32_t open_fd_count = khCountFilesInDir("/proc/self/fd");
    if (open_fd_count == 0) {
      notify(NFY_WARN,
             "TestOSFdLimit: khCountFilesInDir(\"/proc/self/fd\") returned 0");
    } else {
      notify(NFY_DEBUG, "Open fd count = %u",
             static_cast<unsigned>(open_fd_count));
    }
    const int kFdLimit = 5 + open_fd_count; // set OS limit to this
    const int kTestFds = 5 + kFdLimit;      // open this many files
    RLimitGuard fd_guard(RLIMIT_NOFILE, kFdLimit);
    notify(NFY_DEBUG, "kFdLimit = %d, kTestFds = %d",
           kFdLimit, kTestFds);

    geFilePool theFilePool(kFdLimit - open_fd_count - 1);
    khDeletingVector<khTmpFileGuard> tmpfiles;
    khDeletingVector<geFilePool::Writer> writers;

    // Open all the files for writing at the same time.

    for (int i = 0; i < kTestFds; ++i) {
      tmpfiles.push_back(new khTmpFileGuard);
      notify(NFY_DEBUG, "File %d: %s", i, tmpfiles[i]->name().c_str());
      writers.push_back(new geFilePool::Writer(geFilePool::Writer::ReadWrite,
                                               geFilePool::Writer::Truncate,
                                               theFilePool,
                                               tmpfiles[i]->name()));
      writers[i]->Pwrite(tmpfiles[i]->name().c_str(),
                         tmpfiles[i]->name().size(),
                         0);
    }

    // Write more data to each file

    for (int i = 0; i < kTestFds; ++i) {
      writers[i]->Pwrite(tmpfiles[i]->name().c_str(),
                         tmpfiles[i]->name().size(),
                         tmpfiles[i]->name().size());
    }

    // Read back and check data from each file

    for (int i = 0; i < kTestFds; ++i) {
      std::string buffer(tmpfiles[i]->name().size()*2, 0);
      writers[i]->Pread(buffer, buffer.size(), 0);
      if (buffer != (tmpfiles[i]->name() + tmpfiles[i]->name())) {
        notify(NFY_WARN, "TestOSFdLimit: bad compare on file %d: %s",
               i, tmpfiles[i]->name().c_str());
        return false;
      }
    }

    // Close the files

    for (int i = 0; i < kTestFds; ++i) {
      writers[i]->SyncAndClose();
    }

    return true;
  }

  geFilePoolUnitTest(void) : BaseClass("geFilePool") {
    REGISTER(TestWrite);
    REGISTER(TestRead);
    REGISTER(TestWriteRead);
    REGISTER(TestOffsetWriteRead);
    REGISTER(TestWriteReadSimple);
    REGISTER(TestWriteAndReadFromWriter);
    REGISTER(TestClearTrunc);
    REGISTER(TestFdSharing);
    REGISTER(TestFdSharing2);
    REGISTER(TestThreadedReadFdLimit);
    REGISTER(TestWriteFdLimit);
    REGISTER(TestOSFdLimit);
  }
};

int main(int argc, char *argv[]) {
  geFilePoolUnitTest tests;

  return tests.Run();
}
