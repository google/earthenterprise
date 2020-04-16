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



#ifndef _khThread_h_
#define _khThread_h_

#include <sys/time.h>
#include <assert.h>
#include <pthread.h>
#include "khGuard.h"
#include "khFunctor.h"
#include "performancelogger.h"

// This is a statically initializeable base class for khMutex. You use this
// only for file global variables that need to be valid even before a
// contructor may be called. Or in case the global contructors don't get
// called.
//
// Normally you'll just use khMutex
//
// Example:
//   class Foo {
//     static khMutexBase lock;
//   };
//   Foo::lock = KH_MUTEX_BASE_INITIALIZER;
class khMutexBase {
 public:
  pthread_mutex_t mutex;

  void lock(void) { Lock(); }
  void Lock(void);
  void unlock(void) { Unlock(); }
  void Unlock(void);
  bool trylock(void) { return TryLock(); }
  bool TryLock(void);
  bool timedTryLock(unsigned int secToWait) { return TimedTryLock(secToWait); }
  bool TimedTryLock(unsigned int secToWait);
};
#define KH_MUTEX_BASE_INITIALIZER {PTHREAD_MUTEX_INITIALIZER}
#define kH_MUTEX_BASE_RECURSIVE {PTHREAD_MUTEX_RECURSIVE}


// Simple non-recursive, non-checking mutex
class khMutex : public khMutexBase
{
 public:
  khMutex(void);
  ~khMutex(void);
};


// Special type of mutex that is never destroyed
// This is for the rare case when a mutex needs to remain locked forever
// (like when blocking detached threads from doing any work while
//  the application exits)
// Note: this WILL leak resources so only use for singleton type objects
// whose lifetime matches that of the entire application
class khNoDestroyMutex : public khMutexBase {
 public:
  khNoDestroyMutex(void);
  ~khNoDestroyMutex(void) {}
};

class khTimedMutexException : public std::runtime_error
{
 public:
  khTimedMutexException(const std::string &msg) :
    std::runtime_error(msg) { }
  virtual ~khTimedMutexException(void) throw() { }
};

class khLockGuard {
  khMutexBase &mutex;
 public:
  khLockGuard(khMutexBase &mutex_) : mutex(mutex_) {
    mutex.Lock();
  }
  khLockGuard(khMutexBase &mutex_, unsigned int waitTime) : mutex(mutex_) {
    if (!mutex.TimedTryLock(waitTime)) {
      std::string errMsg = "Failed to acquire mutex within " + std::to_string(waitTime) +  " seconds";
      throw khTimedMutexException(errMsg);
    }
  }
  khLockGuard() = delete;
  khLockGuard(const khLockGuard&) = delete;
  khLockGuard(khLockGuard&&) = delete;
  ~khLockGuard(void) {
    mutex.Unlock();
  }
};

class khUnlockGuard {
  khMutexBase &mutex;
 public:
  khUnlockGuard(khMutexBase &mutex_) : mutex(mutex_) {
    mutex.Unlock();
  }
  khUnlockGuard() = delete;
  khUnlockGuard(const khUnlockGuard&) = delete;
  khUnlockGuard(khUnlockGuard&&) = delete;
  ~khUnlockGuard(void) {
    mutex.Lock();
  }
};

class khCondVar {
  pthread_cond_t condvar;

  // private and unimplemented
  khCondVar(const khCondVar &);
  khCondVar& operator=(const khCondVar &);
 public:
  khCondVar(void);
  ~khCondVar(void);
  void signal_one(void);
  void broadcast_all(void);
  void wait(khMutexBase &mutex);
};

class khThreadBase {
  pthread_t tid;
  bool wantAbort_;
  bool started;
  khMutex   lock;
  struct itimerval profile_timer;
  static void* thread_start(void *obj);

  // private and unimplemented
  khThreadBase(const khThreadBase &);
  khThreadBase& operator=(const khThreadBase &);

 protected:
  virtual ~khThreadBase(void) {
    BEGIN_PERF_LOGGING(timingLogger, "khThreadBase::~khThreadBase", "thread");
    if (needJoin()) {
      setWantAbort();
      join();
    }
    END_PERF_LOGGING(timingLogger);
  }
  khThreadBase(void) : tid(0), wantAbort_(false), started(false) { }
  bool wantAbort(void) {
    khLockGuard guard(lock);
    return wantAbort_;
  }
  virtual void thread_main(void) = 0;

 public:
  bool run(void);
  bool needJoin(void) const { return tid != 0; }
  void join(void);

  // Profiling timer management.  Call PrepareProfiling from parent
  // just before starting the thread, call BeginProfiling at start of
  // new thread.  These are no-ops if thread profiling is not enabled.
  static void PrepareProfiling(struct itimerval *profile_timer); 
  static void BeginProfiling(const struct itimerval &profile_timer);

  // only call abort if you know the thread hasn't self destructed
  void setWantAbort(void) {
    khLockGuard guard(lock);
    wantAbort_ = true;
  }
};


class khThread : public khThreadBase
{
 protected:
  khFunctor<void> fun;
  virtual void thread_main(void) { fun(); }

 public:
  khThread(const khFunctor<void> &fun_) : fun(fun_) { }
  ~khThread(void) {
    BEGIN_PERF_LOGGING(timingLogger, "khThread::~khThread", "thread");
    if (needJoin()) {
      setWantAbort();
      join();
    }
    END_PERF_LOGGING(timingLogger);
  }
};

// DetachedThreadParams - envelope for data passed to detached thread

struct DetachedThreadParams {
  DetachedThreadParams(khTransferGuard<khFunctor<void> > fun_,
                       const struct itimerval &profile_timer_)
      : fun(fun_),
        profile_timer(profile_timer_) {
  }
  khDeleteGuard<khFunctor<void> > fun;
  struct itimerval profile_timer;
};

void* detached_thread_start(void* arg);
bool RunInDetachedThread(const khFunctor<void> &fun);

#endif /* _khThread_h_ */
