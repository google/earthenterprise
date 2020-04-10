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

// Unit tests for geindex Writer and Reader and Traverser

#include <cstdint>
#include <math.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstdint>
#include <khGetopt.h>
#include <khFileUtils.h>
#include <khEndian.h>
#include <notify.h>
#include <khSimpleException.h>
#include <tests/qtpathgen.h>
#include <gtest/gtest.h>
#include "Entries.h"
#include "Writer.h"
#include "Reader.h"
#include "Traverser.h"


namespace geindex {

class WriterReaderUnitTest : public testing::Test {
 protected:
  virtual void SetUp() {}
  virtual void TearDown() {}
  geFilePool file_pool_;
 public:

  static const std::string kPathBase;
  static bool full_test_;
  static std::uint32_t num_cache_levels_;

  typedef LoadedSingleEntryBucket<SimpleInsetEntry> TestLoadedEntryBucket;
  typedef TestLoadedEntryBucket::EntryType TestEntry;
  typedef Writer<TestLoadedEntryBucket> SimpleTestWriter;
  typedef SimpleTestWriter::WriteBuffer WriteBuffer;
  typedef SimpleTestWriter::ReadBuffer ReadBuffer;
  typedef Reader<TestEntry> SimpleTestReader;
  typedef Traverser<TestLoadedEntryBucket> SimpleTestTraverser;

  // Hash routines - helpers to put actual data in the offset and size
  // slots that can be verified on read.

  static std::uint64_t HashOffset(QuadtreePath qt_path) {
    // The "hash" of the offset is actually the 64-bit path value.
    // Don't try this at home (i.e. in actual production code)
    std::uint64_t offset;
    assert(sizeof(offset) == sizeof(qt_path));
    memcpy(&offset, &qt_path, sizeof(offset));
    return offset;
  }

  static std::string OffsetHashToString(std::uint64_t offset) {
    // The "hash" of the offset is actually the 64-bit path value.
    // Don't try this at home (i.e. in actual production code)
    QuadtreePath qt_path;
    assert(sizeof(offset) == sizeof(qt_path));
    memcpy(&qt_path, &offset, sizeof(offset));
    return qt_path.IsValid() ? qt_path.AsString() : "INVALID";
  }

  static std::uint32_t HashSize(QuadtreePath qt_path) {
    std::uint32_t hash = std::uint32_t(qt_path.AsIndex(qt_path.Level()) & 0x7FFFFFFF)
                  ^ 0x5A03C276
                  ^ qt_path.Level();
    return (hash != 0) ? hash : 0x7FFFFFFF;
  }

  // Shuffle - shuffle a vector into a different, random order

  template<class T> static void Shuffle(std::vector<T> &records) {
    if (records.size() <= 1)
      return;
    double max_index = records.size() - 1;

    for (int i = 0; i <= max_index; ++i) {
      int swap = static_cast<int>(round(static_cast<double>(random())
                                        / static_cast<double>(RAND_MAX)
                                        * max_index));
      if (swap != i) {
        std::swap(records[i], records[swap]);
      }
    }
  }

