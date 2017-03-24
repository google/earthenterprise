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

#ifndef COMMON_MTTYPES_WAITBASE_H__
#define COMMON_MTTYPES_WAITBASE_H__

#include <common/base/macros.h>
#include <khMTTypes.h>

class QString;

namespace mttypes {

class WaitBaseManager;

class WaitBase {
  friend class WaitBaseManager;

 public:
  class LockGuard : public khLockGuard {
   public:
    LockGuard(WaitBase *self);
  };
  class NoThrowLockGuard : public khLockGuard {
   public:
    NoThrowLockGuard(WaitBase *self) : khLockGuard(self->mutex_) { }
  };

  WaitBase(void);

  // will lock mutex, so it must not already be locked
  void Abort(const QString &msg);
  void Suspend(void);
  void Resume(void);

  // must already be locked before calling
  void LockedWait(void);
  void LockedSignalOne(void);
  void LockedBroadcastAll(void);
  inline bool LockedIsSuspended(void) const { return suspended_; }
  inline bool LockedIsAborted(void) const   { return aborted_; }

 private:
  khMutex   mutex_;
  khCondVar condvar_;
  bool      aborted_;
  bool      suspended_;
  WaitBaseManager *manager_;

  void AbortFromManager(void);
  inline void SetManager(WaitBaseManager *manager) {
    manager_ = manager;
  }
  inline void ClearManager(void) {
    manager_ = 0;
  }

  DISALLOW_COPY_AND_ASSIGN(WaitBase);
};


} // mttypes


#endif // COMMON_MTTYPES_WAITBASE_H__
