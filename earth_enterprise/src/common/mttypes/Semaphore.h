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

#ifndef COMMON_MTTYPES_SEMAPHORE_H__
#define COMMON_MTTYPES_SEMAPHORE_H__

#include "WaitBase.h"

namespace mttypes {

class Semaphore : public WaitBase {
 public:
  Semaphore(uint32 avail);

  void Acquire(uint32 num = 1);
  void Release(uint32 num = 1);

 private:
  bool LockedOKToAcquire(void) const;

  uint32 avail_;
  bool   done_;
};


// ****************************************************************************
// ***  NOTE: NOT threadsafe - nor should it be
// ***
// ***  The entire purpose of this class is to reduce the number of lock/unlock
// ***  cycles by acquiring more than one resource from the samaphore with each
// ***  lock.
// ****************************************************************************
class BatchingSemaphoreAcquirer {
 public:
  BatchingSemaphoreAcquirer(Semaphore &semaphore, uint32 batch_size) :
      semaphore_(semaphore),
      batch_size_(batch_size),
      num_left_(0)
  {
    assert(batch_size_ > 0);
  }
  ~BatchingSemaphoreAcquirer(void) {
    if (num_left_) {
      semaphore_.Release(num_left_);
    }
  }
  void Acquire(void) {
    if (num_left_ == 0) {
      num_left_ = batch_size_;
      semaphore_.Acquire(num_left_);
    }
    --num_left_;
  }

 private:
  Semaphore &semaphore_;
  const uint32 batch_size_;
  uint32 num_left_;
};



} // namespace mttypes

#endif // COMMON_MTTYPES_SEMAPHORE_H__
