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


// geindexcheck
//
// Check the contents of a geindex and (optionally) the associated
// packet files. The underlying index, packetfile, and filebundle code
// will check for errors in the data.
//
// Usage:
//   geindexcheck [--mode=mmmm] [--verbose=n] [--progress=n]
//                { --index=index_path | --database=database_name }
//
//   mode: specifies one of the following:
//     index: the default, just walk the index
//     all: walk the index and packet files (in index order)
//   verbose: specifies the level of messages, from 0 (fatal errors only)
//     to 7 (very verbose). Default is 3. This overrides KH_NFY_LEVEL.
//   progress: requests progress messages every n index locations. Requires
//     verbose level of at least 3. Default is 100000. Set to 0 to disable.
//   index: directly specifies the path of an index (of any type)
//   database: specifies the path from the asset root to a database.
//     The unified index for the database will be checked. Defaults to
//     latest version, override by appending "?version=1" where "1" is
//     the desired version.
//
//   Either the index or the database parameter (but not both) must be
//   specified.

#include <stdio.h>
#include <common/packetfile/packetfile.h>
#include <string>
#include <map>
#include <common/serverdb/serverdb.h>
#include <khGetopt.h>
#include <khstl.h>
#include <notify.h>
#include <khFileUtils.h>
#include <khSimpleException.h>
#include <fusion/autoingest/AssetVersion.h>
#include <fusion/dbgen/.idl/DBConfig.h>
#include <fusion/geindexcheck/checktraverser.h>

namespace {

geFilePool file_pool;

void Usage(const std::string &progn) {
  fprintf(stderr,
          "usage: %s [--mode=mmmm] [--verbose=n] \\\n"
          "          [--progress=n] [--help] \\\n"
          "          { --index=index_path | --database=database_name }\n"
          "  Check the contents of a geindex and (optionally)\n"
          "  the associated packet files.\n"
          "Options (must specify --index or --database, but not both):\n"
          "  --index: specify path for any type of index.\n"
          "  --database: specifies the path from the asset root to\n"
          "          a database.  The unified index for the database\n"
          "          will be checked.\n"
          "e.g /gevol/assets/db/test.kdatabase/gedb.kda/ver007/gedb\n"
          "e.g db/test.kdatabase (for latest version)\n"
          "e.g 'db/test.kdatabase?version=7' (for 7th version, quoting for ?)\n"
          "  --mode: specify extent of index walk:\n"
          "          \"index\" to walk index only (default)\n"
          "          \"all\" to walk index and packets, in index order\n"
          "  --verbose: specify the level of messages,\n"
          "          from 0 (fatal errors only) to 7 (very verbose).\n"
          "          Default is 3.\n"
          "  --progress: print progress messages every n index locations\n"
          "          Requires verbose level of 3 or greater.\n"
          "          Default is 100000.  Set to 0 to disable.\n"
          "  --help: print this message\n",
          khBasename(progn).c_str());
  exit(1);
}

// Define modes for check

enum Mode { kModeIndex, kModeAll };

// Find key in map with a certain value. Returns empty string if not
// found.

template <class ValueType>
std::string FindValue(const std::map<std::string, ValueType> &source,
                      const ValueType &target) {
  for (typename std::map<std::string, ValueType>::const_iterator cur
           = source.begin();
       cur != source.end();
       ++cur) {
    if (cur->second == target)
      return cur->first;
  }
  return std::string();
}
}  // namespace

