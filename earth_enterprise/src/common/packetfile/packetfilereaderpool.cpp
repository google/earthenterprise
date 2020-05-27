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


// PacketFileReaderPool
//
// In merge operations, it is necessary to pass many thousands or
// millions of packet references through the merge.  Each of these
// packet references must include information about the source packet
// file.  PacketFileReaderPool provides a compact token that can be
// used to access the appropriate packet file.

#include <cstdint>
#include <third_party/rsa_md5/crc32.h>
#include <packetfile/packetfilereader.h>
#include <packetfile/packetfilereaderpool.h>

// Constructor

PacketFileReaderPool::PacketFileReaderPool(const std::string &pool_name,
                                           geFilePool &file_pool)
    : file_pool_(file_pool),
      pool_name_(pool_name),
      read_cache_max_blocks_(0),
      read_cache_block_size_(0) {
  // Generate a hash to use for token validation.  This is used for an
  // internal consistency check, so nothing will break in the unlikely
  // case that the hash is not unique.

  PacketFileReaderPool *hashed_value = this;
  pool_hash_ = Crc32(hashed_value, sizeof(hashed_value));
}

// Add a packet file to the reader pool.  The packet file is opened,
// and a token is returned.

PacketFileReaderToken PacketFileReaderPool::Add(
    const std::string &packet_file_name) {
  assert(names_.size() == readers_.size());
  std::uint32_t packet_file_index = readers_.size();
  names_.push_back(packet_file_name);
  PacketFileReaderBase* reader = new PacketFileReaderBase(file_pool_,
                                                          packet_file_name);
  readers_.push_back(TransferOwnership(reader));
  reader->EnableReadCache(read_cache_max_blocks_, read_cache_block_size_);
  return PacketFileReaderToken(pool_hash_, packet_file_index);
}

// Add a vector of packet file names to the pool.  Append tokens to a
// return vector.

void PacketFileReaderPool::Add(
    const std::vector<std::string> &packet_file_names,
    std::vector<PacketFileReaderToken> &generated_tokens) {
  for (std::vector<std::string>::const_iterator file_name
         = packet_file_names.begin();
       file_name != packet_file_names.end();
       ++file_name) {
    generated_tokens.push_back(Add(*file_name));
  }
}

// Get the Reader corresponding to a token

PacketFileReaderBase &PacketFileReaderPool::Reader(
    const PacketFileReaderToken &token) {
  assert(token.pool_hash_ == pool_hash_);
  assert(token.packet_file_index_ < readers_.size());
  assert(readers_[token.packet_file_index_]);
  return *readers_[token.packet_file_index_];
}

// Close packet file(s)

void PacketFileReaderPool::Close(const PacketFileReaderToken &token) {
  assert(token.pool_hash_ == pool_hash_);
  assert(token.packet_file_index_ < readers_.size());
  readers_.Assign(token.packet_file_index_, NULL);
}

void PacketFileReaderPool::Close(
    const std::vector<PacketFileReaderToken> &tokens) {
  for (std::vector<PacketFileReaderToken>::const_iterator token
         = tokens.begin();
       token != tokens.end();
       ++token) {
    Close(*token);
  }
}
