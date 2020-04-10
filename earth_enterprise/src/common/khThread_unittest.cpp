// Copyright 2020 the Open GEE Contributors
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


// Unit tests for khThread 

#include <string>
#include <gtest/gtest.h>
#include "common/khThread.h"

namespace {

class ThreadUnitTest : public testing::Test { };

khNoDestroyMutex mutex;
khMutex syncMutex;
khCondVar condvar;

static void holdTheMutex(unsigned int holdSeconds) {
  khLockGuard lock(mutex);
  condvar.broadcast_all();
  sleep(holdSeconds);
}

typedef struct mutex_acquire_info {
  bool acquired = false;
  timespec start = {0,0};
  timespec stop = {0,0};
  bool timeoutExceptionCaught = false;
  bool otherExceptionCaught = false;
} mutex_acquire_info;

void testMutexAcquire(mutex_acquire_info& info, unsigned int timetowait) {
  try {
    clock_gettime(CLOCK_REALTIME, &info.start);
    khLockGuard timedTryLock(mutex, timetowait);
    info.acquired = true;
  }
  catch (khTimedMutexException e) {
    info.timeoutExceptionCaught = true;
  }
  catch (...) {
    info.otherExceptionCaught = true;
  }

  clock_gettime(CLOCK_REALTIME, &info.stop);
}

// should acquire the mutex if nothing else is holding it
TEST_F(ThreadUnitTest, timedMutex_normal) {
  unsigned int waittime = 5;
  mutex_acquire_info info;
  testMutexAcquire(info, waittime);
  EXPECT_TRUE(info.acquired);
  // shouldn't have taken wait time since mutex was free already
  EXPECT_TRUE(info.start.tv_sec + waittime > info.stop.tv_sec);
}

// should wait at least timeout time and then fail to create khLockGuard if mutex is already held
TEST_F(ThreadUnitTest, timedMutex_timeout) {
  unsigned int waittime = 3; // seconds

  mutex.Lock();

  mutex_acquire_info info;
  testMutexAcquire(info, waittime);

  mutex.Unlock();

  EXPECT_TRUE(info.start.tv_sec + waittime <= info.stop.tv_sec);
  EXPECT_TRUE(info.timeoutExceptionCaught);
  EXPECT_FALSE(info.otherExceptionCaught);
}

TEST_F(ThreadUnitTest, timedMutex_waitSuccess) {
  unsigned int holdTime = 4; // seconds
  khThread thread1(khFunctor<void>(std::ptr_fun(&holdTheMutex), holdTime));
  thread1.run();
  khLockGuard syncLock(syncMutex);
  condvar.wait(syncMutex); // wait until thread1 is holding the mutex;

  // allow slightly more time than the other thread is holding
  unsigned int waittime = holdTime+2;
  mutex_acquire_info info;
  testMutexAcquire(info, waittime);

  EXPECT_TRUE(info.acquired);
  EXPECT_TRUE(info.start.tv_sec + waittime > info.stop.tv_sec);
  EXPECT_FALSE(info.timeoutExceptionCaught);
  EXPECT_FALSE(info.otherExceptionCaught);
}

void reallyBadFunction() {
  throw std::runtime_error("You really shouldn't have done that!");
}

void throwAnExceptionUsingTimedMutex() {
  khLockGuard(mutex, 10); // time doesn't matter since mutex should be free
  reallyBadFunction();

  // we're never getting here
  sleep(100);
}

TEST_F(ThreadUnitTest, timedMutex_guardUnlock) {
  bool exceptionCaught = false;
  // make sure that some abnormal behavior can't leave the timed mutex locked if using the guard
  try {
    throwAnExceptionUsingTimedMutex();
  }
  catch (...) {
    exceptionCaught = true;
  }
  EXPECT_TRUE(exceptionCaught);
  EXPECT_TRUE(mutex.TryLock());
  mutex.Unlock();
}


}  // namespace

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc,argv);
  return RUN_ALL_TESTS();
}

