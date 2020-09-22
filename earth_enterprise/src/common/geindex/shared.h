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

//

#ifndef COMMON_GEINDEX_SHARED_H__
#define COMMON_GEINDEX_SHARED_H__

#include <string>
#include <cstdint>
#include <quadtreepath.h>
#include <filebundle/filebundle.h>

class EndianReadBuffer;
class EndianWriteBuffer;

namespace geindex {

class IndexBundle;

// basic constants on bucket sizes
static const unsigned int kQuadLevelsPerBucket = 4;
static const unsigned int kChildAddrsPerBucket = 256; // 4^kQuadLevelsPerBucket
static const unsigned int kEntrySlotsPerBucket = 1 + 4 + 16 + 64; // 85
static const std::uint16_t kCurrentFileFormatVersion = 0;

// Types just big enough to store child & entry slot numbers relative to a
// bucket. NOTE: These are used for disk storage. If you change them, you will
// need to bump the index's fileFormatVersion and adjust several VersionedPull
// routines. 
typedef std::uint8_t BucketChildSlotType;  // holds kChildAddrsPerBucket indexes
typedef std::uint8_t BucketEntrySlotType;  // holds kEntrySlotsPerBucket indexes

// ****************************************************************************
// ***  BundleAddr
// ***
// ***  Offset and size into index bundle. The index always needs both pieces
// ***  so this stores them together.
// ****************************************************************************

typedef FileBundleAddr BundleAddr;
typedef BundleAddr EntryBucketAddr;

// ****************************************************************************
// ***  ChildBucketAddr
// ***
// ***  ChildBuckets are stored in two pieces (list of ChildBucketAddrs and a
// ***  list of EntryBucketAddrs). They are contiguous in the bundle so only
// ***  one offset but two sizes are needed.
// ****************************************************************************
class ChildBucketAddr {
 public:
  const std::uint64_t offset;
  const std::uint32_t childBucketsSize;
  const std::uint32_t entryBucketsSize;

  inline ChildBucketAddr(std::uint64_t offset_, std::uint32_t childBucketsSize_,
                         std::uint32_t entryBucketsSize_) :
      offset(offset_),
      childBucketsSize(childBucketsSize_),
      entryBucketsSize(entryBucketsSize_)
  { }

  const ChildBucketAddr& operator =(const ChildBucketAddr& other) {
    new (this) ChildBucketAddr(other.offset, other.childBucketsSize,
                               other.entryBucketsSize);
    return *this;
  }
  inline ChildBucketAddr(void) :
      offset(0),
      childBucketsSize(0),
      entryBucketsSize(0)
  { }

  inline operator bool(void) const { return TotalSize() > 0; }
  inline bool operator==(const ChildBucketAddr &o) const {
    return ((offset == o.offset) &&
            (childBucketsSize == o.childBucketsSize) &&
            (entryBucketsSize == o.entryBucketsSize));
  }
  inline bool operator!=(const ChildBucketAddr &o) const {
    return !operator==(o);
  }
  inline std::uint64_t Offset(void) const { return offset; }
  inline std::uint32_t TotalSize(void) const {
    return childBucketsSize+entryBucketsSize;
  }
  inline BundleAddr ChildBucketsAddr(void) const {
    return BundleAddr(offset, childBucketsSize);
  }
  inline BundleAddr EntryBucketsAddr(void) const {
    return BundleAddr(offset+childBucketsSize, entryBucketsSize);
  }
  inline BundleAddr WholeAddr(void) const {
    return BundleAddr(offset, TotalSize());
  }

  void Push(EndianWriteBuffer &buf) const;
  void Pull(EndianReadBuffer &buf);
};

// ****************************************************************************
// ***  LoadedChildBucket
// ***
// ***  A ChildBucket that has been loaded from disk. The Writer derives
// ***  from this to store it's write cache, the Reader pulls into this
// ***  before populating its cache nodes. The Reader doesn't actually use
// ***  this for its cache node.
// ****************************************************************************
class LoadedChildBucket {
 public:
  // Sub-path of first slot in child bucket ("0000")
  static const QuadtreePath kFirstSubPath;

  ChildBucketAddr childAddrs[kChildAddrsPerBucket];
  EntryBucketAddr entryAddrs[kChildAddrsPerBucket];

  // The ReadCache needs to be able to create the bucket and then load it
  // later so provide separate routines for both. Yes, this breaks RAII, but
  // it vastly simplifies the locking mechanism in the cache
  LoadedChildBucket(void) {}
  void Load(const ChildBucketAddr &addr, IndexBundle &bundle,
            EndianReadBuffer &tmpBuf);

  LoadedChildBucket(const ChildBucketAddr &addr, IndexBundle &bundle,
                    EndianReadBuffer &tmpBuf) {
    Load(addr, bundle, tmpBuf);
  }

  // returns a ChildBucketAddr with offset == 0
  // childBucketsSize and entryBucketsSize are filled in
  //
  // LoadedChildBucket is stored as
  //   pair<BucketChildSlotType, ChildBucketAddr>[], crc32
  //   pair<BucketChildSlotType, EntryBucketAddr>[], crc32
  ChildBucketAddr Push(EndianWriteBuffer &buf);

 private:
  // requires addr as an argument so the buffer doesn't have to store
  // the sizes again.
  void Pull(const ChildBucketAddr &addr, EndianReadBuffer &buf);
};



// ****************************************************************************
// ***  BucketPath
// ***
// ***  QuadtreePath truncated to match bucket addresses
// ***
// ***  Provides SubAddr* routines
// ****************************************************************************
class BucketPath : public QuadtreePath {
 public:
  BucketPath(void) : QuadtreePath() {}
  BucketPath(const BucketPath &o) : QuadtreePath(o) {}
  // chop the address to an even multiple of kQuadLevelsPerBucket
  BucketPath(const QuadtreePath &path) : QuadtreePath(
      path, (path.Level()/kQuadLevelsPerBucket) * kQuadLevelsPerBucket) {}
  // chop the address to the requested BucketLevel
  BucketPath(const BucketPath &o, unsigned int bucketLevel) : QuadtreePath(
      o,  bucketLevel * kQuadLevelsPerBucket) {}

  // get the parent bucket address
  BucketPath BucketPathParent(unsigned int count = 1) const {
    return BucketLevel() > count ? BucketPath(*this, BucketLevel() - count)
                                 : BucketPath();
  }

  unsigned int BucketLevel(void) const { return Level() / kQuadLevelsPerBucket;}

  // assumes target is within the Bucket extents
  QuadtreePath QuadtreePathToChild(const QuadtreePath &target) const {
    return QuadtreePath::RelativePath(*this, target);
  }

  // Following 3 assumes target is within the Bucket extents
  // QuadtreePathToChild as pre-order sequence
  BucketEntrySlotType SubAddrAsEntrySlot(const QuadtreePath &target) const;
  // Reverse mapping of SubAddrAsEntrySlot; For testing only
  QuadtreePath EntrySlotAsSubAddr(BucketEntrySlotType slot) const;
  // QuadtreePathToChild as binary sequence based on quad-tree-path
  BucketChildSlotType SubAddrAsChildSlot(const QuadtreePath &target) const;


  bool Contains(const QuadtreePath &other) const {
    return (IsAncestorOf(other) &&
            ((other.Level() - Level()) < kQuadLevelsPerBucket));
  }

  static const unsigned int EntrySlotSpacing[kQuadLevelsPerBucket - 1];
};


} // namespace geindex



#endif // COMMON_GEINDEX_SHARED_H__
