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


// Unit test for the FileBundle classes

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <iostream>
#include <khGuard.h>
#include <khFileUtils.h>
#include <khGetopt.h>
#include <notify.h>
#include <khSimpleException.h>
#include "filebundlewriter.h"
#include "filebundlereader.h"
#include <UnitTest.h>

class FileBundleUnitTest : public UnitTest<FileBundleUnitTest> {
 public:
  FileBundleUnitTest()
      : BaseClass("FileBundleUnitTest") {
    REGISTER(TestBasics);
    REGISTER(TestRandomUnbuffered);
    REGISTER(TestRandomBuffered);
    REGISTER(TestExceptions);
    REGISTER(TestCRCReport);
  }

  static std::string kPathBase;
  static const size_t kMaxBuffer = 4096; // maximum buffer size (without CRC)

  // TestBasics - write, then read, a simple file bundle

  bool TestBasics() {
    static const std::string kTestString("This is a test string for TestBasics");
    static const std::string kBundleName(kPathBase + "TestBasics");
    std::uint64_t location = 0;

    fprintf(stderr, "Running TestBasics()\n");

    // Set umask to known value
    const mode_t kTestDirMode = S_IRUSR | S_IWUSR | S_IXUSR;
    const mode_t kTestFileMode = S_IRUSR | S_IWUSR;
    mode_t old_umask = umask(0);

    try {
      // Write a very simple file
      std::string header_filename;
      std::string segment0_filename;
      {
        FileBundleWriter writer(file_pool_, kBundleName, true, kTestDirMode);

        // Write record with CRC
        const size_t kRecordLength = kTestString.size() + FileBundle::kCRCsize;
        char buffer[kRecordLength];
        memcpy(buffer, kTestString.data(), kTestString.size());
        location = writer.WriteAppendCRC(buffer, kRecordLength);

        // Get file names
        header_filename = writer.HeaderPath();
        segment0_filename = writer.abs_path() + writer.Segment(0)->name();

        // Close the file
        writer.Close();
      }

      // Check permissions on created directory and files
      CheckMode(kBundleName, kTestDirMode);
      CheckMode(header_filename, kTestFileMode);
      CheckMode(segment0_filename, kTestFileMode);

      // Read it back
      {
        FileBundleReader reader(file_pool_, kBundleName);
        const size_t kRecordLength = kTestString.size() + FileBundle::kCRCsize;
        char buffer[kRecordLength];

        reader.ReadAtCRC(location, &buffer, kRecordLength);
        if (0 != memcmp(buffer, kTestString.data(), kTestString.size())) {
          throw khSimpleException("FileBundle_UnitTest::TestBasics: "
                                  "read compare error");
        }

        reader.Close();
      }

      // Read it back with using a cache
      {
        FileBundleReader reader(file_pool_, kBundleName);
        reader.EnableReadCache(2, 64 * 1024);
        const size_t kRecordLength = kTestString.size() + FileBundle::kCRCsize;
        char buffer[kRecordLength];

        reader.ReadAtCRC(location, &buffer, kRecordLength);
        if (0 != memcmp(buffer, kTestString.data(), kTestString.size())) {
          throw khSimpleException("FileBundle_UnitTest::TestBasics: "
                                  "read compare error");
        }

        reader.Close();
      }
    }
    catch (...) {
      umask(old_umask);
      throw;
    }

    umask(old_umask);
    return true;
  }

  // CheckMode - check that the specified path has the specified mode.
  // Throws exception if not, or if unable to read status.

  void CheckMode(const std::string &path, mode_t mode) {
    struct stat path_stat;
    memset(&path_stat, 0, sizeof(path_stat));

    if (stat(path.c_str(), &path_stat) == 0) {
      mode_t path_mode = path_stat.st_mode &
                         (S_IRUSR | S_IWUSR | S_IXUSR
                          | S_IRGRP | S_IWGRP | S_IXGRP
                          | S_IROTH | S_IWOTH | S_IXOTH);
      if (path_mode != mode) {
        // khSimpleException doesn't support format tags, so we
        // pre-format the exception
        std::ostringstream msg;
        msg << "FileBundleUnitTest::CheckMode: "<< std::oct << std::showbase
            << "mode " << path_mode
            << " should be " << mode << " for path " << path;
        throw khSimpleException(msg.str());
      }
    } else {
      throw khSimpleErrnoException("FileBundleUnitTest::CheckMode: path ")
        << path << " stat error: ";
    }
  }

  // TestRandom - write a large set of random blocks, read back, and
  // compare.  The segment break is set low to force multiple segments.

  struct RandomRecord {
   public:
    RandomRecord(char *buffer_, size_t buffer_size_, std::uint64_t position_)
        : buffer(buffer_),
          buffer_size(buffer_size_),
          position(position_) {
    }

