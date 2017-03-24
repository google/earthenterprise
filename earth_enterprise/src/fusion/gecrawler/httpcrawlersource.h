/*
 * Copyright 2017 Google Inc.
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


#ifndef FUSION_GECRAWLER_HTTPCRAWLERSOURCE_H__
#define FUSION_GECRAWLER_HTTPCRAWLERSOURCE_H__

#include <khGuard.h>
#include <common/khConstants.h>
#include <gst/gstEarthStream.h>
#include "archivewriter.h"
#include "crawlersource.h"

namespace gecrawler {

template <class T> class HttpCrawlerSource : public CrawlerSource<T> {
 public:
   // If the jpeg_date is uninitialized, we skip all historical imagery,
   // otherwise, it will get the latest imagery <= the given date, unless
   // the jpeg_date is marked as MatchAllDates() in which case it will get
   // all imagery refs.
   HttpCrawlerSource(const std::string &url,
                    const QuadtreePath &start_path,
                    const keyhole::JpegCommentDate& jpeg_date);
  HttpCrawlerSource(const std::string &url,
                    const QuadtreePath &start_path,
                    const keyhole::JpegCommentDate& jpeg_date,
                    khTransferGuard<ArchiveWriter> archive);
  virtual ~HttpCrawlerSource();

  virtual bool GetPacket(const CrawlerSourceBase::PacketType packet_type,
                         const qtpacket::KhQuadtreeDataReference &qt_ref,
                         std::string *buffer);
  virtual void Close();
 protected:
  static const int kMaxRetries = 2;
  gstEarthStream server_stream_;
  khDeleteGuard<ArchiveWriter> archive_;

  bool GetRawPacket(const CrawlerSourceBase::PacketType packet_type,
                    const qtpacket::KhQuadtreeDataReference &qt_ref,
                    std::string *buffer);
  DISALLOW_COPY_AND_ASSIGN(HttpCrawlerSource);
};

template <class T>
HttpCrawlerSource<T>::HttpCrawlerSource(const std::string &url,
                                     const QuadtreePath &start_path,
                                     const keyhole::JpegCommentDate& jpeg_date)
    : CrawlerSource<T>(url, start_path, jpeg_date),
      server_stream_(url) {
}

template <class T>
HttpCrawlerSource<T>::HttpCrawlerSource(const std::string &url,
                                     const QuadtreePath &start_path,
                                     const keyhole::JpegCommentDate& jpeg_date,
                                     khTransferGuard<ArchiveWriter> archive)
    : CrawlerSource<T>(url, start_path, jpeg_date),
      server_stream_(url),
      archive_(archive) {
}

template <class T>
HttpCrawlerSource<T>::~HttpCrawlerSource() {
}

template <class T>
bool HttpCrawlerSource<T>::GetRawPacket(
    const CrawlerSourceBase::PacketType packet_type,
    const qtpacket::KhQuadtreeDataReference &qt_ref,
    std::string *buffer) {
  if (getNotifyLevel() >= NFY_DEBUG) {
    notify(NFY_DEBUG, "GetRawPacket %s \"%s\" ch %u ver %u",
           CrawlerSourceBase::packet_type_name(packet_type).c_str(),
           qt_ref.qt_path().AsString().c_str(),
           qt_ref.channel(),
           qt_ref.version());
  }
  switch (packet_type) {
  case CrawlerSourceBase::kQuadtree:
    return server_stream_.GetQuadTreePacket(qt_ref.qt_path(), buffer);
  case CrawlerSourceBase::kQuadtreeFormat2:
    return server_stream_.GetQuadTreeFormat2Packet(qt_ref.qt_path(), buffer);
  case CrawlerSourceBase::kImage:
    // This assumes all historical imagery requests have a valid date string.
    // otherwise we'll get empty packets back.
    return server_stream_.GetRawImagePacket(qt_ref.qt_path(),
                                            qt_ref.version(),
                                            this->jpeg_date_,
                                            buffer);
  case CrawlerSourceBase::kTerrain:
    return server_stream_.GetRawTerrainPacket(qt_ref.qt_path(),
                                              qt_ref.version(),
                                              buffer);
  case CrawlerSourceBase::kVector:
    return server_stream_.GetRawVectorPacket(qt_ref.qt_path(),
                                              qt_ref.channel(),
                                              qt_ref.version(),
                                              buffer);
  default:
    throw khSimpleException("GetRawPacket called with invalid packet_type: ")
        << packet_type;
  }
}

template <class T>
bool HttpCrawlerSource<T>::GetPacket(
    const CrawlerSourceBase::PacketType packet_type,
    const qtpacket::KhQuadtreeDataReference &qt_ref,
    std::string *buffer) {
  int retries_left = kMaxRetries;

  // The collector should not return historical imagery if we didn't request it.
  assert(!(qt_ref.IsHistoricalImagery() &&
      this->jpeg_date_.IsCompletelyUnknown()));

  while (!GetRawPacket(packet_type, qt_ref, buffer)) {
    if (retries_left-- > 0) {
      notify(NFY_VERBOSE,
             "HttpCrawlerSource: retrying %s packet \"%s\"",
             CrawlerSourceBase::packet_type_name(packet_type).c_str(),
             qt_ref.qt_path().AsString().c_str());
      sleep(1);                         // in case server is too busy
    } else {
      return false;
    }
  }

  if (archive_) {
    switch (packet_type) {
      case CrawlerSourceBase::kQuadtree:
        archive_->WriteQuadtreePacket(qt_ref, *buffer);
        break;
      case CrawlerSourceBase::kQuadtreeFormat2:
        archive_->WriteQuadtreePacket2(qt_ref, *buffer);
        break;
      case CrawlerSourceBase::kImage:
        archive_->WriteImagePacket(qt_ref, *buffer);
        break;
      case CrawlerSourceBase::kTerrain:
        archive_->WriteTerrainPacket(qt_ref, *buffer);
        break;
      case CrawlerSourceBase::kVector:
        archive_->WriteVectorPacket(qt_ref, *buffer);
        break;
      default:
        throw khSimpleException("GetPacket called with invalid packet_type: ")
            << packet_type;
    }
  }
  return true;
}

template <class T>
void HttpCrawlerSource<T>::Close() {
  if (archive_) {
    archive_->Close();
    archive_.clear();
  }
}

} // namespace gecrawler

#endif  // FUSION_GECRAWLER_HTTPCRAWLERSOURCE_H__
