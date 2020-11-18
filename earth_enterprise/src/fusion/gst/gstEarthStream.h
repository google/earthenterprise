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

#ifndef KHSRC_FUSION_GST_GSTEARTHSTREAM_H__
#define KHSRC_FUSION_GST_GSTEARTHSTREAM_H__

#include <Qt3Support/q3cstring.h>
#include <curl/curl.h>
#include <khThread.h>
#include <khEndian.h>
#include "gst/gstQuadAddress.h"
#include "keyhole/jpeg_comment_date.h"

typedef std::uint16_t ImageVersion;
class QuadtreePath;

class gstEarthStream {
 public:
  gstEarthStream(const std::string& server);
  ~gstEarthStream();

  enum { kImageExistanceCacheSize = 100 };

  static void Init();

  bool GetImage(const gstQuadAddress& addr, char* buffer);
  ImageVersion GetImageVersion(const gstQuadAddress& addr);

  bool GetQuadTreePacket(const QuadtreePath &qt_path,
                         std::string *raw_packet);
  bool GetQuadTreeFormat2Packet(const QuadtreePath &qt_path,
                                std::string *raw_packet);
  // Get the raw imagery packet for the given quadtree path and version.
  // If the jpeg_date is specified/initialized, we request the imagery packet
  // via a timemachine format query by prepending the db=tm and appending
  // the hex value of the date.
  bool GetRawImagePacket(const QuadtreePath &qt_path,
                         const std::uint16_t version,
                         const keyhole::JpegCommentDate& jpeg_date,
                         std::string *raw_packet);
  bool GetRawTerrainPacket(const QuadtreePath &qt_path,
                           const std::uint16_t version,
                           std::string *raw_packet);
  bool GetRawVectorPacket(const QuadtreePath &qt_path,
                          const std::uint16_t channel,
                          const std::uint16_t version,
                          std::string *raw_packet);
  bool GetRawPacket(const std::string &url,
                    std::string *raw_packet);
  bool GetRawPacket(const std::string &url,
                    std::string *raw_packet,
                    bool decrypt);

  static ImageVersion invalid_version;

  static khMutex mutex;

 private:
  std::string server_;
  CURL* image_request_handle_;
  CURL* quadtree_request_handle_;
};

#endif  // !KHSRC_FUSION_GST_GSTEARTHSTREAM_H__
