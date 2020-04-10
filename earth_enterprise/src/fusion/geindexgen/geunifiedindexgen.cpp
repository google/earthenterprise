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


#include <unistd.h>
#include <khGetopt.h>
#include <notify.h>
#include <geFilePool.h>
#include <khAbortedException.h>
#include <geindex/Traverser.h>
#include <geindex/Writer.h>
#include <khFileUtils.h>
#include <geindexgen/.idl/UnifiedConfig.h>
#include <packetfile/packetfile.h>
#include <packetfile/packetindex.h>
#include <geindex/PacketFileAdaptingTraverser.h>
#include <khProgressMeter.h>
#include <common/khConstants.h>

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s --config <configfile> --output <out.geindex>\n"
          "   A unified index is generated from indexeslisted in <configfile>.\n"
          "   If out.geindex already exists, its contents are over written.\n"
          "   Supported options are:\n"
          "      --help | -?:  Display this usage message\n"
          "      --delta <prevdir>: Perform a delta merge against prevdir\n",
          progn.c_str());
  exit(1);
}

void PopulateFilenumTranslations(
    geindex::UnifiedWriter &writer,
    std::map<std::string, std::uint32_t> &unified_filemap,
    std::vector<std::uint32_t> &filenums,
    khProgressMeter &progress_meter,
    const geindex::IndexBundleReader &index_bundle_reader);
void PopulatePacketfileFilenumTranslations(
    geindex::UnifiedWriter &writer,
    std::map<std::string, std::uint32_t> &unified_filemap,
    std::vector<std::uint32_t> &filenums,
    khProgressMeter &progress_meter,
    const std::string &packetfile);

