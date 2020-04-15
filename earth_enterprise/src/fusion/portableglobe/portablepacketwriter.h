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


// Classes for pruning away all quadtree nodes that are not at or below
// the given default level or encompassing the given set of
// quadtree nodes. Data referenced by quadtree nodes that are not pruned
// are stored in packet bundles.

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PORTABLEPACKETWRITER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PORTABLEPACKETWRITER_H_

#include <algorithm>
#include <map>
#include <string>
#include <vector>
#include "common/qtpacket/quadtreepacket.h"
#include "fusion/gst/gstSimpleEarthStream.h"
#include "fusion/portableglobe/shared/packetbundle_reader.h"
#include "fusion/portableglobe/shared/packetbundle_writer.h"
#include "mttypes/DrainableQueue.h"
#include "mttypes/WaitBaseManager.h"

namespace fusion_portableglobe {

// Abstract base class for globe and map builders.
class PortableBuilder {
 public:
  virtual void GetImagePacket(
    const std::string& qtpath, std::uint32_t version, std::string* raw_packet) = 0;

  virtual void GetVectorPacket(
    const std::string& qtpath, std::uint32_t channel, std::uint32_t version,
    std::string* raw_packet) = 0;

  // Does nothing by default.
  virtual void GetTerrainPacket(
    const std::string& qtpath, std::uint32_t version, std::string* raw_packet) = 0;
};


// Each WriteRequest is a request for a packet that gets buffered
class WriteRequest {
 public:
  std::uint32_t version_;
  std::uint32_t channel_;
  PacketType packet_type_;      // kImagePacket/kTerrainPacket/kVectorPacket
  std::string qt_path_;         // This stores qt_path
  std::pair<const char*, size_t> request_result_;  // This stores result packet
  WriteRequest(std::uint32_t version, std::uint32_t channel, PacketType packet_type,
               const std::string& qt_path)
      : version_(version), channel_(channel), packet_type_(packet_type),
        qt_path_(qt_path) {}
};


class Cache : public std::vector<WriteRequest> {
 public:
  std::string* CopyRawPacketBuffer(const std::string& buffer) {
    std::string* ptr = new std::string(buffer);
    raw_packet_buffer_.push_back(ptr);
    return ptr;
  }

  void clear() {
    std::vector<WriteRequest>::clear();
    raw_packet_buffer_.clear();  // This takes care of deleting the strings
  }
 private:
  khDeletingVector<std::string> raw_packet_buffer_;
};

// Buffers requests for packets. It uses bundling of queries, based on the
// current GEE servers ability to combine a lot of queries where qt-path is the
// only variable, into a bundled qt-path(using kQuadTreePathSeparator (i.e 4))
// and the result will be a concatentated PackedString in the same sequence.
// The purpose is to avoid the connection overhead for each query.
//
// Also in case is_multi_thread_ is true, uses a separate thread to write.
// The main thread does the read (i.e querying the GEE server).
class RequestBundlerForPacket : public mttypes::WaitBaseManager {
  typedef mttypes::Queue<Cache*> CacheQueue;

 public:
  // first -> concatenated paths, second -> result vector
  typedef std::pair< std::string, std::vector<WriteRequest*> > RequestBundle;
  // first -> version, second -> the request_bundle for that
  typedef std::map< std::uint32_t, RequestBundle > ImageryBundle;
  typedef std::map< std::uint32_t, RequestBundle > TerrainBundle;
  // first -> {channel, version}, second -> the request_bundle for that
  typedef std::map< std::pair<std::uint32_t, std::uint32_t>, RequestBundle > VectorBundle;

  RequestBundlerForPacket(int bunch_size, PortableBuilder* builder,
                          PacketBundleWriter* writer, bool is_multi_thread,
                          std::uint32_t batch_size);

  ~RequestBundlerForPacket();

  void AddToCache(const WriteRequest& request);

  void WriteImagePacket(const std::string& qtpath, std::uint32_t version);

  void WriteTerrainPacket(const std::string& qtpath, std::uint32_t version);

  void WriteVectorPacket(
      const std::string& qtpath, std::uint32_t channel, std::uint32_t version);

  void FlushCache();

  // Writes one bundle of bunch_size_ queries' result in proper sequence
  void WriteCache(Cache* cache);

  // This is the only function running in child thread in case is_multi_thread_
  // is set.
  void Write();

 private:
  Cache* cache_;

  const size_t bunch_size_;
  PortableBuilder* const caller_;
  PacketBundleWriter* const writer_;
  bool const is_multi_thread_;
  const std::uint32_t batch_size_;
  khDeletingVector<Cache> cache_deleter_;
  std::string raw_packet_;
  khDeletingVector<mttypes::ManagedThread> thread_deleter_;
  CacheQueue write_queue_;
  CacheQueue read_queue_;
  CacheQueue::PullHandle next_;
  mttypes::BatchingQueuePuller<Cache*> batch_puller_;
  mttypes::BatchingQueuePusher<Cache*> batch_pusher_;
};


}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_PORTABLEPACKETWRITER_H_
