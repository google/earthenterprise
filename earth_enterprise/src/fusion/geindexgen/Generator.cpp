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



#include "Generator.h"

#include <khAbortedException.h>
#include <khProgressMeter.h>
#include <khEndian.h>
#include <merge/merge.h>

namespace geindexgen {


// ****************************************************************************
// ***  Simple adapter to convert a reader to a merge source
// ***
// ***  NOT MT SAFE - should only be accessed by one thread
// ****************************************************************************
template <class Reader>
class ReaderMergeSource :
      public MergeSource<typename Reader::MergeEntry> {
  typedef typename Reader::MergeEntry MergeEntry;
 public:
  ReaderMergeSource(Reader *reader, const std::uint32_t cache_size) :
      MergeSource<MergeEntry>(reader->Name()),
      reader_(reader),
      valid_(true),
      remaining_(reader_->NumPackets()),
      cache_size_(cache_size),
      entries_(cache_size_),
      curr_entry_(0),
      num_entries_(0)
  {
    assert(remaining_ > 0);
    (void) Advance();
  }
  virtual ~ReaderMergeSource(void) { }
  virtual const MergeEntry &Current(void) const {
    if (valid_) {
      return entries_[curr_entry_];
    } else {
      throw khSimpleException("ReaderMergeSource::Current: no valid value")
        << " in source " << this->name();
    }
  }
  virtual bool Advance() {
    if (valid_  &&  remaining_ > 0) {
      if (++curr_entry_ >= num_entries_) {
        curr_entry_ = 0;
        num_entries_ = reader_->ReadNextN(&entries_[0], cache_size_,
                                          tmp_read_buffer);
        assert(num_entries_ > 0);
      }
      --remaining_;
    } else {
      valid_ = false;
    }
    return valid_;
  }
  virtual void Close() {
    reader_->Close();
    valid_ = false;
    remaining_ = 0;
  }

 private:
  Reader *reader_;
  bool        valid_;
  std::uint64_t      remaining_;
  std::uint32_t      cache_size_;
  std::vector<MergeEntry> entries_;
  std::uint32_t      curr_entry_;
  std::uint32_t      num_entries_;
  LittleEndianReadBuffer tmp_read_buffer;

  DISALLOW_COPY_AND_ASSIGN(ReaderMergeSource);
};




// ****************************************************************************
// ***  Generator
// ****************************************************************************
template <class Def>
Generator<Def>::Generator(geFilePool &file_pool,
                          const std::string &outdir,
                          unsigned int merge_session_size,
                          unsigned int queue_size,
                          Stack &stack,
                          geindex::TypedEntry::TypeEnum desc,
                          unsigned int num_writer_threads) :
    writer_(file_pool, outdir, Writer::FullIndexMode,
            ToString(desc),
            num_writer_threads * 3 /* num cached write buffers */),
    todo_(writer_, file_pool, stack),
    merge_session_size_(merge_session_size),
    merge_semaphore_(queue_size),
    index_command_queue_(),
    writer_command_queue_(),
    progress_(0),
    num_writer_threads_(num_writer_threads),
    tmp_read_buf(8096)
{
  AddWaitBase(&merge_semaphore_);
  AddWaitBase(&index_command_queue_);
  AddWaitBase(&writer_command_queue_);
  writer_.SetDelayedWriteQueue(&writer_command_queue_);
}


template <class Def>
void Generator<Def>::FlushIndexer(void) {
  writer_.Flush(tmp_read_buf);

  // drain the write queue and then have it send the resume to the merger
  writer_command_queue_.PushWithDrain(
      khFunctor<void>(std::mem_fun(&mttypes::Semaphore::Resume),
                      &merge_semaphore_));
}

template <class Def>
void Generator<Def>::HandleEntry(const MergeEntry &merge_entry) {
  switch (merge_entry.op_) {
    case Reader::DeleteOp:
      writer_.Delete(merge_entry.path_, merge_entry.entry_, tmp_read_buf);
      break;
    case Reader::PutOp:
      writer_.Put(merge_entry.path_, merge_entry.entry_, tmp_read_buf);
      break;
  }
  progress_->increment();
}

template <class Def>
void Generator<Def>::WriterLoop(void) {
  CommandQueue::PullHandle next;

  while (1) {
    // this will block until something else is available
    writer_command_queue_.ReleaseOldAndPull(next);

    if (!next) {
      // we're all done, just return
      return;
    }

    // invoke the command
    next->Data()();
  };
}


template <class Def>
void Generator<Def>::IndexLoop(void) {
  const unsigned int index_pull_batch = 50;

  CommandQueue::PullHandle next;

  IndexCommandPuller index_command_puller(index_pull_batch,
                                          index_command_queue_);

  while (1) {
    // this will block until something else is available
    index_command_puller.ReleaseOldAndPull(next);

    if (!next) {
      // Send Done w/ drain to writer queue
      // and then return
      writer_command_queue_.PushDone();
      return;
    }

    // invoke the command
    next->Data()();

    merge_semaphore_.Release(1);
  };
}


template <class Def>
void Generator<Def>::MergeLoop(void) {
  const unsigned int merge_acquire_batch = 50;

  unsigned int num_merged = 0;

  IndexCommandPusher index_command_pusher(merge_acquire_batch,
                                          index_command_queue_);

  while (num_merged < todo_.readers_.size()) {

    // create and populate a Merger with the readers we're going to use for
    // this batch.
    Merge<MergeEntry> merger("Add Merger");
    do {
      merger.AddSource(
          TransferOwnership(
              new ReaderMergeSource<Reader>(todo_.readers_[num_merged],
                                            50)));
      ++num_merged;
    } while ((num_merged < todo_.readers_.size()) &&
             (num_merged % merge_session_size_));

    // spin through the merger adding tasks to the indexing thread's queue
    merger.Start();
    {
      mttypes::BatchingSemaphoreAcquirer semaphore(merge_semaphore_,
                                                   merge_acquire_batch);
      do {
        semaphore.Acquire();

        // queue up the operation for the indexing thread
        index_command_pusher.Push(
            khFunctor<void>(std::mem_fun(&Generator::HandleEntry),
                            this, merger.Current()));

      } while (merger.Advance());
    }
    merger.Close();


    // suspend my own semaphore and tell the indexer to flush
    // he will resume my queue when he's finished
    merge_semaphore_.Suspend();
    index_command_pusher.PushWithDrain(
        khFunctor<void>(std::mem_fun(&Generator::FlushIndexer),
                        this));
    index_command_pusher.Flush();
  }

  // tell the indexer that he's done
  index_command_pusher.PushDone();
  index_command_pusher.Flush();

  // now just fall off the bottom of the function
}


template <class Def>
void Generator<Def>::DoIndexing(void) {
  progress_ = new khProgressMeter(todo_.TotalTileVisits(), "checks");
  khDeleteGuard<khProgressMeter> guard(TransferOwnership(progress_));

  {
    mttypes::ManagedThread merger(
        *this,
        khFunctor<void>(std::mem_fun(&Generator::MergeLoop), this));
    merger.run();

    mttypes::ManagedThread indexer(
        *this,
        khFunctor<void>(std::mem_fun(&Generator::IndexLoop), this));
    indexer.run();


    khDeletingVector<mttypes::ManagedThread> write_threads;
    for (unsigned int i = 0; i < num_writer_threads_; ++i) {
      write_threads.push_back(
          TransferOwnership(
              new mttypes::ManagedThread(
                  *this,
                  khFunctor<void>(
                      std::mem_fun(&Generator::WriterLoop),
                      this))));
      write_threads.back()->run();
    }

    // Leaving the scope will join with the threads. They are both guaranteed
    // to exit, either sucessfully or with an abort
  }

  writer_.ClearDelayedWriteQueue();

  if (IsAborted()) {
    // One of my threads threw an exception. We should throw too.
    throw khAbortedException();
  }

  writer_.Close();
}


// ****************************************************************************
// ***  Explicit instantiation
// ****************************************************************************
template class Generator<BlendGeneratorDef>;
template class Generator<VectorGeneratorDef>;



} // namespace geindexgen
