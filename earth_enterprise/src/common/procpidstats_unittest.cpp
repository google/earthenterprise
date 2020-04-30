// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


// procpidstats_unittest.cpp - unit tests for procpidstats class

#include <stdlib.h>
#include <iostream>
#include <string>
#include <khSimpleException.h>
#include <UnitTest.h>
#include "procpidstats.h"


class procpidstatsTest : public UnitTest<procpidstatsTest> {
 public:
  procpidstatsTest() : BaseClass("procpidstatsTest") {
    REGISTER(TestBasics);
  }

  // Just the most basic tests.
  bool TestBasics() {
    // get busy temporarily
    BeBusy();
    ProcPidStats proc_pid_stats;
    int pid = proc_pid_stats.GetPid();
    TestValues(proc_pid_stats);

    // get busy temporarily
    BeBusy();
    ProcPidStats proc_pid_stats2(pid);
    TestAssertEquals(pid, proc_pid_stats2.GetPid());
    TestValues(proc_pid_stats2);
    return true;
  }

  void TestValues(ProcPidStats& proc_pid_stats) {
    int cpu_percentage = proc_pid_stats.CpuUsagePercentage();
    int megabytes = proc_pid_stats.MemoryUsageMegaBytes();
    TestAssert(cpu_percentage >= 0 && cpu_percentage <= 400);
    TestAssert(megabytes >= 0 && megabytes <= 250);
  }

  // Pretend to be busy temporarily
  void BeBusy() {
    double result = 0;
    for(unsigned int i = 0; i < 100000000; ++i) {
      result += static_cast<double>(i);
    }
    assert(result > 0);
  }
};

int main(int argc, char *argv[]) {

  procpidstatsTest tests;
  return tests.Run();
}
