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


#include <zlib.h>
#include <malloc.h>
#include <sstream>
#include <khMTTypes.h>
#include <kbf.h>
#include <Qt/q3cstring.h>
#include <khRefCounter.h>
#include <khCache.h>
#include <khException.h>
#include <khEndian.h>
#include <qtpacket/quadtreepacket.h>
#include <qtpacket/quadtree_utils.h>
#include <compressor.h>
#include <packetcompress.h>
#include "common/etencoder.h"

#include "gstEarthStream.h"

// some notes about quadtree packets:
//
// the root quadtree packet q2-0 will contain four levels:
//      ""              1
//     "0" - "3"        4
//    "00" - "33"       16
//   "000" - "333"      64
//
// each additional quadtree packet (ie. q2-0301) will contain five levels,
// but the first level acts as the root of this local tree and it's
// entry is undefined since it overlaps with the previous quadtree packet
//      ""              1       (undefined!)
//     "0" - "3"        4
//    "00" - "33"       16
//   "000" - "333"      64
//  "0000" - "3333"     256
//
// For example, the quadtree packet q2-0301 won't provide any details
// for blist 301 as this is contained in q2-0.  But it will provide
// details for blist 3010, 3011, 3012, ..., 3013333.

ImageVersion gstEarthStream::invalid_version = ImageVersion(-1);

namespace {

class Request {
 public:
  Request(CURL* curl, std::string url)
    : curl_easy_handle_(curl), url_(url) {}
  virtual ~Request() {}

  void Start();

  virtual void Init() = 0;

  void SetStatus(bool status) { status_ = status; }
  bool GetStatus() const { return status_; }

  static size_t CurlWriteFunc(void* buffer, size_t size, size_t nmemb, void* userp);

  void WriteData(void* buffer, size_t size);

  char* GetBuffer() { return stream_.data(); }
  size_t GetBufferSize() { return stream_.size(); }

 private:
  CURL* curl_easy_handle_;

  QByteArray stream_;
  std::string url_;
  bool status_;
};

//------------------------------------------------------------------------------

class DbRootRequest : public Request {
 public:
  DbRootRequest(CURL* curl, std::string url) : Request(curl, url) {}
  virtual ~DbRootRequest() {}

  virtual void Init();
};

//------------------------------------------------------------------------------

class ImageRequest : public Request {
 public:
  ImageRequest(CURL* curl, std::string url) : Request(curl, url) {}
  virtual ~ImageRequest() {}

  virtual void Init();

  const char* GetImage() const { return &jpeg_image_[0]; }

 private:
  std::vector<char> jpeg_image_;
  static khDeleteGuard<Compressor> compressor_;
};

//------------------------------------------------------------------------------

class ImageExistanceImpl;
typedef khRefGuard<ImageExistanceImpl> ImageExistanceHandle;

class ImageExistanceImpl : public khRefCounter {
 public:
  static ImageExistanceHandle Create() {
    return khRefGuardFromNew(new ImageExistanceImpl());
  }

  ImageVersion GetVersion(const std::string& blist);
  void SetVersion(const std::string& blist, int ver) {
    // SetVersion is only called with a relative blist (4 chars or less)
    versions_[NameToIndex(blist)] = ver;
  }

  // determine amount of memory used by ImageExistanceImpl
  std::uint64_t GetSize() {
    return sizeof(versions_);
  }

  static std::string PacketName(const std::string& blist);
  static int NameToIndex(const std::string& blist);

 private:
  ImageExistanceImpl() {
    memset(&versions_[0], gstEarthStream::invalid_version,
           sizeof(ImageVersion) * 341);
  }

