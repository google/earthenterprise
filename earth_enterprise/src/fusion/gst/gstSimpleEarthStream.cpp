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


// Simplified version of gstEarthStream.cpp with many fewer
// dependencies.

#include "fusion/gst/gstSimpleEarthStream.h"

#include <compressor.h>
#include <kbf.h>
#include <khMTTypes.h>
#include <khRefCounter.h>
#include <khCache.h>
#include <khException.h>
#include <khEndian.h>
#include <packetcompress.h>
#include <qtpacket/quadtreepacket.h>
#include <qtpacket/quadtree_utils.h>
#include <zlib.h>
#include <malloc.h>
#include <sstream>
#include <string>
#include <vector>
#include "common/etencoder.h"
#include "common/khStringUtils.h"
#include "common/khConstants.h"

namespace gst {

Request::Request(bool keep_alive)
    : curl_easy_handle_(curl_easy_init()), headers_(NULL) {
  if (curl_easy_handle_ == NULL) {
    throw khException(kh::tr("Unable to initialize HTTP object"));
  }

  const char* kh_cookie = getenv("KH_AUTH_COOKIE");
  if (kh_cookie) {
    std::string cookie("Cookie: ");
    cookie += std::string(kh_cookie);
    headers_ = curl_slist_append(headers_, cookie.c_str());
  }

  if (keep_alive) {
    // Ask for a persistent connection
    headers_ = curl_slist_append(headers_, "Connection: Keep-Alive");
  } else {
    headers_ = curl_slist_append(headers_, "Connection:");
  }

  // Set the Accept: field (copied from wininetimpl.cpp)
  headers_ = curl_slist_append(headers_,
      "Accept: "
      "text/plain, text/html, text/xml, text/xml-external-parsed-entity, "
      "application/octet-stream, application/vnd.google-earth.kml+xml, "
      "application/vnd.google-earth.kmz, image/*");

  if (headers_ != NULL) {
    curl_easy_setopt(curl_easy_handle_, CURLOPT_HTTPHEADER, headers_);
  }

  // disable progress meter
  curl_easy_setopt(curl_easy_handle_, CURLOPT_NOPROGRESS, true);
  // turn off signals
  curl_easy_setopt(curl_easy_handle_, CURLOPT_NOSIGNAL, true);
  // fail on errors
  curl_easy_setopt(curl_easy_handle_, CURLOPT_FAILONERROR, true);
  curl_easy_setopt(curl_easy_handle_, CURLOPT_FOLLOWLOCATION, true);
  curl_easy_setopt(curl_easy_handle_, CURLOPT_MAXREDIRS, 16);

  // Don't require server certificate verification for https.
  curl_easy_setopt(curl_easy_handle_, CURLOPT_SSL_VERIFYPEER, false);
  curl_easy_setopt(curl_easy_handle_, CURLOPT_WRITEFUNCTION,
                   Request::CurlWriteFunc);
  curl_easy_setopt(curl_easy_handle_, CURLOPT_ERRORBUFFER, error_buffer_);
}

Request::~Request() {
  if (curl_easy_handle_ != NULL) {
    if (headers_ != NULL) {
      curl_easy_setopt(curl_easy_handle_, CURLOPT_HTTPHEADER, NULL);
      curl_slist_free_all(headers_);
    }
    curl_easy_cleanup(curl_easy_handle_);
  }
}

//------------------------------------------------------------------------------

void Request::Start(const std::string& url, std::string* stream) {
  stream_ = stream;
  stream_->clear();
  curl_easy_setopt(curl_easy_handle_, CURLOPT_WRITEDATA, stream_);

  curl_easy_setopt(curl_easy_handle_, CURLOPT_URL, url.c_str());
  CURLcode code = curl_easy_perform(curl_easy_handle_);
  if (code != CURLE_OK) {
    notify(NFY_DEBUG, "curl_easy_perform return code: %d", code);
    notify(NFY_DEBUG, "HTTP Error: %s", error_buffer_);
    notify(NFY_DEBUG, "  for: %s", url.c_str());
  }
  if (GetBufferSize() == 0) {
    size_t buffer_size = GetBufferSize();
    notify(NFY_DEBUG, "Curl_easy_perform: Returned buffer size is %lu.\n",
           static_cast<std::uint64_t>(buffer_size));
  }
  std::int64_t result_code = 0;
  curl_easy_getinfo(curl_easy_handle_, CURLINFO_RESPONSE_CODE, &result_code);
  if (result_code != 200 || GetBufferSize() == 0) {
    notify(NFY_DEBUG, "curl_easy_getinfo: response code: %ld\n", result_code);
    SetStatus(false);
  } else {
    SetStatus(true);
  }
}

// static callback that libCURL calls upon read completion
size_t Request::CurlWriteFunc(void* buffer, size_t size, size_t nmemb,
                              void* userp) {
  size_t total_bytes = size * nmemb;
  if (total_bytes != 0) {
    std::string* stream = static_cast<std::string*>(userp);
    stream->append(static_cast<const char*>(buffer), total_bytes);
  }
  return nmemb;
}

//------------------------------------------------------------------------------

void DbRootRequest::Init() {
  // Nothing to do here. Just needed to make sure it came back OK
}

//------------------------------------------------------------------------------

void RawPacketRequest::StartGet(const std::string& url,
                                std::string* stream,
                                bool decrypt) {
  curl_easy_setopt(curl_easy_handle_, CURLOPT_HTTPGET, true);
  Start(url, stream, decrypt);
}

void RawPacketRequest::StartPost(const std::string& url,
                                 const std::string& data,
                                 std::string* stream,
                                 bool decrypt) {
  curl_easy_setopt(curl_easy_handle_, CURLOPT_POST, true);
  curl_easy_setopt(curl_easy_handle_, CURLOPT_POSTFIELDS, data.c_str());
  Start(url, stream, decrypt);
}

void RawPacketRequest::Init() {
  if (GetBuffer() == 0 || GetBufferSize() == 0 || !decrypt_)
    return;

  // decrypt buffer
  etEncoder::DecodeWithDefaultKey(GetBuffer(), GetBufferSize());
}

}  // end namespace gst

