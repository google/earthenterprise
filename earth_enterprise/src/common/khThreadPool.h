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

#ifndef __khThreadPool_h
#define __khThreadPool_h

#include <vector>
#include "khThread.h"

/******************************************************************************
 ***  khThreadPool
 ***
 ***  This object keeps a pool of threads around for servicing requests. It
 ***  is designed to meet specific needs. Over time perhaps it can be
 ***  generalize to be more flexible. For now this is how it works:
 ***
 ***  minAvail & maxAvail limit the number of "extra" threads that are
 ***  waiting for something to do. There is no limit on the total number of
 ***  concurrent threads.
 ***
 ***  All threads are created in a detached state, meaning you cannot join
 ***  with them to get results. If you want results from the thread you
 ***  should use synchronization primatives to store the result somewhere
 ***  predictable (usually in an object instance).
 ***
 ***  You cannot delete/destroy the khThreadPool object. As currently
 ***  designed, it is intended to be left running through the end of the
 ***  application. it's resource will be reclaimed by the OS upon program
 ***  termination. Since the threads are detached, there is no inherant way
 ***  to sync with the running threads before shutting down. Since the
 ***  threads need to reference back to the pool (to try to add themselves
 ***  back into the pool) the pool cannot go away or threads that finish
 ***  later will barf.
 ***
 ***  usage:
 ***        khThreadPool *pool = khThreadPool::Create(1,3);
 ***        pool->run(&globalfunc);
 ***        pool->run(&foo, &Foo::doit);
 ***        pool->run(bindOnly(ptr_func(&globalUnaryFunc), 345));
 ******************************************************************************/
class khThreadPool {
  class poolThread
  {
    khThreadPool &pool;
    khMutex   mutex;
    khCondVar condvar;
    khFunctor<void> fun;
    bool ready;
    poolThread(khThreadPool &pool_) :
        pool(pool_),
        fun(khFunctor<void>()),
        ready(false) {
    }
   protected:
    void thread_main(void);
   public:
    void run(const khFunctor<void> &fun_);
    static poolThread* Create(khThreadPool &pool_) {
      poolThread *ret = new poolThread(pool_);

      RunInDetachedThread
        (khFunctor<void>
         (std::mem_fun(&khThreadPool::poolThread::thread_main), ret));

      return ret;
    }
  };

  friend class poolThread;

  const unsigned int minAvail;
  const unsigned int maxAvail;
  khMutex    mutex;
  std::vector<poolThread*> pool;

  bool returnThread(poolThread *thread) {
    khLockGuard guard(mutex);
    if (pool.size() < maxAvail) {
      pool.push_back(thread);
      return true;
    } else {
      return false;
    }
  }

  void replenishPool(void) {
    for (unsigned int i = pool.size(); i < minAvail; ++i) {
      pool.push_back(poolThread::Create(*this));
    }
  }

  khThreadPool(unsigned int minAvail_, unsigned int maxAvail_)
      : minAvail(minAvail_), maxAvail(maxAvail_) {
    assert(minAvail <= maxAvail);
    khLockGuard guard(mutex);
    replenishPool();
  }

  // intentionally unimplemented - you can't destroy a khThreadPool.
  // Maybe later I'll look at allowing it
  // See comment at the beginning of the class to find out why this
  // is non-trivial
  ~khThreadPool();

 public:
  static khThreadPool* Create(unsigned int minAvail, unsigned int maxAvail) {
    return new khThreadPool(minAvail, maxAvail);
  }

  void run(const khFunctor<void> &fun) {
    khLockGuard guard(mutex);
    poolThread *thread;
    // pool size can be 0 only if minAvail == 0
    if (pool.size()) {
      thread = pool.back();
      pool.pop_back();
    } else {
      thread = poolThread::Create(*this);
    }
    replenishPool();
    thread->run(fun);
  }

};


#endif /* __khThreadPool_h */
