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


#ifndef FUSION_GECRAWLER_CRAWLERSOURCE_H__
#define FUSION_GECRAWLER_CRAWLERSOURCE_H__

#include <vector>
#include <string>
#include <queue>
#include <common/base/macros.h>
//#include "common/khTypes.h"
#include <cstdint>
#include "common/quadtreepath.h"
#include "common/qtpacket/quadtreepacket.h"
#include "common/packetcompress.h"

namespace gecrawler {

// CrawlerSource - define source class for data to crawl.  This is an
// abstract class for use by the server and file crawler sources.
// Base class for static definitions.
class CrawlerSourceBase {
 public:
   // If the jpeg_date is uninitialized, we skip all historical imagery,
   // otherwise, it will get the latest imagery <= the given date, unless
   // the jpeg_date is marked as MatchAllDates() in which case it will get
   // all imagery refs.
   CrawlerSourceBase(const std::string &name, const QuadtreePath &start_path,
                    const keyhole::JpegCommentDate& jpeg_date);
  virtual ~CrawlerSourceBase() {}

  // Constant enums and static name variables.
  enum PacketType { kQuadtree, kQuadtreeFormat2,
    kImage, kTerrain, kVector, kTypeCount };
  static const std::string kPacketTypeNames[kTypeCount];
  static const std::string& packet_type_name(PacketType packet_type);

  virtual void SkipQtPacket() { to_do_.pop(); }

  virtual bool GetPacket(const CrawlerSourceBase::PacketType packet_type,
                         const qtpacket::KhQuadtreeDataReference &qt_ref,
                         std::string *buffer) = 0;
  virtual bool GetQtPacketRefs(
      qtpacket::KhQuadtreeDataReference *qtp_read,
      qtpacket::KhQuadtreeDataReferenceGroup *refs,
      bool output_detailed_info,
      std::string* qt_packet_as_string) = 0;
  virtual void Close() = 0;
  inline bool IsFinished() { return to_do_.empty(); }
  inline const qtpacket::KhQuadtreeDataReference &Next() const {
    return to_do_.top();
  }
  inline const std::string &name() { return name_; }
  inline std::uint64_t packet_count() const { return packet_count_; }
  static void RemoveOddLevelRefs(
      std::vector<qtpacket::KhQuadtreeDataReference> *ter_refs);

  void SetUseQuadtreeFormat2() { packet_type_ = kQuadtreeFormat2; }
 protected:
  std::string name_;

  // Queue of quadtree packets yet to be processed. Done when empty.
  std::priority_queue<
    qtpacket::KhQuadtreeDataReference,
    std::vector<qtpacket::KhQuadtreeDataReference>,
    std::greater<qtpacket::KhQuadtreeDataReference> > to_do_;

  std::string compressed_packet_buffer_;
  LittleEndianReadBuffer packet_buffer_;
  std::uint64_t packet_count_;
  PacketType packet_type_;
  keyhole::JpegCommentDate jpeg_date_;
  DISALLOW_COPY_AND_ASSIGN(CrawlerSourceBase);
};

// CrawlerSource - define a templated source class for data to crawl.
// This is an abstract class for use by the server and file crawler sources.
template <class T> class CrawlerSource : public CrawlerSourceBase {
 public:
   // If the jpeg_date is uninitialized, we skip all historical imagery,
   // otherwise, it will get the latest imagery <= the given date, unless
   // the jpeg_date is marked as MatchAllDates() in which case it will get
   // all imagery refs.
   CrawlerSource(const std::string &name, const QuadtreePath &start_path,
                const keyhole::JpegCommentDate& jpeg_date);
  virtual ~CrawlerSource() {}

  // Get data references from next quadtree packet. Return false if done.
  // returns with the qt_packet_as_string contents set unless the input
  // pointer is NULL.
  virtual bool GetQtPacketRefs(
      qtpacket::KhQuadtreeDataReference *qtp_read,
      qtpacket::KhQuadtreeDataReferenceGroup *refs,
      bool output_detailed_info,
      std::string* qt_packet_as_string);

 private:
  DISALLOW_COPY_AND_ASSIGN(CrawlerSource);
};

//------------------------------------------------------------------------------
// CrawlerSource method definitions (need to be in .h because they're templated.
template <class T>
CrawlerSource<T>::CrawlerSource(const std::string &name,
                             const QuadtreePath &start_path,
                             const keyhole::JpegCommentDate& jpeg_date)
    : CrawlerSourceBase(name, start_path, jpeg_date) {
}

// Get data references from next quadtree packet. Return false if done.
template <class T>
bool CrawlerSource<T>::GetQtPacketRefs(
      qtpacket::KhQuadtreeDataReference *qtp_read,
      qtpacket::KhQuadtreeDataReferenceGroup *refs,
      bool output_detailed_info,
      std::string* qt_packet_as_string) {
  assert(qtp_read);
  assert(refs);

  while (!IsFinished()) {
    qtpacket::KhQuadtreeDataReference next_qtp = to_do_.top();

    if (GetPacket(packet_type_, next_qtp, &compressed_packet_buffer_)) {
      to_do_.pop();
      if (KhPktDecompress(compressed_packet_buffer_.data(),
                           compressed_packet_buffer_.size(),
                           &packet_buffer_)) {
        T qt_packet;
        packet_buffer_ >> qt_packet;

        qt_packet.GetDataReferences(refs, next_qtp.qt_path(),
                                    this->jpeg_date_);

        std::vector<qtpacket::KhQuadtreeDataReference>* qtp_refs =
          (packet_type_ == kQuadtree) ?
              refs->qtp_refs : refs->qtp2_refs;
        for (size_t i = 0; i < qtp_refs->size(); ++i) {
          to_do_.push(qtp_refs->at(i));
        }
        *qtp_read = next_qtp;
        ++packet_count_;

        if (qt_packet_as_string) {
          QuadtreePath qt_path = qtp_read->qt_path();
          *qt_packet_as_string = qt_packet.ToString(qt_path.Level() == 0,
                                                    output_detailed_info);
        }
        return true;
      } else {
        to_do_.pop();
        notify(NFY_WARN,
               "CrawlerSource: quadtree packet decompress failed");
      }
    } else {
      to_do_.pop();
      notify(NFY_WARN,
             "CrawlerSource: quadtree packet request failed at \"%s\"",
             next_qtp.qt_path().AsString().c_str());
    }
  }
  return false;
}


} // namespace gecrawler

#endif  // FUSION_GECRAWLER_CRAWLERSOURCE_H__
