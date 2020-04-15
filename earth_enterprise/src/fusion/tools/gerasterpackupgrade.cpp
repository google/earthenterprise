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


// pidxtopacketindex pack_directory
//
// Tool to convert old style pack.idx index for image or terrain pack
// files into new packetindex format.  The original data files are
// unmodified.  Additional files bundle.hdr and packetindex are
// created in the same directory with the pack.idx file and pack.nn
// files.

#include <string>
#include <string.h>

#include "common/khGetopt.h"
#include "common/khSimpleException.h"
#include "common/khFileUtils.h"
#include "common/geFilePool.h"
#include "common/quadtreepath.h"
#include "common/ff.h"
#include "fusion/ffio/ffioIndex.h"
#include "common/packetfile/packetfilewriter.h"
#include "common/packetfile/packetfilereader.h"
#include "common/khProgressMeter.h"
#include "fusion/khraster/AttributionByExtents.h"
#include "fusion/autoingest/.idl/storage/PacketLevelConfig.h"

void
usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(
      stderr,
      "\nusage: %s [options] --config <file> --input <dir> --output <dir>\n"
      "   Converts old raster pack files to new packet files.\n"
      "   This is a quick operation, requiring only a index conversion.\n"
      "   Supported options are:\n"
      "      --help | -?:      Display this usage message\n"
      "      --readback:       Double-check the conversion\n",
      progn.c_str());
  exit(1);
}

static void ReadbackOutput(geFilePool &file_pool,
                           const std::string path) {
  FileBundleReader data_reader(file_pool, path);
  PacketIndexReader index_reader(file_pool,
                                 khComposePath(path, PacketFile::kIndexBase));

  assert(!index_reader.data_has_crc());

  // Read each entry from the index.  Verify that entries are in
  // order.  For each entry, read the FFRecHeader and verify that the
  // path matches the index.
  QuadtreePath last_qt_path;
  PacketIndexEntry index_entry;
  std::uint64_t readback_count = 0;

  while (index_reader.ReadNext(&index_entry)) {
    if (index_entry.qt_path() < last_qt_path) {
      notify(NFY_FATAL, "index readback out of order at %s",
             index_entry.qt_path().AsString().c_str());
    }
    last_qt_path = index_entry.qt_path();

    // The position in the index entry points to the start of the
    // data.  Read the FFRecHeader which precedes the data.
    FFRecHeader header(0, 0, 0, 0, 0);
    data_reader.ReadAt(index_entry.position() - sizeof(header),
                       &header,
                       sizeof(header));
    header.BigEndianToHost();
    if (QuadtreePath(header.level(), header.y(), header.x())
                     != index_entry.qt_path()) {
      notify(NFY_FATAL, "data/index mismatch at %s, position %llu",
             index_entry.qt_path().AsString().c_str(),
             static_cast<unsigned long long>(index_entry.position()));
    }
    ++readback_count;
  }
  notify(NFY_NOTICE, "Verified %llu records",
         static_cast<unsigned long long>(readback_count));
}


int main(int argc, char *argv[]) {
  try {
    int argn;
    bool readback = false;
    bool help = false;
    std::string input;
    std::string output;
    std::string configfile;
    khGetopt options;
    options.flagOpt("readback", readback);  // read and check output file
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("config", configfile, &khGetopt::FileExists);
    options.opt("input", input, &khGetopt::DirExists);
    options.opt("output", output);
    options.setRequired("input", "output");

    if (!options.processAll(argc, argv, argn)  ||
        help || (argn != argc)) {
      usage(argv[0]);
    }


    // Build object to help with the per-tile picking of the inset to
    // provide the attribution string
    AttributionByExtents attributions;
    {
      RasterPackUpgradeConfig config;
      if (!config.Load(configfile)) {
        notify(NFY_FATAL, "Unable to load config file %s. Cannot proceed.",
               configfile.c_str());
      }
      for (std::vector<PacketLevelConfig::Attribution>::iterator
             attribution = config.attributions.begin();
           attribution != config.attributions.end(); ++attribution) {
        attributions.AddInset(attribution->dataRP,
                              attribution->fuid_resource_,
                              kUnknownDateTimeUTC);
      }
    }


    // copy the inputdir to the output
    // use hardlinks if possible
    notify(NFY_NOTICE, "Copying %s to %s",
           input.c_str(), output.c_str());
    khLinkOrCopyDirContents(input, output);


    geFilePool file_pool;

    // Create a file bundle for the input pack file set
    PacketFilePackIndexer indexer(file_pool, output);

    // Load the pack.idx file
    ffio::IndexReader pack_idx(ffio::IndexFilename(indexer.abs_path()));

    // count how many index entries we need to process
    std::uint64_t num_entries = 0;
    for (size_t level = 0; level < pack_idx.levelvec.size(); ++level) {
      const ffio::IndexReader::LevelInfo &level_info =
        pack_idx.levelvec[level];
      for (std::uint32_t row = level_info.extents.beginRow();
           row < level_info.extents.endRow();
           ++row ) {
        num_entries += level_info.extents.width();
      }
    }

    std::uint64_t indexed_size = 0;              // amount of data indexed
    std::uint64_t indexed_records = 0;           // number of records indexed

    {
      // add scope to control lifetime of progress object (and thereby the
      // emitting of the final progress message)
      khProgressMeter progress(num_entries);

      // Write an index entry for each non-empty tile
      for (size_t level = 0; level < pack_idx.levelvec.size(); ++level) {
        const ffio::IndexReader::LevelInfo &level_info =
          pack_idx.levelvec[level];
        for (std::uint32_t row = level_info.extents.beginRow();
             row < level_info.extents.endRow();
             ++row ) {
          for (std::uint32_t col = level_info.extents.beginCol();
               col < level_info.extents.endCol();
               ++col ) {
            std::uint64_t linear_offset;
            std::uint32_t record_size;
            if (level_info.FindTile(row, col, linear_offset, record_size)) {
              QuadtreePath qt_path(level_info.level, row, col);
              indexer.WriteAppendIndex
                (qt_path,
                 indexer.LinearToBundlePosition(linear_offset),
                 record_size,
                 attributions.GetInsetId(khTileAddr(qt_path)));
              indexed_size += FFRecHeader::paddedLen(record_size)
                              + sizeof(FFRecHeader);
              ++indexed_records;
            }
            progress.increment();
          }
        }
      }
    }

    if (indexed_size != indexer.data_size()) {
      notify(NFY_WARN, "Indexed %llu bytes, should be %llu",
             static_cast<unsigned long long>(indexed_size),
             static_cast<unsigned long long>(indexer.data_size()));
    }

    // Write the file bundle header
    indexer.Close();

    notify(NFY_NOTICE, "indexed  %llu records, %llu bytes",
           static_cast<unsigned long long>(indexed_records),
           static_cast<unsigned long long>(indexed_size));

    // If requested, read back the index and verify all records
    if (readback) {
      ReadbackOutput(file_pool, argv[argc-1]);
    }
  } catch(const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  }

  return 0;
}
