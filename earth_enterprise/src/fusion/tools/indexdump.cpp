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


#include <limits>
#include <stdio.h>
#include <khConstants.h>
#include <khGetopt.h>
#include <notify.h>
#include <khFileUtils.h>
#include <khEndian.h>
#include <geindex/Reader.h>
#include <serverdb/serverdb.h>
#include <quadtreepath.h>
#include <geindex/IndexManifest.h>
#include <ManifestEntry.h>

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s [options] --request <request type> <geindex>\n"
          "   Supported options are:\n"
          "      --help | -?:             Display this usage message\n"
          "      --path <quadtree path>   Only dump file with this path\n"
          "",
          progn.c_str());
  exit(1);
}

const std::uint32_t kInvalidInt = std::numeric_limits<std::uint32_t>::max();

 std::uint32_t ValidateAndInvertRow(std::uint32_t level, std::uint32_t row, std::uint32_t col, char* cmd) {
  if (level == kInvalidInt || row == kInvalidInt || col == kInvalidInt)
    usage(cmd, "must specify level, row & col with ImageryMaps");
  return (1 << level) - row - 1;
}

int main(int argc, char** argv) {
  try {
    std::string progname = argv[0];

    // ********** process commandline options **********
    int argn;
    khGetopt options;
    bool help = false;
    std::string blist;
    std::uint32_t channel = 1;
    std::uint32_t version = 1;
    std::uint32_t level = kInvalidInt;
    std::uint32_t row = kInvalidInt;
    std::uint32_t col = kInvalidInt;
    std::string request;
    bool dump_manifest = false;

    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.flagOpt("manifest", dump_manifest);
    options.opt("path", blist);
    options.opt("channel", channel);
    options.opt("version", version);
    options.opt("request", request);
    options.opt("level", level);
    options.opt("row", row);
    options.opt("col", col);

    if (!options.processAll(argc, argv, argn) || help)
      usage(progname);

    const char* index_path = GetNextArg();
    if (!index_path)
      usage(argv[0], "<geindex> not specified");

    QuadtreePath qpath(blist);

    geFilePool file_pool;

    // dump out index manifest
    if (dump_manifest) {
      printf("Index Manifest:\n");
      std::vector<ManifestEntry> manifest_entries;
      geindex::IndexManifest::GetManifest(file_pool, index_path,
                                          manifest_entries, "", "", "");
      size_t count = 0;
      for (std::vector<ManifestEntry>::iterator it = manifest_entries.begin();
           it != manifest_entries.end(); ++it) {
        printf("[%zu] Data size = %zu  --------------------------------------\n",
               count++, it->data_size);
        printf("  Orig path: %s\n", it->orig_path.c_str());
        if (it->orig_path != it->current_path)
          printf("  Curr path: %s\n", it->current_path.c_str());
      }
    }

    geindex::TypedEntry::TypeEnum request_type = (geindex::TypedEntry::TypeEnum)0;
    if (request == "ImageryGE") {
      request_type = geindex::TypedEntry::Imagery;
    } else if (request == "ImageryMaps") {
      request_type = geindex::TypedEntry::Imagery;
      row = ValidateAndInvertRow(level, row, col, argv[0]);
      qpath = QuadtreePath(level, row, col);
    } else if (request == kVectorMapsType) {
      request_type = geindex::TypedEntry::VectorMaps;
      row = ValidateAndInvertRow(level, row, col, argv[0]);
      qpath = QuadtreePath(level, row, col);
    } else if (request == kVectorMapsRasterType) {
      request_type = geindex::TypedEntry::VectorMapsRaster;
      row = ValidateAndInvertRow(level, row, col, argv[0]);
      qpath = QuadtreePath(level, row, col);
    } else {
      return 0;
    }

    geindex::UnifiedReader index_reader(file_pool, index_path, 1);

    geindex::UnifiedReader::ReadBuffer buf;

    geindex::TypedEntry::ReadKey read_key(version, channel,
                                          request_type, false);
    const bool size_only = false;
    index_reader.GetData(qpath, read_key, buf, size_only);

    printf("writing file: %s.png\n", qpath.AsString().c_str());

    khWriteSimpleFile(qpath.AsString() + ".png", buf.c_str(), buf.length());

  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }

  return 0;
}
