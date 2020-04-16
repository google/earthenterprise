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

// Methods for queueing up packets to be passed to the packet
// writer.

#include "fusion/portableglobe/portablepacketwriter.h"

#include "common/khConstants.h"
#include "common/khEndian.h"
#include "common/khFileUtils.h"
#include "common/khStringUtils.h"
//#include "common/khTypes.h"
#include <cstdint>
#include "fusion/portableglobe/shared/packetbundle.h"
#include "fusion/portableglobe/shared/packetbundle_reader.h"
#include "fusion/portableglobe/shared/packetbundle_writer.h"
#include "mttypes/DrainableQueue.h"
#include "mttypes/WaitBaseManager.h"

namespace fusion_portableglobe {

RequestBundlerForPacket::RequestBundlerForPacket(
    int bunch_size,
    PortableBuilder* builder,
    PacketBundleWriter* writer, bool is_multi_thread,
    std::uint32_t batch_size)
    : bunch_size_(bunch_size), caller_(builder), writer_(writer),
      is_multi_thread_(is_multi_thread),
      batch_size_(batch_size),
      batch_puller_(batch_size_, read_queue_),
      batch_pusher_(batch_size_, write_queue_) {
  if (is_multi_thread_) {
    // Initialize waits for queues
    AddWaitBase(&read_queue_);
    AddWaitBase(&write_queue_);
  }
  // 64 MB for 256 imagery packets in the worst case
  raw_packet_.reserve(256 * 256 * 4 * bunch_size_);
  const int queue_size = is_multi_thread ? batch_size_ * 2 : 1;
  cache_deleter_.reserve(queue_size);
  // seed the read queue
  for (int i = 0; i < queue_size; ++i) {
    cache_ = new Cache();
    cache_->reserve(bunch_size_);
    if (is_multi_thread_) {
      read_queue_.Push(cache_);
    }
    cache_deleter_.push_back(TransferOwnership(cache_));
  }

  if (is_multi_thread_) {
    thread_deleter_.reserve(1);
    // one writer thread
    mttypes::ManagedThread* writer_thread = new mttypes::ManagedThread(
        *this,
        khFunctor<void>(std::mem_fun(&RequestBundlerForPacket::Write), this));
    thread_deleter_.push_back(TransferOwnership(writer_thread));
    thread_deleter_.back()->run();  // The writer is active and waiting now
    batch_puller_.ReleaseOldAndPull(next_);
    cache_ = next_->Data();
  }
}

RequestBundlerForPacket::~RequestBundlerForPacket() {
  if (!cache_->empty()) {
    FlushCache();
  }
  if (is_multi_thread_) {
    batch_pusher_.PushDone();  // This will signal the write queue to end
    // Wait for the write queue to return
    for (; batch_puller_.ReleaseOldAndPull(next_), next_;) {
    }
  }
}

void RequestBundlerForPacket::AddToCache(const WriteRequest& request) {
  cache_->push_back(request);
  if (cache_->size() == bunch_size_) {
    FlushCache();
  }
}

void RequestBundlerForPacket::WriteImagePacket(
    const std::string& qtpath, std::uint32_t version) {
  AddToCache(WriteRequest(version, kGEImageryChannelId, kImagePacket, qtpath));
}

void RequestBundlerForPacket::WriteTerrainPacket(
    const std::string& qtpath, std::uint32_t version) {
  // Mismatch between kGETerrainChannelId (2)
  // and Portable channel (1) :P
  // TODO: Rectify terrain channel ids.
  AddToCache(WriteRequest(
      version, kGETerrainChannelId - 1, kTerrainPacket, qtpath));
}

void RequestBundlerForPacket::WriteVectorPacket(
    const std::string& qtpath, std::uint32_t channel, std::uint32_t version) {
  AddToCache(WriteRequest(version, channel, kVectorPacket, qtpath));
}

void RequestBundlerForPacket::FlushCache() {
  ImageryBundle ib;
  TerrainBundle tb;
  VectorBundle  vb;
  // Separate the requests into imagery/terrain/vector bundles
  // Order (with interleaved types) is preserved in cache_
  // so after results are stored in request, they can be
  // written to packet bundle in the correct order.
  for (size_t i = 0; i < cache_->size(); ++i) {
    RequestBundle* req_bundle;
    // Write request from next position in cache.
    WriteRequest& request = (*cache_)[i];
    if (request.packet_type_ == kImagePacket) {
      req_bundle = &ib[request.version_];
    } else if (request.packet_type_ == kTerrainPacket) {
      req_bundle = &tb[request.version_];
    } else {
      assert(request.packet_type_ == kVectorPacket);
      req_bundle = &vb[std::make_pair(request.channel_, request.version_)];
    }
    std::string& paths = req_bundle->first;
    paths.append(request.qt_path_);
    paths.push_back(kQuadTreePathSeparator);
    std::vector<WriteRequest*>& requests = req_bundle->second;
    requests.push_back(&request);
  }
  // For each imagery version seen in this cache_ of requests,
  // make a bundle query and get results.
  for (ImageryBundle::iterator i = ib.begin(); i != ib.end(); ++i) {
    std::uint32_t version = i->first;
    RequestBundle& b = i->second;
    caller_->GetImagePacket(b.first, version, &raw_packet_);
    std::string* raw_packet = cache_->CopyRawPacketBuffer(raw_packet_);
    std::vector<std::pair<const char*, size_t> > vec(b.second.size());
    static_cast<PackedString*>(raw_packet)->Unpack(&vec);
    for (size_t i = 0; i < vec.size(); ++i) {
      b.second[i]->request_result_ = vec[i];
    }
  }
  // For each terrain version seen in this cache_ of requests,
  // make a bundle query and get results.
  for (TerrainBundle::iterator i = tb.begin(); i != tb.end(); ++i) {
    std::uint32_t version = i->first;
    RequestBundle& b = i->second;
    caller_->GetTerrainPacket(b.first, version, &raw_packet_);
    std::string* raw_packet = cache_->CopyRawPacketBuffer(raw_packet_);
    std::vector<std::pair<const char*, size_t> > vec(b.second.size());
    static_cast<PackedString*>(raw_packet)->Unpack(&vec);
    for (size_t i = 0; i < vec.size(); ++i) {
      b.second[i]->request_result_ = vec[i];
    }
  }
  // For each vector {channel, version} seen in this cache_ of requests,
  // make a bundle query and get results.
  for (VectorBundle::iterator i = vb.begin(); i != vb.end(); ++i) {
    std::uint32_t channel = i->first.first;
    std::uint32_t version = i->first.second;
    RequestBundle& b = i->second;
    caller_->GetVectorPacket(b.first, channel, version, &raw_packet_);
    std::string* raw_packet = cache_->CopyRawPacketBuffer(raw_packet_);
    std::vector<std::pair<const char*, size_t> > vec(b.second.size());
    static_cast<PackedString*>(raw_packet)->Unpack(&vec);
    for (size_t i = 0; i < vec.size(); ++i) {
      b.second[i]->request_result_ = vec[i];
    }
  }
  if (is_multi_thread_) {
    batch_pusher_.Push(cache_);
    // this will block until something else is available, but since write is
    // faster than read we don't expect blocking
    batch_puller_.ReleaseOldAndPull(next_);
    cache_ = next_->Data();
  } else {
    WriteCache(cache_);
  }
}

// Writes one bundle of bunch_size_ queries' result in proper sequence
void RequestBundlerForPacket::WriteCache(Cache* cache) {
  for (size_t i = 0; i < cache->size(); ++i) {
    const WriteRequest& request = (*cache)[i];
    writer_->AppendPacket(std::string("0") + request.qt_path_,
                          request.packet_type_, request.channel_,
                          request.request_result_.first,
                          request.request_result_.second);
  }
  cache->clear();
}

// This is the only function running in child thread in case is_multi_thread_
// is set.
void RequestBundlerForPacket::Write() {
  mttypes::BatchingQueuePuller<Cache*> batch_puller(batch_size_,
                                                    write_queue_);
  mttypes::BatchingQueuePusher<Cache*> batch_pusher(batch_size_, read_queue_);
  CacheQueue::PullHandle next;
  // this will block until something else is available
  for (; batch_puller.ReleaseOldAndPull(next), next;) {
    Cache* cache = next->Data();
    WriteCache(cache);
    batch_pusher.Push(cache);
  }
  batch_pusher.PushDone();
}

}  // namespace fusion_portableglobe
