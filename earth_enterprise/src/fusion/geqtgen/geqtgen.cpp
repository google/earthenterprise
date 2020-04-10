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

// Generate a packet file containing quadtree packets. Input is one or
// more indexes or packet file indexes of imagery, terrain, and/or vector data.
//
// Usage: geqtgen --config

#include <unistd.h>
#include <khGetopt.h>
#include <notify.h>
#include <geFilePool.h>
#include <khConstants.h>
#include <khAbortedException.h>
#include <khSimpleException.h>

#include <khProgressMeter.h>
#include <geqtgen/.idl/Config.h>
#include <geindex/PacketFileAdaptingTraverser.h>
#include <geindex/Entries.h>
#include <khFileUtils.h>
#include <packetcompress.h>
#include <packetfile/packetfile.h>
#include <packetfile/packetfilewriter.h>
#include <protobuf/quadtreeset.pb.h>
#include <qtpacket/quadset_gather.h>
#include <qtpacket/quadtreepacket.h>
#include <qtpacket/quadtree_processing.h>
#include <qtpacket/quadtreepacketitem.h>

namespace {

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s --config <configfile> [--output_format1 <outdir1>] "
          "[--output_format2 <outdir2>]\n"
          "   Quadtree packets are generated from the packet in the indexes\n"
          "   listed in <configfile>. If any of the output packet files "
          "<outdir1/2>\n"
          "   already exists, its contents are overwritten.\n"
          "   One or both of  --output_format1 or --output_format2 must be "
          "present\n"
          "   to specify which output formats are written.\n"
          "     format1 is for Google Earth 4.3 and earlier.\n"
          "     format2 is for Google Earth 4.4 and later.\n"
          "   Supported options are:\n"
          "      --help | -?:  Display this usage message\n"
          "      --delta <prevdir>: Only generate packets that have changed\n"
          "        (not implemented yet)\n"
          "      --sortbuf <buffer_size>: size of packet index sort buf in MB\n"
          "        (default 64)\n",
          progn.c_str());
  exit(1);
}

//-----------------------------------------------------------------------------

// Define an adapter to translate the output of the merge into
// QuadtreePacketItem format

typedef geindex::AdaptingTraverserBase<geindex::AllInfoBucket>::MergeEntry QTPMergeEntryType;
typedef Merge<QTPMergeEntryType> QTPMergeType;

