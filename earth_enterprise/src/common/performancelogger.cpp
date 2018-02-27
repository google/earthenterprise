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
#include <time.h>

#include "common/performancelogger.h"
#include "common/notify.h"
#include "common/timeutils.h"

using namespace std;
using namespace getime;

#ifdef LOG_PERFORMANCE

// Initialize static members of Profiler class
PerformanceLogger * const PerformanceLogger::_instance = new PerformanceLogger();

// Log a profiling message
void PerformanceLogger::log(
    const string & operation,
    const string & object,
    const timespec startTime,
    const timespec endTime,
    const size_t size) {

  const timespec duration = timespecDiff(endTime, startTime);
  stringstream message;

  message.setf(ios_base::fixed, ios_base::floatfield);
  message << startTime << ", "
          << endTime << ", "
          << duration << ", "
          << size << ", "
          << operation << ", "
          << object;

  notify(NFY_NOTICE, "%s\n", message.str().c_str());
}

// Thread safety wrapper for log output
void PerformanceLogger::do_notify( const string& message, ostream& out, khMutex& mutex ) {

  // Get the thread ID
  pthread_t tid = pthread_self();
  pid_t pid = getppid();  // get the ID of the parent process

  {  // atomic inner block
    khLockGuard lock( mutex );
    out << pid << ", " << tid << ", " << message;
  };  // end inner block

};  // end do_notify


#endif  // LOG_PERFORMANCE
