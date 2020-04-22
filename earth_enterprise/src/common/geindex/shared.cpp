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

//

#include "shared.h"
#include <khEndian.h>
#include <third_party/rsa_md5/crc32.h>
#include "IndexBundleReader.h"

namespace geindex {

// ****************************************************************************
// ***  ChildBucketAddr
// ****************************************************************************
void ChildBucketAddr::Push(EndianWriteBuffer &buf) const {
  buf << offset
      << childBucketsSize
      << entryBucketsSize;
}

void ChildBucketAddr::Pull(EndianReadBuffer &buf) {
  buf >> *const_cast<std::uint64_t *>(&offset)
      >> *const_cast<std::uint32_t *>(&childBucketsSize)
      >> *const_cast<std::uint32_t *>(&entryBucketsSize);
}


// ****************************************************************************
// ***  LoadedChildBucket
// ****************************************************************************
const QuadtreePath LoadedChildBucket::kFirstSubPath("0000");

void LoadedChildBucket::Load(const ChildBucketAddr &addr,
                             IndexBundle &bundle,
                             EndianReadBuffer &tmpBuf) {
  if (addr) {
    // No CRC - the ChildBucket is written in two halves, each with
    // it's own CRC. The Pull will check each of the separate
    // CRCs
    bundle.LoadWithoutCRC(addr.WholeAddr(), tmpBuf);
    Pull(addr, tmpBuf);
  }
}


ChildBucketAddr LoadedChildBucket::Push(EndianWriteBuffer &buf) {
  // write the child bucket addrs
  EndianWriteBuffer::size_type start = buf.CurrPos();
  for (unsigned int i = 0; i < kChildAddrsPerBucket; ++i) {
    if (childAddrs[i]) {
      buf << static_cast<BucketChildSlotType>(i) << childAddrs[i];
    }
  }
  EndianWriteBuffer::size_type end = buf.CurrPos();
  if (end > start) {
    buf << Crc32(&buf[start], end - start);
    end = buf.CurrPos();
  }

  // update the childBucketsSize
  const std::uint32_t childBucketsSize = end - start;


  // write the entry bucket addrs
  start = end;
  for (unsigned int i = 0; i < kChildAddrsPerBucket; ++i) {
    if (entryAddrs[i]) {
      buf << static_cast<BucketEntrySlotType>(i) << entryAddrs[i];
    }
  }
  end = buf.CurrPos();
  if (end > start) {
    buf << Crc32(&buf[start], end - start);
    end = buf.CurrPos();
  }

  // update the childBucketsSize
  const std::uint32_t entryBucketsSize = end - start;

  return ChildBucketAddr(0, childBucketsSize, entryBucketsSize);
}

void LoadedChildBucket::Pull(const ChildBucketAddr &addr,
                             EndianReadBuffer &buf) {
  // --- stored as ---
  // { BucketChildSlotType; ChildBucketAddr } []
  // crc32
  // { BucketChildSlotType; EntryBucketAddr } []
  // crc32

  // we must have the ChildBucketAddr in order to correctly parse this
  // otherwise we don't know how big each addr list is

  // extract the children
  if (addr.childBucketsSize) {
    buf.CheckCRC(addr.childBucketsSize,
                 "geindex child bucket (child addrs)");
    EndianReadBuffer::size_type endPos =
      buf.CurrPos() + addr.childBucketsSize - sizeof(std::uint32_t);
    while (buf.CurrPos() < endPos) {
      BucketChildSlotType pos;
      buf >> pos;
      // make this pull into a separate statement in case the address of
      // childAddrs[pos] is calculated before pos is pulled.
      buf >> childAddrs[pos];
    }
    buf.Skip(sizeof(std::uint32_t)); // crc
  }

  // extract the entries
  if (addr.entryBucketsSize) {
    buf.CheckCRC(addr.entryBucketsSize,
                 "geindex child bucket (entry addrs)");
    EndianReadBuffer::size_type endPos =
      buf.CurrPos() + addr.entryBucketsSize - sizeof(std::uint32_t);
    while (buf.CurrPos() < endPos) {
      BucketEntrySlotType pos;
      buf >> pos;
      // make this pull into a separate statement in case the address of
      // entryAddrs[pos] is calculated before pos is pulled.
      buf >> entryAddrs[pos];
    }
    buf.Skip(sizeof(std::uint32_t)); // crc
  }
}


// compute these at compile time since we need to use them a lot
const unsigned int BucketPath::EntrySlotSpacing[kQuadLevelsPerBucket - 1] = {
  1 + 4 + 16,
  1 + 4,
  1
};
// ****************************************************************************
// SubAddrAsEntrySlot
// Compute the entry slot for the sub-path.  The entry slot number is
// computed such that the entry slots are stored in preorder.  The
// traverser depends on this.
// ****************************************************************************
BucketEntrySlotType
BucketPath::SubAddrAsEntrySlot(const QuadtreePath &target) const {
  QuadtreePath path = QuadtreePathToChild(target);

  // The path must have less than kQuadLevelsPerBucket levels, otherwise
  // this function should have been called on a different BucketPath
  unsigned int blevel = path.Level();
  assert(blevel <= 3);
  BucketEntrySlotType slot = blevel;
  for ( ; blevel ; ) {
    --blevel;
    slot += path[blevel] * EntrySlotSpacing[blevel];
  }
  return slot;
}

namespace {
// Helper class to map entry slots to sub-addresses
class EntrySlotQuadtreeMap {
 public:
  EntrySlotQuadtreeMap() {              // initialize table
    QuadtreePath qt_path;
    for (BucketEntrySlotType slot = 0; slot < kEntrySlotsPerBucket; ++slot) {
      map_[slot] = qt_path;
      qt_path.Advance(kQuadLevelsPerBucket-1);
    }
  }

  inline QuadtreePath Map(BucketEntrySlotType slot) const {
    assert(slot < kEntrySlotsPerBucket);
    return map_[slot];
  }
 private:
  QuadtreePath map_[kEntrySlotsPerBucket];
};

EntrySlotQuadtreeMap entry_slot_quadtree_map;
}

// ****************************************************************************
// EntrySlotAsSubAddr
// Compute the sub-path for an entry slot number.  The bucket path must
// have an exact multiple of kQuadLevelsPerBucket levels.  The result is
// the sub-path appended to the bucket path.
// ****************************************************************************

QuadtreePath
BucketPath::EntrySlotAsSubAddr(BucketEntrySlotType slot) const {
  assert(Level() % kQuadLevelsPerBucket == 0);
  return Concatenate(entry_slot_quadtree_map.Map(slot));
}

// ****************************************************************************
// SubAddrAsChildSlot
// Compute the child slot for a level 4 sub-path.  The child slot number is
// computed such that the child slots are stored in preorder.  The
// traverser depends on this.
// ****************************************************************************
BucketChildSlotType
BucketPath::SubAddrAsChildSlot(const QuadtreePath &target) const {
  QuadtreePath subaddr = QuadtreePathToChild(target);
  assert(subaddr.Level() == kQuadLevelsPerBucket);
  return subaddr.AsIndex(kQuadLevelsPerBucket);
}

} // namespace geindex
