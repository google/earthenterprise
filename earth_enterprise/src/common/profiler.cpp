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

#include "common/profiler.h"
#include "common/notify.h"
#include "common/timeutils.h"

using namespace std;
using namespace getime;

// Initialize static members of Profiler class
Profiler * const Profiler::_instance = new Profiler();

// Initialize static members of BlockProfiler class
Profiler * const BlockProfiler::profiler = Profiler::instance();

// Log a profiling message
void Profiler::log(
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

  notify(NFY_NOTICE, "%s\n", message.str().c_str());
}

// Begin profiling
BlockProfiler::BlockProfiler(
    const string & operation,
    const string & object,
    const size_t size
  ) :
    operation(operation),
    object(object),
    size(size),
    startTime(getMonotonicTime())
{
  // Nothing to do - just initialize class members above
}

// Stop profiling and log results
BlockProfiler::~BlockProfiler() {
  const timespec endTime = getMonotonicTime();
  profiler->log(operation, object, startTime, endTime, size);
}
