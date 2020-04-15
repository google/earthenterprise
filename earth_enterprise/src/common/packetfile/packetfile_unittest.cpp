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


// Unit test for the PacketFile classes

#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <khFileUtils.h>
#include <khGuard.h>
#include <khGetopt.h>
#include <notify.h>
#include <khSimpleException.h>
#include <UnitTest.h>
#include <packetfile/packetfilewriter.h>
#include <packetfile/packetfilereader.h>
#include <packetfile/packetfilereaderpool.h>
#include <tests/qtpathgen.h>

typedef qtpathgen::TestDataRecord TestDataRecord;

class PacketFileUnitTest : public UnitTest<PacketFileUnitTest> {
 public:
  PacketFileUnitTest()
      : BaseClass("PacketFileUnitTest") {
    REGISTER(TestBasics);
    REGISTER(TestIndexCRC);
    REGISTER(TestRandomSorted);
    REGISTER(TestRandomWriteAt);
    REGISTER(TestWriteDuplicate);
    REGISTER(TestRandomSortInMemory);
    REGISTER(TestRandomSortAndMerge);
    REGISTER(TestSortedLevels);
    REGISTER(TestPool);
  }

  static std::string kPathBase;
  static const size_t kMaxBuffer = 4096; // maximum buffer size (without CRC)
  static const mode_t kTestDirMode = S_IRUSR | S_IWUSR | S_IXUSR;
  static const mode_t kTestFileMode = S_IRUSR | S_IWUSR;

  // TestBasics - write, then read, a simple packet file

  bool TestBasics() {
    static const std::string kTestString(
        "Test string for TestBasics 0123456780abcdefghijklmnopqrstuvwxyz");
    static const std::string kPacketFileName(kPathBase + "TestBasics");

    // Set umask to known value
    mode_t old_umask = umask(0);

    QuadtreePath test_qt_path("0310221");

    try {
      // Write a very simple file
      std::string index_filename;
      std::string bundle_filename;
      {
        PacketFileWriter writer(file_pool_, kPacketFileName, true, kTestDirMode);

        // Write record with CRC
        const size_t kRecordLength = kTestString.size() + FileBundle::kCRCsize;
        char buffer[kRecordLength];
        memcpy(buffer, kTestString.data(), kTestString.size());
        writer.WriteAppendCRC(test_qt_path, buffer, kRecordLength);

        // Get file names
        index_filename = writer.index_writer_->index_path_;
        bundle_filename = writer.HeaderPath();

        // Close the file
        writer.Close();
      }

      // Check permissions on created directory and files
       CheckMode(kPacketFileName, kTestDirMode);
       CheckMode(index_filename, kTestFileMode);
       CheckMode(bundle_filename, kTestFileMode);

      // Read it back
      {
        PacketFileReader reader(file_pool_, kPacketFileName);
        const size_t kRecordLength = kTestString.size() + FileBundle::kCRCsize;
        char buffer[kRecordLength];

        QuadtreePath read_qt_path;
        reader.ReadNextCRC(&read_qt_path, buffer, kRecordLength);
        if (read_qt_path != test_qt_path) {
          throw khSimpleException("PacketFile_UnitTest::TestBasics: "
                                  "read quadtree path mismatch");
        }
        if (0 != memcmp(buffer, kTestString.data(), kTestString.size())) {
          throw khSimpleException("PacketFile_UnitTest::TestBasics: "
                                  "read compare error");
        }

        // Check manifest.  For this small database, it should have
        // two entries: the bundle file and the bundle header. We check for
        // correct entry count and make sure that the files are included.
        std::vector<ManifestEntry> manifest;
        reader.GetManifest(manifest, "");
        TestAssert(manifest.size() == 2);
        TestAssert(manifest[0].orig_path
                   == reader.abs_path() + FileBundle::kHeaderFileName);
        TestAssert(manifest[0].orig_path == manifest[0].current_path);
        TestAssert(manifest[1].orig_path
                   == reader.abs_path() + "bundle.0000");
        TestAssert(manifest[1].orig_path == manifest[1].current_path);
        TestAssert(manifest[0].data_size ==
                   khGetFileSizeOrThrow(manifest[0].current_path));
        TestAssert(manifest[1].data_size ==
                   khGetFileSizeOrThrow(manifest[1].current_path));
      }
    }
    catch (...) {
      umask(old_umask);
      throw;
    }

    umask(old_umask);
    return true;
  }

  
  // TestIndexCRC - test CRC checking of index header and entries