int main(int argc, char **argv) {
  try {
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    std::string outdir;
    std::string configfilename;
    std::string delta;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("output", outdir);
    options.opt("config", configfilename, &khGetopt::FileExists);
    options.opt("delta", delta, &khGetopt::DirExists);

    if (!options.processAll(argc, argv, argn))
      usage(progname);
    if (help)
      usage(progname);
    if (argn != argc)
      usage(progname);

    // Validate commandline options
    if (!outdir.size()) {
      usage(progname, "No output specified");
    }
    if (!configfilename.size()) {
      usage(progname, "No --config specified");
    }
    if (delta.size()) {
      notify(NFY_FATAL, "--delta not supported yet.");
    }

    geFilePool file_pool;

    geindexgen::UnifiedConfig config;
    if (!config.Load(configfilename)) {
      usage(progname, "Unable to load %s", configfilename.c_str());
    }
    // Print the input file sizes for diagnostic log file info.
    // Here we want to take in the sizes of the indexes from the config file.
    std::vector<std::string> input_files;
    input_files.push_back(configfilename);
    for (std::vector<geindexgen::UnifiedConfig::Entry>::const_iterator
           entry = config.indexes_.begin(); entry != config.indexes_.end();
         ++entry) {
      input_files.push_back(entry->indexdir_);
    }
    khPrintFileSizes("Input File Sizes", input_files);


    // create the writer
    geindex::UnifiedWriter writer(file_pool, outdir,
                                  geindex::UnifiedWriter::FullIndexMode,
                                  kUnifiedType);
    geindex::UnifiedWriter::ReadBuffer  tmp_read_buf;

    // pre-populate this for delta index operations
    // leave empty for new indexes
    std::map<std::string, std::uint32_t> unified_filemap;

    // progress meter - will need to be modified a bit when
    // incremental updates are implemented (TODO: mikegoss)
    khProgressMeter progress_meter(0, "entries");

    // will be filled in with the mapping from old file numbers to new
    // file numbers
    std::vector<std::vector<std::uint32_t> > translated_filenums;
    translated_filenums.resize(config.indexes_.size());
    std::map<std::string, std::uint32_t> dated_imagery_channels_map;

    typedef geindex::AdaptingTraverserBase<geindex::UnifiedBucket>::MergeEntry MergeEntryType;
    typedef Merge<MergeEntryType> MergeType;
    MergeType merger("UnifiedIndex Merger");
    unsigned int source_id = 0;
    for (std::vector<geindexgen::UnifiedConfig::Entry>::const_iterator
           entry = config.indexes_.begin(); entry != config.indexes_.end();
         ++entry, ++source_id) {
      std::string entry_type = entry->type_;
      geindex::TypedEntry::TypeEnum type;
      FromString(entry->type_, type);

      if (type == geindex::TypedEntry::Imagery ||
          type == geindex::TypedEntry::DatedImagery) {
        // For DatedImagery, keep track of the date to channel map.
        dated_imagery_channels_map[entry->date_string_] = entry->channel_id_;

        typedef geindex::Traverser<geindex::BlendBucket> BlendTraverser;
        khTransferGuard<BlendTraverser> traverser =
          TransferOwnership(new BlendTraverser(
                                "ImageryTraverser",
                                file_pool, entry->indexdir_));
        PopulateFilenumTranslations(writer, unified_filemap,
                                    translated_filenums[source_id],
                                    progress_meter,
                                    traverser->GetIndexBundleReader());
        merger.AddSource(
            TransferOwnership(
                new geindex::UnifiedAdaptingTraverser<BlendTraverser>(
                    "ImageryAdaptingTraverser",
                    geindex::TypedEntry::Imagery,
                    traverser,
                    entry->channel_id_,
                    entry->version_)));
      } else if (type == geindex::TypedEntry::Terrain) {
        if (!PacketFile::IsPacketFile(entry->indexdir_)) {
          throw khException(
              kh::tr("INTERNAL ERROR: Terrain path must be a packetfile: %1")
              .arg(entry->indexdir_.c_str()));
        }

        // register packetfile
        PopulatePacketfileFilenumTranslations(writer, unified_filemap,
                                              translated_filenums[source_id],
                                              progress_meter,
                                              entry->indexdir_);

        // make and adapting traverser
        merger.AddSource(
            TransferOwnership(
                new geindex::UnifiedPacketFileAdaptingTraverser(
                    file_pool,
                    "TerrainPacketAdaptingTraverser",
                    geindex::TypedEntry::Terrain,
                    entry->version_,
                    entry->channel_id_,
                    entry->indexdir_)));
      } else if (type == geindex::TypedEntry::VectorGE) {
        typedef geindex::Traverser<geindex::VectorBucket> VectorTraverser;
        khTransferGuard<VectorTraverser> traverser =
          TransferOwnership(new VectorTraverser(
                                "VectorTraverser",
                                file_pool, entry->indexdir_));
        PopulateFilenumTranslations(writer, unified_filemap,
                                    translated_filenums[source_id],
                                    progress_meter,
                                    traverser->GetIndexBundleReader());
        merger.AddSource(
            TransferOwnership(
                new geindex::UnifiedAdaptingTraverser<VectorTraverser>(
                    "VectorGEAdaptingTraverser",
                    geindex::TypedEntry::VectorGE,
                    traverser,
                    0 /* unused override channel id */,
                    0 /* unused override version */)));
      } else if (type == geindex::TypedEntry::VectorMaps) {
        if (!PacketFile::IsPacketFile(entry->indexdir_)) {
          throw khException(
              kh::tr("INTERNAL ERROR: VectorMaps "
                     "path must be a packetfile: %1")
              .arg(entry->indexdir_.c_str()));
        }

        // register packetfile
        PopulatePacketfileFilenumTranslations(writer, unified_filemap,
                                              translated_filenums[source_id],
                                              progress_meter,
                                              entry->indexdir_);

        // make and adapting traverser that uses the packetfile
        // number from above
        merger.AddSource(
            TransferOwnership(
                new geindex::UnifiedPacketFileAdaptingTraverser(
                    file_pool,
                    "VectorMapsAdaptingTraverser",
                    geindex::TypedEntry::VectorMaps,
                    entry->version_,
                    entry->channel_id_,
                    entry->indexdir_)));
      } else if (type == geindex::TypedEntry::VectorMapsRaster) {
        if (!PacketFile::IsPacketFile(entry->indexdir_)) {
          throw khException(
              kh::tr("INTERNAL ERROR: VectorMapsRaster "
                     "path must be a packetfile: %1")
              .arg(entry->indexdir_.c_str()));
        }

        // register packetfile
        PopulatePacketfileFilenumTranslations(writer, unified_filemap,
                                              translated_filenums[source_id],
                                              progress_meter,
                                              entry->indexdir_);

        // make and adapting traverser that uses the packetfile
        // number from above
        merger.AddSource(
            TransferOwnership(
                new geindex::UnifiedPacketFileAdaptingTraverser(
                    file_pool,
                    "VectorMapsRasterAdaptingTraverser",
                    geindex::TypedEntry::VectorMapsRaster,
                    entry->version_,
                    entry->channel_id_,
                    entry->indexdir_)));
      } else if (type == geindex::TypedEntry::QTPacket ||
                 type == geindex::TypedEntry::QTPacket2) {
        if (!PacketFile::IsPacketFile(entry->indexdir_)) {
          throw khException(
              kh::tr("INTERNAL ERROR: QTPacket path must be a packetfile: %1")
              .arg(entry->indexdir_.c_str()));
        }

        // register packetfile
        PopulatePacketfileFilenumTranslations(writer, unified_filemap,
                                              translated_filenums[source_id],
                                              progress_meter,
                                              entry->indexdir_);

        // make and adapting traverser that uses the packetfile
        // number from above
        merger.AddSource(
            TransferOwnership(
                new geindex::UnifiedPacketFileAdaptingTraverser(
                    file_pool,
                    "QTPacketAdaptingTraverser",
                    type,
                    entry->version_,
                    0 /* channel */,
                    entry->indexdir_)));
      } else if (type == geindex::TypedEntry::Unified) {
        typedef geindex::Traverser<geindex::UnifiedBucket> UnifiedTraverser;
        khTransferGuard<UnifiedTraverser> traverser =
          TransferOwnership(new UnifiedTraverser(
                                "VectorMapsTraverser",
                                file_pool, entry->indexdir_));
        PopulateFilenumTranslations(writer, unified_filemap,
                                    translated_filenums[source_id],
                                    progress_meter,
                                    traverser->GetIndexBundleReader());
        merger.AddSource(traverser);
      } else {
        throw khException(
            kh::tr("INTERNAL ERROR: Unknown input type: %1")
            .arg(entry->type_.c_str()));
      }
    }


    //  perform the merge
    merger.Start();
    do {
      const MergeEntryType &slot = merger.Current();
      unsigned int source_id = merger.CurrentSourceId();
      for (unsigned int i = 0; i < slot.size(); ++i) {
        geindex::TypedEntry entry = slot[i];
        entry.dataAddress.fileNum =
          translated_filenums[source_id][entry.dataAddress.fileNum];
        writer.Put(slot.qt_path(), entry, tmp_read_buf);
      }
      progress_meter.incrementDone(slot.size());
    } while (merger.Advance());
    merger.Close();

    writer.Close();

    // Write the dated_imagery_channels.txt
    // only if we have more than 1 dated imagery layer.
    if (dated_imagery_channels_map.size() > 1) {
      std::string dated_channels_file_name = outdir + "/" +
        kDatedImageryChannelsFileName;
      notify(NFY_DEBUG, "Writing Dated Imagery channels to:\n  %s",
             dated_channels_file_name.c_str());
      FILE* fp  = fopen(dated_channels_file_name.c_str(), "w");
      std::map<std::string, std::uint32_t>::const_iterator iter =
        dated_imagery_channels_map.begin();
      for (; iter != dated_imagery_channels_map.end(); ++iter) {
        fprintf(fp, "%s %d\n", iter->first.c_str(), iter->second);
      }
      fclose(fp);
    }

    // On successful completion, print out the output file sizes.
    std::vector<std::string> output_files;
    output_files.push_back(outdir);
    khPrintFileSizes("Output File Sizes", output_files);
  } catch (const khAbortedException &e) {
    notify(NFY_FATAL, "Unable to proceed: See previous warnings");
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }

  return 0;
}