  static bool TestQTPath(QuadtreePath qt_path,
                         const ExternalDataAddress &dataAddress,
                         std::uint32_t packetfile,
                         std::uint32_t cache_level) {
    // Test data for specified path

    bool result = true;
    const std::uint64_t test_offset = HashOffset(qt_path);
    const std::uint32_t test_size = HashSize(qt_path);

    if (dataAddress.fileOffset != test_offset
        || dataAddress.fileNum != packetfile
        || dataAddress.size != test_size) {
      std::string path_string =
        OffsetHashToString(dataAddress.fileOffset);
      std::cerr << "TestAllQTPaths, cache level " << cache_level
                << ", expected \"" << qt_path.AsString()
                << "\"/" << packetfile
                << "/" << test_size
                << ", got \"" << path_string
                << "\"/" << dataAddress.fileNum
                << "/" << dataAddress.size << std::endl;
      result = false;
    }
    return result;
  }
};

// TestEntrySlot - test mapping from subpath <= level 3 to entry
// slot numbers in preorder.  Also test inverse mapping
TEST_F(WriterReaderUnitTest, TestEntrySlot) {
  BucketPath root_path(QuadtreePath("10332110"));
  QuadtreePath sub_path(root_path);

  for (BucketEntrySlotType slot = 0; slot < kEntrySlotsPerBucket; ++slot) {
    // Test mapping from path to slot number
    BucketEntrySlotType entry_slot =
      root_path.SubAddrAsEntrySlot(sub_path);
    notify(NFY_VERBOSE, "TestEntrySlot: %2u: %-3s -> %u",
           static_cast<unsigned int>(slot),
           sub_path.AsString().c_str(),
           static_cast<unsigned int>(entry_slot));
    ASSERT_TRUE(entry_slot == slot);

    // Test mapping from slot number to path
    QuadtreePath entry_path = root_path.EntrySlotAsSubAddr(slot);
    ASSERT_TRUE(entry_path.IsValid());
    notify(NFY_VERBOSE, "TestEntrySlot: slot mapped back to %s",
           entry_path.AsString().c_str());
    ASSERT_TRUE(entry_path == sub_path);

    sub_path.Advance(root_path.Level() + kQuadLevelsPerBucket-1);
  }
}

// TestBasics - for each possible path length, write an index
// consisting of a single entry, read it back and traverse it.
//
// Note: packetfiles are not created in this test, just
// an index file is created/written.
TEST_F(WriterReaderUnitTest, TestBasics) {
  const std::string index_path(kPathBase + "TestBasicsIndex");
  const std::string packet_path(kPathBase + "TestBasicsPacket");

  std::uint32_t packetfile;
  const std::uint16_t test_version = 104;
  const QuadtreePath master_qt_path("202113321110033231003201");

  // Test writing and reading a single record for all possible path lengths

  for (size_t level = 0; level <= master_qt_path.Level(); ++level) {
    const QuadtreePath test_qt_path(master_qt_path, level);
    const std::uint64_t test_offset = HashOffset(test_qt_path);
    const std::uint32_t test_size = HashSize(test_qt_path);

    // Create a Writer
    {
      SimpleTestWriter writer(file_pool_,
                              index_path,
                              SimpleTestWriter::FullIndexMode,
                              "Test");
      packetfile = writer.AddExternalPacketFile(packet_path);

      SimpleInsetEntry test_entry(ExternalDataAddress(test_offset,
                                                      packetfile,
                                                      test_size),
                                  test_version,
                                  0 /* inset */);
      ReadBuffer read_buffer;

      writer.Put(test_qt_path, test_entry, read_buffer);

      writer.Close();
      notify(NFY_DEBUG, "TestBasics: Writer finished for (%s)",
             test_qt_path.AsString().c_str());
    }

    // Create a Reader and read back the record just written.
    // Also try to read a non-existent record and make sure
    // we get the correct exception.
    {
      SimpleTestReader reader(file_pool_,
                              index_path,
                              1);
      SimpleInsetEntry entry_buffer;
      ReadBuffer tmpBuf;
      reader.GetEntry(test_qt_path,
                      SimpleInsetEntry::ReadKey(test_version),
                      &entry_buffer,
                      tmpBuf);
      ASSERT_TRUE(entry_buffer.dataAddress.fileOffset == test_offset);
      ASSERT_TRUE(entry_buffer.dataAddress.fileNum == packetfile);
      ASSERT_TRUE(entry_buffer.dataAddress.size == test_size);

      QuadtreePath no_such_path = (test_qt_path.Level() > 0)
          ? test_qt_path.Parent() : test_qt_path.Child(0);
      try {
        reader.GetEntry(no_such_path,
                        SimpleInsetEntry::ReadKey(test_version),
                        &entry_buffer,
                        tmpBuf);
        std::cerr << "TestBasics: Reader failed to throw for non-existent "
                  << "path \"" << no_such_path.AsString() << "\""
                  << std::endl;
        ADD_FAILURE();
        return;
      }
      catch(khSimpleNotFoundException &not_found) {
        notify(NFY_VERBOSE,
               "TestBasics: correctly caught khSimpleNotFoundException: %s",
               not_found.what());
      }
      catch(khSimpleException &exc) {
        std::cerr << "TestBasics: caught wrong exception khSimpleException ("
                  << exc.what()
                  << ") for non-existent "
                  << "path \"" << no_such_path.AsString() << "\""
                  << std::endl;
        ADD_FAILURE();
        return;
      }
      catch(std::exception &exc) {
        std::cerr << "TestBasics: caught wrong exception ("
                  << exc.what()
                  << ") for non-existent "
                  << "path \"" << no_such_path.AsString() << "\""
                  << std::endl;
        ADD_FAILURE();
        return;
      }
      catch(...) {
        std::cerr << "TestBasics: caught wrong exception for non-existent "
                  << "path \"" << no_such_path.AsString() << "\""
                  << std::endl;
        ADD_FAILURE();
        return;
      }

      notify(NFY_DEBUG, "TestBasics: Reader finished for (%s)",
             test_qt_path.AsString().c_str());
    }

#ifndef SINGLE_THREAD
    // Create a Traverser and use it to read back the record just written
    {
      SimpleTestTraverser traverser("TestBasics_Traverser_"
                                    + test_qt_path.AsString(),
                                    file_pool_,
                                    index_path);
      const SimpleTestTraverser::MergeEntry &entry = traverser.Current();
      if (test_qt_path != entry.qt_path()) {
        std::cerr << "TestBasics:traverser expected qt_path ("
                  << test_qt_path.AsString()
                  << "), read ("
                  << entry.qt_path().AsString()
                  << ")" << std::endl;
        ADD_FAILURE();
        return;
      }
      ASSERT_TRUE(entry.dataAddress.fileOffset == test_offset);
      ASSERT_TRUE(entry.dataAddress.fileNum == packetfile);
      ASSERT_TRUE(entry.dataAddress.size == test_size);

      ASSERT_TRUE(!traverser.Advance());
      traverser.Close();
      notify(NFY_DEBUG, "TestBasics: Traverser finished for (%s)",
             test_qt_path.AsString().c_str());
    }
#endif  // ifndef SINGLE_THREAD
  }
}

// TestRandomSample - generate a vector of random paths, index
// them all, then try to read them all back.
//
// Note: packetfiles are not created in this test, just
// an index file is created/written.
TEST_F(WriterReaderUnitTest, TestRandomSample) {
  // Build vector of unique random paths, positions, and sizes
  std::vector<qtpathgen::TestPositionRecord> records;
  qtpathgen::BuildRandomRecords(records, 2000, RAND_MAX);
  sort(records.begin(), records.end());

  const std::string index_path(kPathBase + "TestRandomSampleIndex");
  const std::string packet_path(kPathBase + "TestRandomSamplePacket");

  std::uint32_t packetfile;
  std::uint16_t test_version = 5922;
  SimpleInsetEntry::ReadKey test_version_key(test_version);

  // Write records
  {
    SimpleTestWriter writer(file_pool_,
                            index_path,
                            SimpleTestWriter::FullIndexMode,
                            "Test");
    packetfile = writer.AddExternalPacketFile(packet_path);

    ReadBuffer read_buffer;
    for (size_t i = 0; i < records.size(); ++i) {
      SimpleInsetEntry test_entry(
          ExternalDataAddress(records[i].position(),
                              packetfile,
                              records[i].buffer_size()),
          test_version,
          0 /* inset */);
      writer.Put(records[i].qt_path(), test_entry, read_buffer);
      notify(NFY_VERBOSE, "TestRandomSample: wrote %s",
             records[i].qt_path().AsString().c_str());
    }

    writer.Close();
    notify(NFY_DEBUG, "TestRandomSample: Writer finished, %u records",
           static_cast<unsigned int>(records.size()));
  }

#ifndef SINGLE_THREAD
  // Create a Traverser and read back the records just written
  {
    SimpleTestTraverser traverser("TestRandomSample_Traverser",
                                  file_pool_,
                                  index_path);
    for (size_t i = 0; i < records.size(); ++i) {
      const SimpleTestTraverser::MergeEntry &entry = traverser.Current();
      if (records[i].qt_path() != entry.qt_path()) {
        std::cerr << "TestRandomSample:traverser expected qt_path ("
                  << records[i].qt_path().AsString()
                  << "), read ("
                  << entry.qt_path().AsString()
                  << ")" << std::endl;
        ADD_FAILURE();
        return;
      }
      ASSERT_TRUE(entry.dataAddress.fileOffset == records[i].position());
      ASSERT_TRUE(entry.dataAddress.fileNum == packetfile);
      ASSERT_TRUE(entry.dataAddress.size == records[i].buffer_size());
      notify(NFY_VERBOSE, "TestRandomSample: traverse %s",
             records[i].qt_path().AsString().c_str());

      if (traverser.Advance()) {
        ASSERT_TRUE(i+1 < records.size());
      } else {
        ASSERT_TRUE(i+1 == records.size());
      }
    }
    ASSERT_TRUE(traverser.Finished());
    traverser.Close();
    notify(NFY_DEBUG, "TestRandomSample: Traverser finished");
  }
#endif  // ifndef SINGLE_THREAD

  // Create a Reader and read back the records just written
  {
    Shuffle(records);
    SimpleTestReader reader(file_pool_,
                            index_path,
                            3);
    SimpleInsetEntry entry_buffer;
    ReadBuffer tmpBuf;

    for (size_t i = 0; i < records.size(); ++i) {
      reader.GetEntry(records[i].qt_path(),
                      test_version_key,
                      &entry_buffer,
                      tmpBuf);
      ASSERT_TRUE(entry_buffer.dataAddress.fileOffset == records[i].position());
      ASSERT_TRUE(entry_buffer.dataAddress.fileNum == packetfile);
      ASSERT_TRUE(entry_buffer.dataAddress.size == records[i].buffer_size());
      notify(NFY_VERBOSE, "TestRandomSample: read %s",
             records[i].qt_path().AsString().c_str());
    }
    notify(NFY_DEBUG, "TestRandomSample: Reader finished");
  }
}

// TestAllQTPaths - write an index with all possible paths up to
// length 10, and read back the entries.  Try the read with all
// possible cache depths (1, 2, and 3).
TEST_F(WriterReaderUnitTest, TestAllQTPaths) {
  bool result = true;
  const std::uint32_t kMaxPathDepth = 10;
  const std::string index_path(kPathBase + "TestAllQTPathsIndex");
  const std::string packet_path(kPathBase + "TestAllQTPathsPacket");
  std::uint32_t packetfile;
  const std::uint16_t test_version = 1600;
  std::uint64_t record_count = 0;

  // Write the database
  {
    SimpleTestWriter writer(file_pool_,
                            index_path,
                            SimpleTestWriter::FullIndexMode,
                            "Test");
    packetfile = writer.AddExternalPacketFile(packet_path);

    // Cycle through all possible quadtree paths <= max depth

    QuadtreePath qt_path;
    ReadBuffer read_buffer;
    do {
      const std::uint64_t test_offset = HashOffset(qt_path);
      const std::uint32_t test_size = HashSize(qt_path);
      SimpleInsetEntry test_entry(ExternalDataAddress(test_offset,
                                                         packetfile,
                                                         test_size),
                                     test_version,
                                     0 /* inset */);

      writer.Put(qt_path, test_entry, read_buffer);
      ++record_count;
    } while (qt_path.Advance(kMaxPathDepth));

    writer.Close();
    notify(NFY_DEBUG, "TestAllQtPaths: Writer finished "
           "(depth %u, %llu records)",
           static_cast<unsigned int>(kMaxPathDepth),
           static_cast<unsigned long long>(record_count));
  }

#ifndef SINGLE_THREAD
  // Read back the data using the traverser
  {
    std::uint64_t read_count = 0;
    SimpleTestTraverser traverser("TestAllQTPaths_Traverser",
                                  file_pool_,
                                  index_path);

    // Cycle through all possible quadtree paths <= max depth
    QuadtreePath qt_path;
    do {
      const SimpleTestTraverser::MergeEntry &entry = traverser.Current();
      if (qt_path != entry.qt_path()) {
        std::cerr << "TestRandomSample:traverser expected qt_path ("
                  << qt_path.AsString()
                  << "), read ("
                  << entry.qt_path().AsString()
                  << ")" << std::endl;
        ADD_FAILURE();
        return;
      }
      if (!TestQTPath(qt_path,
                      entry.dataAddress,
                      packetfile,
                      9999)) {
        result = false;
      }

      ++read_count;
      if (traverser.Advance()) {
        ASSERT_TRUE(read_count < record_count);
      } else {
        ASSERT_TRUE(read_count == record_count);
      }
    } while (qt_path.Advance(kMaxPathDepth));
    ASSERT_TRUE(traverser.Finished());
    traverser.Close();
    notify(NFY_DEBUG, "TestAllQtPaths: Traverser finished "
           "(%llu records)",
           static_cast<unsigned long long>(read_count));
  }
#endif  // ifndef SINGLE_THREAD

  // Read back and verify the database. If full_test == false, test at single
  // cache_level else test every possible cache level.
  for (std::uint32_t cache_level = (full_test_ ? 1 : num_cache_levels_);
       cache_level <= num_cache_levels_; ++cache_level) {
    SimpleTestReader reader(file_pool_,
                            index_path,
                            cache_level);

    // Cycle through all possible quadtree paths <= max depth

    QuadtreePath qt_path;
    ReadBuffer tmpBuf;
    do {
      SimpleInsetEntry entry_buffer;
      reader.GetEntry(
          qt_path, SimpleInsetEntry::ReadKey(test_version),
          &entry_buffer, tmpBuf);
      if (!TestQTPath(qt_path,
                      entry_buffer.dataAddress,
                      packetfile,
                      cache_level)) {
        result = false;
      }
    } while (qt_path.Advance(kMaxPathDepth));
    notify(NFY_DEBUG, "TestBasics: Reader finished for level %u",
           static_cast<unsigned int>(cache_level));
  }
  ASSERT_TRUE(result);
}



// Temporary file tree
// WARNING: this tree will normally be deleted on exit!
const std::string WriterReaderUnitTest::kPathBase(
    "/tmp/tests/geindex_WriteRead_unittest_data/");
bool WriterReaderUnitTest::full_test_ = false;
 std::uint32_t  WriterReaderUnitTest::num_cache_levels_ = geindex::kMaxLevelsCached;
}

