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


// CombineTerrainPackets
//
// The terrain packets delivered to the server appear only at
// even-level nodes of the quadtree path.  Each even-level terrain
// node also contains terrain for its odd-level children.  This
// routine takes as input a quadset group containing TerrainPacketItem
// objects for all of the terrain present in the quadset.  It produces
// (in the output packet file) merged terrain packets.

#include <third_party/rsa_md5/crc32.h>
#include <qtpacket/quadtree_utils.h>
#include "combineterrain.h"
#include "common/performancelogger.h"

static_assert(qtpacket::QuadtreeNumbering::kDefaultDepth == 5,
                   "Quadset packet depth is not 5");

namespace geterrain {

// CountedPacketFileReaderPool - reader pool which maintains count of
// data packets in files added to pool.

// Add a packet file to the reader pool.  The packet file is opened,
// and a token is returned.  The number of packets is added to the total.
PacketFileReaderToken CountedPacketFileReaderPool::Add(
    const std::string &packet_file_name) {
  BEGIN_PERF_LOGGING(openPacketFile,"CombineTerrain_OpenPacket","Open/add packet file to reader pool");
  pool_packet_count_ += PacketIndexReader::NumPackets(
      PacketFile::IndexFilename(packet_file_name));
  PacketFileReaderToken retval = PacketFileReaderPool::Add(packet_file_name);
  END_PERF_LOGGING(openPacketFile);
  return retval;
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
  BEGIN_PERF_LOGGING(perfLog, "CombineTerrain_CombinePackets", quadset_root.AsString());
  if (quadset_root == QuadtreePath()) {
    // Should be no terrain at root
    for (size_t i = 0; i < quadset_group.size(); ++i) {
      assert(quadset_group.Node(i).empty());
    }
    return;
  } else {
    assert(quadset_root.Level() & 1);   // level must be odd
  }

  for (std::uint32_t root_child = 0;
       root_child < QuadtreePath::kChildCount;
       ++root_child) {
    QuadtreePath even_child = quadset_root.Child(root_child);

    // Combine even-level child with odd-level children
    CombineChildren(quadset_group, even_child);

    // Combine even-level grand-children of this child
    for (std::uint32_t i = 0; i < QuadtreePath::kChildCount; ++i) {
      QuadtreePath odd_path = even_child.Child(i);
      for (std::uint32_t j = 0; j < QuadtreePath::kChildCount; ++j) {
        CombineChildren(quadset_group, odd_path.Child(j));
      }
    }
  }
  END_PERF_LOGGING(perfLog);
}

// CombineChildren - given a path to an even-level node, find the four
// children.  If any terrain is present in any of the nodes, write a
// combined terrain packet.

void TerrainCombiner::CombineChildren(const TerrainQuadsetGroup &quadset_group,
                                      const QuadtreePath even_path) {
  BEGIN_PERF_LOGGING(combineChildrenProf, "CombineTerrain_CombineChildren", even_path.AsString());
  std::uint64_t quadset_num;
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
  unsigned int progress_increment = quadset_group.Node(inorder).size();

  for (std::uint32_t i = 0; i < QuadtreePath::kChildCount; ++i) {
    std::uint64_t odd_quadset_num;
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

  END_PERF_LOGGING(combineChildrenProf);
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
  BEGIN_PERF_LOGGING(closeTC,"CombineTerrain_Close","Flush and Close TerrainCombiner file bundle");
  writer_.Close(max_sort_buffer);
  END_PERF_LOGGING(closeTC);
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
    unsigned int progress_increment) {
  BEGIN_PERF_LOGGING(calcReadSize, "CombineTerrain_CalcPacketReadSize", even_path.AsString());
  // Determine size of read buffer and allocate space if necessary.
  // Sizes include CRC, where applicable.
  // figure out our provider id while we're at it
  std::uint32_t providerid = 0;
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

  END_PERF_LOGGING(calcReadSize);
  BEGIN_PERF_LOGGING(readPackets, "CombineTerrain_ReadPacket", even_path.AsString(), read_buffer_size);
  
  // Create the new PacketInfo and initialize its raw buffer.
  PacketInfo* packet = new PacketInfo(even_path, providerid, progress_increment);
  // Start profiling here because the variable "packet" needs to be defined
  // outside the profiling block.
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

  END_PERF_LOGGING(readPackets);

  // We add the packet to both the compress queue and the write queue for
  // processing by other threads that do compression and writing.
  AddPacketToQueues(packet);
}

void TerrainCombiner::CompressPacket(PacketInfo* packet) {
  std::string& buffer = packet->RawBuffer();
  std::string& compressed_buffer = packet->CompressedBuffer();

  BEGIN_PERF_LOGGING(perfLog, "CombineTerrain_CompressPacket", packet->EvenPath().AsString(), buffer.size());

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
  END_PERF_LOGGING(perfLog);
}

void TerrainCombiner::WritePacket(PacketInfo* packet) {
  std::string& compressed_buffer = packet->CompressedBuffer();
  BEGIN_PERF_LOGGING(perfLog, "CombineTerrain_WritePacket", packet->EvenPath().AsString(),
                     compressed_buffer.size());
  writer_.WriteAppendCRC(packet->EvenPath(), &compressed_buffer[0],
                         compressed_buffer.size(), packet->ProviderId());
  progress_meter_.incrementDone(packet->ProgressIncrement());
  END_PERF_LOGGING(perfLog);
}


// Threaded processing of the Compression and writing.
// ----------------------------------------------------------------------------
void TerrainCombiner::StartThreads() {
  writer_thread_ = new khThread(
    khFunctor<void>(std::mem_fun(&TerrainCombiner::PacketWriteThread), this));
  writer_thread_->run();

  notify(NFY_WARN, "combineterrain num_cpus_: %llu ",
               static_cast<long long unsigned int>(num_cpus_));
    
  // Use one less cpu than num cpus for doing the compression work.
  unsigned int compress_cpus = num_cpus_;
  PERF_CONF_LOGGING( "proc_exec_config_internal_numcpus", "combine_terrain_compress", num_cpus_ );
    
  PERF_CONF_LOGGING( "proc_exec_compress_internal_numcpus", "combine_terrain_compress", compress_cpus );
  notify(NFY_WARN, "gecombineterrain compress_cpus: %llu ",
               static_cast<long long unsigned int>(compress_cpus));
    
  for(unsigned int i = 0; i < compress_cpus; ++i) {
    khThread* compressor_thread = new khThread(
      khFunctor<void>(std::mem_fun(&TerrainCombiner::PacketCompressThread), this));

    compressor_threads.push_back(compressor_thread);
    compressor_thread->run();
  }
}

void TerrainCombiner::WaitForThreadsToFinish() {
  BEGIN_PERF_LOGGING(perfLog, "CombineTerrain_WaitForFinish", "AllThreads");
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
  for(unsigned int i = 0; i < compressor_threads.size(); ++i) {
    if (compressor_threads[i]) {
      delete compressor_threads[i];
    }
  }
  compressor_threads.clear();
  END_PERF_LOGGING(perfLog);
}


void TerrainCombiner::MarkQueuesComplete() {
  SetProcessState(false /* canceled */, true /* queues_finished */);
}

void TerrainCombiner::CancelThreads() {
  SetProcessState(true /* canceled */, false /* queues_finished */);
}

void TerrainCombiner::SetProcessState(bool canceled, bool queues_finished) {
  BEGIN_PERF_LOGGING(perfLog, "CombineTerrain_Set", "ProcessState");
  {
    khLockGuard lock(mutex_);
    canceled_ = canceled;
    queues_finished_ = queues_finished;
  }
  WakeThreads();
  END_PERF_LOGGING(perfLog);
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
    BEGIN_PERF_LOGGING(perfLog, "CombineTerrain_WaitFor", "QueueMutex");
    khLockGuard lock(mutex_);
    wake_compressors = compress_queue_.empty();
    compress_queue_.push(packet);
    write_queue_.push(packet);
    sleep_until_woken = write_queue_.size() == max_queue_size_;
    END_PERF_LOGGING(perfLog);
  }
  BEGIN_PERF_LOGGING(perfLog, "CombineTerrain_Sleep", "ReadThread");
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
  END_PERF_LOGGING(perfLog);
}

void TerrainCombiner::MarkPacketReadyForWrite(PacketInfo* packet) {
  BEGIN_PERF_LOGGING(perfLog, "CombineTerrain_MarkForWrite", packet->EvenPath().AsString());
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
  END_PERF_LOGGING(perfLog);
}

bool TerrainCombiner::PopCompressQueue(PacketInfo** packet_out) {
  BEGIN_PERF_LOGGING(perfLog, "CombineTerrain_CompressQueue_Pop", "CompressQueue");
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

  END_PERF_LOGGING(perfLog);
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
  BEGIN_PERF_LOGGING(perfLog, "CombineTerrain_WriteQueue_Pop", "WriteQueue");
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

  END_PERF_LOGGING(perfLog);
  return true;
}

void TerrainCombiner::PacketWriteThread() {
  std::string fail_message;
  unsigned long packets_processed = 0;
  try {
    PacketInfo* packet = NULL;
    while(PopWriteQueue(&packet)) {
      if (packet != NULL) {
        BEGIN_PERF_LOGGING(pwThread,"CombineTerrain_PacketWrite","Packet write thread");
        WritePacket(packet);
        delete packet;
        packets_processed++;
        END_PERF_LOGGING(pwThread);
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
