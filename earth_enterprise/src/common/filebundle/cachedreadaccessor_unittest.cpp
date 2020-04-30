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


// Unit test for the CachedReadAccessor classes

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
#include <khStringUtils.h>
#include "filebundlewriter.h"
#include "filebundlereader.h"
#include "cachedreadaccessor.h"
#include <UnitTest.h>

class CachedReadAccessorUnitTest : public UnitTest<CachedReadAccessorUnitTest> {
 public:
  CachedReadAccessorUnitTest(bool keep_temp_bundle)
      : BaseClass("CachedReadAccessorUnitTest"),
        keep_temp_bundle_(keep_temp_bundle) {
    REGISTER(TestCacheBlockAddress);
    REGISTER(TestCacheBlock);
    REGISTER(TestCachedReadAccessorBasics);
    REGISTER(TestCachedReadAccessorPread);
    REGISTER(TestCachedReadAccessorPreadPerformance);
    // Create a temporary test segment.
    // WARNING: this directory will be deleted on exit!
    segment_size_ = 100 * 1024 * 1024;
    file_bundle_size_ = segment_size_ / 3;
    path_base_ = "/tmp/tests/cachedreadaccessor_unittest_data/";
    test_buffer_ = new char[file_bundle_size_];
    char next_char = 0;
    for(std::uint32_t i = 0; i < file_bundle_size_; ++i) {
      test_buffer_[i] = next_char++;
    }
    bundle_name_ = path_base_ + "TestBasics";
    if (khDirExists(path_base_)) {
      khPruneDir(path_base_);
    }
    CreateTestBundle();
  }
  ~CachedReadAccessorUnitTest() {
    if (!keep_temp_bundle_) {
      khPruneDir(path_base_);
    }
    delete [] test_buffer_;
  }

  // Maintain Test bundle info for all the tests.
  std::uint32_t file_bundle_size_;
  std::uint32_t segment_size_;
  std::string path_base_;
  std::string bundle_name_;
  char* test_buffer_;
  std::vector<std::string> segment_names_;
  std::string header_path_;
  geFilePool file_pool_;
  bool keep_temp_bundle_;

  void CreateTestBundle() {
    // Write a very simple file
    std::string header_filename;
    std::string segment0_filename;
    FileBundleWriter writer(file_pool_, bundle_name_, true, 0777, segment_size_);

    // Write record with CRC
    // must only create one segment.
    std::uint64_t location = writer.WriteAppendCRC(test_buffer_, file_bundle_size_);
    (void)location;
    header_path_ = writer.HeaderPath();
    std::uint32_t segment_count = writer.SegmentCount();
    for(std::uint32_t i = 0; i < segment_count; ++i) {
      std::string filename = writer.abs_path() + writer.Segment(i)->name();
      segment_names_.push_back(filename);
    }

    // Close the file
    writer.Close();
  }

  // Utility for comparing buffers nicely
  bool TestBuffer(char* actual, char* expected, std::uint32_t size,
                  std::string prefix) {
    for(std::uint32_t i = 0; i < size; ++i) {
      if (actual[i] != expected[i]) {
        std::cerr << "FAILED [" << prefix <<
          "]: character missmatch at index " << i <<
          " actual: " << actual[i] <<
          " expected: " << expected[i] << std::endl;
        return false;
      }
    }
    return true;
  }