int main(int argc, char **argv) {
  std::string progname = argv[0];
  int error_count = 0;

  // Process commandline options
  int argn;
  bool help = false;
  std::string index_path;
  std::string database_name;
  int verbose = NFY_PROGRESS;
  std::uint32_t progress = 100000;

  Mode mode = kModeIndex;
  std::map<std::string, Mode> mode_map;
  mode_map.insert(std::make_pair(std::string("index"), kModeIndex));
  mode_map.insert(std::make_pair(std::string("all"), kModeAll));

  khGetopt options;
  options.helpOpt(help);
  options.choiceOpt("mode", mode, mode_map);
  options.opt("verbose",
              verbose,
              &khGetopt::RangeValidator<int, 0, NFY_VERBOSE>);
  options.opt("progress", progress);

  options.opt("index", index_path);
  options.opt("database", database_name);
  options.setExclusiveRequired("index", "database");

  if (!options.processAll(argc, argv, argn)
      || help
      || argn != argc) {
    Usage(progname);
  }

  setNotifyLevel(static_cast<enum khNotifyLevel>(verbose));

  std::string mode_string = FindValue(mode_map, mode);

  std::string prefix;
  // If database specified, find path to unified index
  if (!database_name.empty()) {
    std::string gedb_path;
    // we call the simple (fewer checks) version because we want to be
    // able to check the index of old database versions even if we can't
    // publish them.
    if (!AssetVersionImpl::SimpleGetGedbPath(database_name, &gedb_path)) {
      notify(NFY_FATAL, "geindexcheck Failed: Unable to interpret DB '%s'.",
             database_name.c_str());
    }
    notify(NFY_NOTICE, "Checking database \"%s\"", gedb_path.c_str());
    if (khExists(khComposePath(gedb_path, kServerConfigFileName))) {
      // Case of published DB
      ServerdbConfig config;
      if (config.Load(khComposePath(gedb_path, kServerConfigFileName))) {
        index_path = config.index_path;
      }
    } else {
      DbHeader fusion_config;
      if (fusion_config.LoadAndResolvePaths(
          gedb_path, kHeaderXmlFile, &prefix)) {
        index_path = prefix + fusion_config.index_path_;
      }
    }
    if (index_path.empty()) {
      notify(NFY_FATAL, "Failed to load header for database: %s",
             database_name.c_str());
    }
  }

  notify(NFY_NOTICE, "Checking index at: %s", index_path.c_str());
  notify(NFY_NOTICE, "Mode is: \"%s\"", mode_string.c_str());

  // Get the index header

  khDeleteGuard<geindex::Header> header;
  try {
    header = khTransferGuard<geindex::Header>
        (new geindex::Header(file_pool, index_path));
  }
  catch (khSimpleException e) {
    notify(NFY_FATAL, "ERROR: unable to read index header (%s)",
           e.what());
    return 2;
  }

  notify(NFY_NOTICE, "Index content type: \"%s\"",
         header->contentDesc.c_str());
  notify(NFY_INFO, "  File format version: %u",
         static_cast<unsigned>(header->fileFormatVersion));
  notify(NFY_INFO, "  Root child addr: offset=%llu, cbsize=%lu, ebsize=%lu",
         static_cast<unsigned long long>(header->rootChildAddr.offset),
         static_cast<unsigned long>(header->rootChildAddr.childBucketsSize),
         static_cast<unsigned long>(header->rootChildAddr.entryBucketsSize));
  notify(NFY_INFO, "  Root entry addr: offset=%llu, size=%lu",
         static_cast<unsigned long long>(header->rootEntryAddr.Offset()),
         static_cast<unsigned long>(header->rootEntryAddr.Size()));
  notify(NFY_INFO, "  Slots are single: %d", header->slotsAreSingle);
  int packet_file_count = header->PacketFileCount();
  notify(NFY_INFO, "  Packet files (%d):", packet_file_count);
  for (int i = 0; i < packet_file_count; ++i) {
    std::string file_path = header->GetPacketFile(i);
    if (file_path.size() != 0) {
      if (FileBundleReader::IsFileBundle(&file_pool, file_path, prefix)) {
        notify(NFY_INFO, "    %s", file_path.c_str());
      } else {
        ++error_count;
        notify(NFY_WARN, "ERROR: Bundle file '%s' is corrupt:",
               file_path.c_str());
        notify(NFY_WARN, "    %s", file_path.c_str());
      }
    } else {
      notify(NFY_INFO, "    [empty slot]");
    }
  }
  if (error_count != 0) {
    notify(NFY_FATAL, "ERROR: problems found in index header.");
    return 2;
  }

  // Create a traverser for the index

  khDeleteGuard<geindexcheck::CheckTraverser> traverser;

  if (header->contentDesc == kUnifiedType) {
    // Primary unified index
    traverser = khTransferGuard<geindexcheck::CheckTraverser>
        (new geindexcheck::SpecificCheckTraverser<geindex::UnifiedBucket>
         (file_pool, index_path));
  } else if (header->contentDesc == kImageryType) {
    // Index of image files
    traverser = khTransferGuard<geindexcheck::CheckTraverser>
        (new geindexcheck::SpecificCheckTraverser<geindex::BlendBucket>
         (file_pool, index_path));
  } else if (header->contentDesc == kTerrainType) {
    // Index of terrain files
    traverser = khTransferGuard<geindexcheck::CheckTraverser>
        (new geindexcheck::SpecificCheckTraverser<geindex::BlendBucket>
         (file_pool, index_path));
  } else if (header->contentDesc == kVectorGeType) {
    // Index of vector files
    traverser = khTransferGuard<geindexcheck::CheckTraverser>
        (new geindexcheck::SpecificCheckTraverser<geindex::VectorBucket>
         (file_pool, index_path));
  } else {
    notify(NFY_FATAL,
           "ERROR: index content type \"%s\" not supported by this program",
           header->contentDesc.c_str());
    return 2;
  }

  // If we're going to read the data as well as the index, open the
  // packet files

  if (mode == kModeAll) {
    traverser->OpenPacketFiles(file_pool);
  }

  // Read all the index locations

  std::uint64_t location_count = 0;
  std::uint64_t entry_count = 0;
  while (traverser->Active()) {
    ++location_count;
    entry_count += traverser->CurrentEntryCount();

    if (progress != 0  &&  (location_count % progress) == 0) {
      notify(NFY_PROGRESS, "Processed %llu locations",
             static_cast<unsigned long long>(location_count));
    }
    notify(NFY_VERBOSE, "Read index entry bucket with %u entries at: \"%s\"",
           static_cast<unsigned>(traverser->CurrentEntryCount()),
           traverser->CurrentPath().AsString().c_str());

    // If requested, read all the data entries
    if (mode == kModeAll) {
      traverser->ReadCurrentEntryData();
    }

    traverser->Advance();
  }
  traverser->Close();

  notify(NFY_NOTICE,
         "Check finished: %llu index locations, %llu total entries.",
         static_cast<unsigned long long>(location_count),
         static_cast<unsigned long long>(entry_count));

  return 0;
}
