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

#include <cassert>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>

#include "common/performancelogger.h"
#include "common/notify.h"
#include "common/timeutils.h"

using namespace std;
using namespace getime;

#ifdef LOG_PERFORMANCE

namespace performance_logger {

// make sure static members get initialized
plMutex PerformanceLogger::instance_mutex;

void plMutex::Lock(void) {
  // if this wasn't properly initialized, we'll get an error
  int err = pthread_mutex_lock(&mutex);
  assert(!err);
  (void) err; // Suppress unused variable 'err' warning.
}

void plMutex::Unlock(void) {
  // if this wasn't properly initialized, we'll get an error
  int err = pthread_mutex_unlock(&mutex);
  assert(!err);
  (void) err; // Suppress unused variable 'err' warning.
}

plMutex::plMutex(void) {
  // always returns 0
  (void)pthread_mutex_init(&mutex, NULL /* simple, fast mutex */);
}

plMutex::~plMutex(void) {
  int err = pthread_mutex_destroy(&mutex);
  assert(!err);
  (void) err; // Suppress unused variable 'err' warning.
}

void PerformanceLogger::generateFileName() {
    time_t t = time(0);
    tm date = *localtime(&t);
    char buf[256];
    if (timeFileName.size() == 0) {
        strftime(buf, sizeof(buf), "time_stats.%m-%d-%Y-%H:%M:%S.csv", &date);
        timeFileName = buf;
    }
}

// Log a profiling message
void PerformanceLogger::logTiming(
    const string & operation,
    const string & object,
    const timespec startTime,
    const timespec endTime,
    const size_t size) {

  const timespec duration = timespecDiff(endTime, startTime);
  stringstream message;

  message.setf(ios_base::fixed, ios_base::floatfield);
  message << setprecision(9)
          << operation << ','
          << object    << ','
          << startTime << ','
          << endTime   << ','
          << duration  << ','
          << size;

  assert(timeFileName.size() > 0);
  do_notify(message.str(), timeFileName);
}

// Thread safety wrapper for log output
void PerformanceLogger::do_notify(const string & message, const string & fileName) {

  // Get the thread and process IDs
  pthread_t tid = pthread_self();
  pid_t pid = getpid();

  {  // atomic inner block
    plLockGuard lock( write_mutex );
    { // Make sure we flush and close the output file before unlocking the mutex:
      ofstream output_stream(fileName.c_str(), ios::app);
      output_stream << pid << ',' << tid << ',' << message << endl;
    }
  }
}

} // namespace performance_logger

#endif  // LOG_PERFORMANCE
