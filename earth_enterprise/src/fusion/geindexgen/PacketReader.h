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


#ifndef FUSION_GEINDEXGEN_PACKETREADER_H__
#define FUSION_GEINDEXGEN_PACKETREADER_H__

#include <quadtreepath.h>
#include <khGuard.h>
#include <geindex/Entries.h>

class LittleEndianReadBuffer;
class PacketIndexReader;
class PacketIndexEntry;

namespace geindexgen {

template <class IndexEntry>
class PacketReader {
 public:
  // delete is "less than" put so delete operations will happen before puts
  enum Operation { DeleteOp = 0, PutOp = 1};

  class MergeEntry {
   public:
    QuadtreePath path_;
    IndexEntry   entry_;
    Operation    op_;
    inline bool operator>(const MergeEntry &o) const {
      return (path_ == o.path_) ? (op_ > o.op_) : (path_ > o.path_);
    }
  };

  inline std::string Name(void) const  { return packet_filename_; }
  std::uint64_t NumPackets(void) const;
  void Close(void);
  std::uint32_t ReadNextN(MergeEntry *entries, std::uint32_t count,
                   LittleEndianReadBuffer &buffer);
  PacketReader(geFilePool &file_pool, const std::string &packetfile,
               std::uint32_t file_number, std::uint32_t version, std::uint32_t channel,
               Operation op);
  virtual ~PacketReader(void);

 protected:
  virtual void AssignMergeEntry(MergeEntry &merge_entry,
                                const PacketIndexEntry &packet_entry) = 0;

  std::string packet_filename_;
  khDeleteGuard<PacketIndexReader> reader_;
  std::uint32_t      file_number_;
  std::uint32_t      version_;
  std::uint32_t      channel_;
  Operation   op_;

  DISALLOW_COPY_AND_ASSIGN(PacketReader);
};


class BlendPacketReader : public PacketReader<geindex::BlendEntry> {
 public:
  BlendPacketReader(geFilePool &file_pool, const std::string &packetfile,
                    std::uint32_t file_number, std::uint32_t version,
                    Operation op)
      : PacketReader<geindex::BlendEntry>(file_pool, packetfile, file_number,
                                           version, 0 /* unused channel id */,
                                           op)
  { }

 protected:
  virtual void AssignMergeEntry(MergeEntry &merge_entry,
                                const PacketIndexEntry &packet_entry);
};


class VectorPacketReader : public PacketReader<geindex::VectorEntry> {
 public:
  VectorPacketReader(geFilePool &file_pool, const std::string &packetfile,
                     std::uint32_t file_number, std::uint32_t version, std::uint32_t channel,
                     Operation op)
      : PacketReader<geindex::VectorEntry>(file_pool, packetfile, file_number,
                                           version, channel, op)
  { }

 protected:
  virtual void AssignMergeEntry(MergeEntry &merge_entry,
                                const PacketIndexEntry &packet_entry);
};


} // namespace geindexgen

#endif // FUSION_GEINDEXGEN_PACKETREADER_H__
