/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// ProcPidStats is a utility class for accessing process stats.
//

#ifndef COMMON_PROCPIDSTATS_H__
#define COMMON_PROCPIDSTATS_H__

#include <vector>
#include <string>

class ProcPidStats {
public:
  // see: man 5 proc for a description of each of these.
  enum Keys {
    PID	= 0,
    TCOMM	= 1,
    STATE	= 2,
    PPID	= 3,
    PGID	= 4,
    SID	= 5,
    TTY_NR	= 6,
    TTY_PGRP	= 7,
    FLAGS	= 8,
    MIN_FLT	= 9,
    CMIN_FLT	= 10,
    MAJ_FLT	= 11,
    CMAJ_FLT	= 12,
    UTIME	= 13,
    STIME	= 14,
    CUTIME	= 15,
    CSTIME	= 16,
    PRIORITY	= 17,
    NICE	= 18,
    UNUSED_3	= 19, // NumThreads...this is unused.
    IT_REAL_VALUE	= 20,
    START_TIME	= 21,
    VSIZE	= 22,
    RSS	= 23,
    RSSLIM	= 24,
    START_CODE	= 25,
    END_CODE	= 26,
    START_STACK	= 27,
    ESP	= 28,
    EIP	= 29,
    PENDING	= 30,
    BLOCKED	= 31,
    SIGIGN	= 32,
    SIGCATCH	= 33,
    WCHAN	= 34,
    UNUSED_1	= 35, // NSWAP deprecated
    UNUSED_2	= 36, // CNSWAP deprecated
    EXIT_SIGNAL	= 37,
    CPU	= 38,
    RT_PRIORITY	= 39,
    POLICY	= 40,
  };

  // Note: Constructor will load the stats immediately, which involves
  // opening /proc/####/stat where #### is the current process id.
  ProcPidStats();
  // opens a specific Process ID.
  ProcPidStats(int pid);
  ~ProcPidStats() {}

  // Given a pid collect VmPeak information and append to status_string.
  // If cannot collect, does't do anything.
  static void GetVmPeak(pid_t pid, std::string* status_string);

  // Collect child process (pid) status summary just before it exits and appends
  // it to status_string. If any system call fails or if cannot collect, doesn't
  // do anything.
  // Assumes that
  // 1. the monitored process pid is a child of this process.
  // 2. the monitored process has enabled tracing by ptrace(PTRACE_TRACEME, ...)
  static void GetProcessStatus(pid_t pid, std::string* status_string,
                               bool* success, bool* coredump, int* signum);

  int GetPid() const { return pid_; }

  // Return the statistic for the specified key.
  std::string GetStat(Keys key) const;

  // Return the statistic for the specified key.
  // This assumes that the key is an integer...will assert for TCOMM which is a
  // string.
  long GetIntegerStat(Keys key) const;

  // Handy Accessors
  // Get the percentage of CPU usage of the process.
  // Returns an int between 0 and 100.
  // Includes children.
  int CpuUsagePercentage();
  // Get the number of megabytes used by the process.
  // Includes children.
  int MemoryUsageMegaBytes() const;
  // Get the number of Page faults of the process.
  // Includes children.
  long MajorPageFaults() const;
  // Get the number of pages of the process.
  // Includes children.

private:
  // Do the work of loading the stats from the pid file.
  void LoadStats();

  int pid_;
  std::vector<std::string> values_;
  float cpu_percentage_;
  long mem_size_megabytes_;
  long vmem_size_megabytes_;
  long resident_set_size_;
};

#endif  // COMMON_PROCPIDSTATS_H__
