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


#ifndef __khMTTypes_h
#define __khMTTypes_h

#include "khThread.h"
#include "khGuard.h"
#include <cstdint>
#include <queue>
#include <deque>
#include <set>

template <class T>
class khAtomic {
  // TODO: now that we're on modern std libraries, use their
  // atomic stuff instead!

  mutable khMutex lock;
  T t;

  // private and unimplemented
  khAtomic(const khAtomic &);
  khAtomic& operator=(const khAtomic &);

 public:
  khAtomic(T t_) : t(t_) { }
  khAtomic& operator=(T t_) {
    khLockGuard guard(lock);
    t = t_;
    return *this;
  }

  operator T (void) const {
    khLockGuard guard(lock);
    return t;
  }
};


template <class T>
class khMTQueue
{
 protected:
  typedef std::deque<T> Queue;
  Queue     queue;
  khMutex   mutex;
  khCondVar condvar;

 private:
  // private and unimplimented
  khMTQueue(const khMTQueue&);
  khMTQueue& operator=(const khMTQueue&);


 public:
  khMTQueue(void) { }

  void push(const T &val) {
    khLockGuard guard(mutex);
    queue.push_back(val);
    condvar.signal_one();
  }
  void push_front(const T &val) {
    khLockGuard guard(mutex);
    queue.push_front(val);
    condvar.signal_one();
  }
  template <class Iter>
  void push(Iter begin, Iter end) {
    khLockGuard guard(mutex);
    for (Iter i = begin; i != end; ++i) {
      queue.push_back(*i);
    }
    condvar.broadcast_all();
  }
  T pop(void) {
    khLockGuard guard(mutex);
    while (queue.empty()) {
      condvar.wait(mutex);
    }
    khMemFunCallGuard<Queue> guard2(&queue, &Queue::pop_front);
    return queue.front();
  }
  bool trypop(T &t) {
    khLockGuard guard(mutex);
    if (!queue.empty()) {
      t = queue.front();
      queue.pop_front();
      return true;
    } else {
      return false;
    }
  }
  void clear(void) {
    khLockGuard guard(mutex);
    // queue doesn't have a 'clear' :-(
    while (queue.size()) {
      queue.pop_front();
    }
  }
  typename Queue::size_type size(void) const {
    khLockGuard guard(mutex);
    return queue.size();
  }
};


template <class T>
class khMTSet
{
 protected:
  typedef std::set<T> Set;
  Set       set;
  mutable khMutex mutex;

 private:
  // private and unimplimented
  khMTSet(const khMTSet&);
  khMTSet& operator=(const khMTSet&);


 public:
  khMTSet(void) { }

  void insert(const T &val) {
    khLockGuard guard(mutex);
    (void)set.insert(val);
  }
  void erase(const T &val) {
    khLockGuard guard(mutex);
    (void)set.erase(val);
  }
  std::vector<T> keys(void) const {
    khLockGuard guard(mutex);
    return std::vector<T>(set.begin(), set.end());
  }
};



// ****************************************************************************
// ***  MultiThreadingPolicy
// ***
// ***  Multithreading version of a threading policy used by some keyhole
// ***  template classes. See khThreadingPolicy for more details.
// ****************************************************************************
class MultiThreadingPolicy
{
 public:
  class MutexHolder {
   public:
    mutable khMutex mutex;
  };
  class LockGuard {
    khLockGuard guard;
   public:
    LockGuard(const MutexHolder *h) : guard(h->mutex) { }
  };
};


// ****************************************************************************
// ***  khMTRefCounter

// ***  !!! WARNING !!! This object doesn't automatically solve all of your
// ***  MT lifetime issues. You have to know the limitations of this
// ***  implementation and stay within them.

// ***  khMTRefCounter makes lifetime control MT-safe. It DOES NOT provide
// ***  general MT-safe access to the entire object. The lifetile control
// ***  MT-safeness is only ensured when all accesses to the object are
// ***  through khRefGuard and its various cousins. Additionally, RefGuards
// ***  used by multiple threads must themselves be protected by some other
// ***  syncronization primitive. For example if thread 1 is making a copy
// ***  of RefGuard A and thread 2 deletes RefGuard A before thread 1 is
// ***  finished making the copy, the world may not be happy. But this
// ***  protection of the Guard is really no different than that for any
// ***  other shared variables.

// ***  The refcount() member function is only marginally useful. If it
// ***  returns 1 then you known that the only guard referencing this obj
// ***  is the one you are using to call this function. If you know that no
// ***  other thread has access to that copy, you can safely rely on
// ***  refcount()==1 staying true until you expose your copy of the guard to
// ***  another thread. This is particularly important when using
// ***  khRefGuard<khMTRefCounter> objects inside of khCache. So long as
// ***  access to the cache is limited to one thread (by code design or by
// ***  mutex protection), the cache can safely know that it has the only
// ***  reference to the shared object by checking refcount()==1.
// ****************************************************************************
#include "khRefCounter.h"
typedef khRefCounterImpl<MultiThreadingPolicy> khMTRefCounter;


#endif /* __khMTTypes_h */