void PopulateFilenumTranslations(
    geindex::UnifiedWriter &writer,
    std::map<std::string, std::uint32_t> &unified_filemap,
    std::vector<std::uint32_t> &filenums,
    khProgressMeter &progress_meter,
    const geindex::IndexBundleReader &index_bundle_reader)
{
  filenums.resize(index_bundle_reader.PacketFileCount());
  for (unsigned int i = 0; i < index_bundle_reader.PacketFileCount(); ++i) {
    std::string packetfile = index_bundle_reader.GetPacketFile(i);
    // skip empty ones - results of delta indexing
    if (!packetfile.empty()) {
      std::map<std::string, std::uint32_t>::const_iterator found =
        unified_filemap.find(packetfile);
      if (found != unified_filemap.end()) {
        filenums[i] = found->second;
      } else {
        unified_filemap[packetfile] =
          filenums[i] = writer.AddExternalPacketFile(packetfile);
        progress_meter.incrementTotal(
            PacketIndexReader::NumPackets(
                PacketFile::IndexFilename(packetfile)));
      }
    }
  }
}


void PopulatePacketfileFilenumTranslations(
    geindex::UnifiedWriter &writer,
    std::map<std::string, std::uint32_t> &unified_filemap,
    std::vector<std::uint32_t> &filenums,
    khProgressMeter &progress_meter,
    const std::string &packetfile)
{
  filenums.resize(1);
  std::map<std::string, std::uint32_t>::const_iterator found =
    unified_filemap.find(packetfile);
  if (found != unified_filemap.end()) {
    filenums[0] = found->second;
  } else {
    unified_filemap[packetfile] =
      filenums[0] = writer.AddExternalPacketFile(packetfile);
    progress_meter.incrementTotal(
        PacketIndexReader::NumPackets(
            PacketFile::IndexFilename(packetfile)));
  }
}
