// Copyright 2018 The Open GEE Contributors
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

#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>

#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>

#include "common/performancelogger.h"
#include "common/notify.h"
#include "common/timeutils.h"

using namespace std;
using namespace getime;

namespace performance_logger {

void khMutexBase::Lock(void) {
  // if this wasn't properly initialized, we'll get an error
  int err = pthread_mutex_lock(&mutex);
  assert(!err);
  (void) err; // Suppress unused variable 'err' warning.
}

void khMutexBase::Unlock(void) {
  // if this wasn't properly initialized, we'll get an error
  int err = pthread_mutex_unlock(&mutex);
  assert(!err);
  (void) err; // Suppress unused variable 'err' warning.
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
  int err = pthread_mutex_destroy(&mutex);
  assert(!err);
  (void) err; // Suppress unused variable 'err' warning.
}

// Log a profiling message
void PerformanceLogger::logTiming(
    const string & operation,
    const string & object,
    const timespec startTime,
    const timespec endTime,
    const size_t size) {

  const timespec duration = timespecDiff(endTime, startTime);
  const pid_t tid = syscall(SYS_gettid);
  stringstream message;

  message.setf(ios_base::fixed, ios_base::floatfield);
  message << setprecision(9)
          << operation << " " << object << ": "
          << "start time: " << startTime
          << ", "
          << "end time: " << endTime
          << ", "
          << "duration: " << duration
          << ", "
          << "thread: " << tid;
  if (size > 0) {
    message << ", size: " << size;
  }
  message << std::endl;

  doNotify(message.str().c_str(), "timingfile.csv");
}

void PerformanceLogger::doNotify(std::string data, std::string fileName)
{
  {
    khLockGuard lock(write_mutex);

    { // Make sure we flush and close the output file before unlocking the mutex:
      std::ofstream output_stream(fileName.c_str(), std::ios_base::app);

      output_stream << data;
    }
  }
}

}
