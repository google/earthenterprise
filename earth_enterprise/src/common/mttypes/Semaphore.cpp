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

#include "Semaphore.h"

namespace mttypes {


Semaphore::Semaphore(std::uint32_t avail) :
    WaitBase(),
    avail_(avail)
{
}

void Semaphore::Acquire(std::uint32_t num) {
  LockGuard lock(this);
  while (1) {
    if (LockedOKToAcquire()) {
      if (avail_ >= num) {
        avail_ -= num;
        return;
      }
    }

    LockedWait();
  }
}

void Semaphore::Release(std::uint32_t num) {
  LockGuard lock(this);
  avail_ += num;
  if (LockedOKToAcquire()) {
    if (num == 1) {
      LockedSignalOne();
    } else {
      LockedBroadcastAll();
    }
  }
}

bool Semaphore::LockedOKToAcquire(void) const {
  return !LockedIsSuspended();
}


} // namespace mttypes
