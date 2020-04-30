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

#include <cstdint>
#include <fusion/gecrawler/crawlersource.h>

namespace gecrawler {

//------------------------------------------------------------------------------
// CrawlerSourceBase
const std::string CrawlerSourceBase::kPacketTypeNames[kTypeCount] = {
  "quadtree", "quadtree(protobuf)", "image", "terrain", "vector"
};

const std::string& CrawlerSourceBase::packet_type_name(PacketType packet_type) {
  static const std::string unknown = "unknown";
  if (packet_type < kTypeCount) {
    return kPacketTypeNames[packet_type];
  } else {
    return unknown;
  }
}

CrawlerSourceBase::CrawlerSourceBase(const std::string &name,
                             const QuadtreePath &start_path,
                             const keyhole::JpegCommentDate& jpeg_date)
    : name_(name),
      packet_count_(0),
      packet_type_(CrawlerSourceBase::kQuadtree),
      jpeg_date_(jpeg_date) {
  to_do_.push(qtpacket::KhQuadtreeDataReference(start_path, 0, 0, 0));
}

// RemoveOddLevelRefs - older servers mistakenly generated references
// in the quadtree packet to (non-existent) odd level terrain packets
// (they are actually bundled with the even level parent packet).
// This routine strips out all the odd-level packets in the vector.
void CrawlerSourceBase::RemoveOddLevelRefs(
    std::vector<qtpacket::KhQuadtreeDataReference> *ter_refs) {
  size_t src = 0;
  size_t dst = 0;

  while (src < ter_refs->size()) {
    if ((ter_refs->at(src).qt_path().Level() & 1) == 0) { // if even level
      if (src != dst) {
        ter_refs->at(dst) = ter_refs->at(src);
      }
      ++dst;
    } else {
      notify(NFY_VERBOSE, "Discarding terrain reference \"%s\"",
             ter_refs->at(src).qt_path().AsString().c_str());
    }
    ++src;
  }
  ter_refs->resize(dst);
}

} // namespace gecrawler