    char *buffer;
    size_t buffer_size;
    std::uint64_t position;
  };

  std::uint64_t WriteRandomRecords(FileBundleWriter &writer,
                            std::vector<RandomRecord> &records,
                            int buffer_count) {
    std::uint64_t total_size = 0;
    for (int i = 0; i < buffer_count; ++i) {
      const size_t buffer_size = FileBundle::kCRCsize + 1 + static_cast<size_t>
                                 ((static_cast<float>(rand())
                                   / RAND_MAX * kMaxBuffer));
      char *buffer = new char[buffer_size];

      for (size_t j = 0; j < buffer_size; ++j) {
        buffer[j] = rand() & 0xFF;
      }

      records.push_back(RandomRecord(buffer,
                                     buffer_size,
                                     writer.WriteAppendCRC(buffer,
                                                           buffer_size)));

      notify(NFY_VERBOSE, "WriteRandomRecords: wrote size %u at %llu",
             (unsigned int)buffer_size,
             static_cast<unsigned long long>(records.back().position));
      total_size += buffer_size;
    }
    return total_size;
  }

  void ReadRandomRecords(FileBundleReader &reader,
                         std::vector<RandomRecord> &records) {
    size_t records_left = records.size();

    while (records_left > 0) {
      // Select a random record
      size_t index = static_cast<size_t>
                     ((static_cast<float>(rand()) / RAND_MAX * records_left));

      // Swap selected record with last record
      RandomRecord record = records.at(index);
      records[index] = records.back();
      records.back() = record;
      --records_left;

      // Read and compare record
      notify(NFY_VERBOSE, "ReadRandomRecords: reading size %u at %llu",
             (unsigned int)record.buffer_size,
             static_cast<unsigned long long>(record.position));
      char file_buffer[kMaxBuffer + FileBundle::kCRCsize];
      if (record.buffer_size <= sizeof(file_buffer)) {
        reader.ReadAtCRC(record.position, file_buffer, record.buffer_size);
        if (0 != memcmp(file_buffer, record.buffer, record.buffer_size)) {
          throw khSimpleException("ReadRandomRecords: read data mismatch");
        }
      } else {
        throw khSimpleException
          ("ReadRandomRecords: read size larger than file buffer");
      }
    }
  }

