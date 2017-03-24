// Copyright 2017 Google Inc.
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

#include <khAbortedException.h>
#include "WaitBase.h"
#include "WaitBaseManager.h"

namespace mttypes {


WaitBase::LockGuard::LockGuard(WaitBase *self) :
    khLockGuard(self->mutex_) {
  if (self->aborted_) {
    throw khAbortedException();
  }
}

WaitBase::WaitBase(void) :
    mutex_(),
    condvar_(),
    aborted_(false),
    suspended_(false),
    manager_(0)
{
}

void WaitBase::LockedWait(void) {
  condvar_.wait(mutex_);
  if (aborted_) {
    throw khAbortedException();
  }
}

void WaitBase::LockedSignalOne(void) {
  condvar_.signal_one();
}

void WaitBase::LockedBroadcastAll(void) {
  condvar_.broadcast_all();
}

void WaitBase::Abort(const QString &msg) {
  // NOTE: Don't use our own LockGuard, it will throw if we're already aborted.
  // We want it to be valid to Abort something that's already been aborted.
  // It makes our life easier everywhere else
  khLockGuard lock(mutex_);

  // notify my manager
  if (!aborted_ && manager_) {
    manager_->Abort(msg);
  }

  aborted_ = true;

  // Wake up everybody
  LockedBroadcastAll();
}

void WaitBase::AbortFromManager(void) {
  // NOTE: Don't use our own LockGuard, it will throw if we're already aborted.
  // We want it to be valid to Abort something that's already been aborted.
  // It makes our life easier everywhere else
  khLockGuard lock(mutex_);
  aborted_ = true;

  // Wake up everybody
  LockedBroadcastAll();
}


void WaitBase::Suspend(void) {
  LockGuard lock(this);
  assert(!suspended_);
  suspended_ = true;
}

void WaitBase::Resume(void) {
  LockGuard lock(this);
  assert(suspended_);
  suspended_ = false;

  // Wake up everybody
  LockedBroadcastAll();
}


} // namespace mttypes
