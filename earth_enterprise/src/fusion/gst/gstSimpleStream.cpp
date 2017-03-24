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


// Simplified version of gstEarthStream.cpp with many fewer
// dependencies.

#include "fusion/gst/gstSimpleStream.h"

#include <string>


void gstSimpleStream::Init() {
  curl_global_init(CURL_GLOBAL_ALL);
}

gstSimpleStream::gstSimpleStream(const std::string& server)
  : server_(server), raw_packet_request_() {
}

gstSimpleStream::~gstSimpleStream() {
}

bool gstSimpleStream::GetRawPacket(
    const std::string &url,
    std::string *raw_packet) {
  assert(raw_packet);
  notify(NFY_VERBOSE, "GetRawPacket: %s", url.c_str());
  raw_packet_request_.Start(url, raw_packet, false);
  if (raw_packet_request_.GetBufferSize() > 0) {
    return true;
  } else {
    raw_packet->clear();
    return false;
  }
}
