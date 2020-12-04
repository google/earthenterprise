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

// Simplified version of gstEarthStream.h with many fewer
// dependencies.
#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSIMPLEEARTHSTREAM_H__
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSIMPLEEARTHSTREAM_H__

#include <Qt/q3cstring.h>
#include <curl/curl.h>
#include <khEndian.h>
#include <string>
#include <vector>
#include "fusion/gst/gstQuadAddress.h"

typedef std::uint16_t ImageVersion;
class QuadtreePath;

class RawPacketRequest;
class DbRootRequest;

namespace gst {
class Request {
 public:
  Request(bool keep_alive);
  ~Request();

  void Start(const std::string& url, std::string* stream);
  void SetStatus(bool status) { status_ = status; }
  bool GetStatus() const { return status_; }
  char* GetBuffer() { return &(*stream_)[0]; }
  size_t GetBufferSize() { return stream_->size(); }
  bool IsInitialized() const { return curl_easy_handle_ != NULL; }

  static size_t CurlWriteFunc(void* buffer, size_t size,
                              size_t nmemb, void* userp);
 protected:
  CURL* const curl_easy_handle_;

 private:
  curl_slist* headers_;

  std::string* stream_;
  bool status_;
  char error_buffer_[CURL_ERROR_SIZE];
};


//------------------------------------------------------------------------------

class RawPacketRequest : public Request {
 public:

  RawPacketRequest() : Request(true), decrypt_(true) {}
  ~RawPacketRequest() {}

  void Start(const std::string& url, std::string* stream, bool decrypt) {
    decrypt_ = decrypt;
    Request::Start(url, stream);
    if (GetStatus()) {
      Init();
    }
  }

  void StartGet(const std::string& url, std::string* stream, bool decrypt);
  void StartPost(const std::string& url,
                 const std::string& data,
                 std::string* stream,
                 bool decrypt);

  void Init();
 private:
  bool decrypt_;
  DISALLOW_COPY_AND_ASSIGN(RawPacketRequest);
};


//------------------------------------------------------------------------------

class DbRootRequest : public Request {
 public:
  DbRootRequest() : Request(false) {}
  ~DbRootRequest() {}
  void Start(const std::string& url, std::string* stream) {
    Request::Start(url, stream);
    if (GetStatus()) {
      Init();
    }
  }

  void Init();
};
}  // end namespace gst

class gstSimpleEarthStream {
 public:
  gstSimpleEarthStream(const std::string& server,
                       const std::string& additional_args="",
                       const bool use_post_=false);
  ~gstSimpleEarthStream();

  static void Init();

  bool GetQuadTreePacket(const QuadtreePath &qt_path,
                         std::string *raw_packet,
                         const std::string& additional_args);
  void GetQuadTreePacketBunch(const std::vector<std::string>& qt_paths,
                              LittleEndianReadBuffer* buffer_array,
                              const std::string& additional_args);
  bool GetQuadTreeFormat2Packet(const QuadtreePath &qt_path,
                                std::string *raw_packet,
                                const std::string& additional_args);
  bool GetRawPacket(const std::string& url,
                    std::string* raw_packet,
                    bool decrypt);
  bool PostRawPacket(const std::string& url,
                     const std::string& data,
                     std::string* raw_packet,
                     bool decrypt);

 private:
  const std::string server_;
  gst::RawPacketRequest raw_packet_request_;
  const bool use_post_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_GST_GSTSIMPLEEARTHSTREAM_H__
