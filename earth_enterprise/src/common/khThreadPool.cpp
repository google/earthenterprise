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


#include "khThreadPool.h"

void
khThreadPool::poolThread::thread_main(void)
{
  while (1) {
    {
      // This must be in an inner scope so the mutex is released
      // before we delete below
      khLockGuard guard(mutex);
      while (!ready) condvar.wait(mutex);
      fun();
      ready = false;

      // clear out the functor in case it holds resources that need
      // to be released. That way if this thread gets stored in the pool,
      // it won't hold the resources busy until it's reused.
      fun = khFunctor<void>();
    }
    if (!pool.returnThread(this)) {
      // we can't return ourself to the pool (it's already full) so we
      // go ahead and delete our object and return from the thread
      // func. Since this is a detached thread, returning from this
      // func will clean up the thread
      delete this;
      return;
    }
  }
}

void
khThreadPool::poolThread::run(const khFunctor<void> &fun_)
{
  khLockGuard guard(mutex);
  fun = fun_;
  ready = true;
  condvar.signal_one();
}