  bool TestIndexCRC() {
    bool success = true;
    static const std::string kTestString(
        "Test string for TestIndexCRC 0123456780abcdefghijklmnopqrstuvwxyz");
    static const std::string kPacketFileName(kPathBase + "TestIndexCRC");

    QuadtreePath test_qt_path("0310221");

    // Write a very simple file
    std::string index_filename;
    std::string bundle_filename;
    {
      PacketFileWriter writer(file_pool_, kPacketFileName, true, kTestDirMode);

      // Write record with CRC
      const size_t kRecordLength = kTestString.size() + FileBundle::kCRCsize;
      char buffer[kRecordLength];
      memcpy(buffer, kTestString.data(), kTestString.size());
      writer.WriteAppendCRC(test_qt_path, buffer, kRecordLength);

      // Get file names
      index_filename = writer.index_writer_->index_path_;
      bundle_filename = writer.HeaderPath();

      // Close the file
      writer.Close();
    }

    // Change one bit in the header to invalidate the CRC
    std::string index;
    std::string orig_index;
    file_pool_.ReadStringFile(index_filename, &index);
    orig_index = index;
    index[PacketFile::kIndexHeaderSize / 2] ^= 0x04;
    file_pool_.WriteStringFile(index_filename, index);

    // Open the packet file and make sure an exception is thrown
    try {
      PacketIndexReader reader(file_pool_, index_filename);
      notify(NFY_WARN, "TestIndexCRC: header CRC error undetected in file: %s",
             index_filename.c_str());
      success = false;
    }
    catch (khSimpleException e) {
      notify(NFY_DEBUG, "TestIndexCRC: header test successful: %s", e.what());
    }

    // Restore file contents and change a bit in the index entry
    index = orig_index;
    index[PacketFile::kIndexHeaderSize + PacketIndexEntry::kStoreSize / 2]
        ^= 0x20;
    file_pool_.WriteStringFile(index_filename, index);

    // Try to read the index entry and make sure an exception is thrown
    {
      PacketIndexReader reader(file_pool_, index_filename);
      PacketIndexEntry entry;
      try {
        reader.ReadNext(&entry);
        notify(NFY_WARN,
               "TestIndexCRC: entry CRC error undetected in file: %s",
               index_filename.c_str());
        success = false;
      }
      catch (khSimpleException e) {
        notify(NFY_DEBUG, "TestIndexCRC: entry test successful: %s", e.what());
      }
    }

    return success;
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
        msg << "PacketFileUnitTest::CheckMode: "<< std::oct << std::showbase
            << "mode " << path_mode
            << " should be " << mode << " for path " << path;
        throw khSimpleException(msg.str());
      }
    } else {
      throw khSimpleErrnoException("PacketFileUnitTest::CheckMode: path ")
        << path << " stat error: ";
    }
  }

  // Random record writer and reader. Options allow sorting by keys
  // before writing.

  void WriteRandomRecords(PacketFileWriter &writer,
                          std::vector<TestDataRecord> &records,
                          int record_count,
                          bool sort_records = false) {
    qtpathgen::BuildRandomRecords(records, record_count, kMaxBuffer);
    if (sort_records) {
      std::sort(records.begin(), records.end());
    }
    for (std::vector<TestDataRecord>::iterator record = records.begin();
         record != records.end();
         ++record) {
      writer.WriteAppendCRC(record->qt_path(),
                            record->mutable_buffer_ptr(),
                            record->buffer_size());
      notify(NFY_VERBOSE, "WriteRandomRecords: wrote %s size %u",
             record->qt_path().AsString().c_str(),
             (unsigned int)record->buffer_size());
    }
  }

  void WriteAtRandomRecords(PacketFileWriter &writer,
                            std::vector<TestDataRecord> &records,
                            int record_count,
                            bool sort_records = false) {
    qtpathgen::BuildRandomRecords(records, record_count, kMaxBuffer);
    if (sort_records) {
      std::sort(records.begin(), records.end());
    }

    std::vector<PacketFileWriter::AllocatedBlock> allocations;

    for (int i = 0; i < record_count; ++i) {
      allocations.push_back(writer.AllocateAppend(records[i].qt_path(),
                                                  records[i].buffer_size()));
    }

    for (int i = record_count - 1; i >= 0; --i) {
      writer.WriteAtCRC(records[i].qt_path(),
                        records[i].mutable_buffer_ptr(),
                        records[i].buffer_size(),
                        allocations[i]);
      notify(NFY_VERBOSE, "WriteAtRandomRecords: wrote %s size %u",
             records[i].qt_path().AsString().c_str(),
             (unsigned int)records[i].buffer_size());
    }
  }

  void WriteDuplicateRecords(PacketFileWriter &writer,
                             std::vector<TestDataRecord> &records,
                             int record_count)
  {
    const int kDuplicateInterval = 7;
    const int kSourceDivisor = 10;
    
    qtpathgen::BuildRandomRecords(records, record_count, kMaxBuffer);

    std::vector<PacketFileWriter::AllocatedBlock> allocations;
    std::vector<bool> is_duplicate(record_count);

    for (int i = 0; i < record_count; ++i) {
      if (i > 0  &&  (i & kDuplicateInterval) == 0) {
        is_duplicate[i] = true;
        int source = i / (record_count/kSourceDivisor); // multiple duplicates
        assert(source < i);
        allocations.push_back(allocations[source]);
        records[i].set_buffer_size(records[source].buffer_size());
        memcpy(records[i].mutable_buffer_ptr(), // expected data
               records[source].buffer_ptr(),
               records[i].buffer_size());
        notify(NFY_VERBOSE, "WriteDuplicateRecords: duplicated %s from %d to %d",
               records[i].qt_path().AsString().c_str(), source, i);
      } else {
        is_duplicate[i] = false;
           allocations.push_back(writer.AllocateAppend(records[i].qt_path(),
                                                    records[i].buffer_size()));
      }
    }

    for (int i = record_count-1; i >= 0; --i) { // test by writing reverse order
      if (is_duplicate[i]) {
        writer.WriteDuplicate(records[i].qt_path(), allocations[i]);
        notify(NFY_VERBOSE, "WriteDuplicateRecords: duplicated %s",
               records[i].qt_path().AsString().c_str());
      } else {
        writer.WriteAtCRC(records[i].qt_path(),
                          records[i].mutable_buffer_ptr(),
                          records[i].buffer_size(),
                          allocations[i]);
        notify(NFY_VERBOSE, "WriteDuplicateRecords: wrote %s size %u",
               records[i].qt_path().AsString().c_str(),
               (unsigned int)records[i].buffer_size());
      }
    }
  }

  void ReadRandomRecords(PacketFileReader &reader,
                         std::vector<TestDataRecord> &records) {
    for (std::vector<TestDataRecord>::const_iterator record = records.begin();
         record != records.end();
         ++record) {
      // Read and compare record
      notify(NFY_VERBOSE, "ReadRandomRecords: reading %s size %u",
             record->qt_path().AsString().c_str(),
             (unsigned int)record->buffer_size());
      char file_buffer[kMaxBuffer + FileBundle::kCRCsize];
      QuadtreePath read_path;
      size_t read_size =
        reader.ReadNextCRC(&read_path, file_buffer, sizeof(file_buffer));

      if (read_path != record->qt_path()) {
        throw khSimpleException("ReadRandomRecords: order error, ")
          << "expected " << record->qt_path().AsString()
          << " read " << read_path.AsString();
      }
      if (read_size != record->buffer_size() - FileBundle::kCRCsize) {
        throw khSimpleException("ReadRandomRecords: ")
          << "expected size " << record->buffer_size() - FileBundle::kCRCsize
          << " got size " << read_size;
      }
      if (0 != memcmp(file_buffer, record->buffer_ptr(), read_size)) {
        throw khSimpleException("ReadRandomRecords: read data mismatch");
      }     
    }
  }

  // TestRandom - generate specified number of random records, with
  // specified sort buffer size.  The combination of record count and
  // sort buffer size can be used to force a one-pass in-memory sort
  // or a sort and merge.

  enum WriteMode { kAppend, kAt, kDuplicate };

  bool TestRandom(const std::string &packet_file_name,
                  std::uint32_t record_count,
                  size_t sort_buffer_size,
                  WriteMode write_mode,
                  bool sort_records = false)
    {
    const std::uint64_t kSegmentBreak = 1024*1024;
  
    if (0 > chdir(kPathBase.c_str())) {
      throw khSimpleException("TestRandom: chdir failed for ") << kPathBase;
    }

    std::vector<TestDataRecord> records;
    records.reserve(record_count);

    // Generate and write buffers
    {
      notify(NFY_VERBOSE, "TestRandom: writing records");
      
      PacketFileWriter writer(file_pool_,
                              packet_file_name,
                              true,
                              kTestDirMode,
                              kSegmentBreak);
      switch (write_mode) {
        case kAppend:
          WriteRandomRecords(writer, records, record_count, sort_records);
          break;
        case kAt:
          WriteAtRandomRecords(writer, records, record_count, sort_records);
          break;
        case kDuplicate:
          WriteDuplicateRecords(writer, records, record_count);
          break;
        default:
          throw khSimpleException("TestRandom: unknown write_mode");
      }

      // Specify desired sort buffer size
      writer.Close(sort_buffer_size);
    }

    // Sort buffers by quadtree path
    if (!sort_records) {                // if not sorted before writing
      std::sort(records.begin(), records.end());
    }

    // Read  buffers in order
    {
      notify(NFY_VERBOSE, "TestRandom: reading records");
      
      PacketFileReader reader(file_pool_, packet_file_name);
      ReadRandomRecords(reader, records);
    }

    std::uint64_t num_packets = PacketIndexReader::NumPackets(
        PacketFile::IndexFilename(packet_file_name));
    if (num_packets != record_count) {
      throw khSimpleException("TestRandom: NumPackets returned ")
          << num_packets << ", should be " << record_count;
    }

    return true;
  }

  // Test writing random records, already sorted
  bool TestRandomSorted() {
    notify(NFY_DEBUG, "TestRandomSorted started");
    const std::uint32_t kRecordCount = 1000;
    return TestRandom("relative/Sorted",
                      kRecordCount,
                      1024 * 1024,
                      kAppend,
                      true);
  }

  // Test writing random records, using WriteAtCRC
  bool TestRandomWriteAt() {
    notify(NFY_DEBUG, "TestRandomWriteAt started");
    const std::uint32_t kRecordCount = 1000;
    return TestRandom("relative/WriteAt",
                      kRecordCount,
                      1024 * 1024,
                      kAt,
                      false);
  }

  // Test writing duplicates
  bool TestWriteDuplicate() {
    notify(NFY_DEBUG, "TestWriteDuplicate started");
    const std::uint32_t kRecordCount = 1000;
    return TestRandom("relative/WriteDup",
                      kRecordCount,
                      1024 * 1024,
                      kDuplicate,
                      false);
  }

  // Test writing random records, with sort but not merge
  bool TestRandomSortInMemory() {
    notify(NFY_DEBUG, "TestRandomSortInMemory started");
    const std::uint32_t kRecordCount = 2000;
    return TestRandom("relative/SortInMemory",
                      kRecordCount,
                      kRecordCount * PacketIndexEntry::kStoreSize * 11/10,
                      kAppend);
  }

  // Test writing random records, multi-region sort with merge
  bool TestRandomSortAndMerge() {
    notify(NFY_DEBUG, "TestRandomSortAndMerge started");
    const std::uint32_t kRecordCount = 2100;
    return TestRandom("relative/SortInMemory",
                      kRecordCount,
                      kRecordCount * PacketIndexEntry::kStoreSize / 8,
                      kAppend);
  }

  // NextInLevel - return next node at same level in quadtree, update
  // binary blist
  QuadtreePath NextInLevel(std::uint32_t level, unsigned char blist[]) {
    assert(level <= QuadtreePath::kMaxLevel);
    QuadtreePath previous(level, blist);
    int i = level - 1;
    while (i >= 0  &&  blist[i] == 3) {
      --i;
    }
    if (i < 0) throw khSimpleException("NextInLevel: no more nodes at level");
    ++blist[i];
    for (std::uint32_t j = i+1; j < level; ++j) {
      blist[j] = 0;
    }
    QuadtreePath result(level, blist);
    assert(previous < result);
    return result;
  }

  // Write sequential records at the same level
  void WriteLevel(PacketFileWriter &writer, std::uint32_t level, std::uint32_t count) {
    assert(level <= QuadtreePath::kMaxLevel);
    unsigned char blist[QuadtreePath::kMaxLevel];
    memset(blist, 0, sizeof(blist));
    // Generate a random path for the level that's not too near the end
    for (std::uint32_t i = 0; i < level; ++i) {
      blist[i] = (rand() >> 5) & 0x01;
    }

    // Write records in order for the specified level. Contents are
    // the string-encoded quadtree paths.
    while (count-- > 0) {
      QuadtreePath path = NextInLevel(level, blist);
      std::string content = path.AsString();
      char buffer[level + 1 + FileBundle::kCRCsize];
      memcpy(buffer, content.c_str(), level + 1); // string with 0 terminator
      writer.WriteAppendCRC(path, buffer, sizeof(buffer));
    }
  }

  // Test merge of multiple sorted regions
  bool TestSortedLevels() {
    notify(NFY_DEBUG, "TestSortedLevels started");
    const std::string kPacketFileName(kPathBase + "TestSortedLevels");
    std::uint32_t record_count = 0;
    {
      PacketFileWriter writer(file_pool_, kPacketFileName);
      
      WriteLevel(writer, 17, 200); record_count += 200;
      WriteLevel(writer, 14, 100); record_count += 100;
      WriteLevel(writer, 16, 300); record_count += 300;
      WriteLevel(writer, 20, 150); record_count += 150;
      writer.Close();
    }
    {
      PacketFileReader reader(file_pool_, kPacketFileName);

      QuadtreePath last_path;
      std::string buffer;
      for (std::uint32_t i = 0; i < record_count; ++i) {
        QuadtreePath read_path;
        size_t read_size =
          reader.ReadNextCRC(&read_path, buffer);
        std::string path_string = read_path.AsString();

        if (!(last_path < read_path)) {
          throw khSimpleException("TestSortedLevels: data read out of order: ")
            << path_string << ", record " << i;
        } else {
          last_path = read_path;
        }
        if (read_size != read_path.Level()+1
            ||  path_string.compare(0, read_size-1, buffer, 0, read_size-1) != 0
            ||  buffer.at(read_size-1) != 0) {
          throw khSimpleException("TestSortedLevels: bad data read at: ")
            << path_string << ", record " << i;
        }
      }
    }
    return true;
  }

  // TestPool - test PacketFileReaderPool

  bool TestPool() {
    notify(NFY_DEBUG, "PacketFileReaderPool started");

    // Write three very simple packet files.  Then create a pool, add
    // them all to the pool, and make sure we can read them all back.

    std::vector<std::string> test_strings;
    test_strings.push_back("Test string for packet file 0");
    test_strings.push_back("Test string for packet file 1");
    test_strings.push_back("Test string for packet file 2");

    std::vector<std::string> packet_file_names;
    packet_file_names.push_back(kPathBase + "TestPool0");
    packet_file_names.push_back(kPathBase + "TestPool1");
    packet_file_names.push_back(kPathBase + "TestPool2");

    std::vector<QuadtreePath> test_qt_paths;
    test_qt_paths.push_back(QuadtreePath("00000000"));
    test_qt_paths.push_back(QuadtreePath("00000001"));
    test_qt_paths.push_back(QuadtreePath("00000002"));

    // Write the simple packet files

    for (size_t i = 0; i < packet_file_names.size(); ++i) {
      PacketFileWriter writer(file_pool_, packet_file_names[i]);

      // Write record with CRC
      const size_t kRecordLength = test_strings[i].size()
                                   + FileBundle::kCRCsize;
      char buffer[kRecordLength];
      memcpy(buffer, test_strings[i].data(), test_strings[i].size());
      writer.WriteAppendCRC(test_qt_paths[i], buffer, kRecordLength);

      // Close the file
      writer.Close();
    }

    // Read back the files using the reader pool

    PacketFileReaderPool reader_pool("test pool", file_pool_);
    std::vector<PacketFileReaderToken> tokens;
    reader_pool.Add(packet_file_names, tokens);

    for (size_t i = 0; i < packet_file_names.size(); ++i) {
      const size_t kRecordLength = test_strings[i].size()
                                   + FileBundle::kCRCsize;
      char buffer[kRecordLength];
      QuadtreePath read_qt_path;
      PacketFileReader reader(file_pool_,
                              reader_pool.Reader(tokens[i]).abs_path());
      reader.ReadNextCRC(&read_qt_path, buffer, kRecordLength);
      if (read_qt_path != test_qt_paths[i]) {
        throw khSimpleException("PacketFile_UnitTest::TestPool: "
                                "read quadtree path mismatch");
      }
      if (0 != memcmp(buffer, test_strings[i].data(), test_strings[i].size())) {
        throw khSimpleException("PacketFile_UnitTest::TestPool: "
                                "read compare error");
      }
      {
        // Since only one record has been added try using PacketFileReaderBase
        // with position 0.
        std::string str_buffer;
        reader_pool.Reader(tokens[i]).ReadAtCRC(0, str_buffer, kRecordLength);
        if (0 != memcmp(&str_buffer[0], test_strings[i].data(),
                        test_strings[i].size())) {
          throw khSimpleException("PacketFile_UnitTest::TestPool: "
                                  "read compare error");
        }
      }
    }

    reader_pool.Close(tokens);

    return true;
  }

 private:
  geFilePool file_pool_;
};

// Temporary file tree
// WARNING: this tree will be deleted on exit!
std::string PacketFileUnitTest::kPathBase("/tmp/packetfile_unittest_data/");

int main(int argc, char *argv[]) {
  int argn;
  bool debug = false;
  bool verbose = false;
  bool help = false;
  bool keep = false;
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

  PacketFileUnitTest tests;
  int status = tests.Run();
  if (!keep) {
    khPruneDir(PacketFileUnitTest::kPathBase);
  }
  return status;
}
