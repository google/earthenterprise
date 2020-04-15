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

#ifndef COMMON_GEINDEX_WRITERTMPL_H__
#define COMMON_GEINDEX_WRITERTMPL_H__

#include <khEndian.h>
#include <geFilePool.h>
#include <third_party/rsa_md5/crc32.h>
#include "IndexBundleWriter.h"
#include "Entries.h"

namespace geindex {

namespace writerimpl {


// ****************************************************************************
// ***  EntryBucket
// ****************************************************************************
template <class LoadedEntryBucket>
EntryBucket<LoadedEntryBucket>::EntryBucket(
    const BucketPath &path, EntryBucketAddr addr,
    IndexBundle &bundle, EndianReadBuffer &tmpBuf)
  : LoadedEntryBucket(addr, bundle, tmpBuf),
    bucketPath(path),
    prevAddr(addr),
    dirty(false)
{
}

template <class LoadedEntryBucket>
EntryBucket<LoadedEntryBucket>::~EntryBucket(void) {
  if (!std::uncaught_exception()) {
    assert(!dirty);
  }
}


template <class LoadedEntryBucket>
EntryBucketAddr EntryBucket<LoadedEntryBucket>::Close(
    IndexBundleWriter &bundleWriter) {
  if (dirty) {
    IndexBundleWriter::CachedBuffer buf(bundleWriter.GetWriteBuffer());
    *buf << *this;
    prevAddr = bundleWriter.StoreAndReturnBuffer(prevAddr, buf);
    dirty = false;
  }

  return prevAddr;
}


// ****************************************************************************
// ***  ChildBucket
// ****************************************************************************
template <class EntryBucket>
ChildBucket<EntryBucket>::ChildBucket(const BucketPath &path,
                                      ChildBucketAddr addr,
                                      IndexBundle &bundle,
                                      EndianReadBuffer &tmpBuf) :
    LoadedChildBucket(addr, bundle, tmpBuf),
    dirty(false),
    bucketPath(path),
    prevAddr(addr),
    currChildBucket(),
    currEntryBucket()
{
}

template <class EntryBucket>
ChildBucket<EntryBucket>::~ChildBucket(void) {
  if (!std::uncaught_exception()) {
    assert(!dirty);
    assert(!currChildBucket);
    assert(!currEntryBucket);
  }
}

template <class EntryBucket>
void ChildBucket<EntryBucket>::CloseCurrEntryBucket(
    IndexBundleWriter &bundleWriter) {
  BucketChildSlotType slot =
    bucketPath.SubAddrAsChildSlot(currEntryBucket->bucketPath);
  EntryBucketAddr addr = currEntryBucket->Close(bundleWriter);
  if (entryAddrs[slot] != addr) {
    entryAddrs[slot] = addr;
    dirty = true;
  }
  currEntryBucket.clear();
}

template <class EntryBucket>
void ChildBucket<EntryBucket>::CloseCurrChildBucket(
    IndexBundleWriter &bundleWriter) {
  BucketChildSlotType slot =
    bucketPath.SubAddrAsChildSlot(currChildBucket->bucketPath);
  ChildBucketAddr addr = currChildBucket->Close(bundleWriter);
  if (childAddrs[slot] != addr) {
    childAddrs[slot] = addr;
    dirty = true;
  }
  currChildBucket.clear();
}

template <class EntryBucket>
void ChildBucket<EntryBucket>::Put
(const QuadtreePath &path, const Entry &newEntry,
 IndexBundleWriter &bundleWriter, EndianReadBuffer &tmpReadBuf) {
  // since I'm being asked to Put it, it must belong at least to one
  // of my children.
  assert(path.Level() >= bucketPath.Level() + kQuadLevelsPerBucket);
  BucketPath nextPath(path, 1+bucketPath.BucketLevel());

  if (path.Level() < bucketPath.Level() + 2*kQuadLevelsPerBucket) {
    // This belongs to one of my EntryBucket children
    if (currEntryBucket && (currEntryBucket->bucketPath != nextPath)) {
      CloseCurrEntryBucket(bundleWriter);
    }
    if (!currEntryBucket) {
      BucketChildSlotType slot = bucketPath.SubAddrAsChildSlot(nextPath);
      currEntryBucket =
        TransferOwnership(new EntryBucket(nextPath, entryAddrs[slot],
                                          bundleWriter, tmpReadBuf));
    }
    currEntryBucket->Put(path, newEntry);
  } else {
    // This belongs below one of my ChildBuckets
    if (currChildBucket && (currChildBucket->bucketPath != nextPath)) {
      CloseCurrChildBucket(bundleWriter);
    }
    if (!currChildBucket) {
      BucketChildSlotType slot = bucketPath.SubAddrAsChildSlot(nextPath);
      currChildBucket = TransferOwnership
                        (new ChildBucket(nextPath, childAddrs[slot],
                                         bundleWriter, tmpReadBuf));
    }
    currChildBucket->Put(path, newEntry, bundleWriter, tmpReadBuf);
  }
}


template <class EntryBucket>
void ChildBucket<EntryBucket>::CloseSubBuckets(
    IndexBundleWriter &bundleWriter) {
  // flush active children below me
  if (currChildBucket) {
    CloseCurrChildBucket(bundleWriter);
  }

  // flush active entry
  if (currEntryBucket) {
    CloseCurrEntryBucket(bundleWriter);
  }
}


template <class EntryBucket>
ChildBucketAddr ChildBucket<EntryBucket>::Close(
    IndexBundleWriter &bundleWriter) {
  CloseSubBuckets(bundleWriter);

  // flush myself
  if (dirty) {
    IndexBundleWriter::CachedBuffer buf(bundleWriter.GetWriteBuffer());
    ChildBucketAddr newAddrWithSizes = Push(*buf);
    BundleAddr newAddr = bundleWriter.StoreAndReturnBuffer(
        prevAddr.WholeAddr(), buf);
    if (newAddr) {
      assert(newAddr.Size() == newAddrWithSizes.TotalSize());
      prevAddr = ChildBucketAddr(newAddr.Offset(),
                                 newAddrWithSizes.childBucketsSize,
                                 newAddrWithSizes.entryBucketsSize);
    } else {
      prevAddr = ChildBucketAddr();
    }
    dirty = false;
  }

  return prevAddr;
}



} // namespace writerimpl




// ****************************************************************************
// ***  Writer
// ****************************************************************************
template <class EntryBucket>
Writer<EntryBucket>::Writer(geFilePool &filePool, const std::string &fname,
                            WriteMode mode, const std::string &desc,
                            std::uint32_t num_write_buffers) :
    bundleWriter(TransferOwnership(
                     new IndexBundleWriter(filePool, fname,
                                           (mode==DeltaIndexMode), desc,
                                           EntryBucket::isSingle,
                                           num_write_buffers))),
    rootChildBucket(),
    rootEntryBucket(),
    last_pos()
{
  assert(num_write_buffers > 0);
  
  ReadBuffer tmpBuf;
  rootChildBucket = TransferOwnership
                    (new ChildBucket(BucketPath(),
                                     bundleWriter->header.rootChildAddr,
                                     *bundleWriter, tmpBuf));
  rootEntryBucket =
    TransferOwnership(new EntryBucket(BucketPath(),
                                      bundleWriter->header.rootEntryAddr,
                                      *bundleWriter, tmpBuf));
}

template <class EntryBucket>
Writer<EntryBucket>::~Writer(void) {
}

#ifndef SINGLE_THREAD
template <class EntryBucket>
void Writer<EntryBucket>::SetDelayedWriteQueue(
    DelayedWriteQueue *delayed_write_queue) {
  bundleWriter->SetDelayedWriteQueue(delayed_write_queue);
}

template <class EntryBucket>
void Writer<EntryBucket>::ClearDelayedWriteQueue(void) {
  bundleWriter->ClearDelayedWriteQueue();
}
#endif


template <class EntryBucket>
void Writer<EntryBucket>::Flush(ReadBuffer &tmp_read) {
  rootChildBucket->CloseSubBuckets(*bundleWriter);
  last_pos = QuadtreePath();
}

template <class EntryBucket>
void Writer<EntryBucket>::Close(void) {
  if (rootEntryBucket->IsDirty()) {
    bundleWriter->header.rootEntryAddr =
      rootEntryBucket->Close(*bundleWriter);
    rootEntryBucket.clear();
  }
  bundleWriter->header.rootChildAddr =
    rootChildBucket->Close(*bundleWriter);
  rootChildBucket.clear();
  last_pos = QuadtreePath();

  bundleWriter->Close();
  bundleWriter.clear();

}

template <class EntryBucket>
void Writer<EntryBucket>::Put(const QuadtreePath &pos,
                              const Entry &entry,
                              ReadBuffer &tmpReadBuf) {
  // Enforce preorder on sequence of inputs
  if (pos < last_pos) {
    throw khSimpleException("geindex::Writer::Put: quadtree path (")
      << pos.AsString() << ") precedes last path ("
      << last_pos.AsString() << ")";
  } else {
    last_pos = pos;
  }

  if (rootEntryBucket->bucketPath.Contains(pos)) {
    rootEntryBucket->Put(pos, entry);
  } else {
    rootChildBucket->Put(pos, entry, *bundleWriter, tmpReadBuf);
  }
}

template <class EntryBucket>
 std::uint32_t Writer<EntryBucket>::AddExternalPacketFile(
    const std::string &packetfile) {
  return bundleWriter->header.AddPacketFile(packetfile);
}

template <class EntryBucket>
void Writer<EntryBucket>::RemovePacketFile(const std::string &packetfile) {
  bundleWriter->header.RemovePacketFile(packetfile);
}

template <class EntryBucket>
void Writer<EntryBucket>::SetPacketExtra(std::uint32_t packetfile_num, std::uint32_t extra) {
  bundleWriter->header.SetPacketExtra(packetfile_num, extra);
}

template <class EntryBucket>
 std::uint32_t Writer<EntryBucket>::GetPacketExtra(std::uint32_t packetfile_num) const {
  return bundleWriter->header.GetPacketExtra(packetfile_num);
}


template <class EntryBucket>
void Writer<EntryBucket>::Delete(const QuadtreePath &pos, const Entry &entry,
                                 ReadBuffer &tmpReadBuf) {
  // Just in case it already isn't, force the dataAddress portion of the
  // entry to be invalid. That's Put's signal to perform a delete.
  Entry delEntry = entry;
  delEntry.dataAddress = ExternalDataAddress();

  Put(pos, delEntry, tmpReadBuf);
}


} // namespace geindex



#endif // COMMON_GEINDEX_WRITERTMPL_H__