namespace testing {
namespace internal {
  extern bool g_help_flag;
}
}


int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  bool keep = false;
#ifndef SINGLE_THREAD
  int argn;
  bool debug = false;
  bool verbose = false;
  khGetopt options;
  options.flagOpt("debug", debug);
  options.flagOpt("verbose", verbose);
  options.flagOpt("keep_output", keep);
  options.flagOpt("full", geindex::WriterReaderUnitTest::full_test_);
  options.opt("num_cache_levels",
              geindex::WriterReaderUnitTest::num_cache_levels_,
              &khGetopt::RangeValidator<std::uint32_t, 1, 3>);
  options.setExclusive("full", "num_cache_levels");

  if (testing::internal::g_help_flag || !options.processAll(argc, argv, argn) ||
      argn < argc) {
    std::cerr << "usage: " << std::string(argv[0])
              << " [-debug] [-verbose] [-keep_output] [-full] "
              << "[-num_cache_levels] [-help]"
              << std::endl;
    exit(1);
  }

  if (verbose) {
    setNotifyLevel(NFY_VERBOSE);
  } else if (debug) {
    setNotifyLevel(NFY_DEBUG);
  }
#endif  // ifndef SINGLE_THREAD

  int status = RUN_ALL_TESTS();
  if (!keep) {
    khPruneDir(geindex::WriterReaderUnitTest::kPathBase);
  }
  return status;
}
