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

// Simplified version of gstSimpleEarthStream.h.
#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSIMPLESTREAM_H__
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSIMPLESTREAM_H__

#include <curl/curl.h>
#include <gst/gstSimpleEarthStream.h>

#include <string>

typedef std::uint16_t ImageVersion;

class Request;
class RawPacketRequest;


class gstSimpleStream {
 public:
  explicit gstSimpleStream(const std::string& server);
  ~gstSimpleStream();

  static void Init();
  bool GetRawPacket(const std::string &url,
                    std::string *raw_packet);

 private:
  const std::string server_;
  gst::RawPacketRequest raw_packet_request_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSIMPLESTREAM_H__