  ImageVersion versions_[341];
};

std::string ImageExistanceImpl::PacketName(const std::string& blist) {
  // The server requires a path of "0" for the root.
  if (blist.length() < 4) {
    // blist "", "0", "00", "000" all map to the root packet q2-0
    // For example blist "301" comes from q2-0.
    return std::string("0");
  } else {
    // The blist cannot map to the root of a quadtree packet as
    // this is invalid.  It must come from the previous packet.
    // For example blists "3010", "30100", "301000" & "3010000" will
    // all map to q2-0301.
    std::string pak_name = std::string("0") + blist;
    int take_away = pak_name.length() % 4;
    if (take_away == 0)
      take_away = 4;
    pak_name.resize(pak_name.length() - take_away);
    return pak_name;
  }
}

ImageVersion ImageExistanceImpl::GetVersion(const std::string& blist) {
  // GetVersion() will get called with the full blist, so
  // must special case the first quadtree packet which contains
  // the top 4 levels (1 + 4 + 16 + 64) = 85 entries
  // each additional quadtree packet will cover 5 levels
  // (1 + 4 + 16 + 64 + 256) = 341 entries
  if (blist.length() < 4) {
    return versions_[NameToIndex(blist)];
  } else {
    int quads = (blist.length() / 4) - 1;
    std::string tail = blist.substr(3 + (quads * 4));
    return versions_[NameToIndex(tail)];
  }
}


int ImageExistanceImpl::NameToIndex(const std::string& blist) {
  int index = 0;
  switch (blist.length()) {
    case 1:
      index = (blist[0] - '0' + 1);
      break;
    case 2:
      index = ((blist[0] - '0' + 1) * 4) +
               (blist[1] - '0' + 1);
      break;
    case 3:
      index = ((blist[0] - '0' + 1) * 16) +
              ((blist[1] - '0' + 1) * 4) +
               (blist[2] - '0' + 1);
      break;
    case 4:
      index = ((blist[0] - '0' + 1) * 64) +
              ((blist[1] - '0' + 1) * 16) +
              ((blist[2] - '0' + 1) * 4) +
               (blist[3] - '0' + 1);
      break;
  }
  return index;
}

//------------------------------------------------------------------------------

class RawPacketRequest : public Request {
 public:
  RawPacketRequest(CURL* curl, std::string url)
      : Request(curl, url) {
    decrypt_ = true;
  }

  RawPacketRequest(CURL* curl, std::string url, bool decrypt)
      : Request(curl, url), decrypt_(decrypt) { }

  virtual ~RawPacketRequest() {}

  virtual void Init();
 private:
  bool decrypt_;
  DISALLOW_COPY_AND_ASSIGN(RawPacketRequest);
};

//------------------------------------------------------------------------------

class QuadTreeRequest : public RawPacketRequest {
 public:
  QuadTreeRequest(CURL* curl, std::string url)
    : RawPacketRequest(curl, url),
      image_existance_(ImageExistanceImpl::Create()) {}
  virtual ~QuadTreeRequest() {}

  virtual void Init();
  ImageExistanceHandle GetImageExistance() {
    return image_existance_;
  }

