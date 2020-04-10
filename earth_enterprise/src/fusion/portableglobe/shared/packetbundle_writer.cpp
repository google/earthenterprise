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


#include "fusion/portableglobe/shared/packetbundle_writer.h"
#include <cstring>
#include <iostream>  // NOLINT(readability/streams)
#include <string>
#include <utility>
#include <vector>
#include "common/notify.h"
#include "fusion/portableglobe/shared/packetbundle.h"

namespace fusion_portableglobe {

/**
 * Constructor sets the packetbundle's directory and initiates the files
 * for writing packets.
 */
PacketBundleWriter::PacketBundleWriter(const std::string& directory,
                                       std::uint64_t max_file_size,
                                       std::uint16_t file_id)
    : PacketBundle(directory), packet_file_(&crc_calculator_),
      file_id_(file_id), max_file_size_(max_file_size),
      unpacker_(0), is_delta_(false) {
  Init();
}

/**
 * Constructor sets the packetbundle's directory and initiates the files
 * for writing packets.
 */
PacketBundleWriter::PacketBundleWriter(const std::string& directory,
                                       const std::string& base_file,
                                       bool is_qtp_bundle,
                                       std::uint64_t max_file_size,
                                       std::uint16_t file_id)
    : PacketBundle(directory), packet_file_(&crc_calculator_),
      file_id_(file_id), max_file_size_(max_file_size),
      unpacker_(new FileUnpacker(base_file.c_str())),
      is_qtp_bundle_(is_qtp_bundle), is_delta_(true),
      base_file_ptr_(base_file.c_str(), std::ios::in | std::ios::binary) {
  // Guesstimate max packet size as 15k.
  std::uint32_t khMaxPacketSize = 1024 * 15;
  packet_data_.reserve(khMaxPacketSize);
  Init();
}

/**
 * Destructor needs to close up the files.
 */
PacketBundleWriter::~PacketBundleWriter() {
  if (is_delta_) {
    delete unpacker_;
    base_file_ptr_.close();
  }
  index_file_.close();
  packet_file_.WriteCrcSideFile();
  packet_file_.close();
}

/**
 * Prime the packet bundle for writing.
 */
void PacketBundleWriter::Init() {
  // Open the index file.
  std::string index_file_name = IndexFileName();
  index_file_.open(index_file_name.c_str(), std::ios::out | std::ios::binary);

  // ... and the first packetbundle file.
  current_file_size_ = 0;
  std::string packet_file_name = PacketBundleFileName(file_id_);
  packet_file_.open(packet_file_name.c_str(),
                    std::ios::out | std::ios::binary);

  // Initialize the first index item
  last_index_item_.Fill("0", 0, 0);
}


/**
 * Append packet to the next position in the open packetbundle. If that
 * packetbundle is full, close it and start a new one.
 */
bool PacketBundleWriter::AppendPacket(std::string qtpath,
                                      std::uint8_t packet_type,
                                      std::uint16_t channel,
                                      const char* const data,
                                      const size_t data_size) {
  if (is_delta_) {
    bool found;
    if (is_qtp_bundle_) {
      found = unpacker_->FindQtpPacket(qtpath.c_str(), packet_type, channel);
    } else {
      found = unpacker_->FindDataPacket(qtpath.c_str(), packet_type, channel);
    }
    if (found) {
      // If size is the same, read in actual packet to see if they
      // are the same.
      if (static_cast<size_t>(unpacker_->Size()) == data_size) {
        packet_data_.resize(data_size);
        base_file_ptr_.seekg(unpacker_->Offset());
        base_file_ptr_.read(&packet_data_[0], data_size);
        if (memcmp(&packet_data_[0], data, data_size) == 0) {
          // We already have this data so we don't need it in
          // the delta.
          return false;
        }
      }
    }
  }

  // If we topped off the last packetbundle file, create a new one.
  if (data_size + current_file_size_ > max_file_size_) {
    packet_file_.close();

    ++file_id_;
    std::string packet_file_name = PacketBundleFileName(file_id_);
    current_file_size_ = 0;
    packet_file_.open(packet_file_name.c_str(),
                      std::ios::out | std::ios::binary);
    // Note that this will give data that is greater than the max_file_size_
    // its own bundle file, which is probably a reasonable approach.
  }

  // Create and save the index item.
  IndexItem index_item;
  index_item.Fill(qtpath, packet_type, channel);
  index_item.file_id = file_id_;
  index_item.offset = current_file_size_;
  index_item.packet_size = data_size;

  // Do not create a globe where packets are out of order.
  // We won't be able to find the right packets anyway.
  if (!(last_index_item_ < index_item)) {
    notify(NFY_WARN, "Packet is not in increasing order; not adding it.");
    return false;
  }

  index_file_.write(reinterpret_cast<char*>(&index_item), sizeof(IndexItem));

  // Save with the packet.
  packet_file_.write(const_cast<char*>(data), data_size);

  current_file_size_ += data_size;

  // Save the last index item for next time.
  last_index_item_.btree_high = index_item.btree_high;
  last_index_item_.btree_low = index_item.btree_low;
  last_index_item_.level = index_item.level;
  last_index_item_.packet_type = index_item.packet_type;

  return true;
}

}  // namespace fusion_portableglobe