//------------------------------------------------------------------------------

void gstSimpleEarthStream::Init() {
  curl_global_init(CURL_GLOBAL_ALL);

  // XXX cleanup curl when app terminates?
  // XXX curl_global_cleanup();
}

gstSimpleEarthStream::gstSimpleEarthStream(
    const std::string& server, const std::string& additional_args, bool use_post)
  : server_(server), raw_packet_request_(), use_post_(use_post) {
}

gstSimpleEarthStream::~gstSimpleEarthStream() {
}

bool gstSimpleEarthStream::GetQuadTreePacket(
    const QuadtreePath &qt_path,
    std::string *raw_packet,
    const std::string& additional_args) {
  assert(raw_packet);
  // Please note that in serverdbReader.cpp for quadtree packets
  // versions_matters = false. Version number can be included in  additional
  // args as to be able to crawl Portable Server, which only supports latest
  // version of clients.
  // I.e quadtree requests that come with version number.
  const std::string url = server_ + "/query";
  const std::string args = "request=QTPacket&blist=" +
                           qt_path.AsString() + additional_args;
  notify(NFY_DEBUG, "GetQuadTreePacket: %s %s", url.c_str(), args.c_str());
  return PostRawPacket(url, args, raw_packet, true);
}


void gstSimpleEarthStream::GetQuadTreePacketBunch(
    const std::vector<std::string>& qt_paths,
    LittleEndianReadBuffer* buffer_array,
    const std::string& additional_args) {
  // Please note that in serverdbReader.cpp for quadtree packets
  // versions_matters = false. Version number can be included in  additional
  // args as to be able to crawl Portable Server, which only supports latest
  // version of clients.
  // I.e quadtree requests that come with version number.
  std::string paths;
  for (size_t i = 0; i < qt_paths.size(); ++i) {
    paths.append(qt_paths[i]);
    paths.append(1, kQuadTreePathSeparator);  // 4 is the separator
  }

  const std::string url = server_ + "/query";
  const std::string args = "request=QTPacket&blist=" +
                           paths + additional_args;
  std::string raw_packet;
  raw_packet.reserve(1024 * 1024 * 128);  // 128 MB for around 512 packets
  if (!PostRawPacket(url, args, &raw_packet, false)) {
    return;
  }
  std::vector<std::string> compressed_strings(qt_paths.size());
  notify(NFY_DEBUG, "GetQuadTreePacketBunch: %s %s", url.c_str(), args.c_str());
  static_cast<PackedString*>(&raw_packet)->Unpack(&compressed_strings[0]);
  for (size_t i = 0; i < qt_paths.size(); ++i, ++buffer_array) {
    size_t ret_size = compressed_strings[i].size();
    if (ret_size == 0) {
      continue;
    }
    etEncoder::DecodeWithDefaultKey(&compressed_strings[i][0], ret_size);
    const std::string& compressed = compressed_strings[i];
    KhPktDecompress(compressed.data(), compressed.size(), buffer_array);
  }
}


// TODO: Switch to POST when this starts getting used.
bool gstSimpleEarthStream::GetQuadTreeFormat2Packet(
    const QuadtreePath &qt_path,
    std::string *raw_packet,
    const std::string& additional_args) {
  assert(raw_packet);
  std::string url = server_ + "/query?request=QTPacket2&blist=" +
      qt_path.AsString() + "&version=1" + additional_args;
  return GetRawPacket(url, raw_packet, true);
}


bool gstSimpleEarthStream::GetRawPacket(
    const std::string& url,
    std::string* raw_packet,
    bool decrypt) {
  assert(raw_packet);
  notify(NFY_VERBOSE, "GetRawPacket: %s", url.c_str());
  raw_packet_request_.StartGet(url, raw_packet, decrypt);
  if (raw_packet_request_.GetBufferSize() > 0) {
    return true;
  } else {
    raw_packet->clear();
    return false;
  }
}

bool gstSimpleEarthStream::PostRawPacket(
    const std::string& url,
    const std::string& data,
    std::string* raw_packet,
    bool decrypt) {

  if (!use_post_) {
    return GetRawPacket(url + "?" + data, raw_packet, decrypt);
  }

  assert(raw_packet);
  notify(NFY_VERBOSE, "PostRawPacket: %s", url.c_str());
  notify(NFY_VERBOSE, "PostRawPacket data: %s", data.c_str());
  raw_packet_request_.StartPost(url, data, raw_packet, decrypt);
  if (raw_packet_request_.GetBufferSize() > 0) {
    return true;
  } else {
    raw_packet->clear();
    return false;
  }
}