  // Test the CachedReadAccessor Pread() method.
  // This is the main method exposed by CachedReadAccessor.
  bool TestCachedReadAccessorPreadPerformance() {
    std::uint32_t segment_id = 0;
    // Read request that is contained in 1 block.
    FileBundleReaderSegment segment(file_pool_,
                                    path_base_,
                                    segment_names_[segment_id],
                                    segment_names_[segment_id],
                                    segment_id);
    std::uint32_t request_count = 1500; // typical 1500B for some Fusion packets.
    std::uint32_t best_mb_per_sec = 0;
    std::uint32_t best_kilobytes = 0;
    for(std::uint32_t bytes = 1 << 14; bytes < 1 << 22; bytes <<= 1) {
      std::uint32_t kilobytes = bytes / 1024;
      std::string prefix = "Cached read 2x " + Itoa(kilobytes) + "KB";
      std::uint32_t mb_per_sec = TestReadAll(segment, request_count, 2, bytes, prefix);
      if (mb_per_sec > best_mb_per_sec) {
        best_mb_per_sec = mb_per_sec;
        best_kilobytes = kilobytes;
      }
    }
    std::uint32_t uncached_mb_per_sec = TestReadAll(segment, request_count, 0, 0,
                std::string("Uncached read"));
    if (best_mb_per_sec > uncached_mb_per_sec) {
      std::cerr << "Cached is optimal " << best_mb_per_sec <<
        " MB/sec cached with " << best_kilobytes << "KB buffers vs Uncached "
        << uncached_mb_per_sec << " MB/sec" << std::endl;
    } else {
      std::cerr << "Uncached is optimal " << uncached_mb_per_sec <<
        " MB/sec vs Cached " << best_mb_per_sec << " MB/sec with " <<
        best_kilobytes << "KB buffers" << std::endl;
    }
    return true;
  }

  // Utility for running timings of reads of the filebundle segment with
  // different caching.
  std::uint32_t TestReadAll(FileBundleReaderSegment& segment,
                   std::uint32_t request_count,
                   std::uint32_t max_blocks,
                   std::uint32_t block_size,
                   std::string prefix) {
    std::string buffer;
    buffer.reserve(block_size);
    buffer.resize(request_count);
    char* out_buffer = const_cast<char*>(buffer.data());

    // Test Read uncached of entire filebundle
    CachedReadAccessor accessor(max_blocks < 2 ? 2 : max_blocks, block_size);
    khTimer_t begin = khTimer::tick();
    std::uint32_t read_count = 0;
    std::uint32_t packet_count = 0;
    std::uint64_t stats_bytes_read = 0;
    std::uint64_t stats_disk_accesses = 0;
    for(std::uint32_t offset = 0; offset < file_bundle_size_; offset += request_count) {
      std::uint32_t size = std::min(request_count, file_bundle_size_ - offset);
      if (max_blocks == 0) { // No cache...read via the segment directly.
        segment.Pread(out_buffer, size, offset);
        stats_bytes_read += size;
        stats_disk_accesses++;
      }
      else { // Read via the accessor.
        accessor.Pread(segment, out_buffer, size, offset);
      }
      read_count += size;
      packet_count++;
    }
    khTimer_t end = khTimer::tick();
    std::uint32_t megabytes_per_second = static_cast<std::uint32_t>(read_count /
      (khTimer::delta_s(begin, end) * 1000000));
    std::cerr << prefix << ": " << request_count << " bytes per packet " <<
      packet_count << " packets " <<
      megabytes_per_second << " MB/sec" << std::endl;
    // Print some basic stats to make sure the cache is doing it's job right.
    if (max_blocks >= 2) {
      stats_bytes_read = accessor.StatsBytesRead();
      stats_disk_accesses = accessor.StatsDiskAccesses();
    }
    std::uint64_t chunk_sizes = block_size;
    if (max_blocks < 2) {
      chunk_sizes = request_count;
    }
    std::uint64_t desired_disk_accesses = (file_bundle_size_ + chunk_sizes - 1) / chunk_sizes;
    TestAssertEquals(stats_bytes_read, static_cast<std::uint64_t>(file_bundle_size_));
    TestAssertEquals(stats_disk_accesses,
                     static_cast<std::uint64_t>(desired_disk_accesses));

    std::cerr << "Read " << stats_bytes_read << " bytes from disk in " <<
      stats_disk_accesses << " reads" << std::endl;

    return megabytes_per_second;
  }

