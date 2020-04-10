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


// gecrawler - crawl database tree, saving or comparing to serialized
// version on disk or to another server tree.  The tree is accessed
// through http protocol, or from a serialized disk file already
// saved.

#include <cstdint>
#include <sstream>
#include <khGetopt.h>
#include <notify.h>
#include <khEndian.h>
#include <khAbortedException.h>
#include <khSimpleException.h>
#include <packetcompress.h>
#include "httpcrawlersource.h"
#include "archivesource.h"
#include "archivewriter.h"
#include "crawlcompare.h"
#include "keyhole/jpeg_comment_date.h"

namespace {

geFilePool file_pool;

void usage(const std::string &progn, const char *msg = 0, ...) {
  if (msg) {
    va_list ap;
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    va_end(ap);
    fprintf(stderr, "\n");
  }

  fprintf(stderr,
          "\nusage: %s \\\n"
          "           --source1=http_or_archive \\\n"
          "           [--write_archive1=archive] \\\n"
          "           [--source2=http_or_archive] \\\n"
          "           [--write_archive2=archive] \\\n"
          "           [--use_quadtree_format2] \\\n"
          "           [--start=subtree_qt_path] \\\n"
          "           [--[no]compare_image] \\\n"
          "           [--[no]compare_terrain] \\\n"
          "           [--[no]compare_vector] \\\n"
          "           [--compare_all] \\\n"
          "           [--[no]ignore_mesh] \\\n"
          "           [--[no]check_image_provider \\\n"
          "           [--[no]check_terrain_provider \\\n"
          "           [--[no]check_vector_provider \\\n"
          "           [--[no]dump_packets \\\n"
          "           [--kml_log=diff.kml] \\\n"
          "           [--notify_prefix=prefix]\n"
          "           [--date=YYYY:MM:DD]\n"
          " Crawl quadtree starting at root (or optionally a subtree),\n"
          " optionally archiving and/or comparing with another quadtree.\n"
          " Sources may be servers (name starting with \"http:\") or\n"
          " previously generated archives.\n"
          " Default is to compare quadtree packet contents only. Use the\n"
          " various --compare options to control compare of data packets.\n"
          " When crawling a single source, the --compare flags control which\n"
          " data packet types will be fetched (and optionally archived).\n"
          " Options:\n"
          "   --source1, --source2: specify server or archive source.\n"
          "   --write_archive1, --write_archive2: specify archive to\n"
          "       write (directory name). Any old data is overwritten.\n"
          "   --use_quadtree_format2: use the quadtree protocol buffer\n"
          "                           format (default root).\n"
          "   --start: specify starting point in quadtree (default root).\n"
          "       Only the subtree rooted at the start position is crawled.\n"
          "   --compare_<option>: compare the specified type of data.\n"
          "       For a single source, read/archive the specified type.\n"
          "   --ignore_mesh: if terrain comparison is active, compare\n"
          "       only terrain packet header, not mesh points or connectivity.\n"
          "   --check_<type>_provider: check and compare the provider\n"
          "       information in the quadtree packets for the specified type.\n"
          "   --kml_log: in addition to the normal text log notify messages,\n"
          "       also generate a KML file illustrating differences found.\n"
          "   --notify_prefix: changes the prefix for text log (notify)\n"
          "       messages. Set to \"[time]\" to use date and time as\n"
          "       message prefix.\n"
          "   --date: for use with historical imagery enabled databases.\n"
          "           Crawls as if time slider were at the specified date.\n"
          "   --dump_packets: hex dump of the contents of all data packets.\n",
          progn.c_str());
  exit(1);
}

template <class T>
khTransferGuard<gecrawler::CrawlerSource<T> > MakeSource(
    const std::string &source_name,
    const std::string &archive_name,
    const QuadtreePath &start_path,
    const keyhole::JpegCommentDate& jpeg_date) {
  static const std::string kHttpPrefix("http:");

  bool http_source =
      0 == source_name.compare(0, kHttpPrefix.size(), kHttpPrefix);

  khDeleteGuard<gecrawler::ArchiveWriter> archive;
  if (!archive_name.empty()) {
    if (!http_source) {
      notify(NFY_FATAL, "write_archive (%s) requires http source",
             archive_name.c_str());
      throw khSimpleException("write_archive requires http source");
    }
    archive = TransferOwnership(
        new gecrawler::ArchiveWriter(file_pool, archive_name));
  }

  if (0 == source_name.compare(0, kHttpPrefix.size(), kHttpPrefix)) {
    return TransferOwnership(
        new gecrawler::HttpCrawlerSource<T>(source_name,
                                         start_path,
                                         jpeg_date,
                                         TransferOwnership(archive)));
  } else {
    return TransferOwnership(
        new gecrawler::ArchiveSource<T>(&file_pool,
                                     source_name,
                                     start_path,
                                     jpeg_date));
  }
}

void FetchRefData(const gecrawler::CrawlerSourceBase::PacketType packet_type,
                  const std::vector<qtpacket::KhQuadtreeDataReference> &refs,
                  khDeleteGuard<gecrawler::CrawlerSourceBase> *crawler_source) {
  // Fetch actual data packets referred to by the quadtree packet.  We
  // don't do anything with them here other than make sure that the
  // packets exist in the source (server or archive).  Note that the
  // act of fetching a packet may have a side-effect of archiving the
  // packet if the --write_archive1 option was specified.
  std::string buffer;
  LittleEndianReadBuffer decomp_buffer;
  for (size_t i = 0; i < refs.size(); ++i) {
    if (!(*crawler_source)->GetPacket(packet_type, refs[i], &buffer)) {
      notify(NFY_WARN, "Fetch failed for %s packet at \"%s\" chan %u ver %u",
             gecrawler::CrawlerSourceBase::
             packet_type_name(packet_type).c_str(),
             refs[i].qt_path().AsString().c_str(),
             refs[i].channel(),
             refs[i].version());
    } else if (packet_type != gecrawler::CrawlerSourceBase::kImage &&
               !KhPktDecompress(buffer.data(), buffer.size(),
                &decomp_buffer)) {
      // Check decompression of all vector, terrain and quadtree packets.
      // Imagery packets are not compressed using KhPktDecompress.
      notify(NFY_WARN, "Bad %s packet (size: %lu) at \"%s\" chan %u ver %u",
             gecrawler::CrawlerSourceBase::
             packet_type_name(packet_type).c_str(),
             static_cast<unsigned long>(buffer.size()),
             refs[i].qt_path().AsString().c_str(),
             refs[i].channel(),
             refs[i].version());
      const std::uint32_t* local_buf = reinterpret_cast<const std::uint32_t*>(buffer.data());
      notify(NFY_WARN, "Packet header expected %x, got %x",
             kPktMagic, local_buf[0]);
    }
  }
}

void CrawlSingleSource(
    khTransferGuard<gecrawler::CrawlerSourceBase> source,
    gecrawler::CrawlCompareOptions::DataTypeSet archive_types) {
  // Just crawl the source.  Useful to archive a source without
  // comparing, or just for testing.
  std::vector<qtpacket::KhQuadtreeDataReference> qtp_refs;
  std::vector<qtpacket::KhQuadtreeDataReference> qtp2_refs;
  std::vector<qtpacket::KhQuadtreeDataReference> img_refs;
  std::vector<qtpacket::KhQuadtreeDataReference> ter_refs;
  std::vector<qtpacket::KhQuadtreeDataReference> vec_refs;
  qtpacket::KhQuadtreeDataReferenceGroup refs(&qtp_refs,
                                              &qtp2_refs,
                                              &img_refs,
                                              &ter_refs,
                                              &vec_refs);

  khDeleteGuard<gecrawler::CrawlerSourceBase> crawler_source(source);

  while (!crawler_source->IsFinished()) {
    refs.Reset();
    qtpacket::KhQuadtreeDataReference qtp_read;
    crawler_source->GetQtPacketRefs(&qtp_read, &refs,
                                    false /* don't care */,
                                    NULL /* don't care */);

    if (archive_types.test(gecrawler::CrawlerSourceBase::kImage)) {
      FetchRefData(gecrawler::CrawlerSourceBase::kImage,
                   img_refs, &crawler_source);
    }

    if (archive_types.test(gecrawler::CrawlerSourceBase::kTerrain)) {
      gecrawler::CrawlerSourceBase::RemoveOddLevelRefs(&ter_refs);
      FetchRefData(gecrawler::CrawlerSourceBase::kTerrain,
                   ter_refs, &crawler_source);
    }

    if (archive_types.test(gecrawler::CrawlerSourceBase::kVector)) {
      FetchRefData(gecrawler::CrawlerSourceBase::kVector,
                   vec_refs, &crawler_source);
    }
  }
  crawler_source->Close();
}

} // namespace


