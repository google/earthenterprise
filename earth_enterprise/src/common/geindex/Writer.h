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

#ifndef COMMON_GEINDEX_WRITER_H__
#define COMMON_GEINDEX_WRITER_H__

#include <vector>
#include "WriterImpl.h"
#include "Entries.h"
#include <common/khFunctor.h>
#include <mttypes/Queue.h>

class geFilePool;
class LittleEndianReadBuffer;
class LittleEndianWriteBuffer;

namespace geindex {


// ****************************************************************************
// ***  Writer<LoadedEntryBucket>
// ***
// *** Only certain LoadedEntryBucket's are instantiated.
// *** See the end of Entries.cpp for the list of valid buckets.
// ****************************************************************************
template <class LoadedEntryBucket>
class Writer {
 public:
  typedef typename LoadedEntryBucket::EntryType Entry;
  typedef writerimpl::EntryBucket<LoadedEntryBucket> EntryBucket;
  typedef writerimpl::ChildBucket<EntryBucket> ChildBucket;
  typedef LittleEndianReadBuffer ReadBuffer;
  typedef LittleEndianWriteBuffer WriteBuffer;
  typedef mttypes::Queue<khFunctor<void> >   DelayedWriteQueue;


  enum WriteMode { FullIndexMode, DeltaIndexMode };
  Writer(geFilePool &filePool, const std::string &fname, WriteMode mode,
         const std::string &desc, std::uint32_t num_write_buffers = 1);
  ~Writer(void);

  // It is the caller's responsibility to manager the threads that process
  // this queue.
  void SetDelayedWriteQueue(DelayedWriteQueue *delayed_write_queue);
  void ClearDelayedWriteQueue(void);

  void Flush(ReadBuffer &tmp_read);
  void Close(void);

  void Put(const QuadtreePath &pos, const Entry &entry,
           ReadBuffer &tmpReadBuf);

  // To delete you still must specify an Entry because we may need to
  // specify which of many entires in a singe quadnode to delete. For
  // example, do delete a verctor packet, you need the Entry to specify
  // which channel to delete. The same WriteMatches logic is used for
  // Delete() and Put(). In fact Delete() just calls Put().
  void Delete(const QuadtreePath &pos, const Entry &entry,
              ReadBuffer &tmpReadBuf);

  std::uint32_t AddExternalPacketFile(const std::string &packetfile);
  void   RemovePacketFile(const std::string &packetfile);
  void   SetPacketExtra(std::uint32_t packetfile_num, std::uint32_t extra);
  std::uint32_t GetPacketExtra(std::uint32_t packetfile_num) const;
 private:
  khDeleteGuard<IndexBundleWriter> bundleWriter;

  khDeleteGuard<ChildBucket> rootChildBucket;
  khDeleteGuard<EntryBucket> rootEntryBucket;
  QuadtreePath last_pos;
};

typedef Writer<CombinedTmeshBucket> CombinedTmeshWriter;
typedef Writer<BlendBucket>         BlendWriter;
typedef Writer<VectorBucket>        VectorWriter;
typedef Writer<UnifiedBucket>       UnifiedWriter;


} // namespace geindex


#endif // COMMON_GEINDEX_WRITER_H__