  bool TestRandom(bool buffer_writes, const std::string &bundlename) {
    const int kBufferCount = 2000;
    const std::uint64_t kSegmentBreak = 1024*1024;
    const size_t kWriteBufferSize = 100*1024;

    std::string abs_path;
    std::vector<ManifestEntry> manifest;
    std::string abs_path2;
    std::vector<ManifestEntry> manifest2;

    fprintf(stderr, "Running TestRandom()\n");


    if (0 > chdir(kPathBase.c_str())) {
      throw khSimpleException("TestRandom: chdir failed for ") << kPathBase;
    }

    std::vector<RandomRecord> records;
    records.reserve(kBufferCount);
    std::uint64_t total_data_size = 0;

    // Generate and write buffers
    {
      notify(NFY_VERBOSE, "TestRandom: writing records");

      FileBundleWriter writer(file_pool_, bundlename, true, 0777, kSegmentBreak);
      if (buffer_writes)
        writer.BufferWrites(kWriteBufferSize);
      total_data_size = WriteRandomRecords(writer, records, kBufferCount);
      TestAssert(total_data_size == writer.data_size());
      writer.Close();
      // Require that at least 3 segments were created for a good test.
      TestAssert(writer.data_size() / writer.segment_break() >= 3);
    }

    // Read  buffers in random order
    {
      notify(NFY_VERBOSE, "TestRandom: reading records");

      FileBundleReader reader(file_pool_, bundlename);
      TestAssert(total_data_size == reader.data_size());
      ReadRandomRecords(reader, records);

      // Try again using a cache.
      reader.EnableReadCache(5, 64 * 1024);
      ReadRandomRecords(reader, records);

      manifest.clear();
      reader.AppendManifest(manifest, "");
      abs_path = reader.abs_path();

      reader.Close();
    }

    // Rename directory to test orig_abs_path feature
    std::string bundlename2 = bundlename + "2";
    TestAssert(khRename(bundlename, bundlename2));

    // Test update mode
    {
      // Open in update mode, add some records to bundle
      notify(NFY_VERBOSE, "TestRandom: updating records");

      FileBundleUpdateWriter updater(file_pool_, bundlename2);
      std::vector<RandomRecord> update_records;
      total_data_size +=
        WriteRandomRecords(updater, update_records, kBufferCount); // add more

      // Test IsWriteable
      for (size_t i = 0; i < update_records.size(); ++i) {
        TestAssert(updater.IsWriteable(update_records.at(i).position));
      }
      for (size_t i = 0; i < records.size(); ++i) {
        TestAssert(!updater.IsWriteable(records.at(i).position));
      }
      TestAssert(total_data_size == updater.data_size());

      // Modify and rewrite one of the updated records
      size_t i = update_records.size() / 2;
      RandomRecord mod_record = update_records.at(i);
      mod_record.buffer[0] = mod_record.buffer[0] ^ 0xFF;
      updater.WriteAtCRC(mod_record.position,
                         mod_record.buffer,
                         mod_record.buffer_size);

      // Add update records to main record list
      records.reserve(records.size() + update_records.size());
      while (!update_records.empty()) {
        records.push_back(update_records.back());
        update_records.pop_back();
      }

      updater.Close();
    }
    {
      // Reopen after update, make sure all records (original and
      // updated) are readable
      notify(NFY_VERBOSE, "TestRandom: verifying updated records");

      FileBundleReader reader(file_pool_, bundlename2);
      ReadRandomRecords(reader, records);
      // Try again using a cache.
      reader.EnableReadCache(5, 64 * 1024);
      ReadRandomRecords(reader, records);

      manifest2.clear();
      reader.AppendManifest(manifest2, "");
      abs_path2 = reader.abs_path();

      reader.Close();
    }

    // Check manifests
    size_t i;
    TestAssert(abs_path != abs_path2);
    for (i = 0; i < manifest.size(); ++i) {
      notify(NFY_VERBOSE,
             "manifest[%u]  = %s %s %u",
             static_cast<unsigned int>(i),
             manifest[i].orig_path.c_str(),
             manifest[i].current_path.c_str(),
             static_cast<unsigned int>(manifest[i].data_size));
      notify(NFY_VERBOSE,
             "manifest2[%u] = %s %s %u",
             static_cast<unsigned int>(i),
             manifest2[i].orig_path.c_str(),
             manifest2[i].current_path.c_str(),
             static_cast<unsigned int>(manifest2[i].data_size));
      if (i > 0) {
        TestAssert(khDirname(manifest[i].orig_path)+"/" == abs_path);
        TestAssert(khDirname(manifest[i].current_path)+"/" == abs_path);
        TestAssert(khDirname(manifest2[i].orig_path)+"/" == abs_path2);
        TestAssert(khDirname(manifest2[i].current_path)+"/" == abs_path2);
        TestAssert(khBasename(manifest[i].orig_path) ==
                   khBasename(manifest2[i].orig_path));
        TestAssert(khBasename(manifest[i].current_path) ==
                   khBasename(manifest2[i].current_path));
        TestAssert(manifest[i].data_size != 0);
        TestAssert(manifest[i].data_size == manifest2[i].data_size);
      }
    }

    size_t j;
    for (j = i; j < manifest2.size(); ++j) {
      notify(NFY_VERBOSE,
             "manifest2[%u] = %s %s %u",
             static_cast<unsigned int>(j),
             manifest2[j].orig_path.c_str(),
             manifest2[j].current_path.c_str(),
             static_cast<unsigned int>(manifest2[j].data_size));
      TestAssert(khDirname(manifest2[j].orig_path)+"/" == abs_path2);
      TestAssert(manifest2[j].orig_path == manifest2[j].current_path);
      TestAssert(manifest2[j].data_size != 0);
    }
    TestAssert(khBasename(manifest[0].orig_path)
               == FileBundle::kHeaderFileName);
    TestAssert(khBasename(manifest[0].current_path)
               == FileBundle::kHeaderFileName);
    TestAssert(khBasename(manifest2[0].orig_path)
               == FileBundle::kHeaderFileName);
    TestAssert(khBasename(manifest2[0].current_path)
               == FileBundle::kHeaderFileName);
    TestAssert(khDirname(manifest[0].orig_path)+"/" == abs_path);
    TestAssert(khDirname(manifest[0].current_path)+"/" == abs_path);
    TestAssert(khDirname(manifest2[0].orig_path)+"/" == abs_path2);
    TestAssert(khDirname(manifest2[0].current_path)+"/" == abs_path2);
    TestAssert(manifest[0].data_size > 0);
    TestAssert(manifest2[0].data_size > manifest[0].data_size);

    return true;
  }

  bool TestRandomUnbuffered() {
    return TestRandom(false, "relative/test_bundle_unbuffered");
  }

  bool TestRandomBuffered() {
    return TestRandom(true, "relative/test_bundle_buffered");
  }

