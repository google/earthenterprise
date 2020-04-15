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


// Unit tests for IndexManifest

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <khEndian.h>
#include <khFileUtils.h>
#include <packetfile/packetfilewriter.h>
#include <geindex/Entries.h>
#include <geindex/Writer.h>
#include <khGetopt.h>
#include "IndexManifest.h"
#include <UnitTest.h>

namespace geindex {

class IndexManifestUnitTest : public UnitTest<IndexManifestUnitTest> {
 public:
  static const std::string kPathBase;
  typedef LoadedSingleEntryBucket<SimpleInsetEntry> TestLoadedEntryBucket;
  typedef TestLoadedEntryBucket::EntryType TestEntry;
  typedef Writer<TestLoadedEntryBucket> SimpleTestWriter;

  IndexManifestUnitTest()
      : BaseClass("IndexManifestUnitTest") {
    REGISTER(TestManifest);
  }

  bool CompareManifest(const std::vector<ManifestEntry> &expected_manifest,
                       const std::vector<ManifestEntry> &actual_manifest) {
    bool result = true;

    if (getNotifyLevel() >= NFY_VERBOSE) {
      for (size_t i = 0; i < actual_manifest.size(); ++i) {
        notify(NFY_VERBOSE,
               "TestManifest: actual_manifest[%zu]   = %s, %s, %zu",
               i,
               actual_manifest[i].orig_path.c_str(),
               actual_manifest[i].current_path.c_str(),
               actual_manifest[i].data_size);
      }
      for (size_t i = 0; i < expected_manifest.size(); ++i) {
        notify(NFY_VERBOSE,
               "TestManifest: expected_manifest[%zu] = %s, %s, %zu",
               i,
               expected_manifest[i].orig_path.c_str(),
               expected_manifest[i].current_path.c_str(),
               expected_manifest[i].data_size);
      }
    }

    if (expected_manifest.size() != actual_manifest.size()) {
      notify(NFY_WARN,
             "TestManifest: manifest expected size (%u) != actual size (%u)",
             (unsigned int)expected_manifest.size(),
             (unsigned int)actual_manifest.size());
      result = false;
    }

    for (size_t i = 0; i < expected_manifest.size(); ++i) {
      int found = std::count(actual_manifest.begin(),
                             actual_manifest.end(),
                             expected_manifest[i]);
      if (found != 1) {
        notify(NFY_WARN, "TestManifest: expected_manifest[%u] "
               " found %d times", (unsigned int)i, found);
        result = false;
      }
    }
    return result;
  }

  bool TestManifest() {
    const std::string kServerPrefix("/tmp/khserver"); // no trailing slash
    const std::string kServerPrefixSlash("/tmp/khserver"); // trailing slash

    bool result = TestManifestWithServerPrefix(kServerPrefix);
    result = result
             && TestManifestWithServerPrefix(kServerPrefixSlash);
    return result;
  }

