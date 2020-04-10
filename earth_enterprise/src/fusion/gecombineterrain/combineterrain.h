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


#ifndef FUSION_GECOMBINETERRAIN_COMBINETERRAIN_H__
#define FUSION_GECOMBINETERRAIN_COMBINETERRAIN_H__

#include <cstdint>
#include <vector>
#include <common/base/macros.h>
#include <khMTProgressMeter.h>
#include <packetcompress.h>
#include <packetfile/packetfilereaderpool.h>
#include <packetfile/packetfilewriter.h>
#include <qtpacket/quadset_gather.h>

namespace geterrain {

// CountedPacketFileReaderPool - reader pool which maintains count of
// data packets in files added to pool.

class CountedPacketFileReaderPool : public PacketFileReaderPool {
 public:
  CountedPacketFileReaderPool(const std::string &pool_name,
                              geFilePool &file_pool)
      : PacketFileReaderPool(pool_name, file_pool),
        pool_packet_count_(0) {
  }
  ~CountedPacketFileReaderPool() {}
  virtual PacketFileReaderToken Add(const std::string &packet_file_name);
  inline const std::uint64_t &pool_packet_count() const { return pool_packet_count_; }
 private:
  std::uint64_t pool_packet_count_;
};

// TerrainPacketItem - contain basic information about a terrain
// packet during a Merge and QuadsetGather.  This item is always
// contained within a QuadsetItem.  Must allow copy/assign.

class TerrainPacketItem {
 public:
  TerrainPacketItem(const PacketFileReaderToken &token,
                    const std::uint64_t position,
                    const std::uint32_t record_size,
                    const std::uint32_t version,
                    const std::uint32_t providerId)
      : position_(position),
        token_(token),
        record_size_(record_size),
        version_(version),
        providerId_(providerId) {
  }
  TerrainPacketItem() {}
  inline const PacketFileReaderToken &token() const { return token_; }
  inline std::uint64_t position() const { return position_; }
  inline std::uint32_t record_size() const { return record_size_; }
  inline std::uint32_t version() const { return version_; }
  inline std::uint32_t providerId() const { return providerId_; }
 private:
  std::uint64_t position_;
  PacketFileReaderToken token_;
  std::uint32_t record_size_;
  std::uint32_t version_;
  std::uint32_t providerId_;
};

typedef qtpacket::QuadsetItem<TerrainPacketItem> TerrainQuadsetItem;
typedef qtpacket::QuadsetGroup<TerrainPacketItem> TerrainQuadsetGroup;

// TerrainCombiner - do 5-to-1 terrain packet combining.
// TODO: this might be a candidate for multi-threading of
// the terrain packet file reads and writes (in which case, make sure
// to give each thread its own buffers)

class TerrainCombiner {
 public:
  TerrainCombiner(CountedPacketFileReaderPool &packet_reader_pool,
                  const std::string &packet_file_name,
                  unsigned int num_cpus)
      : packet_reader_pool_(packet_reader_pool),
        writer_(packet_reader_pool_.file_pool(), packet_file_name),
        num_cpus_(num_cpus),
        canceled_(false), queues_finished_(false), writer_thread_(NULL),
        min_queue_size_(num_cpus * 50),
        max_queue_size_(num_cpus * 100),
        progress_meter_(packet_reader_pool.pool_packet_count(), "packets") {
    // We're doing single-threaded output of many small packets appended to a
    // single file. Buffer writes to improve performance.
    writer_.BufferWrites(kWriteBufferSize, kIndexWriteBufferSize);
  }
  ~TerrainCombiner();
  void CombineTerrainPackets(const TerrainQuadsetGroup &quadset_group);
  void Close(size_t max_sort_buffer = PacketIndexWriter::kDefaultMaxSortBuffer);

  // Start the worker threads: 1+ (num_cpus_ compressor threads
  // and 1 writer thread.
  void StartThreads();

  // Wait for the compressor/writer threads to finish.
  void WaitForThreadsToFinish();

 private:
  // The packets themselves are small, but larger than the index entries. Use a
  // larger buffer for the packet writes than index writes.
  static const size_t kWriteBufferSize = 10 * 1024 * 1024;
  static const size_t kIndexWriteBufferSize = 10 * 1024 * 1024;
  static const std::string kEmptyPacket; // zero fill for empty packet

  void CombineChildren(const TerrainQuadsetGroup &quadset_group,
                     const QuadtreePath even_path);
  const TerrainPacketItem *SelectPacketFromMaxCombiningLayer (
      const TerrainQuadsetGroup::NodeContents &items) const;
  void WriteCombinedTerrain(
      const QuadtreePath even_path,
      const std::vector<const TerrainPacketItem *> &items,
      unsigned int progress_increment);

