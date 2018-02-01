// Copyright 2018 the Open GEE Contributors
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

#include "common/profiler.h"
#include "common/notify.h"
#include "common/timeutils.h"

using namespace std;

// Initialize static members of Profiler class
Profiler * const Profiler::_instance = new Profiler();

// Initialize static members of BlockProfiler class
Profiler * const BlockProfiler::profiler = Profiler::instance();

// Log a profiling message
void Profiler::log
    (ProfileEvent event, const string & operation, const string & object, const size_t size) {
  stringstream message;
  
  message.setf(ios_base::fixed, ios_base::floatfield);
  
  message << "Thread " << syscall(SYS_gettid) << ", Time "
          << setprecision(6) << getTime() << ": ";
  
  switch(event) {
    case BEGIN:
      message << "Begin";
      break;
    case END:
      message << "End";
      break;
  }
  
  message << " " << operation << " " << object;
  
  if (size > 0) {
      message << ", size " << size;
  }
  
  notify(NFY_NOTICE, "%s\n", message.str().c_str());
}
