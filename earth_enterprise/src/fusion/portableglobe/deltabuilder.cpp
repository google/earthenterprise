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


// Classes for creating the files for a delta glb given the files for the
// original glb and the latest glb.

#include "fusion/portableglobe/deltabuilder.h"
#include <fstream>  // NOLINT(readability/streams)
#include <iostream>  // NOLINT(readability/streams)
#include <sstream>  // NOLINT(readability/streams)
#include <string>
#include "common/notify.h"
#include "fusion/portableglobe/shared/packetbundle_writer.h"
#include "fusion/portableglobe/shared/packetbundle.h"

namespace fusion_portableglobe {


/**
 * Constructor.
 */
PortableDeltaBuilder::PortableDeltaBuilder(
    const std::string& original_directory,
    const std::string& latest_directory,
    const std::string& delta_directory)
    : delta_directory_(delta_directory),
    original_(original_directory),
    latest_(latest_directory) {
}

/**
 * Destructor.
 */
PortableDeltaBuilder::~PortableDeltaBuilder() {
}

/**
 * Display qtnodes added to delta (debugging).
 */
void PortableDeltaBuilder::DisplayDeltaQtNodes() {
  std::ifstream index;
  PacketBundleSource src(delta_directory_);

  std::uint64_t src_index_size =  src.SourceIndexSize();
  notify(NFY_INFO, "Size: %Zu", src_index_size);
  index.open(src.SourceIndexFileName().c_str(), std::ios::binary);

  IndexItem index_item;
  for (std::uint64_t i = 0; i < src_index_size; ++i) {
    index.read(reinterpret_cast<char*>(&index_item), sizeof(IndexItem));
    notify(NFY_INFO, "%s", index_item.QuadtreeAddress().c_str());
  }

  index.close();
}

/**
 * Load packet into buffer from packet bundle file.
 */
void PortableDeltaBuilder::LoadPacket(
    const char* file_name, const IndexItem& index_item, std::string* buffer) {
  std::ifstream packet_file;
  packet_file.open(file_name, std::ios::binary);
  packet_file.seekg(index_item.offset, std::ios::beg);
  buffer->resize(index_item.packet_size);
  packet_file.read(&(*buffer)[0], index_item.packet_size);
  packet_file.close();
}

/**
 * Returns whether latest packet is different from original packet. If it
 * is, the updated packet will be returned in the latest buffer. The
 * read_original_data indicates whether the original data was already
 * checked and should therefore be advanced.
 */
bool PortableDeltaBuilder::IsLatestPacketDifferent(
    const IndexItem& original_index_item,
    const std::ifstream& original_index,
    std::string* original_buffer,
    const IndexItem& latest_index_item,
    const std::ifstream& latest_index,
    std::string* latest_buffer,
    std::string* qtpath,
    bool* read_original_data) {
  *read_original_data = false;
  // If both have packets at same address, check if they are equal.
  if (latest_index_item == original_index_item) {
    *read_original_data = true;
    LoadPacket(latest_.SourcePacketBundleFileName(
                   original_index_item.file_id).c_str(),
               latest_index_item,
               latest_buffer);

    // If size doesn't match, we know they are not the same.
    if (latest_index_item.packet_size != original_index_item.packet_size) {
      *qtpath = latest_index_item.QuadtreeAddress();
      notify(NFY_INFO,
             "%s Packet size does not match. File id: %d",
             qtpath->c_str(),
             original_index_item.file_id);

      return true;

    // If size matches, we need to check the data.
    } else {
      // TODO: consider leaving file open until file_id changes.
      LoadPacket(original_.SourcePacketBundleFileName(
                     original_index_item.file_id).c_str(),
                 original_index_item,
                 original_buffer);

      if (*latest_buffer != *original_buffer) {
        *qtpath = latest_index_item.QuadtreeAddress();
        notify(NFY_INFO,
               "%s Data does not match. File id: %d",
               qtpath->c_str(),
               original_index_item.file_id);

        return true;
      }
    }
  }

  return false;
}

/**
 * Builds a delta packet bundle filled with packets that differ in the
 * original glb and the latest glb.
 */
void PortableDeltaBuilder::BuildDeltaGlobe() {
  std::uint32_t MAX_BUFFER_SIZE = 1024 * 20;
  std::string original_buffer;
  std::string latest_buffer;
  std::string qtpath;

  // Rerserve probable space for data packets.
  original_buffer.reserve(MAX_BUFFER_SIZE);
  latest_buffer.reserve(MAX_BUFFER_SIZE);

  notify(NFY_INFO,
         "original index: %s",
         original_.SourceIndexFileName().c_str());
  notify(NFY_INFO,
         "original packet bundle: %s",
         original_.SourcePacketBundleFileName(0).c_str());

  std::ifstream original_index;
  std::ifstream latest_index;
  std::ifstream original_packet_file;
  std::ifstream latest_packet_file;
  IndexItem original_index_item;
  IndexItem latest_index_item;

  std::uint64_t latest_index_size_ =  latest_.SourceIndexSize();
  std::uint64_t original_index_size_ =  original_.SourceIndexSize();

  std::string delta_data_directory = delta_directory_ + "/data";
  PacketBundleWriter writer(
      delta_data_directory, PacketBundle::kMaxFileSize, 0);

  original_index.open(
      original_.SourceIndexFileName().c_str(), std::ios::binary);
  latest_index.open(latest_.SourceIndexFileName().c_str(), std::ios::binary);
  std::uint16_t last_file_id = 0xffff;
  original_index.read(
      reinterpret_cast<char*>(&original_index_item), sizeof(IndexItem));
  bool has_more_original_data = true;
  for (std::uint64_t i = 0; i < latest_index_size_; ++i) {
    latest_index.read(
        reinterpret_cast<char*>(&latest_index_item), sizeof(IndexItem));
    std::uint16_t file_id = latest_index_item.file_id;
    if (file_id != last_file_id) {
      notify(NFY_INFO, "file id: %u", file_id);
      last_file_id = file_id;
    }

    // If the current original_data packet has fallen behind, catch up to the
    // lastest position.
    while (has_more_original_data
           && (original_index_item < latest_index_item)) {
      original_index.read(
          reinterpret_cast<char*>(&original_index_item), sizeof(IndexItem));
      has_more_original_data = !original_index.eof();
      notify(NFY_INFO, "stepping ahead in orignal");
    }

    bool read_original_data;
    // Check if latest does not match original.
    if (IsLatestPacketDifferent(original_index_item,
                                original_index,
                                &original_buffer,
                                latest_index_item,
                                latest_index,
                                &latest_buffer,
                                &qtpath,
                                &read_original_data)) {
      writer.AppendPacket(qtpath,
                          latest_index_item.packet_type,
                          latest_index_item.channel,
                          latest_buffer);
    }
    if (read_original_data && has_more_original_data) {
      original_index.read(
          reinterpret_cast<char*>(&original_index_item), sizeof(IndexItem));
      has_more_original_data = !original_index.eof();
    }
  }

  notify(NFY_INFO,
         "Original index items: %lu",
         original_index_size_);
  notify(NFY_INFO,
         "Latest index items: %lu",
         latest_index_size_);

  original_index.close();
  latest_index.close();
}


}  // namespace fusion_portableglobe
