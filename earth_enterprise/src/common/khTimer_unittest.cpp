// Copyright 2017 Google Inc.
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


// khTimer_unittest.cpp - unit tests for khTimer class

#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <iostream>
#include <string>
#include <list>
#include <algorithm>
#include <khSimpleException.h>
#include <UnitTest.h>
#include "khTimer.h"


class khTimerTest : public UnitTest<khTimerTest> {
 public:
  khTimerTest() : BaseClass("khTimerTest") {
    REGISTER(TimingTest);
  }

  // Test the timer for a specified duration.
  // Expectation is that it will be within 50 microseconds.
  bool TestTiming(unsigned int seconds) {
    const double timing_accuracy_threshold = 50e-3; // 50 milliseconds
    khTimer_t begin = khTimer::tick();

    if (sleep(seconds) != 0) {
      std::cerr << "Sleep Failed during Testtiming for " << seconds
        << " secs" << std::endl;
      return false;
    }

    khTimer_t end = khTimer::tick();
    double measured_seconds = khTimer::delta_s(begin, end);
    double measured_milliseconds = khTimer::delta_m(begin, end);
    TestAssertEqualsWithinEpsilon(1e6 * measured_seconds,
                     1e3 * measured_milliseconds,
                     1e-8 /* for precision issues */);

    double delta_seconds = measured_seconds - seconds;
    bool failure = fabs(delta_seconds) > timing_accuracy_threshold;
    if (failure) {
      std::cerr << "Timing of : " << seconds << "secs failed! Measured "
        << measured_seconds << " secs (difference of " << delta_seconds <<
        ")" << std::endl;
      return false;
    }
    return true;
  }

  // Run a number of random timings under 1 second.
  bool TimingTest() {
  // Note that we ran into issues with /proc/cpuinfo changing the MHz during
  // runs which caused timings to be off significantly
  // test a variety of timing intervals.
    unsigned int n = 5;
    for(unsigned int seconds = 1; seconds < n; ++seconds) {
      if (!TestTiming(seconds)) {
        return false;
      }
    }
    return true;
  }
};

int main(int argc, char *argv[]) {

  khTimerTest tests;
  return tests.Run();
}
