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


#include "Todo.h"
#include <khException.h>
#include <khFileUtils.h>
#include <packetfile/packetfile.h>
#include <packetfile/packetindex.h>

namespace geindexgen {


// ****************************************************************************
// ***  BlendTodo
// ****************************************************************************

// Build a Todo list for a new index (no delta)
BlendTodo::BlendTodo(geindex::BlendWriter &writer, geFilePool &file_pool,
                     const BlendStack &stack) :
    total_tile_visits_(0)
{
  fprintf(stdout, "Calculating work to be done ... ");
  fflush(stdout);
  for (unsigned int i = 0; i < stack.insets_.size(); ++i) {
    for (unsigned int j = 0; j < stack.insets_[i].levels_.size(); ++j) {
      const std::string &packet_filename =
        stack.insets_[i].levels_[j].packetfile_;
      std::uint64_t packet_count = PacketIndexReader::NumPackets(
          PacketFile::IndexFilename(packet_filename));

      // degenerate case w/ no packets. Just skip it.
      if (packet_count == 0) continue;

      std::uint32_t file_num = writer.AddExternalPacketFile(packet_filename);

      // extra defaults to 0. We add 1 so our stack order is distinct from
      // the default. It probably doesn't matter anyway, but it keeps things
      // clean.
      writer.SetPacketExtra(file_num, i+1);

      readers_.push_back(
          TransferOwnership(
              new BlendPacketReader(file_pool, packet_filename, file_num,
                                    stack.insets_[i].levels_[j].index_version_,
                                    BlendPacketReader::PutOp)));
      total_tile_visits_ += packet_count;
    }
  }
  fprintf(stdout, "Finished.\n");
  fflush(stdout);
}


// ****************************************************************************
// ***  VectorTodo
// ****************************************************************************

// Build a Todo list for a new index (no delta)
VectorTodo::VectorTodo(geindex::VectorWriter &writer, geFilePool &file_pool,
                       const VectorStack &stack) :
    total_tile_visits_(0)
{
  fprintf(stdout, "Calculating work to be done ... ");
  fflush(stdout);
  for (unsigned int i = 0; i < stack.layers_.size(); ++i) {
    const std::string &packet_filename = stack.layers_[i].packetfile_;
    std::uint64_t packet_count = PacketIndexReader::NumPackets(
        PacketFile::IndexFilename(packet_filename));

    // degenerate case w/ no packets. Just skip it.
    if (packet_count == 0) continue;

    std::uint32_t file_num = writer.AddExternalPacketFile(packet_filename);
    readers_.push_back(
        TransferOwnership(
            new VectorPacketReader(file_pool, packet_filename, file_num,
                                   stack.layers_[i].index_version_,
                                   stack.layers_[i].channel_id_,
                                   VectorPacketReader::PutOp)));
    total_tile_visits_ += packet_count;
  }
  fprintf(stdout, "Finished.\n");
  fflush(stdout);
}




} // namespace geindexgen
