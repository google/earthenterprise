/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef FUSION_GECRAWLER_ARCHIVEWRITER_H__
#define FUSION_GECRAWLER_ARCHIVEWRITER_H__

#include <cstdint>
#include <common/base/macros.h>
#include <khEndian.h>
#include <filebundle/filebundlewriter.h>

// The crawler archive routines are used to write and read packets in
// a serialized format.  An archive consists of up to four file
// bundles, one each for quadtree packets (required), image packets,
// terrain packets, and vector packets.  The archive files are written
// in crawl order (see HttpCrawlerSource for details).
//
// Each packet in the archive is written exactly as received from the
// server. The packet is prefixed by a 32-bit length field, and 64-bit
// QuadtreePath.  The packet is suffixed by a CRC32.  The length field
// gives the length of the packet only, not including the prefix and
// suffix fields.

namespace qtpacket {
class KhQuadtreeDataReference;
}

namespace gecrawler {

// ArchiveWriter - create serial archive from traversed server database

class ArchiveWriter {
 public:
  ArchiveWriter(geFilePool &file_pool,
                const std::string &archive_name);
  ~ArchiveWriter();
  void WriteQuadtreePacket(const qtpacket::KhQuadtreeDataReference &qt_path,
                           const std::string &buffer);
  void WriteQuadtreePacket2(const qtpacket::KhQuadtreeDataReference &qt_path,
                            const std::string &buffer);
  void WriteImagePacket(const qtpacket::KhQuadtreeDataReference &qt_path,
                        const std::string &buffer);
  void WriteVectorPacket(const qtpacket::KhQuadtreeDataReference &qt_path,
                         const std::string &buffer);
  void WriteTerrainPacket(const qtpacket::KhQuadtreeDataReference &qt_path,
                          const std::string &buffer);
  void Close();
 private:
  FileBundleWriter qtp_writer_;
  FileBundleWriter qtp2_writer_;
  FileBundleWriter img_writer_;
  FileBundleWriter ter_writer_;
  FileBundleWriter vec_writer_;

  LittleEndianWriteBuffer write_buffer_;

  void WritePacket(FileBundleWriter &writer,
                   const qtpacket::KhQuadtreeDataReference &qt_path,
                   const std::string &buffer);
  DISALLOW_COPY_AND_ASSIGN(ArchiveWriter);
};

} // namespace gecrawler

#endif  // FUSION_GECRAWLER_ARCHIVEWRITER_H__
