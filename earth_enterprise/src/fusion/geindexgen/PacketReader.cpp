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


#include "PacketReader.h"

#include <packetfile/packetfile.h>
#include <packetfile/packetindex.h>
#include <khFileUtils.h>
#include <khException.h>


namespace geindexgen {


template <class IndexEntry>
PacketReader<IndexEntry>::PacketReader(
    geFilePool &file_pool, const std::string &packetfile,
    uint32 file_number, uint32 version, uint32 channel, Operation op) :
    packet_filename_(packetfile),
    reader_(TransferOwnership(
                new PacketIndexReader(
                    file_pool,
                    khComposePath(packet_filename_, PacketFile::kIndexBase)))),
    file_number_(file_number),
    version_(version),
    channel_(channel),
    op_(op)
{
}

// defined in .cpp so .h doesn't need complete type for PacketIndexReader
template <class IndexEntry>
PacketReader<IndexEntry>::~PacketReader(void)
{
}


template <class IndexEntry>
uint32 PacketReader<IndexEntry>::ReadNextN(MergeEntry *entries, uint32 count,
                                           LittleEndianReadBuffer &buffer) {
  PacketIndexEntry packet_entries[count];
  uint32 num_read = reader_->ReadNextN(packet_entries, count, buffer);
  if (num_read == 0) {
    throw khException("Internal Error: PacketReader::ReadNextN failed");
  }

  for (uint i = 0; i < num_read; ++i) {
    AssignMergeEntry(entries[i], packet_entries[i]);
  }

  return num_read;
}


template <class IndexEntry>
uint64 PacketReader<IndexEntry>::NumPackets(void) const {
  return reader_->NumPackets();
}


template <class IndexEntry>
void PacketReader<IndexEntry>::Close(void) {
  reader_.clear();
}


// ****************************************************************************
// ***  BlendEntry
// ****************************************************************************
void BlendPacketReader::AssignMergeEntry(
    MergeEntry &merge_entry, const PacketIndexEntry &packet_entry) {
  merge_entry.path_ = packet_entry.qt_path();
  merge_entry.entry_ = geindex::BlendEntry(
      geindex::ExternalDataAddress(packet_entry.position(),
                                   file_number_,
                                   packet_entry.record_size()),
      version_,
      packet_entry.extra());
  merge_entry.op_ = op_;
}


// ****************************************************************************
// ***  VectorEntry
// ****************************************************************************
void VectorPacketReader::AssignMergeEntry(
    MergeEntry &merge_entry, const PacketIndexEntry &packet_entry) {
  merge_entry.path_ = packet_entry.qt_path();
  merge_entry.entry_ = geindex::VectorEntry(
      geindex::ExternalDataAddress(packet_entry.position(),
                                   file_number_,
                                   packet_entry.record_size()),
      version_, channel_);
  merge_entry.op_ = op_;
}



// ****************************************************************************
// ***  Explicit instantiation
// ****************************************************************************
template class PacketReader<geindex::BlendEntry>;
template class PacketReader<geindex::VectorEntry>;



} // namespace geindexgen

