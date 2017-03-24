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


// CombineTerrainPackets
//
// The terrain packets delivered to the server appear only at
// even-level nodes of the quadtree path.  Each even-level terrain
// node also contains terrain for its odd-level children.  This
// routine takes as input a quadset group containing TerrainPacketItem
// objects for all of the terrain present in the quadset.  It produces
// (in the output packet file) merged terrain packets.

#include <third_party/rsa_md5/crc32.h>
#include <khAssert.h>
#include <qtpacket/quadtree_utils.h>
#include "combineterrain.h"

COMPILE_TIME_CHECK(qtpacket::QuadtreeNumbering::kDefaultDepth == 5,
                   Quadset_packet_depth_is_not_5);

namespace geterrain {

// CountedPacketFileReaderPool - reader pool which maintains count of
// data packets in files added to pool.

// Add a packet file to the reader pool.  The packet file is opened,
// and a token is returned.  The number of packets is added to the total.
PacketFileReaderToken CountedPacketFileReaderPool::Add(
    const std::string &packet_file_name) {
  pool_packet_count_ += PacketIndexReader::NumPackets(
      PacketFile::IndexFilename(packet_file_name));
  return PacketFileReaderPool::Add(packet_file_name);
}

// CombineTerrainPackets

void TerrainCombiner::CombineTerrainPackets(
    const TerrainQuadsetGroup &quadset_group) {
  // The (phantom) root node of the quadset is always at an odd-level
  // (except the root quadset, which never has any terrain).  To
  // access the even-level nodes, we iterate through the even-level
  // children of the quadset root, and the grand-children of those
  // nodes.
  QuadtreePath quadset_root = quadset_group.qt_root();
  if (quadset_root == QuadtreePath()) {
    // Should be no terrain at root
    for (size_t i = 0; i < quadset_group.size(); ++i) {
      assert(quadset_group.Node(i).empty());
    }
    return;
  } else {
    assert(quadset_root.Level() & 1);   // level must be odd
  }

  for (uint32 root_child = 0;
       root_child < QuadtreePath::kChildCount;
       ++root_child) {
    QuadtreePath even_child = quadset_root.Child(root_child);

    // Combine even-level child with odd-level children
    CombineChildren(quadset_group, even_child);

    // Combine even-level grand-children of this child
    for (uint32 i = 0; i < QuadtreePath::kChildCount; ++i) {
      QuadtreePath odd_path = even_child.Child(i);
      for (uint32 j = 0; j < QuadtreePath::kChildCount; ++j) {
        CombineChildren(quadset_group, odd_path.Child(j));
      }
    }
  }
}

// CombineChildren - given a path to an even-level node, find the four
// children.  If any terrain is present in any of the nodes, write a
// combined terrain packet.

void TerrainCombiner::CombineChildren(const TerrainQuadsetGroup &quadset_group,
                                      const QuadtreePath even_path) {
  uint64 quadset_num;
  int even_subindex;
  qtpacket::QuadtreeNumbering::TraversalPathToQuadsetAndSubindex(
      even_path,
      &quadset_num,
      &even_subindex);
  assert(quadset_num == quadset_group.quadset_num());

  // Get items (if any) for even-level node and its children. items[0]
  // is even-level node, followed by odd-level entries.
  std::vector<const TerrainPacketItem *> items(1+QuadtreePath::kChildCount);
  size_t inorder =
    qtpacket::QuadtreeNumbering::QuadsetAndSubindexToInorder(quadset_num,
                                                             even_subindex);
  items[0] = SelectPacketFromMaxCombiningLayer(quadset_group.Node(inorder));
  bool has_any_terrain = items[0] != NULL;

  // Pass the progress count through to the writer.
  uint progress_increment = quadset_group.Node(inorder).size();

  for (uint32 i = 0; i < QuadtreePath::kChildCount; ++i) {
    uint64 odd_quadset_num;
    int odd_subindex;
    qtpacket::QuadtreeNumbering::TraversalPathToQuadsetAndSubindex(
        even_path.Child(i),
        &odd_quadset_num,
        &odd_subindex);
    assert(odd_quadset_num == quadset_num);

    inorder = qtpacket::QuadtreeNumbering::QuadsetAndSubindexToInorder(
        quadset_num,
        odd_subindex);
    progress_increment += quadset_group.Node(inorder).size();
    items[1+i] = SelectPacketFromMaxCombiningLayer(quadset_group.Node(inorder));
    has_any_terrain = has_any_terrain  ||  items[1+i] != NULL;
  }

  if (has_any_terrain) {
    WriteCombinedTerrain(even_path, items, progress_increment);
  } else {
    progress_meter_.incrementDone(progress_increment);
  }
}

// Given a vector of TerrainQuadsetItem objects,
// return a pointer to the one with the highest layer (terrain resource) in
// a terrain project (=~ highest PacketFileIndex) (NULL if vector is empty).
//
// Note: Terrain tile packet for a layer combines terrain tiles for all lower
// layers (having coverage at the packet) based on alpha mask. So the terrain
// packet for a lower layer doesn't have complete details as it doesn't take-in
// a terrain packet for any upper layer. Thus the top-most layer is the best.
// Due to the way configuration files are generated and read, the upper layers
// have higher PacketFileIndex.
//
// The stackfile generator RasterGEIndexAssetImplD in RasterGEIndex.src needs to
// take care that the top most resource in project has bottom-most place in
// stack file.
const TerrainPacketItem *TerrainCombiner::SelectPacketFromMaxCombiningLayer(
    const TerrainQuadsetGroup::NodeContents &items) const {
  if (items.empty())
    return NULL;

  TerrainQuadsetGroup::NodeContents::const_iterator cur = items.begin();
  TerrainQuadsetGroup::NodeContents::const_iterator max_item = items.begin();

  while (++cur != items.end()) {
    if (cur->token().PacketFileIndex() > max_item->token().PacketFileIndex()) {
      max_item = cur;
    }
  }
  return &(*max_item);
}

TerrainCombiner::~TerrainCombiner() {
  notify(NFY_DEBUG, "Deleting TerrainCombiner");
}

void TerrainCombiner::Close(size_t max_sort_buffer) {
  notify(NFY_DEBUG, "Closing TerrainCombiner");
  writer_.Close(max_sort_buffer);
}

// WriteCombinedTerrain - write a combined terrain packet for the
// specified even-level packet and its children.  Reads the source
// data from the packets, concatenates the sources (inserting 16 bytes
// of 0 for a missing tile).  The resultant packet is compressed and
// written to the output file.
// As of 3.1.1 (2008-09-29) the work of WriteCombinedTerrain is done in
// 3+ threads, 1 reader (main thread), 1+ compressor threads, 1 writer thread.
// The reader fills a PacketInfo object with a raw buffer of the uncompressed
// packet data and pushes it into a queue.

const std::string TerrainCombiner::kEmptyPacket(16, 0);

void TerrainCombiner::WriteCombinedTerrain(
    const QuadtreePath even_path,
    const std::vector<const TerrainPacketItem *> &items,
    uint progress_increment) {
  // Determine size of read buffer and allocate space if necessary.
  // Sizes include CRC, where applicable.
  // figure out our provider id while we're at it
  uint32 providerid = 0;
  size_t read_buffer_size = 0;
  std::vector<const TerrainPacketItem *>::const_iterator item = items.begin();
  for (; item != items.end(); ++item) {
    if (*item != NULL) {
      read_buffer_size += (*item)->record_size();
      if (providerid==0) {
        providerid = (*item)->providerId();
      }
    } else {
      read_buffer_size += kEmptyPacket.size();
    }
  }

  // Create the new PacketInfo and initialize it's raw buffer.
  PacketInfo* packet = new PacketInfo(even_path, providerid, progress_increment);
  std::string& read_buffer = packet->RawBuffer();
  read_buffer.resize(read_buffer_size);

  // Read or fill each packet in the read_buffer.
  size_t read_buffer_next = 0;
  for (item = items.begin(); item != items.end(); ++item) {
    if (*item != NULL) {
      read_buffer_next +=
        packet_reader_pool_.Reader((*item)->token()).ReadAtCRC(
            (*item)->position(), read_buffer, read_buffer_next,
            (*item)->record_size());
    } else {                            // missing packet
      read_buffer.replace(read_buffer_next, kEmptyPacket.size(), kEmptyPacket);
      read_buffer_next += kEmptyPacket.size();
    }
  }
  // The buffer in general shrinks because of removal of CRC bytes.
  read_buffer.resize(read_buffer_next);

  // We add the packet to both the compress queue and the write queue for
  // processing by other threads that do compression and writing.
  AddPacketToQueues(packet);
}

void TerrainCombiner::CompressPacket(PacketInfo* packet) {
  std::string& buffer = packet->RawBuffer();
  std::string& compressed_buffer = packet->CompressedBuffer();

    // Compress data
  size_t compress_buffer_size = KhPktGetCompressBufferSize(buffer.size())
                                + kCRC32Size;
  compressed_buffer.resize(compress_buffer_size);
  if (!KhPktCompressWithBuffer(buffer.data(),
                               buffer.size(),
                               &compressed_buffer[0],
                               &compress_buffer_size)) {
    throw khSimpleException("TerrainCombiner::CompressPacket: "
                            "KhPktCompressWithBuffer failed");
  }

  // Make room for the CRC checksum, and adjust final size of the
  // compressed buffer to match the actual size.
  compressed_buffer.resize(compress_buffer_size + kCRC32Size);
}

void TerrainCombiner::WritePacket(PacketInfo* packet) {
  std::string& compressed_buffer = packet->CompressedBuffer();
  writer_.WriteAppendCRC(packet->EvenPath(), &compressed_buffer[0],
                         compressed_buffer.size(), packet->ProviderId());
  progress_meter_.incrementDone(packet->ProgressIncrement());
}


// Threaded processing of the Compression and writing.
// ----------------------------------------------------------------------------
void TerrainCombiner::StartThreads() {
  writer_thread_ = new khThread(
    khFunctor<void>(std::mem_fun(&TerrainCombiner::PacketWriteThread), this));
  writer_thread_->run();

  // Use one less cpu than num cpus for doing the compression work.
  uint compress_cpus = num_cpus_;
  if (num_cpus_ >= 4) {
    compress_cpus--; // If we have more than 4 cpus save one for reads/writes.
  }
  for(uint i = 0; i < compress_cpus; ++i) {
    khThread* compressor_thread = new khThread(
      khFunctor<void>(std::mem_fun(&TerrainCombiner::PacketCompressThread), this));

    compressor_threads.push_back(compressor_thread);
    compressor_thread->run();
  }
}

void TerrainCombiner::WaitForThreadsToFinish() {
  if (writer_thread_) {
    // We're done creating PacketInfo's for the threads.
    MarkQueuesComplete();
    // Only care to join the writer thread...that will indicate the
    // packets are written.
    writer_thread_->join();

    // Clean up the threads.
    delete writer_thread_;
    writer_thread_ = NULL;
  }

  // Just delete the compressor threads...this will force them to close
  // if not already.
  for(uint i = 0; i < compressor_threads.size(); ++i) {
    if (compressor_threads[i]) {
      delete compressor_threads[i];
    }
  }
  compressor_threads.clear();
}


void TerrainCombiner::MarkQueuesComplete() {
  SetProcessState(false /* canceled */, true /* queues_finished */);
}

void TerrainCombiner::CancelThreads() {
  SetProcessState(true /* canceled */, false /* queues_finished */);
}

void TerrainCombiner::SetProcessState(bool canceled, bool queues_finished) {
  {
    khLockGuard lock(mutex_);
    canceled_ = canceled;
    queues_finished_ = queues_finished;
  }
  WakeThreads();
}

void TerrainCombiner::WakeThreads() {
  WakeCompressorThreads();
  WakeWriteThread();
}

void TerrainCombiner::WakeCompressorThreads() {
  compressor_thread_event_.broadcast_all();
}

void TerrainCombiner::WakeWriteThread() {
  write_thread_event_.signal_one();
}

void TerrainCombiner::WakeReadThread() {
  read_thread_event_.signal_one();
}

void TerrainCombiner::AddPacketToQueues(PacketInfo* packet) {
  bool wake_compressors = false;
  bool sleep_until_woken = false;
  {
    // We add to the write queue at the same time to guarantee that
    // the writes are executed in order.
    khLockGuard lock(mutex_);
    wake_compressors = compress_queue_.empty();
    compress_queue_.push(packet);
    write_queue_.push(packet);
    sleep_until_woken = write_queue_.size() == max_queue_size_;
  }
  if (wake_compressors) {
    WakeCompressorThreads();
    // The compressor threads will wake the writer thread when there are
    // things to write.
  }
  // If our packet count is too high we need to put the reader to sleep
  // temporarily so that we don't overflow our buffers.
  if (sleep_until_woken) {
    khLockGuard lock(mutex_);
    // Just check that the queue isn't drained already...not likely.
    if (write_queue_.size() >= min_queue_size_) {
      read_thread_event_.wait(mutex_);
    }
  }
}

void TerrainCombiner::MarkPacketReadyForWrite(PacketInfo* packet) {
  bool wake_write_thread = false;
  {
    khLockGuard lock(mutex_);
    packet->SetIsReadyToWrite(true);
    packet->FreeRawBuffer(); // Free up memory...we don't need the raw buffer.
    // We only need to wake the write thread when we the top
    // of the queue is marked ready.
    if (write_queue_.front() && write_queue_.front()->IsReadyToWrite()) {
      wake_write_thread = true;
    }
  }
  if (wake_write_thread) {
    WakeWriteThread();
  }
}

bool TerrainCombiner::PopCompressQueue(PacketInfo** packet_out) {
  *packet_out = NULL;
  bool wake_reader = false;
  {
    khLockGuard lock(mutex_);
    while(*packet_out == NULL) {
      if (canceled_) {
        return false; // Process is cancelled, no need to proceed.
      }

      // Peek at the top packet to see if it's ready to write to disk.
      if (compress_queue_.empty()) {
        if (queues_finished_) {
          return false; // No more packets coming...this indicates we're done.
        }
      } else {
        *packet_out = compress_queue_.front();
        compress_queue_.pop();
        assert(*packet_out); // Better be non-null.
        // When the queue size drops below threshold, wake the reader.
        wake_reader = write_queue_.size() < min_queue_size_;
        break;
      }
      // Nothing now, but supposedly more coming.
      // Let's wait for a signal that the queue has been populated.
      // Must wait until we're given a signal.
      compressor_thread_event_.wait(mutex_);
    }
  }
  if (wake_reader) {
    WakeReadThread();
  }

  return true;
}

void TerrainCombiner::PacketCompressThread() {
  std::string fail_message;
  unsigned long packets_processed = 0;
  try {
    PacketInfo* packet = NULL;
    while(PopCompressQueue(&packet)) {
      if (packet != NULL) {
        // Compress the data in the packet.
        CompressPacket(packet);

        // Mark it as ready for the Write thread (and wake the write thread
        // if necessary).
        MarkPacketReadyForWrite(packet);
        packets_processed++;
      }
    }
  } catch(const khSimpleException &e) {
    fail_message = std::string("PacketCompressThread khSimpleException: ") +
      std::string(e.what());
  } catch(const std::exception &e) {
    fail_message = std::string("PacketCompressThread std::exception: ") +
      std::string(e.what());
  } catch(...) {
    fail_message = "PacketCompressThread unknown exception";
  }
  if (!fail_message.empty()) {
    notify(NFY_DEBUG, "%s", fail_message.c_str());
    notify(NFY_DEBUG, "Compress Stats: compress queue size = %d, "
           "write queue size = %d",
           static_cast<int>(compress_queue_.size()),
           static_cast<int>(write_queue_.size()));
  }
  notify(NFY_DEBUG, "Compressor Thread processed %lu packets",
         packets_processed);
}

bool TerrainCombiner::PopWriteQueue(PacketInfo** packet_out) {
  *packet_out = NULL;
  bool wake_reader = false;
  {
    khLockGuard lock(mutex_);
    while(*packet_out == NULL) {
      if (canceled_) {
        return false; // Process is cancelled, no need to proceed.
      }

      // Peek at the top packet to see if it's ready to write to disk.
      if (write_queue_.empty()) {
        if (queues_finished_) {
          return false; // No more packets coming...this indicates we're done.
        }
      } else {
        // Peek at the top packet to see if it's ready to write to disk.
        PacketInfo* packet = write_queue_.front();
        if (packet && packet->IsReadyToWrite()) {
          *packet_out = packet;
          write_queue_.pop();
          // When the queue size drops below threshold, wake the reader.
          wake_reader = write_queue_.size() < min_queue_size_;
          break;
        }
      }
      // Nothing now, but supposedly more coming.
      // Let's wait for a signal that the queue has been populated.
      // Must wait until we're given a signal that something is ready for writing.
      write_thread_event_.wait(mutex_);
    }
  }
  if (wake_reader) {
    WakeReadThread();
  }

  return true;
}

void TerrainCombiner::PacketWriteThread() {
  std::string fail_message;
  unsigned long packets_processed = 0;
  try {
    PacketInfo* packet = NULL;
    while(PopWriteQueue(&packet)) {
      if (packet != NULL) {
        WritePacket(packet);
        delete packet;
        packets_processed++;
      }
    }
  } catch(const khSimpleException &e) {
    fail_message = std::string("PacketWriteThread khSimpleException: ") +
    std::string(e.what());
  } catch(const std::exception &e) {
    fail_message = std::string("PacketWriteThread std::exception: ") +
    std::string(e.what());
  } catch(...) {
    fail_message = "PacketWriteThread unknown exception";
  }
  if (!fail_message.empty()) {
    notify(NFY_DEBUG, "%s", fail_message.c_str());
    notify(NFY_DEBUG, "Compress Stats: compress queue size = %d, "
           "write queue size = %d",
           static_cast<int>(compress_queue_.size()),
           static_cast<int>(write_queue_.size()));
  }
  notify(NFY_DEBUG, "Write Thread processed %lu packets",
         packets_processed);
}

} // namespace geterrain
