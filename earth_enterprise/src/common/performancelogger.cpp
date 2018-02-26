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
#include <sstream>
#include <string>
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>  // required for 'pthread_getunique_np'
#include <time.h>

#include "common/performancelogger.h"
#include "common/notify.h"
#include "common/timeutils.h"
#include "common/mutex.h"  // defines thrd::mutex

using namespace std;
using namespace getime;

// Initialize static members of Profiler class
PerformanceLogger * const PerformanceLogger::_instance = new PerformanceLogger();

// Log a profiling message
void PerformanceLogger::log(
    const string & operation,
    const string & object,
    const timespec startTime,
    const timespec endTime,
    const size_t size) {

  timespec duration;
  stringstream message;
  pthread_t tid = pthread_self();

  duration = timespecDiff( endTime, startTime );  // thread safe call
  message.setf(ios_base::fixed, ios_base::floatfield);
  message << startTime << ", "
          << endTime << ", "
          << duration << ", "
          << tid << ", "
          << size << ", "
          << operation << ", "
          << object;

  try {  // attempt to lock the system wide mutex and log the message

    thrd::mutex lock;  // create the mutex object - attempt to lock it

    notify(NFY_NOTICE, "%s\n", message.str().c_str());

  } catch( exception& except ) {

   notify( NFY_FATAL, "%s\n", except.what() );

  };  // end try/catch block

}
