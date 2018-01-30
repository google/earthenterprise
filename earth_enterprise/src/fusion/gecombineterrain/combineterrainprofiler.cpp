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
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>

#include "common/notify.h"
#include "combineterrain.h"
#include "combineterrainprofiler.h"

using namespace std;

// Initialize static members
CombineTerrainProfiler * const CombineTerrainProfiler::_instance =
    new CombineTerrainProfiler();
CombineTerrainProfiler * const BlockProfiler::terrainProf =
    CombineTerrainProfiler::instance();

// Log a profiling message
void CombineTerrainProfiler::log
    (TerrainEvent event, const string operation, const string object) {
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
  
  notify(NFY_NOTICE, "%s\n", message.str().c_str());
}

double CombineTerrainProfiler::getTime() const {
  struct timespec currTime;
  clock_gettime(CLOCK_MONOTONIC, &currTime);
  return static_cast<double>(currTime.tv_sec) + (currTime.tv_nsec * NSEC_TO_SEC);
}
