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


#ifndef COMMON_FILEBUNDLE_CACHEDREADACCESSOR_H__
#define COMMON_FILEBUNDLE_CACHEDREADACCESSOR_H__

#include "filebundle.h"

class FileBundleSegment;

// CachedReadAccessor provides a wrapper class for cached reads of FileBundle
// segments.
// For processes that read through the file bundle, CachedReadAccessor manages
// a queue of CacheBlocks (within a segment) which are sorted by LRU.
//
// example usage:
// std::uint32_t blocks_count = 5; // 5 blocks to be cached
// std::uint32_t block_size = 64 * 1024; // each cached block will be 64KB
// CachedReadAccessor accessor(blocks_count, block_size);
// ...
// FileBundleSegment segment;
// accessor.Pread(segment, buffer, size, offset);
//
// The accessor will cache blocks (chunks) of segments in an LRU cache
// while reads occur.
// In the above example, the caller specified a cache of 5x 64KB blocks to be
// cached during the read.
//
// Notes:
// This will be really effective for any process reading through the entire file
// bundle.
// Reads from disk are done at the granularity of block_size.
// If it is a linear pass through, 2x 64KB would be a good setting (note that
// you'll need at least 2 blocks since some reads will straddle the blocks).
// For dynamic walks through the quadtree, recommend at least 5x 64KB,
// perhaps 10x or more.
// This will be typically used within a FileBundle, so you must take care not
// to abuse (or to reduce the size and number of the blocks) this if many
// FileBundles will be opened simultaneously.
class CachedReadAccessor {
  friend class CachedReadAccessorUnitTest;
protected:
  // CacheBlockAddress identifies a specific CacheBlock by segment id and
  // offset within the segment.
  class CacheBlockAddress {
  public:
    CacheBlockAddress(std::uint32_t segment_id, std::uint32_t offset) :
      segment_id_(segment_id), offset_(offset) {}
    // the start offset of the block
    std::uint32_t Offset() const { return offset_; }

    // Operator overloads
    bool operator<(const CacheBlockAddress& other) const {
      if (segment_id_ < other.segment_id_)
        return true;
      if (segment_id_ == other.segment_id_ && offset_ < other.offset_)
        return true;
      return false;
    }
    bool operator==(const CacheBlockAddress& other) const {
      return (segment_id_ == other.segment_id_ && offset_ == other.offset_);
    }
    bool operator!=(const CacheBlockAddress& other) const {
      return (segment_id_ != other.segment_id_ || offset_ != other.offset_);
    }
  private:
    std::uint32_t segment_id_;
    std::uint32_t offset_;
  };

  // CacheBlock buffers the read data for a chunk of a segment.
  // It also maintains the tick count for the last access so the CacheBlocks
  // can be sorted in LRU order.
  class CacheBlock {
  public:
    CacheBlock(CacheBlockAddress address, std::uint32_t block_size) :
      address_(address), last_access_tick_(0),
      block_size_(block_size), initialized_(false)  {
    }

    // Read the requested data out of the CacheBlock.
    // Return the number of bytes read.
    // stats_bytes_read and stats_disk_accesses are incremented if a read from
    // disk is required (i.e., on first Read of a CacheBlock). These are
    // used to keep track of disk reads and accesses for stats purposes.
    std::uint32_t Read(FileBundleSegment& segment,
              void *out_buffer, size_t size, off64_t offset,
              std::uint64_t access_tick,
              std::uint64_t& stats_bytes_read, std::uint64_t& stats_disk_accesses);

    std::uint64_t LastAccessTick() const { return last_access_tick_; }
    void SetLastAccessTick(std::uint64_t last_access_tick) {
      last_access_tick_ = last_access_tick;
    }

  private:
    CacheBlockAddress address_;
    std::uint64_t last_access_tick_;
    std::uint32_t block_size_;
    std::string buffer_;
    bool initialized_;
  };
  typedef std::map<CacheBlockAddress, CacheBlock*> CacheBlockMap;
public:
  // max_blocks must be >= 2.
  CachedReadAccessor(std::uint32_t max_blocks, std::uint32_t block_size);
  ~CachedReadAccessor();

  // Read from a segment. This routine will use it's cache as much as possible
  // and will defer to the segment to read when necessary.
  // It reads "size" bytes into buffer from the FileBundleSegment "segment"
  // starting at "offset" bytes within the segment.
  void Pread(FileBundleSegment& segment,
             void *buffer, size_t size, off64_t offset);

protected:
  // return the CacheBlockAddress for the given segment and offset.
  // Note: that CachedReadAccessor allocates CacheBlocks aligned with
  // block_size memory chunks.
  CacheBlockAddress BlockAddress(std::uint32_t segment_id, std::uint32_t offset) const {
    std::uint32_t block_index = offset / block_size_;
    return CacheBlockAddress(segment_id, block_index * block_size_);
  }

  // Return the cache block pointer(s) for the segment if it exists, or 2 cache
  // blocks if the request straddles a cacheblock boundary.
  // If a cache block does not exist in the cache, go ahead and add it.
  // Return the number of blocks overlapping the request.
  std::uint32_t FindBlocks(std::uint32_t segment_id, std::uint32_t size, std::uint32_t offset,
                 CacheBlock** block_0, CacheBlock** block_1);

  // Return the CacheBlock pointer for the specified CacheBlockAddress.
  // Will get the CacheBlock from our cache or create it if necessary.
  CacheBlock* GetCacheBlock(const CacheBlockAddress& address);

  // Create and returns a new CacheBlock while keeping it in its LRU cache.
  // ASSUMES: it doesn't already exist in the CachedReadAccessor.
  CacheBlock* AddCacheBlock(const CacheBlockAddress& address);

  // Basic Stats are kept which may be useful for debugging.
  std::uint64_t StatsBytesRead() const { return stats_bytes_read_; }
  std::uint64_t StatsDiskAccesses() const { return stats_disk_accesses_; }
private:
  // We track the blocks in a map (for easy random access).
  // For each removal, we just run through the list and pick the LRU CacheBlock
  // rather than maintain a priority queue which would be costlier.
  // Maintaining a priority queue would be costlier because the number of
  // cache block accesses (touching the block and hence modifying the queue order)
  // will be much higher (roughly 2 orders of magnitude) in general than the
  // number of times a block is removed from memory.
  CacheBlockMap cache_blocks_map_;
  std::uint32_t max_blocks_count_;
  std::uint32_t block_size_;
  std::uint64_t access_tick_counter_;
  std::uint64_t stats_bytes_read_;
  std::uint64_t stats_disk_accesses_;
  DISALLOW_COPY_AND_ASSIGN(CachedReadAccessor);
};

#endif  // COMMON_FILEBUNDLE_CACHEDREADACCESSOR_H__
