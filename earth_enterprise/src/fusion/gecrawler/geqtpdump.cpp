// Copyright 2017 Google Inc.
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


// geqtpdump - dump quadtree packet from server or archive

#include <sstream>
#include <vector>
#include <khGetopt.h>
#include <notify.h>
#include <khAbortedException.h>
#include <khSimpleException.h>
#include <common/qtpacket/quadtreepacket.h>
#include <common/khConstants.h>
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
          "           --source=http_or_archive \\\n"
          "           --path=quadtree_path \\\n"
          "           [--use_quadtree_format2] \\\n"
          "           [--show_protobuf] \\\n"
          " Dump the contents of a specific quadtree packet.\n"
          " Options:\n"
          "   --source: specify server or archive source.\n"
          "   note the archive source is built using gecrawler.\n"
          "   --path: specify the quadtree path of the packet\n"
          "           note the root node is --path=\"\" as a leading 0\n"
          "           is prepended to all quadtree paths (i.e., root = 0\n"
          "           is implicit. Note also, valid quadtree packets\n"
          "           are only found every 4 levels, so: \n"
          "           \"\", \"123\" and \"1230123\" would be valid paths.\n"
          "           would be valid.\n"
          "   --use_quadtree_format2: use the quadtree protocol buffer\n"
          "                           format (default root).\n"
          "   --show_protobuf: output the quadtree protobuffer.\n"
          " Example:\n"
          "  %s --source=\"http://localhost/default_ge/\" "
          "--path=123 --use_quadtree_format2\n",
          progn.c_str(), progn.c_str());
}

template <class T>
khTransferGuard<gecrawler::CrawlerSource<T> > MakeSource(
    const std::string &source_name,
    const std::string &archive_name,
    const QuadtreePath &start_path) {
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
                                         // No date specified.
                                         keyhole::kUnknownJpegCommentDate,
                                         TransferOwnership(archive)));
  } else {
    return TransferOwnership(
        new gecrawler::ArchiveSource<T>(&file_pool,
                                     source_name,
                                     start_path,
                                     // No date specified.
                                     keyhole::kUnknownJpegCommentDate));
  }
}

} // namespace


int main(int argc, char **argv) {
  std::string progname = argv[0];
  std::string source;
  std::string path;
  bool use_format2_quadtree_packets = false;  // Use format2 (protocol buffer)
                                               // quadtree packets.
  bool help = false;
  bool show_protobuf = false;

  khGetopt options;
  options.flagOpt("help", help);
  options.flagOpt("?", help);
  options.opt("source", source);
  options.opt("path", path);
  options.opt("show_protobuf", show_protobuf);
  options.opt("use_quadtree_format2", use_format2_quadtree_packets);
  options.setRequired("source");
  options.setRequired("path");
  int argn;
  if (!options.processAll(argc, argv, argn)
      || help
      || argn != argc) {
    usage(progname);
    return 1;
  }



  try {
    QuadtreePath qt_path(path);

    // Set up source
    khDeleteGuard<gecrawler::CrawlerSourceBase> crawler_source;

    gecrawler::CrawlerSourceBase::PacketType packet_type =
      gecrawler::CrawlerSourceBase::kQuadtree;
    if (use_format2_quadtree_packets) {
      crawler_source =
        MakeSource<qtpacket::KhQuadTreePacketProtoBuf>(source, "", qt_path);
      packet_type = gecrawler::CrawlerSourceBase::kQuadtreeFormat2;
      crawler_source->SetUseQuadtreeFormat2();
    } else {  // Use Format1 quadtree packets
      crawler_source =
        MakeSource<qtpacket::KhQuadTreePacket16>(source, "", qt_path);
    }
    // Fetch packet

    std::string readable_buffer;
    LittleEndianReadBuffer qtp_buffer;
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
    qtpacket::KhQuadtreeDataReference qtp_read;
    qtpacket::KhQuadtreeDataReference qt_ref(qt_path, 0, 0, 0);
    if (!crawler_source->GetQtPacketRefs(&qtp_read, &refs,
                                         show_protobuf,
                                         &readable_buffer)) {
      notify(NFY_FATAL, "Failed to fetch %s packet at %s.\n"
             "Note: be sure your request is on a quadtree packet address\n"
             "      remembering that quadtree packets span 4 levels and\n"
             "      the root level is implicitly 0. For example, valid paths\n"
             "      could be \"\", \"123\" and \"1230123\", i.e., empty, or\n"
             "      any combination of 3 digits plus 4 digits for every\n"
             "      subsequent quadtree packet.\n",
             gecrawler::CrawlerSourceBase::packet_type_name(
                 packet_type).c_str(),
             qt_path.AsString().c_str());
      exit(2);
    }

    std::cout << "Quadtree Packet " << qt_path.AsString() << std::endl
              << readable_buffer << std::endl;

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