  CountedPacketFileReaderPool &packet_reader_pool_;
  PacketFileWriter writer_;

  // Thread state --- The compression and writing process is broken
  // up into 3+ threads:
  // Main thread: reading the packet data into combined packets
  // 1 or more Compressor thread(s): compressing the packets
  // 1 Writer thread: writing the compressed packets

  // A Helper class to hold the packet information needed by the
  // Compressor and Writer threads.
  // It basically holds the raw packet and the compressed packet.
  class PacketInfo {
  public:
    PacketInfo(QuadtreePath even_path, std::uint32_t provider_id,
               unsigned int progress_increment)
    : even_path_(even_path), provider_id_(provider_id),
      is_ready_to_write_(false), progress_increment_(progress_increment) {}
    ~PacketInfo() {}

    std::string& RawBuffer() { return buffer_; }
    std::string& CompressedBuffer() { return compressed_buffer_; }
    bool IsReadyToWrite() const { return is_ready_to_write_; }
    void SetIsReadyToWrite(bool is_ready_to_write) {
      is_ready_to_write_ = is_ready_to_write;
    }
    QuadtreePath EvenPath() const { return even_path_; }
    std::uint32_t ProviderId() const { return provider_id_; }
    // Free up memory used by the uncompressed buffer.
    void FreeRawBuffer() { buffer_ = std::string(); }
    unsigned int ProgressIncrement() const { return progress_increment_; }
  private:
    QuadtreePath even_path_;
    std::uint32_t provider_id_;
    bool is_ready_to_write_;
    unsigned int progress_increment_;
    std::string buffer_;
    std::string compressed_buffer_;
  };

  // Methods used by the worker threads.
  // Compress the specified packet.
  void CompressPacket(PacketInfo* packet);

  // Write the specified packet to the output file bundle.
  void WritePacket(PacketInfo* packet);

  // Communicate to the worker threads that no more PacketInfo objects will
  // be added to the queues.
  void MarkQueuesComplete();

  // Communicate to the worker threads that the process is canceled.
  void CancelThreads();

  // Helper function to set the thread state variables and wake the threads.
  void SetProcessState(bool canceled, bool queues_finished);

  // Wake the compressor and writer threads.
  void WakeThreads();

  // Wake the compressor threads.
  void WakeCompressorThreads();
  // Wake the writer thread.
  void WakeWriteThread();
  // Wake the reader thread.
  void WakeReadThread();

  // Add the specified packet to the compressor and writer queues and wake
  // the compressor threads.
  void AddPacketToQueues(PacketInfo* packet);

  // Indicate that the specified packet is ready to be written, and if
  // appropriate, wake the writer thread.
  void MarkPacketReadyForWrite(PacketInfo* packet);

  // Get the frontmost packet in the compressor queue.
  // This will block and wait on a condition variable until either a packet
  // is available or we are done/canceled.
  // Return false if the process is canceled or no more packets are available.
  bool PopCompressQueue(PacketInfo** packet_out);

  // The main thread routine for the compressor threads.
  void PacketCompressThread();

  // Get the frontmost packet in the writer queue.
  // This will block and wait on a condition variable until either a packet
  // is available or we are done/canceled.
  // Return false if the process is canceled or no more packets are available.
  bool PopWriteQueue(PacketInfo** packet_out);

  // The main thread routine for the compressor threads.
  void PacketWriteThread();

  // Thread state
  khMutex mutex_; // One mutext to protect all the thread state.
  std::queue<PacketInfo*> compress_queue_;
  std::queue<PacketInfo*> write_queue_;
  unsigned int num_cpus_; // Useful for specifying number of compressor threads.
  bool canceled_;
  bool queues_finished_;

  khThread* writer_thread_;
  std::vector<khThread*> compressor_threads;
  khCondVar compressor_thread_event_; // Allows compressor threads to wait for
                                      // data to process.
  khCondVar write_thread_event_; // Allows writer thread to wait for data to
                                 // process.
  khCondVar read_thread_event_; // Allows reader thread to throttle the rate of
                                // adding packets into the compressor queue.
  unsigned int min_queue_size_;
  unsigned int max_queue_size_;

  // Progress
  khMTProgressMeter progress_meter_;

  DISALLOW_COPY_AND_ASSIGN(TerrainCombiner);
};

} // namespace geterrain

#endif  // FUSION_GECOMBINETERRAIN_COMBINETERRAIN_H__
