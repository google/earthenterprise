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

// CachedReadAccessor Implementation
#include<limits.h>
#include <string.h>
#include <iostream>
#include "cachedreadaccessor.h"
#include "khSimpleException.h"

CachedReadAccessor::CachedReadAccessor(std::uint32_t max_blocks, std::uint32_t block_size)
 : max_blocks_count_(max_blocks), block_size_(block_size),
   access_tick_counter_(0),
   stats_bytes_read_(0),
   stats_disk_accesses_(0)
  {
  if (max_blocks_count_ < 2) {
    throw khSimpleException(
      "Illegal arguments for CachedReadAccessor: max_blocks < 2");
  }
}

CachedReadAccessor::~CachedReadAccessor() {
  // Clean up any blocks we still have in memory.
  CacheBlockMap::iterator iter = cache_blocks_map_.begin();
  for(; iter != cache_blocks_map_.end(); ++iter) {
    CacheBlock* block_to_remove = iter->second;
    delete block_to_remove;
  }
}

void CachedReadAccessor::Pread(FileBundleSegment& segment,
           void *buffer, size_t size, off64_t offset) {
  if (size > block_size_) {
    // for any read requests larger than our block size, defer to the
    // normal read mechanism...our cached reader assumes that all reads
    // fit in at most 2 CacheBlocks.
    assert(offset + size <= segment.data_size());
    segment.Pread(buffer, size, offset);
    stats_bytes_read_ += size;
    stats_disk_accesses_++;
    return;
  }

  // We have 2 cases:
  // 1) the data is completely in 1 cache block,
  // 2) the data straddles 2 cache blocks
  // We handle this in 3 steps:
  // 1) get the block(s) for the read (this may be 1 or 2, and they may
  // be empty)
  // 2) read the data from the 1st block (initializing the cache block
  // from disk if necessary)
  // 3) read the remaining data from the 2nd block if necessary (also
  // initializing the cache block from disk if necessary)
  CacheBlock* block_0 = NULL;
  CacheBlock* block_1 = NULL;
  FindBlocks(segment.id(), size, offset, &block_0, &block_1);
  assert(block_0);
  std::uint32_t read_count =
    block_0->Read(segment, buffer, size, offset, access_tick_counter_++,
                  stats_bytes_read_, stats_disk_accesses_);
  if (block_1) {
    char* buffer_continue = static_cast<char*>(buffer) + read_count;
    block_1->Read(segment, buffer_continue,
                  size - read_count, offset + read_count,
                  access_tick_counter_++,
                  stats_bytes_read_, stats_disk_accesses_);
  }
}

 std::uint32_t CachedReadAccessor::FindBlocks(std::uint32_t segment_id,
                                      std::uint32_t size,
                                      std::uint32_t offset,
                                      CacheBlock** block_0,
                                      CacheBlock** block_1) {
  // 2 cases:
  // 1) request fits in a single block
  // 2) request straddles 2 blocks
  CacheBlockAddress address_0 = BlockAddress(segment_id, offset);
  *block_0 = GetCacheBlock(address_0);

  // Check if we straddle 2 blocks.
  std::uint32_t next_block_offset = address_0.Offset() + block_size_;
  bool straddles_blocks = offset + size > next_block_offset;
  if (straddles_blocks) { // both blocks returned
    CacheBlockAddress address_1(segment_id, next_block_offset);
    *block_1 = GetCacheBlock(address_1);
    return 2;
  } else { // Only block_0 returned
    *block_1 = NULL;
    return 1;
  }
}

CachedReadAccessor::CacheBlock*
CachedReadAccessor::GetCacheBlock(const CacheBlockAddress& address) {
  CachedReadAccessor::CacheBlock* block = NULL;
  CachedReadAccessor::CacheBlockMap::iterator iter =
    cache_blocks_map_.find(address);
  if (iter != cache_blocks_map_.end()) { // found it
    block = iter->second;
  } else { // need to add it to our LRU cache
    block = AddCacheBlock(address);
  }
  block->SetLastAccessTick(access_tick_counter_++);
  return block;
}


CachedReadAccessor::CacheBlock*
CachedReadAccessor::AddCacheBlock(const CacheBlockAddress& address) {
  // Initialize the CacheBlock struct, give it the latest tick so it is
  // not booted out of the cache immediately.
  CachedReadAccessor::CacheBlock* block =
    new CachedReadAccessor::CacheBlock(address, block_size_);

  // If necessary, remove the least recently used block.
  if (max_blocks_count_ == cache_blocks_map_.size()) {
    // Identify the Least Recently Used block and remove it from our cache.
    CachedReadAccessor::CacheBlockMap::iterator iter = cache_blocks_map_.begin();
    std::uint64_t least_access_tick = ULONG_LONG_MAX;
    CachedReadAccessor::CacheBlockMap::iterator block_iter_to_remove =
      cache_blocks_map_.begin();
    for(; iter != cache_blocks_map_.end(); ++iter) {
      CachedReadAccessor::CacheBlock* block_i = iter->second;
      if (block_i->LastAccessTick() < least_access_tick) {
        least_access_tick = block_i->LastAccessTick();
        block_iter_to_remove = iter;
      }
    }
    CachedReadAccessor::CacheBlock* block_to_remove =
       block_iter_to_remove->second;
    cache_blocks_map_.erase(block_iter_to_remove);
    if (block_to_remove) {
      delete block_to_remove;
    }
  }

  // Add the new block to our map
  cache_blocks_map_[address] = block;

  return block;
}

 std::uint32_t CachedReadAccessor::CacheBlock::Read(FileBundleSegment& segment,
          void *out_buffer, size_t size, off64_t offset,
          std::uint64_t access_tick,
          std::uint64_t& stats_bytes_read, std::uint64_t& stats_disk_accesses) {
  last_access_tick_ = access_tick; // keep track of our last read access.

  // Check that requested size is within the bounds of this block.
  if (!initialized_) {
    // Must read the whole contents of this cache block from the segment.
    std::uint32_t segment_remainder = segment.data_size() - address_.Offset();
    assert(address_.Offset() < segment.data_size());
    std::uint32_t request_size = std::min(block_size_, segment_remainder);
    // We're going to read directly into our buffer_
    buffer_.resize(request_size);
    void* cache_buffer = const_cast<char*>(buffer_.data());
    segment.Pread(cache_buffer, request_size, address_.Offset());
    initialized_ = true;
    stats_bytes_read += request_size;
    stats_disk_accesses++;
  }

  // At this point the requested data should be in buffer_,
  // need to copy it out.
  // Remember, this CacheBlock starts at address_.Offset()
  std::uint32_t cache_buffer_offset = offset - address_.Offset();
  void* cache_buffer = const_cast<char*>(buffer_.data() + cache_buffer_offset);
  // Need to catch case where request extends beyond this CacheBlock.
  assert(cache_buffer_offset < buffer_.size());
  std::uint32_t request_bytes_in_cache_block = buffer_.size() - cache_buffer_offset;
  std::uint32_t read_count = std::min(static_cast<std::uint32_t>(size),
                               request_bytes_in_cache_block);
  memcpy(out_buffer, cache_buffer, read_count);
  return read_count;
}
