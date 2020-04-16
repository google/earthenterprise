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

#ifndef COMMON_GEINDEX_INDEXBUNDLEWRITER_H__
#define COMMON_GEINDEX_INDEXBUNDLEWRITER_H__

#include "IndexBundle.h"
#include <khGuard.h>
#include <mttypes/Queue.h>
#include <khFunctor.h>

class FileBundleWriter;
class LittleEndianWriteBuffer;

namespace geindex {

class BundleFreePool {
 public:
  BundleFreePool(std::uint32_t minBlockSize = 32);

  void Add(const BundleAddr &addr);
  BundleAddr Get(std::uint32_t size);
  inline std::uint64_t TotalFreeSpace(void) const { return totalFreeSpace; }
  inline std::uint64_t TotalWastedSpace(void) const { return wastedSpace; }

 private:
  const std::uint32_t kMinBlockSize;

  std::uint64_t totalFreeSpace;
  std::uint64_t wastedSpace;
};


class IndexBundleWriter : public IndexBundle {
 public:
#ifndef SINGLE_THREAD
  typedef mttypes::Queue<khFunctor<void> >   DelayedWriteQueue;
  typedef khTransferGuard<EndianWriteBuffer> CachedBuffer;
  typedef mttypes::Queue<CachedBuffer>       BufferCache;
#else
  typedef khTransferGuard<EndianWriteBuffer> CachedBuffer;
  typedef CachedBuffer                       BufferCache;
#endif

  IndexBundleWriter(geFilePool &filePool, const std::string &fname,
                    bool deltaMode, const std::string &desc,
                    bool slotsAreSingle,
                    std::uint32_t num_write_buffers);
  ~IndexBundleWriter(void);

  // caller's responsibility to make sure CRC is already added as necessary
  // This will try to reuse the previous block in the bundle. Failing that
  // it will try to get a slot from the free pool. Finally it will append to
  // the end of the bundle.
  // buf.size() is used to know how much to write
  // The buffer must have come from a request to GetWriteBuffer()
  // We return it to the cache when we are done.
  // This routine can also delay the write if SetDelayedWriteQueue has been
  // set.
  BundleAddr StoreAndReturnBuffer(const BundleAddr &prevAddr,
                                  CachedBuffer buf);

  void Close(void);

#ifndef SINGLE_THREAD
  // It is the caller's responsibility to manager the threads that process
  // this queue.
  void SetDelayedWriteQueue(DelayedWriteQueue *delayed_write_queue) {
    delayed_write_queue_ = delayed_write_queue;
  }
  void ClearDelayedWriteQueue(void) {
    delayed_write_queue_ = 0;
  }
#endif

  CachedBuffer GetWriteBuffer(void);
  void ReturnWriteBuffer(CachedBuffer buf);

 private:
  void WriteAndReturn(const BundleAddr &addr, CachedBuffer buf);
  BundleAddr GetNewAddr(const BundleAddr &preAddr, std::uint32_t newSize);

  FileBundleWriter &bundleWriter;
  BundleFreePool freePool;
  geFilePool &filePool;
  const std::string &fname;
#ifndef SINGLE_THREAD
  DelayedWriteQueue *delayed_write_queue_;
#endif

  BufferCache buffer_cache_;
};

} // namespace geindex

#endif // COMMON_GEINDEX_INDEXBUNDLEWRITER_H__