  bool TestManifestWithServerPrefix(const std::string &server_prefix) {
    bool result = true;
    const std::uint16_t kVersion(1);
    const std::string index_path(kPathBase + "TestManifest/index/");
    const std::string header_path(Header::HeaderPath(index_path));
    const std::string bundle_hdr(index_path + FileBundle::kHeaderFileName);
    const std::string bundle_data(index_path + "bundle.0000");
    const std::string kTmpDir(server_prefix + index_path);
    LittleEndianReadBuffer read_buf;
    std::vector<std::string> packetfiles;
    packetfiles.push_back(kPathBase + "TestManifest/packet1/");
    packetfiles.push_back(kPathBase + "TestManifest/packet2/");
    std::vector<ManifestEntry> expected_manifest;

    {
      // Create index writer
      SimpleTestWriter writer(file_pool_,
                              index_path,
                              SimpleTestWriter::FullIndexMode,
                              "Test");
      notify(NFY_VERBOSE,
             "TestManifest: created writer for %s",
             index_path.c_str());

      // Create packet files and index records
      QuadtreePath pf_path;
      for (std::vector<std::string>::const_iterator pfile = packetfiles.begin();
           pfile != packetfiles.end();
           ++pfile) {
        // Create packet writer
        PacketFileWriter pf_writer(file_pool_, *pfile);

        // Add packetfile to index
        std::uint32_t packetfile =  writer.AddExternalPacketFile(*pfile);
        notify(NFY_VERBOSE,
               "TestManifest: added packet file %u: %s",
               static_cast<unsigned int>(packetfile),
               pfile->c_str());

        // Write dummy data to packetfile
        std::uint32_t dummy_data[2];
        pf_path.Advance(QuadtreePath::kMaxLevel);
        std::uint64_t offset =
          pf_writer.WriteAppendCRC(pf_path, dummy_data, sizeof(dummy_data));
        notify(NFY_VERBOSE,
               "TestManifest: wrote record at path (%s), "
               "offset %llu, size %u",
               pf_path.AsString().c_str(),
               static_cast<unsigned long long>(offset),
               (unsigned int)sizeof(dummy_data));
        // Index the data
        writer.Put(pf_path,
                   SimpleInsetEntry(ExternalDataAddress(offset,
                                                           packetfile,
                                                           sizeof(dummy_data)),
                                       kVersion,
                                       0),
                   read_buf);
        notify(NFY_VERBOSE, "TestManifest: Put index record");

        pf_writer.Close();

        const std::string pktindex_path(*pfile + PacketFile::kIndexBase);
        const std::string bundle_hdr(*pfile + FileBundle::kHeaderFileName);
        const std::string bundle_data(*pfile + "bundle.0000");
        expected_manifest.push_back(
            ManifestEntry(bundle_hdr, bundle_hdr, khDiskUsage32(bundle_hdr)));
        expected_manifest.push_back(
            ManifestEntry(bundle_data, bundle_data,
                          khDiskUsage32(bundle_data)));

      }
      notify(NFY_VERBOSE, "TestManifest: closing index Writer");
      writer.Close();

      expected_manifest.push_back(
          ManifestEntry(header_path, header_path, khDiskUsage32(header_path)));
      expected_manifest.push_back(
          ManifestEntry(bundle_hdr, bundle_hdr, khDiskUsage32(bundle_hdr)));
      expected_manifest.push_back(
          ManifestEntry(bundle_data, bundle_data, khDiskUsage32(bundle_data)));
    }

    // Now get the manifest and check contents (NOTE: this will need
    // to be changed if any of the manifest code generation puts the
    // entries in a different order).
    std::vector<ManifestEntry> actual_manifest;
    IndexManifest::GetManifest(
        file_pool_, index_path, actual_manifest, "", "", "");
    if (!CompareManifest(expected_manifest, actual_manifest)) {
      result = false;
    }

    // Get a server manifest and check for correct results
    actual_manifest.clear();
    TestAssert(khMakeDir(kTmpDir));
    IndexManifest::GetManifest(file_pool_, index_path, actual_manifest,
                               kTmpDir, "", "");
    const std::string kResPath = khComposePath(kTmpDir, index_path);
    const std::string tmp_header_path(Header::HeaderPath(kResPath));
    const std::string tmp_bundle_hdr(kResPath + FileBundle::kHeaderFileName);
    // Since we have changed to relative path and there will be no TMP_DIR file
    // writing, in case of such headers the followings are not required.
    if (!CompareManifest(expected_manifest, actual_manifest)) {
      result = false;
    }
    TestAssert(khExists(tmp_header_path) == false);
    TestAssert(khExists(tmp_bundle_hdr) == false);

    return result;
  }

 private:
  geFilePool file_pool_;
};

// Temporary file tree
// WARNING: this tree will normally be deleted on exit!
const std::string IndexManifestUnitTest::kPathBase(
    "/tmp/tests/geindex_IndexManifest_unittest_data/unifiedindex.kda/");

} // namespace geindex

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
              << " [-debug] [-verbose] [-keep_output] [-help]"
              << std::endl;
    exit(1);
  }

  if (verbose) {
    setNotifyLevel(NFY_VERBOSE);
  } else if (debug) {
    setNotifyLevel(NFY_DEBUG);
  }

  geindex::IndexManifestUnitTest tests;
  int status = tests.Run();
  if (!keep) {
    khPruneDir(geindex::IndexManifestUnitTest::kPathBase);
  }
  return status;
}
