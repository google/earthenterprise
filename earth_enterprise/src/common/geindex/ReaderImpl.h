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

#ifndef COMMON_GEINDEX_READERIMPL_H__
#define COMMON_GEINDEX_READERIMPL_H__

#include <khGuard.h>
#include "shared.h"
#include <khThread.h>

class geFilePool;
class EndianReadBuffer;
class PacketFileReader;

namespace geindex {

class IndexBundleReader;
class ExternalDataAddress;

namespace readerimpl {


// -1 -> unknown, 0 -> no such bucket, 1 -> index into bucketIds below
typedef int32 ChildBucketId; 
static const ChildBucketId kUnknownBucket = -1;
static const ChildBucketId kNonExistentBucket = 0;
static const uint32 kInvalidAddrIndex = 0;

class ChildBucketCache;

class ReaderBase {
 public:
  ~ReaderBase(void);

  uint32 GetPacketExtra(uint32 packetfile_num);
 protected:
  ReaderBase(geFilePool &filepool_, const std::string &filename,
             uint numBucketLevelsCached);

  ChildBucketId
  FetchCachedChildBucketId(const BucketPath &targetChildBucketPath,
                           EndianReadBuffer &tmpBuf);

  EntryBucketAddr
  GetEntryBucketAddr(const BucketPath &targetEntryBucketPath,
                     EndianReadBuffer &tmpBuf);

  void LoadExternalData(const ExternalDataAddress &addr,
                        EndianReadBuffer &buf);

  const uint NumBucketLevelsCached;
  khDeleteGuard<IndexBundleReader> bundleReader;
  const uint fileFormatVersion;

 private:
  ChildBucketAddr LoadChildBucketToGetChildAddr
  (ChildBucketAddr addr, const BucketPath &subpath, EndianReadBuffer &tmpBuf);

  EntryBucketAddr LoadChildBucketToGetEntryAddr
  (ChildBucketAddr addr, const BucketPath &subpath, EndianReadBuffer &tmpBuf);

  PacketFileReader* GetPacketFileReader(uint32 fileNum);


  geFilePool &filePool;
  khMutex cacheMutex;
  khDeleteGuard<ChildBucketCache> cache;
  khMutex packetfileMutex;
  khDeletingVector<PacketFileReader> packetFileReaders;
};


} // namespace geindex::readerimpl

} // namespace geindex

#endif // COMMON_GEINDEX_READERIMPL_H__
