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


#include <third_party/rsa_md5/crc32.h>
#include <khFileUtils.h>
#include <qtpacket/quadtreepacket.h>
#include "archivewriter.h"

namespace gecrawler {

ArchiveWriter::ArchiveWriter(geFilePool &file_pool,
                             const std::string &archive_name)
    : qtp_writer_(file_pool, khComposePath(archive_name, "qtp")),
      qtp2_writer_(file_pool, khComposePath(archive_name, "qtp2")),
      img_writer_(file_pool, khComposePath(archive_name, "img")),
      ter_writer_(file_pool, khComposePath(archive_name, "ter")),
      vec_writer_(file_pool, khComposePath(archive_name, "vec")) {
}

ArchiveWriter::~ArchiveWriter() {
}

void ArchiveWriter::WriteQuadtreePacket(
    const qtpacket::KhQuadtreeDataReference &qt_ref,
    const std::string &buffer) {
  WritePacket(qtp_writer_, qt_ref, buffer);
}

void ArchiveWriter::WriteQuadtreePacket2(
    const qtpacket::KhQuadtreeDataReference &qt_ref,
    const std::string &buffer) {
  WritePacket(qtp2_writer_, qt_ref, buffer);
}

void ArchiveWriter::WriteImagePacket(
    const qtpacket::KhQuadtreeDataReference &qt_ref,
    const std::string &buffer) {
  WritePacket(img_writer_, qt_ref, buffer);
}

void ArchiveWriter::WriteTerrainPacket(
    const qtpacket::KhQuadtreeDataReference &qt_ref,
    const std::string &buffer) {
  WritePacket(ter_writer_, qt_ref, buffer);
}

void ArchiveWriter::WriteVectorPacket(
    const qtpacket::KhQuadtreeDataReference &qt_ref,
    const std::string &buffer) {
  WritePacket(vec_writer_, qt_ref, buffer);
}

void ArchiveWriter::WritePacket(
    FileBundleWriter &writer,
    const qtpacket::KhQuadtreeDataReference &qt_ref,
    const std::string &buffer) {
  // Header info with its own CRC
  write_buffer_.reset();
  std::uint32_t buffer_size = buffer.size();   // force size to 32 bits on all platforms
  write_buffer_ << buffer_size;
  write_buffer_ << qt_ref;
  write_buffer_ << Crc32(write_buffer_.data(), write_buffer_.size());

  // Packet data with CRC
  write_buffer_.rawwrite(buffer.data(), buffer.size());
  write_buffer_ << Crc32(buffer.data(), buffer.size());

  writer.WriteAppend(write_buffer_.data(), write_buffer_.size());
}

void ArchiveWriter::Close() {
  qtp_writer_.Close();
  qtp2_writer_.Close();
  img_writer_.Close();
  ter_writer_.Close();
  vec_writer_.Close();
}

} // namespace gecrawler