 private:
  void InstanceDrillDown(const qtpacket::KhQuadTreePacket16* packet,
                         const std::string& path, int* index);
  ImageExistanceHandle image_existance_;
  DISALLOW_COPY_AND_ASSIGN(QuadTreeRequest);
};

//------------------------------------------------------------------------------

void Request::Start() {
  khLockGuard lock(gstEarthStream::mutex);

  char error_buffer[CURL_ERROR_SIZE];
  // disable progress meter
  curl_easy_setopt(curl_easy_handle_, CURLOPT_NOPROGRESS, true);
  // turn off signals
  curl_easy_setopt(curl_easy_handle_, CURLOPT_NOSIGNAL, true);
  curl_easy_setopt(curl_easy_handle_, CURLOPT_URL, url_.c_str());
  curl_easy_setopt(curl_easy_handle_, CURLOPT_WRITEFUNCTION, Request::CurlWriteFunc);
  curl_easy_setopt(curl_easy_handle_, CURLOPT_WRITEDATA, (void*)this);
  // fail on errors
  curl_easy_setopt(curl_easy_handle_, CURLOPT_FAILONERROR, true);
  curl_easy_setopt(curl_easy_handle_, CURLOPT_FOLLOWLOCATION, true);
  curl_easy_setopt(curl_easy_handle_, CURLOPT_MAXREDIRS, 16);
  curl_easy_setopt(curl_easy_handle_, CURLOPT_ERRORBUFFER, error_buffer);

  curl_slist* headers = NULL;
  const char* kh_cookie = getenv("KH_AUTH_COOKIE");
  if (kh_cookie) {
    std::string cookie("Cookie: ");
    cookie += std::string(kh_cookie);
    headers = curl_slist_append(headers, cookie.c_str());
  }

  // Ask for a persistent connection
  headers = curl_slist_append(headers, "Connection: Keep-Alive");

  // Set the Accept: field (copied from wininetimpl.cpp)
  headers = curl_slist_append(headers,
      "Accept: "
      "text/plain, text/html, text/xml, text/xml-external-parsed-entity, "
      "application/octet-stream, application/vnd.google-earth.kml+xml, "
      "application/vnd.google-earth.kmz, image/*");


  if (headers != NULL)
    curl_easy_setopt(curl_easy_handle_, CURLOPT_HTTPHEADER, headers);

  CURLcode code = curl_easy_perform(curl_easy_handle_);
  if (code != CURLE_OK) {
    notify(NFY_DEBUG, "curl_easy_perform return code: %d", code);
    notify(NFY_DEBUG, "HTTP Error: %s", error_buffer);
    notify(NFY_DEBUG, "  for: %s", url_.c_str());
  }
  if (GetBufferSize()==0) {
    unsigned long buffer_size= static_cast<unsigned long>(GetBufferSize());
    notify(NFY_DEBUG, "Curl_easy_perform: Returned buffer size is %lu.\n", buffer_size);
  }
  long result_code = 0;
  curl_easy_getinfo(curl_easy_handle_, CURLINFO_RESPONSE_CODE, &result_code);
  if (result_code != 200 || GetBufferSize()==0) {
    notify(NFY_DEBUG, "curl_easy_getinfo: response code: %ld\n", result_code);
    SetStatus(false);
  } else {
    SetStatus(true);
    Init();
  }

  curl_easy_setopt(curl_easy_handle_, CURLOPT_HTTPHEADER, NULL);

  if (headers != NULL)
    curl_slist_free_all(headers);
}

// static callback that libCURL calls upon read completion
size_t Request::CurlWriteFunc(void* buffer, size_t size, size_t nmemb, void* request) {
  ((Request*)request)->WriteData(buffer, size * nmemb);
  return nmemb;
}

// a single request may generate multiple write callbacks so continue to
// grow memory buffer with new data
void Request::WriteData(void* buffer, size_t size) {
  if (size == 0)
    return;

  size_t prev_size = stream_.size();
  stream_.resize(prev_size + size);
  size_t new_size = stream_.size();
  if (new_size == 0 || new_size == prev_size)
    return;

  memcpy(stream_.data() + prev_size, buffer, size);
}

//------------------------------------------------------------------------------

void DbRootRequest::Init() {
  // nothing to do here - just wanted to make sure it came back OK
}

//------------------------------------------------------------------------------

khDeleteGuard<Compressor> ImageRequest::compressor_;

void ImageRequest::Init() {
  // decrypt the buffer
  etEncoder::DecodeWithDefaultKey(GetBuffer(), GetBufferSize());
  // validate the jpeg header
  const unsigned char jfifhdrA[] = { 0xff, 0xd8, 0xff, 0xe0 };
  const unsigned char jfifhdrB[] = { 'J', 'F', 'I', 'F' };
  if (GetBufferSize()<10 ||
      memcmp(jfifhdrA, GetBuffer(), 4) != 0 ||
      memcmp(jfifhdrB, GetBuffer() + 6, 4) != 0) {
    notify(NFY_WARN, "texture buffer read from HTTP doesn't contain a valid JFIF header, skipping!");
    return;
  }

  std::vector<char> tmp_image;
  tmp_image.resize(256 * 256 * 3);

  if (!compressor_) {
    compressor_ = TransferOwnership(new JPEGCompressor(256, 256, 3, 0));
  }
  compressor_->decompress(GetBuffer(), GetBufferSize(), &tmp_image[0]);

  jpeg_image_.resize(256 * 256 * 3);

  // flip image orientation
  const int linesz = 256 * 3;
  for (int y = 0; y < 256; ++y) {
    memcpy(&jpeg_image_[0] + (y * linesz), &tmp_image[0] + ((255 - y) * linesz), linesz);
  }
}

//------------------------------------------------------------------------------

void RawPacketRequest::Init() {
  if (GetBuffer() == 0 || GetBufferSize() == 0 || !decrypt_)
    return;

  // decrypt buffer
  etEncoder::DecodeWithDefaultKey(GetBuffer(), GetBufferSize());
}

//------------------------------------------------------------------------------

void QuadTreeRequest::Init() {
  RawPacketRequest::Init();

  // decompress buffer
  LittleEndianReadBuffer data;
  if (!KhPktDecompress(GetBuffer(), GetBufferSize(), &data)) {
    notify(NFY_WARN, "Unable to decompress quadtree buffer");
    return;
  }

  // convert buffer to data packet
  qtpacket::KhQuadTreePacket16 packet;
  data >> packet;

  // iterate over each instance in packet
  std::string path = "";
  int index = 0;
  InstanceDrillDown(&packet, path, &index);
}

void QuadTreeRequest::InstanceDrillDown(
    const qtpacket::KhQuadTreePacket16* packet,
    const std::string& path,
    int* index) {
  const qtpacket::KhQuadTreeQuantum16* instance =
    packet->GetDataInstanceP((*index)++);
  // Only add if there is an image here.  This is necessary because
  // we can also end up here if there is an image neighbor tag.
  if (instance->children.GetImageBit()) {
    image_existance_->SetVersion(path, instance->image_version);
  }
  if (!instance->children.GetCacheNodeBit()) {
    for (int child = 0; child < 4; ++child) {
      if (instance->children.GetBit(child)) {
        switch (child) {
          case 0:       // SW
            InstanceDrillDown(packet, path + "0", index);
            break;
          case 1:       // SE
            InstanceDrillDown(packet, path + "1", index);
            break;
          case 2:       // NE
            InstanceDrillDown(packet, path + "2", index);
            break;
          case 3:       // NW
            InstanceDrillDown(packet, path + "3", index);
            break;
        }
      }

    }
  }

}

} // namespace