  // Test for disallow of read/write in appropriate places
  bool TestExceptions() {
    bool success = true;
    bool thrown;
    static const std::string kBundleName(kPathBase + "TestExceptions");

    // Create a test database
    {
      FileBundleWriter writer(file_pool_, kBundleName);
      char buffer[kMaxBuffer];
      for (size_t i = 0; i < kMaxBuffer - FileBundle::kCRCsize; ++i) {
        buffer[i] = i & 0xFF;
      }
      writer.WriteAppendCRC(buffer, kMaxBuffer);
      writer.Close();
    }

    {
      FileBundleUpdateWriter updater(file_pool_, kBundleName);
      char buffer[kMaxBuffer];

      // Try to write to a read segment
      thrown = false;
      try {
        updater.WriteAt(0, buffer, kMaxBuffer);
      }
      catch (khSimpleException e) {
        thrown = true;
        notify(NFY_DEBUG, "TestExceptions: caught \"%s\"", e.what());
      }
      if (!thrown) {
        success = false;
        notify(NFY_WARN,
               "TestExceptions: no exception for WriteAt to read segment");
      }

      updater.Close();
    }

    return success;
  }

  // Test CRC error reporting
  bool TestCRCReport() {
    bool success = true;
    static const std::string kBundleName(kPathBase + "TestCRCReport");

    // Set these constants so that we write more than one segment, and
    // so that the bad CRC record will be in the second segment at a
    // non-zero offset
    const std::uint64_t kSegmentBreak = 2 * kMaxBuffer + 10;
    const int kRecordCount = 4;
    std::vector<FileBundleAddr> record_addrs;

    // Create a test database
    {
      FileBundleWriter writer(file_pool_,
                              kBundleName,
                              true,
                              FileBundleWriter::kDefaultMode,
                              kSegmentBreak);
      char buffer[kMaxBuffer];

      for (int record_num = 0; record_num < kRecordCount; ++record_num) {
        size_t record_size = kMaxBuffer - record_num;
        for (size_t i = 0; i < record_size - FileBundle::kCRCsize; ++i) {
          buffer[i] = (i ^ record_num) & 0xFF;
        }
        record_addrs.push_back(
            FileBundleAddr(writer.WriteAppendCRC(buffer, record_size),
                           record_size));
      }

      // Rewrite last record with one bad bit
      buffer[record_addrs.back().Size()/2] ^= 1;
      writer.WriteAt(record_addrs.back(), buffer);

      writer.Close();
    }

    // Read records with CRC checking - should fail at last record
    {
      FileBundleReader reader(file_pool_, kBundleName);
      char buffer[kMaxBuffer];

      for (int record_num = 0; record_num < kRecordCount; ++record_num) {
        try {
          const FileBundleAddr &address = record_addrs[record_num];
          reader.ReadAtCRC(address.Offset(), buffer, address.Size());
          if (record_num ==  kRecordCount - 1) {
            notify(NFY_WARN,
                   "TestCRCReport: ERROR, bad CRC not detected at record %d",
                   record_num);
            success = false;
          }
        }
        catch (khSimpleException e) {
          if (record_num == kRecordCount - 1) {
            notify(NFY_DEBUG,
                   "TestCRCReport: bad CRC correctly detected, message: %s",
                   e.what());
          } else {
            notify(NFY_WARN,
                   "TestCRCReport: exception thrown at wrong record (%d): %s",
                   record_num,
                   e.what());
            success = false;
          }
        }
      }
    }

    return success;
  }

 private:
  geFilePool file_pool_;
};

// Temporary file tree
// WARNING: this tree will be deleted on exit!
std::string FileBundleUnitTest::kPathBase(
    "/tmp/tests/filebundle_unittest_data/");


int main(int argc, char *argv[]) {
  bool keep = false;
#ifndef SINGLE_THREAD
  int argn;
  bool debug = false;
  bool verbose = false;
  bool help = false;
  khGetopt options;
  options.flagOpt("debug", debug);
  options.flagOpt("verbose", verbose);
  options.flagOpt("keep_output", keep);
  options.flagOpt("help", help);
  options.flagOpt("?", help);

  if (!options.processAll(argc, argv, argn)  ||  help  ||  argn < argc) {
    std::cerr << "usage: " << std::string(argv[0])
              << " [-debug] [-verbose] [-keep_output] [-help]" << std::endl;
    exit(1);
  }

  if (verbose) {
    setNotifyLevel(NFY_VERBOSE);
  } else if (debug) {
    setNotifyLevel(NFY_DEBUG);
  }
#endif

  FileBundleUnitTest tests;
  if (khDirExists(FileBundleUnitTest::kPathBase)) {
    khPruneDir(FileBundleUnitTest::kPathBase);
  }
  int status = tests.Run();
  if (!keep) {
    khPruneDir(FileBundleUnitTest::kPathBase);
  }
  return status;
}
