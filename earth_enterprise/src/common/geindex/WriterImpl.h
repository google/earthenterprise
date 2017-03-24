/*
 * Copyright 2017 Google Inc.
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

#ifndef COMMON_GEINDEX_WRITERIMPL_H__
#define COMMON_GEINDEX_WRITERIMPL_H__

#include "shared.h"
#include <khGuard.h>

namespace geindex {

class IndexBundle;
class IndexBundleWriter;

namespace writerimpl {


// ****************************************************************************
// ***  EntryBucket
// ***
// ***  Write wrapper around LoadedEntryBucket (defined in Entries.h)
// ****************************************************************************
template <class LoadedEntryBucket>
class EntryBucket : public LoadedEntryBucket {
 public:
  typedef typename LoadedEntryBucket::EntryType EntryType;

  BucketPath    bucketPath;
  EntryBucketAddr prevAddr;

  inline bool IsDirty(void) const { return dirty; }

  inline void Put(const QuadtreePath &path, const EntryType &newEntry) {
    LoadedEntryBucket::Put(bucketPath.SubAddrAsEntrySlot(path), newEntry);
    dirty = true;
  }

  // methods defined in WriterTmpl.h
  EntryBucket(const BucketPath &path, EntryBucketAddr addr,
              IndexBundle &bundle, EndianReadBuffer &tmpBuf);
  ~EntryBucket(void);
  EntryBucketAddr Close(IndexBundleWriter &bundleWriter);
 private:
  bool          dirty;
};


// ****************************************************************************
// ***  ChildBucket
// ***
// ***  Write wrapper around LoadedChildBucket (defined in shared.h)
// ****************************************************************************
template <class EntryBucket>
class ChildBucket : public LoadedChildBucket {
  typedef typename EntryBucket::EntryType Entry;

  bool            dirty;
  BucketPath      bucketPath;
  ChildBucketAddr prevAddr;

 public:
  // methods defined in WriterTmpl.h
  void Put(const QuadtreePath &path, const Entry &newEntry,
           IndexBundleWriter &bundleWriter,
           EndianReadBuffer &tmpReadBuf);

  ChildBucket(const BucketPath &path, ChildBucketAddr addr,
              IndexBundle &bundle,
              EndianReadBuffer &tmpBuf);
  ~ChildBucket(void);
  void CloseSubBuckets(IndexBundleWriter &bundleWriter);
  ChildBucketAddr Close(IndexBundleWriter &bundleWriter);
 private:
  void CloseCurrEntryBucket(IndexBundleWriter &bundleWriter);
  void CloseCurrChildBucket(IndexBundleWriter &bundleWriter);

  khDeleteGuard<ChildBucket> currChildBucket;
  khDeleteGuard<EntryBucket> currEntryBucket;
};


} // namespace writerimpl

} // namespace geindex



#endif // COMMON_GEINDEX_WRITERIMPL_H__