//------------------------------------------------------------------------------

khMutex gstEarthStream::mutex;

void gstEarthStream::Init() {
  curl_global_init(CURL_GLOBAL_ALL);

  // XXX cleanup curl when app terminates?
  // XXX curl_global_cleanup();
}

gstEarthStream::gstEarthStream(const std::string& server)
  : server_(server) {
  image_request_handle_ = curl_easy_init();
  quadtree_request_handle_ = curl_easy_init();
  if (image_request_handle_ == NULL || quadtree_request_handle_ == NULL) {
    throw khException(kh::tr("Unable to initialize HTTP object"));
  } else {
    // fetch crypt key from dbroot
    std::string url = server_ + "/dbRoot.v5";

    DbRootRequest request(image_request_handle_, url);
    request.Start();
    if (!request.GetStatus()) {
      throw khException(kh::tr("Unable to open HTTP database: %1").arg(server.c_str()));
    }
  }
}

gstEarthStream::~gstEarthStream() {
}

bool gstEarthStream::GetImage(const gstQuadAddress& addr, char* buffer) {
  ImageVersion ver = GetImageVersion(addr);
  if (ver == invalid_version)
    return false;
  std::ostringstream ver_str;
  ver_str << ver;
  std::string url = server_ + "/flatfile?f1-0" + addr.BlistAsString() + "-i." + ver_str.str();
  ImageRequest request(image_request_handle_, url);
  request.Start();
  if (request.GetStatus()) {
    memcpy(buffer, request.GetImage(), 256 * 256 * 3);
    return true;
  } else {
    return false;
  }
}

