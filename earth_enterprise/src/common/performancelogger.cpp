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

#include <sys/types.h>


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

void PerformanceLogger::initializeFile() {
  // Create a unique file name
  time_t t = time(0);
  tm date = *localtime(&t);
  char buf[256];
  if (timeFileName.size() == 0) {
    strftime(buf, sizeof(buf), "time_stats.%m-%d-%Y-%H:%M:%S.csv", &date);
    timeFileName = buf;
  }

  // Open the file
  timeFile.open(timeFileName.c_str(), ios::app);
  
  // Write the header for the CSV file
  timeFile << "pid,tid,pthread_id,operation,object,startTime_sec,endTime_sec,duration_sec,size" << endl;
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

  do_notify(message.str());
}



void PerformanceLogger::logConfig(
    const string & operation,
    const string & object,
    const unsigned int value) {

  const timespec now = getime::getMonotonicTime();
  stringstream message;

  message.setf(ios_base::fixed, ios_base::floatfield);
  message << setprecision(9)
          << operation << ','
          << object << ','
          << getime::getMonotonicTime() << ','
          << getime::getMonotonicTime() << ','
          << 0 << ','
          << value;

  assert(timeFileName.size() > 0);
  do_notify(message.str());
}

// Thread safety wrapper for log output
void PerformanceLogger::do_notify(const string & message) {

  // Get the thread and process IDs
  pthread_t pthread_tid = pthread_self();
  pid_t pid = getpid();
  // Call the Linux kernel function directly.
  // This won't work on other platforms:
  pid_t tid = syscall(SYS_gettid);

  {  // atomic inner block
    plLockGuard lock( write_mutex );
    {
      if (buffer.size() + message.size() > BUF_SIZE) {
        flushBuffer();
      }
      stringstream newLine;
      newLine << pid << ',' << tid << ',' << pthread_tid << ',' << message <<
        endl;
      buffer.append(newLine.str());
    }
  }
}

void PerformanceLogger::flushBuffer() {
  timeFile << buffer;
  buffer.clear();
}

} // namespace performance_logger

#endif  // LOG_PERFORMANCE
