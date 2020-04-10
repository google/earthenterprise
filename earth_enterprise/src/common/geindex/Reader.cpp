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

#include <memory>
#include "Reader.h"
#include "IndexBundleReader.h"
#include "Entries.h"
#include <khEndian.h>
#include <geSegmentedArray.h>
#include <packetfile/packetfilereader.h>

namespace geindex {

// Define the maximum levels permitted in the cache

const std::uint32_t kMaxLevelsCached = 3;

namespace readerimpl {

// ****************************************************************************
// ***  ChildBucketCache
// ***
// ***  Cache the child buckets for N levels(kNumLevelsCached),
// ***  N <= kMaxLevelsCached(3), unless you have more than
// ***  a Gig of RAM available for the cache)
// ***  This class does no I/O of its own.
// ***  NOTE: This class is NOT mt-safe, it must be protected externally.
// ***  Single cache for each bundle filesystem.
// ****************************************************************************
class ChildBucketCache {
 public:
  class Bucket {
   public:
    std::uint32_t childAddrIndexes[kChildAddrsPerBucket];
    std::uint32_t entryAddrIndexes[kChildAddrsPerBucket];

    Bucket(void) {
      std::uninitialized_fill(&childAddrIndexes[0],
                              &childAddrIndexes[kChildAddrsPerBucket],
                              kInvalidAddrIndex);
      std::uninitialized_fill(&entryAddrIndexes[0],
                              &entryAddrIndexes[kChildAddrsPerBucket],
                              kInvalidAddrIndex);
    }
  };
  
  ~ChildBucketCache() {}

  static khTransferGuard<ChildBucketCache> Create(unsigned int numLevelsCached) {
    assert(numLevelsCached > 0);
    assert(numLevelsCached <= kMaxLevelsCached);
    return TransferOwnership(new ChildBucketCache(numLevelsCached));
  }

  ChildBucketAddr GetChildBucketAddr(ChildBucketId childId,
                                     const BucketPath &subaddr);
  EntryBucketAddr GetEntryBucketAddr(ChildBucketId childId,
                                     const BucketPath &subaddr);

  ChildBucketId Find(const BucketPath &addr);
  void MarkBucketNonExistent(const BucketPath &addr);
  ChildBucketId FillBucket(const BucketPath &addr,
                           const LoadedChildBucket &loadBucket);

 private:
  // compute these at compile time since we need to use them a lot
  // RAM size limitations mean we can never cache more than 3 levels
  static const std::uint32_t TotalBucketCountByMaxLevel[kMaxLevelsCached];

  // Since BucketPath can be of bucket_level [0, NumBucketLevelsCached-1]
  // and NumBucketLevelsCached <= kMaxLevelsCached(i.e 3), bucket_level in [0,2]
  // i.e path_level in [0,8] i.e path_bits in [0,16] i.e path_as_index [0,2^16]
  static std::uint32_t Hash(const BucketPath &path) {
    unsigned int bucket_level = path.BucketLevel();
    if (bucket_level == 0) {
      return 0;
    }
    return (
        TotalBucketCountByMaxLevel[bucket_level-1] +  // index upto prior level
        static_cast<std::uint32_t>(path.AsIndex(path.Level())));
  }

  const unsigned int kNumLevelsCached;
  // The hash table to buckets, the hashing takes the BucketPath and converts to
  // index. The BucketPath can be of bucket_level [0, NumBucketLevelsCached-1]
  // maps from BucketPath to index to childBuckets where the bucket is stored.
  std::vector<ChildBucketId>        bucket_path_to_index_map;
  geSegmentedArray<Bucket>          childBuckets;
  geSegmentedArray<ChildBucketAddr> childAddrs;
  geSegmentedArray<EntryBucketAddr> entryAddrs;