  // Test the CachedReadAccessor Pread() method.
  // This is the main method exposed by CachedReadAccessor.
  bool TestCachedReadAccessorPread() {
    std::uint32_t offset = 13;
    std::uint32_t segment_id = 0;
    std::uint32_t block_size = file_bundle_size_ / 4;
    CachedReadAccessor accessor(2, block_size);

    // Test Read
    // Read request that is contained in 1 block.
    std::uint32_t request_count = block_size - offset - 10;
    FileBundleReaderSegment segment(file_pool_,
                                    path_base_,
                                    segment_names_[segment_id],
                                    segment_names_[segment_id],
                                    segment_id);
    std::string buffer;
    buffer.reserve(block_size);
    buffer.resize(request_count);
    char* out_buffer = const_cast<char*>(buffer.data());

    // Test read within a single block
    // uncached block
    accessor.Pread(segment, out_buffer, request_count, offset);
    char* expected = test_buffer_ + offset;
    if (!TestBuffer(out_buffer, expected, request_count,
               std::string("TestCachedReadAccessorPread: 1 non-cached block")))
      return false;

    // same block cached read
    offset *= 2;
    request_count -= offset;
    buffer.resize(request_count);
    
    accessor.Pread(segment, out_buffer, request_count, offset);
    expected = test_buffer_ + offset;
    if (!TestBuffer(out_buffer, expected, request_count,
               std::string("TestCachedReadAccessorPread: 1 cached block")))
      return false;

    // Test read straddling 2 blocks
    // uncached 2nd block
    offset = block_size / 3;
    request_count = block_size;
    buffer.resize(request_count);
    
    accessor.Pread(segment, out_buffer, request_count, offset);
    expected = test_buffer_ + offset;
    if (!TestBuffer(out_buffer, expected, request_count,
               std::string("TestCachedReadAccessorPread: 2 non-cached blocks")))
      return false;

    // same 2 blocks cached read
    offset *= 2;
    request_count -= offset;
    buffer.resize(request_count);
    
    accessor.Pread(segment, out_buffer, request_count, offset);
    expected = test_buffer_ + offset;
    if (!TestBuffer(out_buffer, expected, request_count,
               std::string("TestCachedReadAccessorPread: 2 cached blocks")))
      return false;

    return true;
  }

