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


#ifndef FUSION_GEINDEXGEN_GENERATOR_H__
#define FUSION_GEINDEXGEN_GENERATOR_H__

#include "Todo.h"
#include <mttypes/WaitBaseManager.h>
#include <mttypes/Queue.h>
#include <mttypes/DrainableQueue.h>
#include <mttypes/Semaphore.h>
#include <khEndian.h>

class geFilePool;
class khProgressMeter;

namespace geindexgen {


class BlendGeneratorDef {
  public:
  typedef geindex::BlendWriter Writer;
  typedef BlendPacketReader    Reader;
  typedef BlendTodo            Todo;
};

class VectorGeneratorDef {
  public:
  typedef geindex::VectorWriter Writer;
  typedef VectorPacketReader    Reader;
  typedef VectorTodo            Todo;
};

static const unsigned int kDefaultMergeSessionSize = 500;

template <class Def>
class Generator : public mttypes::WaitBaseManager {
  typedef typename Def::Writer        Writer;
  typedef typename Def::Reader        Reader;
  typedef typename Reader::MergeEntry MergeEntry;
  typedef typename Def::Todo          Todo;
  typedef typename Todo::Stack        Stack;

  typedef mttypes::Semaphore                        MergeSemaphore;
  typedef mttypes::DrainableQueue<khFunctor<void> > CommandQueue;
  typedef mttypes::BatchingDrainableQueuePusher<khFunctor<void> >
  IndexCommandPusher;
  typedef mttypes::BatchingDrainableQueuePuller<khFunctor<void> >
  IndexCommandPuller;

 public:
  Generator(geFilePool &file_pool, const std::string &outdir,
            unsigned int merge_session_size,
            unsigned int queue_size,
            Stack &stack,
            geindex::TypedEntry::TypeEnum desc,
            unsigned int num_writer_threads);
  void DoIndexing(void);

 private:
  void FlushIndexer(void);
  void HandleEntry(const MergeEntry &merge_entry);
  void CloseWriter(void);
  void IndexLoop(void);
  void MergeLoop(void);
  void WriterLoop(void);

  Writer             writer_;
  Todo               todo_;
  const unsigned int         merge_session_size_;
  MergeSemaphore     merge_semaphore_;
  CommandQueue       index_command_queue_;
  CommandQueue       writer_command_queue_;
  khProgressMeter    *progress_;
  const unsigned int         num_writer_threads_;

  typename Writer::ReadBuffer  tmp_read_buf;
};


typedef Generator<BlendGeneratorDef>  BlendGenerator;
typedef Generator<VectorGeneratorDef> VectorGenerator;


} // namespace geindexgen

#endif // FUSION_GEINDEXGEN_GENERATOR_H__