  ChildBucketCache(unsigned int numLevelsCached)
      : kNumLevelsCached(numLevelsCached),
        // -1 as indexing starts from 0 where as numLevelsCached starts from 1
        bucket_path_to_index_map(TotalBucketCountByMaxLevel[numLevelsCached-1],
                                 kUnknownBucket),
        childBuckets(10 /* 1<<10 -> 1024/segment * 2048b -> 2MB/segment */),
        childAddrs(17 /* 1<<17 -> 131072/segment * 16b -> 2MB/segment */),
        entryAddrs(17 /* 1<<17 -> 131072/segment * 16b -> 2MB/segment */)
  {
    // Seed each of the segmented arrays with an empty record in slot 0.
    // Index 0 is used to represent non-existence, and the emptry records in
    // that slot help enforce that. Ideally nobody would access the 0th
    // slot, but just in case they do, an invalid address will be waiting
    // for them. :-)
    childBuckets.push_back(Bucket());
    childAddrs.push_back(ChildBucketAddr());
    entryAddrs.push_back(EntryBucketAddr());
  }
};


// compute these at compile time since we need to use them a lot
// RAM size limitations mean we can never cache more than 3 levels
const std::uint32_t ChildBucketCache::TotalBucketCountByMaxLevel[kMaxLevelsCached] = {
  1,                  // numLevelsCached == 1, only 1 bucket
  1 + 256,            // numLevelsCached == 2, 1 + 4 ^ kQuadLevelsPerBucket
  1 + 256 + 256*256   // same logic as above, note that each quad level is 2 bit
};



// ****************************************************************************
// ***  ChildBucketCache
// ****************************************************************************
ChildBucketAddr ChildBucketCache::GetChildBucketAddr
(ChildBucketId childId, const BucketPath &subaddr) {
  // The subaddr may be precisely for one of my children.
  // Or it may be for one if it's children.
  // But it must have at least kQuadLevelsPerBucket levels
  assert(subaddr.Level() >= kQuadLevelsPerBucket);
  unsigned int pos = subaddr.AsIndex(kQuadLevelsPerBucket);

  std::uint32_t index = childBuckets[childId].childAddrIndexes[pos];
  if (index != kInvalidAddrIndex) {
    return childAddrs[index];
  }
  return ChildBucketAddr();
}

EntryBucketAddr ChildBucketCache::GetEntryBucketAddr
(ChildBucketId childId, const BucketPath &subaddr) {
  assert(childId != kUnknownBucket);
  assert(childId != kNonExistentBucket);

  // The subaddr must have exactly kQuadLevelsPerBucket levels, otherwise
  // This function should have been called on a different Bucket
  assert(subaddr.Level() == kQuadLevelsPerBucket);
  unsigned int pos = subaddr.AsIndex(kQuadLevelsPerBucket);

  // 0 is a sentinel that means it has no Addr
  std::uint32_t index = childBuckets[childId].entryAddrIndexes[pos];
  if (index != kInvalidAddrIndex) {
    return entryAddrs[index];
  }
  return EntryBucketAddr();
}

ChildBucketId ChildBucketCache::Find(const BucketPath &addr) {
  std::uint32_t pos = Hash(addr);
  assert(pos < bucket_path_to_index_map.size());
  return bucket_path_to_index_map[pos];
}

void ChildBucketCache::MarkBucketNonExistent(const BucketPath &addr) {
  std::uint32_t pos = Hash(addr);
  assert(pos < bucket_path_to_index_map.size());
  // Make sure it's still "unknown" when we mark it as non-existent
  assert(bucket_path_to_index_map[pos] == kUnknownBucket);

  bucket_path_to_index_map[pos] = kNonExistentBucket;
}

ChildBucketId ChildBucketCache::FillBucket(const BucketPath &addr,
                                           const LoadedChildBucket &loadBucket)
{
  Bucket newBucket;
  for (unsigned int i = 0; i < kChildAddrsPerBucket; ++i) {
    if (loadBucket.childAddrs[i]) {
      newBucket.childAddrIndexes[i] = childAddrs.size();
      childAddrs.push_back(loadBucket.childAddrs[i]);
    }
    if (loadBucket.entryAddrs[i]) {
      newBucket.entryAddrIndexes[i] = entryAddrs.size();
      entryAddrs.push_back(loadBucket.entryAddrs[i]);
    }
  }


  // add the new bucket into the appropriate slot
  const ChildBucketId newId = childBuckets.size();
  childBuckets.push_back(newBucket);
  std::uint32_t pos = Hash(addr);
  assert(pos < bucket_path_to_index_map.size());
  // Make sure it's still "unknown" when we mark it as non-existent
  assert(bucket_path_to_index_map[pos] == kUnknownBucket);
  bucket_path_to_index_map[pos] = newId;

  return newId;
}


// ****************************************************************************
// ***  ReaderBase
// ****************************************************************************

// defined in cpp so the call to ~khDeleteGuard<ChildBucketCache>
// will happen here where ChildBucketCache is defined
ReaderBase::~ReaderBase(void) {
}

ReaderBase::ReaderBase(geFilePool &filepool_, const std::string &filename,
                       unsigned int numBucketLevelsCached) :
    NumBucketLevelsCached(numBucketLevelsCached),
    bundleReader(TransferOwnership(new IndexBundleReader(filepool_,
                                                         filename))),
    fileFormatVersion(bundleReader->header.fileFormatVersion),
    filePool(filepool_),
    cache(ChildBucketCache::Create(numBucketLevelsCached)),
    packetFileReaders(bundleReader->PacketFileCount())
{
  // Seed the root child bucket into the cache. This makes the other routines
  // easier to implement. This is used as a sentinel technique.
  if (!bundleReader->header.rootChildAddr) {
    // This is a degenerate index (no records), mark the root as non-existent
    cache->MarkBucketNonExistent(BucketPath());
  } else {
    LittleEndianReadBuffer tmpBuf;
    // fetch w/o CRC since the child bucket format is actually split in two
    // pieces each having their own CRC. The LoadedChildBucket constructor
    // will check both CRCs.
    LoadedChildBucket loaded(bundleReader->header.rootChildAddr,
                             *bundleReader, tmpBuf);
    cache->FillBucket(BucketPath(), loaded);
  }
}

EntryBucketAddr
ReaderBase::GetEntryBucketAddr(const BucketPath &targetEntryBucketPath,
                               EndianReadBuffer &tmpBuf)
{
  if (targetEntryBucketPath.BucketLevel() == 0) {
    return bundleReader->header.rootEntryAddr;
  }

  // the child bucket one bucket level above the targetEntryBucketPath
  // will contain the EntryBucketAddr for our target path
  BucketPath parent = targetEntryBucketPath.BucketPathParent();

  // start the search for the target child bucket by reaching
  // into the cache of child buckets.
  const BucketPath cachedParentPath =
      BucketPath(parent, NumBucketLevelsCached-1);
  const ChildBucketId cachedParentId =
      FetchCachedChildBucketId(cachedParentPath, tmpBuf);

  // FetchCachedChildBucketId should have resolved any "unknown" states
  assert(cachedParentId != kUnknownBucket);

  if (cachedParentId == kNonExistentBucket) {
    // parent doesn't exist => targetEntryBucket doesn't exist
    return EntryBucketAddr();
  }

  if (cachedParentPath == parent) {
    khLockGuard lock(cacheMutex);
    // The cached child bucket is the parent of our entry bucket.
    // Ask the cached child bucket for the entry addr
    return cache->GetEntryBucketAddr(cachedParentId,
        cachedParentPath.QuadtreePathToChild(targetEntryBucketPath));
  }

  // we have to load at least one more child, ask the cache for it's address
  BucketPath grandParent =
      BucketPath(parent, cachedParentPath.BucketLevel() + 1);
  ChildBucketAddr grandAddr;
  {
    khLockGuard lock(cacheMutex);
    grandAddr = cache->GetChildBucketAddr(
        cachedParentId,
        cachedParentPath.QuadtreePathToChild(grandParent));
  }

  // If we still don't have the addr of our target child bucket, keep
  // drilling down until we do. While we do this , we know we only need to
  // load the child addr portion of each child bucket
  while (grandAddr && grandParent != parent) {
    const BucketPath currParentPath = grandParent;
    grandParent = BucketPath(parent, currParentPath.BucketLevel()+1);
    grandAddr = LoadChildBucketToGetChildAddr(
        grandAddr, currParentPath.QuadtreePathToChild(grandParent), tmpBuf);
  }

  if (!grandAddr) {  // parent doesn't exist => targetEntryBucket doesn't exist
    return EntryBucketAddr();
  }

  // now we have the addr of our target child bucket, use it to fetch the
  // target entry bucket addr
  return LoadChildBucketToGetEntryAddr(
      grandAddr, grandParent.QuadtreePathToChild(targetEntryBucketPath),
      tmpBuf);
};


ChildBucketId
ReaderBase::FetchCachedChildBucketId(const BucketPath &targetChildBucketPath,
                                     EndianReadBuffer &tmpBuf)
{
  assert(targetChildBucketPath.BucketLevel() < NumBucketLevelsCached);

  khLockGuard lock(cacheMutex);

  ChildBucketId currChildId = cache->Find(targetChildBucketPath);
  if (currChildId != kUnknownBucket) {
    return currChildId;  // we got a hit, just return it
  }

  // Walk up the tree looking for a parent that's already in the cache.
  BucketPath currChildPath = targetChildBucketPath;
  // while (currChildPath.BucketLevel() > 0) {
  // Since we have preseeded cache with the root child bucket, we are bound to
  // hit that sentinel.
  for (;;) {
    currChildPath = currChildPath.BucketPathParent();
    currChildId = cache->Find(currChildPath);
    if (currChildId != kUnknownBucket) {
      break;  // we found a parent, stop traversing up
    }
  }

  if (currChildId == kNonExistentBucket) {
    return currChildId;  // parent doesn't exist, target cannot exist either
  }

  // Now start walking back down, loading each child bucket as we go. We
  // will walk all the way back down to the target, populating the cache as
  // we go. If we hit an empty child, we'll just populate the rest of the
  // cache entries with empty handles.
  while (currChildPath.BucketLevel() < targetChildBucketPath.BucketLevel()) {
    BucketPath nextChildPath = BucketPath(targetChildBucketPath,
                                          currChildPath.BucketLevel()+1);
    ChildBucketAddr nextAddr = cache->GetChildBucketAddr(
        currChildId, currChildPath.QuadtreePathToChild(nextChildPath));
    if (!nextAddr) {
      // The next level doesn't exist. Mark it non-existent
      cache->MarkBucketNonExistent(nextChildPath);
      return kNonExistentBucket;
    } else {
      LoadedChildBucket loaded;
      { // unlock and load the bucket and then again lock
        khUnlockGuard unlock(cacheMutex);
        loaded.Load(nextAddr, *bundleReader, tmpBuf);
      }
      // double check that we still need to add this to the cache
      // somebody else may have done it while I was unlocked to Load the block
      ChildBucketId newId = cache->Find(nextChildPath);
      if (newId == kUnknownBucket) {
        newId = cache->FillBucket(BucketPath(nextChildPath), loaded);
      }

      // we can't be non-existent, the !nextAddr check above would have
      // handled that case
      assert(newId != kUnknownBucket);
      assert(newId != kNonExistentBucket);

      currChildId = newId;
    }
    currChildPath = nextChildPath;
  }

  return currChildId;
}


ChildBucketAddr ReaderBase::LoadChildBucketToGetChildAddr
(ChildBucketAddr addr, const BucketPath &subpath, EndianReadBuffer &tmpBuf) {
  // The subaddr may be precisely for one of my children.
  // Or it may be for one if it's children.
  // But it must have at least kQuadLevelsPerBucket levels
  assert(subpath.Level() >= kQuadLevelsPerBucket);
  BucketChildSlotType pos = subpath.AsIndex(kQuadLevelsPerBucket);

  BundleAddr bundleAddr = addr.ChildBucketsAddr();
  if (!bundleAddr) {
    // we don't have and child buckets beneath us
    return ChildBucketAddr();
  }

  // just load the children half of the child bucket
  bundleReader->LoadWithCRC(bundleAddr, tmpBuf);

  // walk the buffer to find a match
  ChildBucketAddr ret;
  while (!tmpBuf.AtEnd()) {
    BucketChildSlotType slot;
    tmpBuf >> slot >> ret;
    if (slot == pos) {
      return ret;
    }
  }

  // no match found, return invalid addr
  return ChildBucketAddr();
}

EntryBucketAddr ReaderBase::LoadChildBucketToGetEntryAddr
(ChildBucketAddr addr, const BucketPath &subpath, EndianReadBuffer &tmpBuf) {
  // The subaddr must have exactly kQuadLevelsPerBucket levels, otherwise
  // This function should have been called with a different child bucket
  assert(subpath.Level() == kQuadLevelsPerBucket);
  BucketChildSlotType pos = subpath.AsIndex(kQuadLevelsPerBucket);

  BundleAddr bundleAddr = addr.EntryBucketsAddr();
  if (!bundleAddr) {
    // we don't have any entry buckets beneath us
    return EntryBucketAddr();
  }

  // just load the entry half of the child bucket
  bundleReader->LoadWithCRC(bundleAddr, tmpBuf);

  // walk the buffer to find a match
  EntryBucketAddr ret;
  while (!tmpBuf.AtEnd()) {
    BucketChildSlotType slot;
    tmpBuf >> slot >> ret;
    if (slot == pos) {
      return ret;
    }
  }

  // no match found, return invalid addr
  return EntryBucketAddr();
}

PacketFileReaderBase*
ReaderBase::GetPacketFileReader(std::uint32_t fileNum)
{
  khLockGuard lock(packetfileMutex);
  if (fileNum >= packetFileReaders.size()) {
    throw khSimpleException("INTERNAL ERROR: Invalid fileNum ")
      << fileNum
      << " >= "
      << packetFileReaders.size();
  }
  if (!packetFileReaders[fileNum]) {
    PacketFileReaderBase *tmp = 0;
    {
      // unlock while I create the new reader, it has to do I/O and
      // could take a while
      khUnlockGuard unlock(packetfileMutex);
      tmp = new PacketFileReaderBase(filePool,
                                 bundleReader->GetPacketFile(fileNum));
    }
    if (packetFileReaders[fileNum]) {
      // somebody else already opened this same reader while I was opening it.
      // delete the one I just created.
      delete tmp;
    } else {
      packetFileReaders.Assign(fileNum, tmp);
    }
  }
  return packetFileReaders[fileNum];
}

void ReaderBase::LoadExternalData(const ExternalDataAddress &addr,
                                  EndianReadBuffer &buf) {
  (void)GetPacketFileReader(addr.fileNum)
    ->ReadAtCRC(addr.fileOffset, buf, addr.size);
}

 std::uint32_t ReaderBase::GetPacketExtra(std::uint32_t packetfile_num) {
  return bundleReader->header.GetPacketExtra(packetfile_num);
}


} // namespace readerimpl


// ****************************************************************************
// ***  Reader<Entry> explicit instatiations
// ****************************************************************************

// for testing purposes only
template class Reader<SimpleInsetEntry>;

// used by server
template class Reader<TypedEntry>;

// used by GUI for background rendering
template class Reader<BlendEntry>;


} // namespace geindex