  // Test the CachedReadAccessor utility methods BlockAddress(),
  // AddCacheBlock(), GetCacheBlock() and FindBlocks().
  bool TestCachedReadAccessorBasics() {
    std::uint32_t offset = 13;
    std::uint32_t segment_id = 0;
    std::uint32_t block_size = file_bundle_size_ / 4;
    std::uint64_t access_tick = 1033;

    CachedReadAccessor accessor(2, block_size);

    // Test BlockAddress
    CachedReadAccessor::CacheBlockAddress address1 =
      accessor.BlockAddress(segment_id, offset);
    TestAssertEquals(address1.Offset(), static_cast<std::uint32_t>(0));
    std::uint32_t n = 7;
    CachedReadAccessor::CacheBlockAddress address2 =
      accessor.BlockAddress(segment_id, offset + n * block_size);
    TestAssertEquals(address2.Offset(), n * block_size);
    CachedReadAccessor::CacheBlockAddress address3 =
      accessor.BlockAddress(segment_id, (n+2) * block_size - 10);
    TestAssertEquals(address3.Offset(), (n+1) * block_size);

    // Test that AddCacheBlock and GetCacheBlock are compatible.
    CachedReadAccessor::CacheBlock* block1 = accessor.AddCacheBlock(address1);
    CachedReadAccessor::CacheBlock* block1_b = accessor.GetCacheBlock(address1);
    TestAssertEquals(block1, block1_b);
    block1->SetLastAccessTick(access_tick);

    // Test getting a new block twice.
    CachedReadAccessor::CacheBlock* block2 = accessor.GetCacheBlock(address2);
    CachedReadAccessor::CacheBlock* block2_b = accessor.GetCacheBlock(address2);
    TestAssertEquals(block2, block2_b);
    TestAssert(block1 != block2);
    block2->SetLastAccessTick(access_tick+1);
    std::uint64_t block1_access_tick = block1->LastAccessTick();

    // Ok, we have block1 and block2
    // Now we try to get block3, should force one of the cached blocks out of
    // the cache (namely block1).
    CachedReadAccessor::CacheBlock* block3 = accessor.GetCacheBlock(address3);
    TestAssert(block3 != block1);
    TestAssert(block3 != block2);
    block3->SetLastAccessTick(0); // mark this as older for fun.

    // We should have block2 and block3
    // Getting block2 again should be the same object as before.
    block2_b = accessor.GetCacheBlock(address2);
    TestAssertEquals(block2, block2_b);

    // Getting block1 again should be a newly created object and block3 should
    // be flushed.
    block1_b = accessor.GetCacheBlock(address1);
    TestAssert(block1_b->LastAccessTick() != block1_access_tick);
    block1 = block1_b; // block1 was deleted...better be safe here.

    // Getting block2 yet again should be the same object as before.
    block2_b = accessor.GetCacheBlock(address2);
    TestAssertEquals(block2, block2_b);


    // Test FindBlocks
    // case 1: request spans a single block
    // case 2: request spans two blocks
    std::uint32_t in_block_offset = 33;
    offset = block_size + in_block_offset;
    address1 = accessor.BlockAddress(segment_id, offset);
    address2 = accessor.BlockAddress(segment_id, offset + block_size);

    // test these cases when not cached and then again when cached.
    std::uint32_t request_size = block_size - 2 * in_block_offset;
    // set request size to guarantee to be within 1 block.
    std::uint32_t blocks_returned = accessor.FindBlocks(segment_id, request_size, offset,
                                        &block1, &block2);
    TestAssertEquals(blocks_returned, static_cast<std::uint32_t>(1));
    TestAssert(block2 == NULL);
    TestAssert(block1 != NULL);
    block1->SetLastAccessTick(access_tick++); // Make sure it stays in the cache.
    // Try again, slightly different request in the same block, block1
    // should be cached.
    blocks_returned = accessor.FindBlocks(segment_id,
                                        request_size - 3 * in_block_offset,
                                        offset + in_block_offset,
                                        &block1_b, &block2);
    TestAssertEquals(blocks_returned, static_cast<std::uint32_t>(1));
    TestAssert(block2 == NULL);
    TestAssert(block1 == block1_b);

    // Now try the same FindBlocks test on requests that span 2 CacheBlocks.
    request_size = block_size - in_block_offset / 2;
    blocks_returned = accessor.FindBlocks(segment_id, request_size, offset,
                                        &block1_b, &block2);
    TestAssertEquals(blocks_returned, static_cast<std::uint32_t>(2));
    TestAssert(block2 != NULL); // Make sure it stays in the cache.
    block2->SetLastAccessTick(access_tick++);
    TestAssert(block1 == block1_b);
    // Try again, slightly different request in the same block, block1
    // should be cached.
    blocks_returned = accessor.FindBlocks(segment_id,
                                        request_size,
                                        offset + in_block_offset,
                                        &block1_b, &block2_b);
    TestAssertEquals(blocks_returned, static_cast<std::uint32_t>(2));
    TestAssert(block2 == block2_b);
    TestAssert(block1 == block1_b);
    return true;
  }

