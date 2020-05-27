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


// PacketFileReaderPool
//
// In merge operations, it is necessary to pass many thousands or
// millions of packet references through the merge.  Each of these
// packet references must include information about the source packet
// file.  PacketFileReaderPool provides a compact token that can be
// used to access the appropriate packet file.

#ifndef COMMON_PACKETFILE_PACKETFILEREADERPOOL_H__
#define COMMON_PACKETFILE_PACKETFILEREADERPOOL_H__

#include <cstdint>
#include <string>
#include <vector>
#include <khGuard.h>
#include <packetfile/packetfilereader.h>

// PacketFileReaderToken
//
// Opaque token that can be used to access a packet file.  All data
// fields are private, accessible only by the friend class
// PacketFileReaderPool.  The constructor is also private; a factory
// method in the pool class is the only way to get a new token.
// Copying and assignment are allowed.
//
// The data fields consist of a hash code (to make sure that the token
// is decoded against the proper pool) and a file index.
//
// The pool hash is used instead of just including a pointer or
// reference for a couple reasons: on 64 bit systems, the pointer or
// reference would double in size, and also there is the possibility
// of referencing an invalid/deallocated memory area.

class geFilePool;
class PacketFileReaderBase;
class PacketFileReaderPool;

class PacketFileReaderToken {
 public:
  // Public default constructor, creates invalid token used as place holder
  PacketFileReaderToken()
      : pool_hash_(0xFFFFFFFF),
        packet_file_index_(0xFFFFFFFF) {
  }

  std::uint32_t PacketFileIndex() const { return packet_file_index_; }

 private:
  PacketFileReaderToken(std::uint32_t pool_hash, std::uint32_t packet_file_index)
      : pool_hash_(pool_hash),
        packet_file_index_(packet_file_index) {
  }
  friend class PacketFileReaderPool;

  std::uint32_t pool_hash_;
  std::uint32_t packet_file_index_;
};

// PacketFileReaderPool
//
//

class PacketFileReaderPool {
 public:
  PacketFileReaderPool(const std::string &pool_name,
                       geFilePool &file_pool);
  virtual ~PacketFileReaderPool() {}

  // Add a packet file to the reader pool.  The packet file is opened,
  // and a token is returned.
  virtual PacketFileReaderToken Add(const std::string &packet_file_name);

  // Add a vector of packet file names to the pool.  Append tokens to a
  // return vector.
  void Add(const std::vector<std::string> &packet_file_names,
           std::vector<PacketFileReaderToken> &generated_tokens);

  // Get the Reader corresponding to a token
  PacketFileReaderBase &Reader(const PacketFileReaderToken &token);

  // Enable cached reading, with "max_blocks" number of cache blocks each
  // of size "block_size".
  // Suggested usage is max_blocks=2, block_size=30MB for a linear read through
  // a FileBundle.
  // For a quadtree traversal, perhaps use max_blocks=10, block_size=5MB.
  // max_blocks must be >= 2.
  void EnableReadCache(std::uint32_t max_blocks, std::uint32_t block_size) {
    read_cache_max_blocks_ = max_blocks;
    read_cache_block_size_ = block_size;
  }

  // Close packet file(s)
  void Close(const PacketFileReaderToken &token);
  void Close(const std::vector<PacketFileReaderToken> &tokens);

  geFilePool &file_pool() const { return file_pool_; }
 private:
  geFilePool &file_pool_;
  std::uint32_t pool_hash_;
  const std::string pool_name_;

  std::vector<std::string> names_;
  khDeletingVector<PacketFileReaderBase> readers_;

  // Support for optional read caching
  std::uint32_t read_cache_max_blocks_;
  std::uint32_t read_cache_block_size_;

  DISALLOW_COPY_AND_ASSIGN(PacketFileReaderPool);
};

#endif  // COMMON_PACKETFILE_PACKETFILEREADERPOOL_H__