class QuadtreePacketSource
    : public MergeSource<qtpacket::QuadsetItem<qtpacket::QuadtreePacketItem> > {
 public:
  QuadtreePacketSource(const std::string &name,
                       khTransferGuard<MergeSource<QTPMergeEntryType> > source,
                       const std::map<std::uint32_t,std::uint32_t> &inset_provider_map,
                       const std::map<std::uint32_t, std::string> &channel_to_date_map)
      : MergeSource<MergeType>(name),
        source_(source),
        source_item_index_(0),
        current_(MergeType(QuadtreePath(),
                           qtpacket::QuadtreePacketItem(0, 0,
                                                        0, kUnknownDate))),
        finished_(false),
        inset_provider_map_(inset_provider_map),
        channel_to_date_map_(channel_to_date_map)
  {
    Advance();
  }
  virtual ~QuadtreePacketSource() {
    if (!std::uncaught_exception()) {
      if (source_) {
        throw khSimpleException("QuadtreePacketSource: Close() not called: ")
          << MergeSource<MergeType>::name();
      }
    }
  }
  virtual const MergeType &Current() const {
    if (!finished_) {
      return current_;
    } else {
      throw khSimpleException("QuadtreePacketSource: late call to Current(): ")
        << MergeSource<MergeType>::name();
    }
  }
  virtual bool Advance() {
    if (!finished_) {
      assert(source_);
      while (source_item_index_ >= source_->Current().size()) {
        if (!source_->Advance()) {
          finished_ = true;
          return false;
        }
        source_item_index_ = 0;
      }

      TranslateSource(source_->Current().qt_path(),
                      source_->Current()[source_item_index_]);
      ++source_item_index_;
      return true;
    } else {
      return false;
    }
  }
  virtual void Close() {
    if (source_) {
      source_->Close();
      source_.clear();
    }
    finished_ = true;
  }
 private:
  khDeleteGuard<MergeSource<QTPMergeEntryType> > source_;
  size_t source_item_index_;
  MergeType current_;
  bool finished_;
  const std::map<std::uint32_t,std::uint32_t> &inset_provider_map_;
  const std::map<std::uint32_t, std::string> &channel_to_date_map_;

  std::uint32_t LookupProvider(std::uint32_t inset) const {
    if (inset != 0) {
      std::map<std::uint32_t, std::uint32_t>::const_iterator found =
        inset_provider_map_.find(inset);
      if (found != inset_provider_map_.end()) {
        //        notify(NFY_NOTICE, "%u -> %u", inset, found->second);
        return found->second;
      } else {
        //        notify(NFY_NOTICE, "%u -> ???", inset);
      }
    }
    return 0;
  }

  // Translate from AllInfoEntry output of source to QuadtreePacketItem
  void TranslateSource(const QuadtreePath qt_path,
                       const geindex::AllInfoEntry &source_item) {
    std::uint32_t level, row, col;
    qt_path.GetLevelRowCol(&level, &row, &col);
    switch (source_item.type) {
      case geindex::TypedEntry::Imagery: {
        // not emitting a provider for the first couple of levels helps to
        // eliminate the "sticky" provider problem (where NASA shows up as
        // provider even when zoomed all the way in to high res inset) the
        // client never pulls from level 0 or 1 anyway to draw the front of
        // the globe. So we still the the NASA copyright even when zoomed
        // all the way out.
        std::uint32_t provider = (level > 1)
                          ? LookupProvider(source_item.insetId)
                          : 0;
        std::string date_string;
        std::map<std::uint32_t, std::string>::const_iterator iter =
          channel_to_date_map_.find(source_item.channel);
        if (iter != channel_to_date_map_.end()) {
          date_string = iter->second;
        }
        current_ = MergeType(qt_path,
                             qtpacket::QuadtreePacketItem(
                               qtpacket::QuadtreePacketItem::kLayerImagery,
                               source_item.version,
                               provider,
                               date_string));
        break;
      }
      case geindex::TypedEntry::DatedImagery: {
        // not emitting a provider for the first couple of levels helps to
        // eliminate the "sticky" provider problem (where NASA shows up as
        // provider even when zoomed all the way in to high res inset) the
        // client never pulls from level 0 or 1 anyway to draw the front of
        // the globe. So we still the the NASA copyright even when zoomed
        // all the way out.
        std::uint32_t provider = (level > 1)
                          ? LookupProvider(source_item.insetId)
                          : 0;
        std::string date_string;
        std::map<std::uint32_t, std::string>::const_iterator iter =
          channel_to_date_map_.find(source_item.channel);
        if (iter != channel_to_date_map_.end()) {
          date_string = iter->second;
        }
        current_ = MergeType(qt_path,
                             qtpacket::QuadtreePacketItem(
                               qtpacket::QuadtreePacketItem::kLayerDatedImagery,
                               source_item.version,
                               provider,
                               date_string));
        break;
      }
      case geindex::TypedEntry::Terrain: {
        // not emitting a provider for the first couple of levels helps to
        // eliminate the "sticky" provider problem (where NASA shows up as
        // provider even when zoomed all the way in to high res inset) the
        // client never pulls from level 0 or 1 anyway to draw the front of
        // the globe. So we still the the NASA copyright even when zoomed
        // all the way out.
        std::uint32_t provider = (level > 1)
                          ? LookupProvider(source_item.insetId)
                          : 0;
        current_ = MergeType(qt_path,
                             qtpacket::QuadtreePacketItem(
                                 qtpacket::QuadtreePacketItem::kLayerTerrain,
                                 source_item.version,
                                 provider,
                                 kUnknownDate));
        break;
      }
      case geindex::TypedEntry::VectorGE:
        current_ = MergeType(qt_path,
                             qtpacket::QuadtreePacketItem(
                                 qtpacket::QuadtreePacketItem::kLayerVectorMin
                                 + source_item.channel,
                                 source_item.version,
                                 0 /* unused providerId */,
                                 kUnknownDate));
        break;
      default:
        throw khSimpleException(
            "QuadtreePacketSource: unsupported packet type: ")
              << ToString(source_item.type);
    }
  }

  DISALLOW_COPY_AND_ASSIGN(QuadtreePacketSource);
};