  // Test the CacheBlock utility object Read() and LastAccessTick().
  bool TestCacheBlock() {
    std::uint32_t offset = 13;
    std::uint32_t segment_id = 0;
    std::uint32_t block_size = file_bundle_size_ / 4;
    CachedReadAccessor::CacheBlockAddress address1(segment_id, 0);
    std::uint64_t access_tick = 1033;
    CachedReadAccessor::CacheBlock block1(address1, block_size);

    // Check the initial access tick
    TestAssert(block1.LastAccessTick() == 0);
    block1.SetLastAccessTick(access_tick - 1);
    TestAssert(block1.LastAccessTick() == (access_tick - 1));

    // Test Read
    // Read request that is contained in 1 block.
    std::uint32_t request_count = block_size - offset - 10;
    FileBundleReaderSegment segment(file_pool_,
                                    path_base_,
                                    segment_names_[segment_id],
                                    segment_names_[segment_id],
                                    segment_id);
    std::string buffer;
    buffer.reserve(block_size);
    buffer.resize(request_count);
    char* out_buffer = const_cast<char*>(buffer.data());

    // Test read within non-cached (uninitialized) block
    std::uint64_t stats_bytes_read = 0;
    std::uint64_t stats_disk_accesses = 0;
    std::uint32_t read_count =
      block1.Read(segment, out_buffer, request_count, offset, access_tick++,
                  stats_bytes_read, stats_disk_accesses);
    TestAssert(block1.LastAccessTick() + 1 == access_tick);
    TestAssertEquals(stats_bytes_read, static_cast<std::uint64_t>(block_size));
    TestAssertEquals(stats_disk_accesses, static_cast<std::uint64_t>(1));
    TestAssertEquals(request_count, read_count);
    char* expected = test_buffer_ + offset;
    if (!TestBuffer(out_buffer, expected, read_count,
               std::string("TestCacheBlock: all data in one non-cached block")))
      return false;

    // Test read from the NOW cached block within the segment.
    offset = 23;
    request_count /= 3;
    buffer.resize(request_count);
    read_count =
      block1.Read(segment, out_buffer, request_count, offset, access_tick++,
                  stats_bytes_read, stats_disk_accesses);
    TestAssertEquals(stats_bytes_read, static_cast<std::uint64_t>(block_size));
    TestAssertEquals(stats_disk_accesses, static_cast<std::uint64_t>(1));
    TestAssertEquals(request_count, read_count);
    expected = test_buffer_ + offset;
    if (!TestBuffer(out_buffer, expected, read_count,
               std::string("TestCacheBlock: all data in one cached block")))
      return false;

    // Test a PARTIAL reads from a block (i.e., a request past the block
    // boundary.
    // create a new CacheBlock to start with an empty cache.
    block1 = CachedReadAccessor::CacheBlock(address1, block_size);
    request_count = 200;
    buffer.resize(request_count);

    std::uint32_t expected_read_count = 133;
    offset = block_size - expected_read_count;
    // Read from a non-cached block
    read_count =
      block1.Read(segment, out_buffer, request_count, offset, access_tick++,
                  stats_bytes_read, stats_disk_accesses);
    TestAssertEquals(stats_bytes_read, static_cast<std::uint64_t>(2*block_size));
    TestAssertEquals(stats_disk_accesses, static_cast<std::uint64_t>(2));
    TestAssertEquals(expected_read_count, read_count);
    expected = test_buffer_ + offset;
    if (!TestBuffer(out_buffer, expected, read_count,
               std::string("TestCacheBlock: partial data in cached block")))
      return false;

    // Read from a cached block
    read_count =
      block1.Read(segment, out_buffer, request_count, offset, access_tick++,
                  stats_bytes_read, stats_disk_accesses);
    TestAssertEquals(expected_read_count, read_count);
    expected = test_buffer_ + offset;
    if (!TestBuffer(out_buffer, expected, read_count,
               std::string("TestCacheBlock: partial data in cached block")))
      return false;

    return true;
  }

  // Test CacheBlockAddress Utility class, mostly that constructor and
  // comparison operators are properly defined.
  bool TestCacheBlockAddress() {
    std::uint32_t offset = 100;
    std::uint32_t segment_id = 1010;
    CachedReadAccessor::CacheBlockAddress address1(segment_id, offset);
    TestAssertEquals(offset, address1.Offset());

    { // Equality and copy constructor tests.
      CachedReadAccessor::CacheBlockAddress address2(segment_id, offset);
      TestAssert(address1 == address2);
      TestAssert((address1 < address2) == false);
      CachedReadAccessor::CacheBlockAddress address3 = address1;
      TestAssert(address2 == address3);
    }
    { // less than by segment id
      CachedReadAccessor::CacheBlockAddress address2(segment_id+1, offset);
      TestAssert(address1 < address2);
      TestAssert(address1 != address2);
      TestAssert((address2 < address1) == false);
    }
    { // less than by offset
      CachedReadAccessor::CacheBlockAddress address2(segment_id, offset+1);
      TestAssert(address1 < address2);
      TestAssert(address1 != address2);
      TestAssert((address2 < address1) == false);
    }
    return true;
  }
};

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

  CachedReadAccessorUnitTest tests(keep);

  int status = tests.Run();
  return status;
}
