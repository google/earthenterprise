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

#ifndef COMMON_MTTYPES_DRAINABLEQUEUE_H__
#define COMMON_MTTYPES_DRAINABLEQUEUE_H__

#include "Queue.h"

namespace mttypes {

template <class T>
class DrainableQueue : public Queue<T> {
 public:
  typedef typename Queue<T>::Item       Item;
  typedef typename Queue<T>::PullHandle PullHandle;
  typedef typename Queue<T>::LockGuard  LockGuard;

  DrainableQueue(void) : Queue<T>() { }

  // t will be pushed. Queue will be drained just prior to popping t.
  void PushWithDrain(const T &t) {
    LockGuard guard(this);
    Queue<T>::LockedPush(Item(t, Item::DrainFlag));
  }
  void PushDoneWithDrain(void) {
    LockGuard lock(this);
    LockedPush(Item(Item::DoneFlag | Item::DrainFlag));
  }
  virtual void ReleaseOldAndPull(PullHandle &h) {
    LockGuard lock(this);
    if (h && h->HasBackref()) {
      Queue<T>::LockedReleasePullHandle(&*h);
    }
    Queue<T>::LockedPull(h, true /* want_backref */);
  }

};


// ****************************************************************************
// ***  WARNING: This class is intentionally NOT MT safe
// ***
// ***  Designed to be used by a single thread to batch the pushes into
// ***  the MT safe version above
// ****************************************************************************
template <class T>
class BatchingDrainableQueuePusher : public BatchingQueuePusher<T> {
 public:
  typedef typename Queue<T>::Item       Item;

  BatchingDrainableQueuePusher(std::uint32_t batch_size, DrainableQueue<T> &queue) :
      BatchingQueuePusher<T>(batch_size, queue)
  {
  }

  void PushWithDrain(const T &t) {
    BatchingQueuePusher<T>::AddToBatch(Item(t, Item::DrainFlag));
  }
  void PushDoneWithDrain(void) {
    AddToBatch(Item(Item::DoneFlag | Item::DrainFlag));
    this->Flush();
  }
};



// ****************************************************************************
// ***  WARNING: This clas is intentionally NOT MT safe
// ***
// ***  Designed to be used by a single thread to batch the pulls from
// ***  the MT safe version above
// ****************************************************************************
template <class T>
class BatchingDrainableQueuePuller : public BatchingQueuePuller<T> {
 public:
  BatchingDrainableQueuePuller(std::uint32_t batch_size, DrainableQueue<T> &queue) :
      BatchingQueuePuller<T>(batch_size, queue)
  {
    this->want_backref_ = true;
  }
};





} // namespace mttypes


#endif // COMMON_MTTYPES_DRAINABLEQUEUE_H__