int main(int argc, char **argv) {
  try {
    std::string progname = argv[0];

    // Process commandline options
    int argn;
    bool help = false;
    std::string source1;
    std::string write_archive1;
    std::string source2;
    std::string write_archive2;
    std::string start;
    std::string kml_log_name;
    std::string date_string;
    bool compare_image = false;
    bool compare_terrain = false;
    bool compare_vectors = false;
    bool compare_all = false;
    bool check_image_provider = false;  // TODO: enable by default
    bool check_terrain_provider = true;
    bool check_vector_provider = true;
    bool ignore_mesh = true;
    bool dump_packets = false;
    bool use_format2_quadtree_packets = false;  // Use format2 (protocol buffer)
                                                 // quadtree packets.

    khGetopt options;
    options.flagOpt("help", help);
    options.flagOpt("?", help);
    options.opt("source1", source1);
    options.opt("write_archive1", write_archive1);
    options.opt("source2", source2);
    options.opt("write_archive2", write_archive2);
    options.opt("start", start);
    options.opt("kml_log", kml_log_name);
    options.opt("notify_prefix", NotifyPrefix);
    options.opt("compare_image", compare_image);
    options.opt("compare_terrain", compare_terrain);
    options.opt("compare_vector", compare_vectors);
    options.opt("compare_all", compare_all);
    options.opt("ignore_mesh", ignore_mesh);
    options.opt("check_image_provider", check_image_provider);
    options.opt("check_terrain_provider", check_terrain_provider);
    options.opt("check_vector_provider", check_vector_provider);
    options.opt("dump_packets", dump_packets);
    options.opt("use_quadtree_format2", use_format2_quadtree_packets);
    options.opt("date", date_string);
    options.setRequired("source1");
    options.setExclusive("compare_all", "compare_image");
    options.setExclusive("compare_all", "compare_terrain");
    options.setExclusive("compare_all", "compare_vector");

    if (!options.processAll(argc, argv, argn)
        || help
        || argn != argc) {
      usage(progname);
      return 1;
    }

    keyhole::JpegCommentDate jpeg_date;  // By default the crawler ignores the
                                           // date unless specified by the user.
    if (!date_string.empty()) {
      if (!use_format2_quadtree_packets) {
        notify(NFY_FATAL,
               "With --date option, --use_format2_quadtree_packets is "
               "required.");
      }
      if (!compare_image) {
        notify(NFY_FATAL,
               "With --date option, --compare_image is required.");
      }
      // Convert to a JpegCommentDate format (needed for crawler).
      jpeg_date = keyhole::JpegCommentDate(date_string);
      if (jpeg_date.IsCompletelyUnknown()) {
        notify(NFY_FATAL,
               "The input date \"%s\" was not a valid date.\n"
               "The proper format is YYYY:MM:DD, e.g., 2009:03:27.\n",
               date_string.c_str());
      }
    }

    QuadtreePath start_qt_path(start);

    std::string description;

    // Build set of types to compare
    if (compare_all) {
      compare_image = true;
      compare_terrain = true;
      compare_vectors = true;
    }
    gecrawler::CrawlCompareOptions::DataTypeSet compare_types;
    std::string compare_description;
    if (compare_image) {
      compare_types.set(gecrawler::CrawlerSourceBase::kImage);
      compare_description += " Image";
    }
    if (compare_terrain) {
      compare_types.set(gecrawler::CrawlerSourceBase::kTerrain);
      compare_description += " Terrain";
      if (ignore_mesh) {
        compare_description += "(ignore_mesh)";
      }
    }
    if (compare_vectors) {
      compare_types.set(gecrawler::CrawlerSourceBase::kVector);
      compare_description += " Vector";
    }
    if (compare_description.empty())
      compare_description = " [none]";

    // Build set of provider types to check
    gecrawler::CrawlCompareOptions::DataTypeSet check_providers;
    std::string include_providers;
    std::string exclude_providers;
    if (check_image_provider) {
      check_providers.set(gecrawler::CrawlerSourceBase::kImage);
      include_providers += " Image";
    } else {
      exclude_providers += " Image";
    }
    if (check_terrain_provider) {
      check_providers.set(gecrawler::CrawlerSourceBase::kTerrain);
      include_providers += " Terrain";
    } else {
      exclude_providers += " Terrain";
    }
    if (check_vector_provider) {
      check_providers.set(gecrawler::CrawlerSourceBase::kVector);
      include_providers += " Vector";
    } else {
      exclude_providers += " Vector";
    }

    // Set up sources and archives

    description += "Crawler start path: \""
        + start_qt_path.AsString() + "\"\n"
        + "Compare quadtree packets and:" + compare_description + "\n"
        + "Crawler source 1: " + source1 + "\n";
    if (!include_providers.empty()) {
      description += "Checking provider codes for type(s):"
          + include_providers + "\n";
    }
    if (!exclude_providers.empty()) {
      description += "Ignoring provider codes for type(s):"
          + exclude_providers + "\n";
    }
    khDeleteGuard<gecrawler::CrawlerSourceBase> crawler_source1;
    khDeleteGuard<gecrawler::CrawlerSourceBase> crawler_source2;

    if (!write_archive1.empty())
      description += "Crawler write archive 1: " + write_archive1 + "\n";
    if (use_format2_quadtree_packets) {
      crawler_source1 =
        MakeSource<qtpacket::KhQuadTreePacketProtoBuf>(source1, write_archive1,
                                                       start_qt_path,
                                                       jpeg_date);
      crawler_source1->SetUseQuadtreeFormat2();
    } else {  // format1 quadtree packets
      crawler_source1 =
        MakeSource<qtpacket::KhQuadTreePacket16>(source1, write_archive1,
                                                 start_qt_path,
                                                 jpeg_date);
    }

    if (!source2.empty()) {
      description += "Crawler source 2: " + source2 + "\n";
      if (!write_archive2.empty())
        description += "Crawler write archive 2: " + write_archive2 + "\n";
      if (use_format2_quadtree_packets) {
        crawler_source2 =
          MakeSource<qtpacket::KhQuadTreePacketProtoBuf>(source2,
              write_archive2, start_qt_path, jpeg_date);
        crawler_source2->SetUseQuadtreeFormat2();
      } else {  // format1 quadtree packets
        crawler_source2 =
          MakeSource<qtpacket::KhQuadTreePacket16>(source2, write_archive2,
                                                   start_qt_path,
                                                   jpeg_date);
      }
      notify(NFY_NOTICE, "%s", description.c_str());

      khDeleteGuard<gecrawler::KmlLog> kml_log;
      if (!kml_log_name.empty()) {
        kml_log = TransferOwnership(
            new gecrawler::KmlLog(file_pool, kml_log_name));
        kml_log->AddDescription("<pre>" + description + "</pre>");
      }

      // Compare the two sources
      const gecrawler::CrawlCompareOptions
          options(compare_types, check_providers, ignore_mesh, dump_packets);
      gecrawler::CrawlCompare
        source_compare(TransferOwnership(crawler_source1),
                                         TransferOwnership(crawler_source2),
                                         options,
                                         TransferOwnership(kml_log));
      if (source_compare.Compare()) {
        notify(NFY_NOTICE, "Crawl finished, compare OK");
      } else {
        notify(NFY_WARN, "Crawl finished, compare errors (see log)");
      }
      notify(NFY_NOTICE, "Statistics:\n%s",
             source_compare.FormatCounters().c_str());
    } else {
      if (!write_archive2.empty()) {
        usage(progname, "write_archive2 requires source2");
      }

      notify(NFY_NOTICE, "%s", description.c_str());

      // Just crawl the single source
      CrawlSingleSource(TransferOwnership(crawler_source1), compare_types);
      notify(NFY_NOTICE, "Crawl finished (single source)");
    }

    return 0;
  } catch (const khAbortedException &e) {
    notify(NFY_FATAL, "Unable to proceed: See previous warnings");
  } catch (const std::exception &e) {
    notify(NFY_FATAL, "%s", e.what());
  } catch (...) {
    notify(NFY_FATAL, "Unknown error");
  }

  return 2;
}
