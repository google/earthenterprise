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


#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "khGuard.h"
#include "khFunctor.h"
#include "khThread.h"

void khMutexBase::Lock(void) {
#ifndef NDEBUG
  // if this wasn't properly initialized, we'll get an error
  int err = pthread_mutex_lock(&mutex);
  assert(!err);
#else
  pthread_mutex_lock(&mutex);
#endif
}

void khMutexBase::Unlock(void) {
#ifndef NDEBUG
  // if this wasn't properly initialized, we'll get an error
  int err = pthread_mutex_unlock(&mutex);
  assert(!err);
#else
  pthread_mutex_unlock(&mutex);
#endif
}

bool khMutexBase::TryLock(void) {
  // if this wasn't properly initialized, we'll get an error
  int err = pthread_mutex_trylock(&mutex);
  assert(err != EINVAL);
  return (err == 0);
}

khMutex::khMutex(void) {
  // always returns 0
  (void)pthread_mutex_init(&mutex, NULL /* simple, fast mutex */);
}

khMutex::~khMutex(void) {
#ifndef NDEBUG
  int err = pthread_mutex_destroy(&mutex);
  assert(!err);
#else
  pthread_mutex_destroy(&mutex);
#endif
}

khNoDestroyMutex::khNoDestroyMutex(void) {
  // always returns 0
  (void)pthread_mutex_init(&mutex, NULL /* simple, fast mutex */);
}

khCondVar::khCondVar(void) {
  // always returns 0
  (void)pthread_cond_init(&condvar, NULL);
}

khCondVar::~khCondVar(void) {
#ifndef NDEBUG
  int err = pthread_cond_destroy(&condvar);
  assert(!err);
#else
  pthread_cond_destroy(&condvar);
#endif
}

void khCondVar::signal_one(void) {
  // always returns 0
  (void)pthread_cond_signal(&condvar);
}
void khCondVar::broadcast_all(void) {
  // always returns 0
  (void)pthread_cond_broadcast(&condvar);
}
void khCondVar::wait(khMutexBase &mutex) {
  // always returns 0
  (void)pthread_cond_wait(&condvar, &mutex.mutex);
}

void* khThreadBase::thread_start(void *obj) {
  khThreadBase *thread_base = reinterpret_cast<khThreadBase*>(obj);
  try {
    BeginProfiling(thread_base->profile_timer);
    thread_base->thread_main();
  } catch (const std::exception &e) {
    notify(NFY_WARN,
           "Internal error: Exception escaped from thread: %s", e.what());
  } catch (...) {
    notify(NFY_WARN,
           "Internal error: Unknown exception escaped from thread");
  }
  return 0;
}

bool khThreadBase::run(void) {
  assert(!tid);
  wantAbort_ = false;
  PrepareProfiling(&profile_timer);
  if (pthread_create(&tid, NULL, thread_start, this) != 0) {
    tid = 0;
    return false;
  } else {
    return true;
  }
}

void khThreadBase::join(void) {
  if (tid) {
    pthread_join(tid, 0);
    tid = 0;
  }
}

// PrepareProfiling - if thread profiling is enabled, get the profile timer.
// Must be called from parent thread.
void khThreadBase::PrepareProfiling(struct itimerval *profile_timer)
{
#ifdef PROFILE_THREADS
  if (getitimer(ITIMER_PROF, profile_timer) < 0) {
    notify(NFY_WARN, "PrepareProfiling: Unable to getitimer");
  }
#else
  memset(profile_timer, 0, sizeof(struct itimerval));
#endif
}

// BeginProfiling - if thread profiling is enabled, set the profile timer.
// Must be called from spawned thread.

void khThreadBase::BeginProfiling(const struct itimerval &profile_timer)
{
#ifdef PROFILE_THREADS
  if (setitimer(ITIMER_PROF, &profile_timer, 0) < 0) {
    notify(NFY_WARN, "BeginProfiling: Unable to setitimer");
  }
#endif
}

void* detached_thread_start(void* arg) {
  khDeleteGuard<DetachedThreadParams> params(
      TransferOwnership(reinterpret_cast<DetachedThreadParams*>(arg)));
  khThreadBase::BeginProfiling(params->profile_timer);
  (*(params->fun))();
  return 0;
}

bool RunInDetachedThread(const khFunctor<void> &fun) {
  pthread_attr_t attrs;
  if (pthread_attr_init(&attrs) == 0) {
    khCallGuard<pthread_attr_t*, int> guard(&::pthread_attr_destroy,
                                            &attrs);
    if (pthread_attr_setdetachstate(&attrs,
                                    PTHREAD_CREATE_DETACHED) == 0) {
      struct itimerval profile_timer;
      khThreadBase::PrepareProfiling(&profile_timer);
      pthread_t tid;
      return 0 == pthread_create(&tid, &attrs,
                                 detached_thread_start,
                                 new DetachedThreadParams(
                                     TransferOwnership(new khFunctor<void>(fun)),
                                     profile_timer));
    }
  }
  return false;
}