// CountIndexPackets - return count of packets in packet files of index

 std::uint64_t CountIndexPackets(const geindex::IndexBundle &bundle) {
  std::uint64_t packet_count = 0;
  std::uint32_t packet_file_count = bundle.PacketFileCount();
  for (std::uint32_t i = 0; i < packet_file_count; ++i) {
    const std::string &packet_file_name = bundle.GetPacketFile(i);
    if (!packet_file_name.empty()) {
      packet_count +=
          PacketIndexReader::NumPackets(
              PacketFile::IndexFilename(packet_file_name));
    }
  }
  return packet_count;
}

} // namespace

//-----------------------------------------------------------------------------

int main(int argc, char **argv) {
  try {
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    std::string outdir_format1;
    std::string outdir_format2;
    std::string configfilename;
    std::string delta;
    int sortbuf = 64;

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("output_format1", outdir_format1);
    options.opt("output_format2", outdir_format2);
    options.opt("config", configfilename, &khGetopt::FileExists);
    options.opt("delta", delta, &khGetopt::DirExists);
    options.opt("sortbuf", sortbuf);

    if (!options.processAll(argc, argv, argn))
      usage(progname);
    if (help)
      usage(progname);
    if (argn != argc)
      usage(progname);

    // Validate commandline options
    if (outdir_format1.empty() && outdir_format2.empty()) {
      usage(progname, "No output specified");
    }
    if (!configfilename.size()) {
      usage(progname, "No --config specified");
    }
    if (delta.size()) {
      notify(NFY_FATAL, "--delta not supported yet.");
    }
    if (sortbuf <= 0) {
      notify(NFY_FATAL, "--sortbuf must be > 0, is %d", sortbuf);
    }

    geFilePool file_pool;

    geqtgen::Config config;
    if (!config.Load(configfilename)) {
      usage(progname, "Unable to load %s", configfilename.c_str());
    }

    if (config.index_version_ == 0) {
      usage(progname, "Index version missing");
    }

     // Print the input file sizes for diagnostic log file info.
    // Here we want to take in the sizes of the insets from the config file.
    std::vector<std::string> input_files;
    input_files.push_back(configfilename);
    input_files.push_back(config.imagery_index_);
    input_files.push_back(config.terrain_index_);
    input_files.push_back(config.vector_index_);
    for (unsigned int i = 0; i < config.dated_imagery_indexes_.size(); ++i) {
      input_files.push_back(config.dated_imagery_indexes_[i].
                            imagery_index_version_);
    }
    khPrintFileSizes("Input File Sizes", input_files);

    // Must create a map from imagery channels to date strings.
    std::map<std::uint32_t, std::string> channel_to_date_map;

    khDeleteGuard<QTPMergeType> merger(
        TransferOwnership(new QTPMergeType("QTPacketGen Merger")));

    khProgressMeter progress_meter(0, "packets");

    if (!config.imagery_index_.empty()) {
      typedef geindex::Traverser<geindex::BlendBucket> BlendTraverser;
      khDeleteGuard<BlendTraverser>
          traverser(TransferOwnership(new BlendTraverser(
                                          "ImageryTraverser",
                                          file_pool, config.imagery_index_)));
      progress_meter.incrementTotal(
          CountIndexPackets(traverser->GetIndexBundleReader()));
      merger->AddSource(
          TransferOwnership(
              new geindex::AllInfoAdaptingTraverser<BlendTraverser>(
                  "ImageryAdaptingTraverser",
                  geindex::TypedEntry::Imagery,
                  TransferOwnership(traverser),
                  kGEImageryChannelId)));
      std::string date_string = config.imagery_date_string_;
      channel_to_date_map[kGEImageryChannelId] = date_string;
    }
    if (!config.terrain_index_.empty()) {
      if (PacketFile::IsPacketFile(config.terrain_index_)) {
        progress_meter.incrementTotal(
            PacketIndexReader::NumPackets(
                PacketFile::IndexFilename(config.terrain_index_)));
        merger->AddSource(
            TransferOwnership(
                new geindex::AllInfoPacketFileAdaptingTraverser(
                    file_pool,
                    "TerrainPacketTraverser",
                    geindex::TypedEntry::Terrain,
                    config.terrain_packet_version_,
                    kGETerrainChannelId,
                    config.terrain_index_)));
      } else {
        typedef geindex::Traverser<geindex::CombinedTmeshBucket>
            CombinedTmeshTraverser;
        khDeleteGuard<CombinedTmeshTraverser>
            traverser(TransferOwnership(new CombinedTmeshTraverser(
                                            "TerrainTraverser",
                                            file_pool, config.terrain_index_)));
        progress_meter.incrementTotal(
            CountIndexPackets(traverser->GetIndexBundleReader()));
        merger->AddSource(
            TransferOwnership(
                new geindex::AllInfoAdaptingTraverser<CombinedTmeshTraverser>(
                    "TerrainAdaptingTraverser",
                    geindex::TypedEntry::Terrain,
                    TransferOwnership(traverser),
                    kGETerrainChannelId)));
      }
    }
    if (!config.vector_index_.empty()) {
      typedef geindex::Traverser<geindex::VectorBucket> VectorTraverser;
      khDeleteGuard<VectorTraverser>
          traverser(TransferOwnership(new VectorTraverser(
                                          "VectorTraverser",
                                          file_pool, config.vector_index_)));
      progress_meter.incrementTotal(
          CountIndexPackets(traverser->GetIndexBundleReader()));
      merger->AddSource(
          TransferOwnership(
              new geindex::AllInfoAdaptingTraverser<VectorTraverser>(
                  "VectorAdaptingTraverser",
                  geindex::TypedEntry::VectorGE,
                  TransferOwnership(traverser),
                  0 /* unused override channel id*/)));
    }
    for (unsigned int i = 0; i < config.dated_imagery_indexes_.size(); ++i) {
      const DatedImageryIndexInfo& dated_imagery =
        config.dated_imagery_indexes_[i];
      typedef geindex::Traverser<geindex::BlendBucket> BlendTraverser;
      khDeleteGuard<BlendTraverser>
        traverser(TransferOwnership(new BlendTraverser(
          "ImageryTraverser", file_pool,
          dated_imagery.imagery_index_version_)));
      progress_meter.incrementTotal(
          CountIndexPackets(traverser->GetIndexBundleReader()));
      std::string date_string = dated_imagery.date_string_;
      std::int32_t channel_id = dated_imagery.channel_id_;
      merger->AddSource(
          TransferOwnership(
              new geindex::AllInfoAdaptingTraverser<BlendTraverser>(
                  "DatedImageryAdaptingTraverser",
                  geindex::TypedEntry::DatedImagery,
                  TransferOwnership(traverser),
                  channel_id)));
      channel_to_date_map[channel_id] = date_string;
    }
    merger->Start();

    // The merger object constructed above pulls TypedEntry objects
    // out of the various indexes.  QuadtreePacketSource is used to
    // translate this into QuadtreePacketItem objects which are used
    // to construct the quadtree packets.
    khDeleteGuard<QuadtreePacketSource> qtp_source(
        TransferOwnership(new QuadtreePacketSource(
                              "QTP Source",
                              TransferOwnership(merger),
                              config.inset_provider_map_,
                              channel_to_date_map)));

    // The QuadtreePacketItem objects are gathered together by quadset.
    qtpacket::QuadsetGather<qtpacket::QuadtreePacketItem>
      gather("Quadset Gather", TransferOwnership(qtp_source));

    // The group of items for each quadset is converted to a quadtree
    // packet and written to a packetfile.
    // We'll construct a vector of writers (to handle case of writing
    // one or both quadtree formats) and
    const unsigned int kFormatCount = 2;  // We have format1 for 4.3 and earlier,
                                  // format2 for GE 4.4+
     // Determine which formats of QuadTree packets we need to build.
    bool build_format1 = !outdir_format1.empty();
    bool build_format2 = !outdir_format2.empty();
    khDeletingVector<PacketFileWriter> writers(kFormatCount);
    if (build_format1) {
      writers.Assign(0, new PacketFileWriter(file_pool, outdir_format1));
    }
    if (build_format2) {
      writers.Assign(1, new PacketFileWriter(file_pool, outdir_format2));
    }
    LittleEndianWriteBuffer write_buffer;
    std::string protobuf_buffer;
    std::vector<char> compressed_buffer;

    std::vector<bool> build_format(kFormatCount, false);
    build_format[0] = build_format1;
    build_format[1] = build_format2;
    do {
      // Loop through each format (we may be building 1 or both formats and
      // the code is roughly the same except the packet format.
      for (unsigned int format_id = 0; format_id < kFormatCount; ++format_id) {
        if (!build_format[format_id])
          continue;

        const qtpacket::QuadsetGroup<qtpacket::QuadtreePacketItem>&
          current_quadset = gather.Current();

        write_buffer.reset();
        // Compute quadtree packet and serialize into write_buffer.
        // Each format requires a separate class/method.
        if (format_id == 0) {
          // Build the quadtree packet (original Fusion version)
          qtpacket::KhQuadTreePacket16 packet;
          qtpacket::ComputeQuadtreePacketFormat1(current_quadset,
                                          config.index_version_,
                                          packet);
          write_buffer << packet;
        } else if (format_id == 1) {
          // Build the quadtree packet (protocol buffer format)
          qtpacket::KhQuadTreePacketProtoBuf qt_protobuf_packet;
          qtpacket::ComputeQuadtreePacketFormat2(current_quadset,
                                                 config.index_version_,
                                                 &qt_protobuf_packet);
          // serialize the protobuf to a string.
          write_buffer << qt_protobuf_packet;
        }

        // Compress packet
        size_t est_compressed_size
          = KhPktGetCompressBufferSize(write_buffer.size());
        if (est_compressed_size + FileBundle::kCRCsize
            > compressed_buffer.size())  {
          compressed_buffer.resize(est_compressed_size + FileBundle::kCRCsize);
        }
        size_t compressed_size = est_compressed_size;
        if (KhPktCompressWithBuffer(write_buffer.data(),
                                    write_buffer.size(),
                                    &compressed_buffer[0],
                                    &compressed_size)) {
          // Write data to output packet file
          writers[format_id]->WriteAppendCRC(gather.Current().qt_root(),
                                             &compressed_buffer[0],
                                             compressed_size +
                                             FileBundle::kCRCsize);
        }  else {
          notify(NFY_FATAL,
                 "Quadtree packet compression failed at %s for format %d",
                 gather.Current().qt_root().AsString().c_str(), format_id + 1);
        }
      }  // for format_id

      progress_meter.incrementDone(gather.Current().EntryCount());
    } while (gather.Advance());
    gather.Close();

    // Finish the packet file(s)
    for (unsigned int format_id = 0; format_id < kFormatCount; ++format_id) {
      if (writers[format_id] != NULL) {
        writers[format_id]->Close(static_cast<size_t>(sortbuf) * 1024 * 1024);
      }
    }

    // On successful completion, print out the output file sizes.
    std::vector<std::string> output_files;
    output_files.push_back(outdir_format1);
    output_files.push_back(outdir_format2);
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
