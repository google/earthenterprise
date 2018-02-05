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

#include <assert.h>
#include <time.h>
#include <ostream>
#include <stdint.h>

#include "timeutils.h"

using namespace std;

namespace getime {

const uint32_t SEC_TO_NSEC = 1e9;
const double NSEC_TO_SEC = 1e-9;

timespec getMonotonicTime() {
  timespec currTime;
  clock_gettime(CLOCK_MONOTONIC, &currTime);
  return currTime;
}

double timespecToDouble(const timespec tm) {
  return static_cast<double>(tm.tv_sec) + (tm.tv_nsec * NSEC_TO_SEC);
}

timespec timespecDiff(const timespec x, const timespec y) {
  // This function won't calculate negative time values
  assert(x >= y);

  // Cast to larger integers in case of overflow
  uint32_t x_sec = x.tv_sec;
  uint32_t x_nsec = x.tv_nsec;
  uint32_t y_sec = y.tv_sec;
  uint32_t y_nsec = y.tv_nsec;

  // See if we need to borrow
  if (x_nsec < y_nsec) {
    x_nsec += SEC_TO_NSEC;
    --x_sec;
  }
  
  timespec result;
  result.tv_sec = x_sec - y_sec;
  result.tv_nsec = x_nsec - y_nsec;
  return result;
}

bool operator<(const timespec & x, const timespec & y) {
  if (x.tv_sec < y.tv_sec) return true;
  else if (x.tv_sec == y.tv_sec && x.tv_nsec < y.tv_nsec) return true;
  else return false;
}

bool operator>=(const timespec & x, const timespec & y) {
  return !(x < y);
}

ostream & operator<<(ostream & stream, const timespec & tm) {
  stream << timespecToDouble(tm);
  return stream;
}

}