ImageVersion gstEarthStream::GetImageVersion(const gstQuadAddress& addr) {
  std::string blist = addr.BlistAsString();
  static khCache<std::string, ImageExistanceHandle>
    image_existance_cache(kImageExistanceCacheSize, "image existence cache");
  ImageExistanceHandle image_existance;
  if (!image_existance_cache.Find(ImageExistanceImpl::PacketName(blist),
                                   image_existance)) {
    std::string url = server_ + "/flatfile?q2-" + ImageExistanceImpl::PacketName(blist);
    QuadTreeRequest request(quadtree_request_handle_, url);
    request.Start();
    image_existance = request.GetImageExistance();
    image_existance_cache.Add(ImageExistanceImpl::PacketName(blist), image_existance);
  }

  ImageVersion ver = image_existance->GetVersion(blist);
  return ver;
}


bool gstEarthStream::GetQuadTreePacket(
    const QuadtreePath &qt_path,
    std::string *raw_packet) {
  assert(raw_packet);
  // Please note that in serverdbReader.cpp for quadtree packets
  // versions_matters = false. We forced this version 1 so as to be able to
  // crawl Portable Server, which only supports latest version of clients,
  // i.e quadtree requests that come with version number.
  const std::string url = server_ + "/flatfile?q2-0" + qt_path.AsString() +
      "-q.1";
  return GetRawPacket(url, raw_packet);
}

bool gstEarthStream::GetQuadTreeFormat2Packet(
    const QuadtreePath &qt_path,
    std::string *raw_packet) {
  assert(raw_packet);
  std::string url = server_ + "/flatfile?db=tm&qp-0" + qt_path.AsString();
  return GetRawPacket(url, raw_packet);
}

bool gstEarthStream::GetRawImagePacket(
    const QuadtreePath &qt_path,
    const std::uint16_t version,
    const keyhole::JpegCommentDate& jpeg_date,
    std::string *raw_packet) {
  assert(raw_packet);
  std::ostringstream url;
  if (jpeg_date.IsCompletelyUnknown()) {
    url << server_ << "/flatfile?f1-0" << qt_path.AsString() << "-i." << version;
  } else {
    // Dated request has form:    flatfile?db=tm&f1-03-i.7-faee4
    // where the hex code at the end is the jpeg date string in hex format.
    url << server_ << "/flatfile?db=tm&f1-0" << qt_path.AsString() << "-i."
      << version << "-" << jpeg_date.GetHexString();
  }

  return GetRawPacket(url.str(), raw_packet);
}

bool gstEarthStream::GetRawTerrainPacket(
    const QuadtreePath &qt_path,
    const std::uint16_t version,
    std::string *raw_packet) {
  assert(raw_packet);
  std::ostringstream url;
  url << server_ << "/flatfile?f1c-0" << qt_path.AsString() << "-t." << version;
  return GetRawPacket(url.str(), raw_packet);
}

bool gstEarthStream::GetRawVectorPacket(
    const QuadtreePath &qt_path,
    const std::uint16_t channel,
    const std::uint16_t version,
    std::string *raw_packet) {
  assert(raw_packet);
  std::ostringstream url;
  url << server_ << "/flatfile?f1c-0" << qt_path.AsString()
      << "-d." << channel << "." << version;
  return GetRawPacket(url.str(), raw_packet);
}

bool gstEarthStream::GetRawPacket(
    const std::string &url,
    std::string *raw_packet) {
  return GetRawPacket(url, raw_packet, true);
}

bool gstEarthStream::GetRawPacket(
    const std::string &url,
    std::string *raw_packet,
    bool decrypt) {
  assert(raw_packet);
  notify(NFY_VERBOSE, "GetRawPacket: %s", url.c_str());
  RawPacketRequest request(quadtree_request_handle_, url, decrypt);
  request.Start();
  if (request.GetBufferSize() > 0) {
    raw_packet->assign(request.GetBuffer(), request.GetBufferSize());
    return true;
  } else {
    raw_packet->clear();
    return false;
  }
}
