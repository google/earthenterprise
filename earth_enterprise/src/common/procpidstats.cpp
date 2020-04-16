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

// Implementation of utility class ProcPidStats for loading process stats from
// /proc/*/stat and "ps -p pid"

#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <iostream>
#include <common/khSpawn.h>
#include <khStringUtils.h>
#include <khSimpleException.h>
#include <khstl.h>
#include <fstream>
#include "procpidstats.h"

// options set using PTRACE_SETOPTIONS
#define PTRACE_O_TRACEEXIT      0x00000040
// Wait extended result codes for the above trace options.
#define PTRACE_EVENT_EXIT       6
#define PTRACE_SETOPTIONS (__ptrace_request)0x4200

ProcPidStats::ProcPidStats() {
  pid_ = getpid();
  LoadStats();
}

// opens a specific Process ID.
ProcPidStats::ProcPidStats(int pid)
 : pid_(pid) {
  LoadStats();
}

void ProcPidStats::GetVmPeak(pid_t pid, std::string* status_string)  {
  static const std::string kVmPeakKeyWord = "VmPeak:";

  std::ostringstream stream;
  stream << pid;
  std::string file_name = "/proc/" + stream.str() + "/status";
  std::ifstream proc_status_file;
  proc_status_file.open(file_name.c_str());
  if (proc_status_file.fail()) {
    return;
  }
  std::string line;
  while (std::getline(proc_status_file, line)) {
    if (line.find(kVmPeakKeyWord) == 0) {  // line begins with VmPeak:
      // Strip leading whitespace.
      const size_t pos = line.find_first_not_of(" \n\r\t\v",
                                                kVmPeakKeyWord.size());
      if (pos != std::string::npos) {
        (*status_string) += "Peak memory used for this job: ";
        (*status_string) += line.substr(pos);
        (*status_string) += '\n';
      }
      break;
    }
  }
}

namespace {
// The following method prints system error messages as printed by perror only in
// debug mode. The file_name and line_num helps the developer (not the user) to
// find out where the system call got originated.
#ifndef NDEBUG
void GetSysErrorWithFileLineReference(const char * file_name, int line_num,
                                      std::string* status_string) {
  char buff[2048];
  strerror_r(errno, buff, sizeof(buff));
  (*status_string) += buff;
  snprintf(buff, sizeof(buff), "At %s:%d\n", file_name, line_num);
  (*status_string) += buff;
}
#else
void GetSysErrorWithFileLineReference(const char *, int, std::string*) {}
#endif
}  // end namespace

void ProcPidStats::GetProcessStatus(pid_t pid, std::string* status_string,
                                    bool* success, bool* coredump, int* signum) {
  // wait for first signal, which is from child when it execs.
  if (waitpid(pid, NULL, 0) != pid) {
    GetSysErrorWithFileLineReference(__FILE__, __LINE__, status_string);
    return;
  }
  struct timeval before;
  gettimeofday(&before, (struct timezone *)NULL);
  // add trace i.e ask child to tell us when it receives any signal
  if (ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_TRACEEXIT) != 0) {
    GetSysErrorWithFileLineReference(__FILE__, __LINE__, status_string);
    return;
  }
  // allow child to continue
  if (ptrace(PTRACE_CONT, pid, 0, 0) != 0) {
    GetSysErrorWithFileLineReference(__FILE__, __LINE__, status_string);
    return;
  }
  int status;
  while (true) {
    // wait for signal from child
    if (waitpid(pid, &status, 0) != pid) {
      GetSysErrorWithFileLineReference(__FILE__, __LINE__, status_string);
      return;
    }
    // any signal stops the child, so check that the incoming signal is a
    // SIGTRAP and the event_exit flag is set
    if ((WSTOPSIG(status) == SIGTRAP) && (status & (PTRACE_EVENT_EXIT << 8))) {
      break;
    } else {  // if not, pass the original signal onto the child
      if (ptrace(PTRACE_CONT, pid, 0, WSTOPSIG(status)) != 0) {
        GetSysErrorWithFileLineReference(__FILE__, __LINE__, status_string);
        return;
      }
    }
  }
  // Collect information about the process which is about to exit.
  std::string peak_info;
  ProcPidStats::GetVmPeak(pid, &peak_info);
  // Let the child to die without any more signal.
  if (ptrace(PTRACE_CONT, pid, 0, 0) != 0) {
    GetSysErrorWithFileLineReference(__FILE__, __LINE__, status_string);
    return;
  }
  // Now wait for the process and its children to die and collect time data
  struct rusage usage;
  if (!khWaitForPid(pid, *success, *coredump, *signum, &usage)) {
    GetSysErrorWithFileLineReference(__FILE__, __LINE__, status_string);
    return;
  }
  struct timeval after;
  gettimeofday(&after, (struct timezone *)NULL);
  timersub(&after, &before, &after);
  unsigned long real_milli_sec = after.tv_sec * 1000 + after.tv_usec / 1000;
  unsigned long virt_milli_sec = usage.ru_utime.tv_sec * 1000 +
                                 usage.ru_utime.tv_usec/1000 +
                                 usage.ru_stime.tv_sec * 1000 +
                                 usage.ru_stime.tv_usec/1000;
  char buff[2048];
  snprintf(buff, sizeof(buff),
      "----------------------------- ----------\n"
      "Process Stats\n"
      "  %9ld.%02ld real time\n"
      "  %9ld.%02ld user time\n"
      "  %9ld.%02ld system time\n"
      "  %11lu%% CPU usage average\n"
      "  %12ld swaps\n"
      "  %12ld page faults\n"
      "  %12ld I/O context switches\n",
      (long)after.tv_sec, (long)after.tv_usec/10000,
      (long)usage.ru_utime.tv_sec, (long)usage.ru_utime.tv_usec * 100 / 1000000,
      (long)usage.ru_stime.tv_sec, (long)usage.ru_stime.tv_usec * 100 / 1000000,
      virt_milli_sec * 100 / real_milli_sec,
      (long)usage.ru_nswap,
      (long)usage.ru_majflt,
      (long)usage.ru_nvcsw);
  (*status_string) += buff;
  if (!peak_info.empty()) {
    (*status_string) += peak_info;
  }
}

void ProcPidStats::LoadStats() {
  std::string pid_string = Itoa(pid_);
  std::string filename = "/proc/" + pid_string + "/stat";

  FILE* fp = fopen(filename.c_str(), "r");
  if (fp == NULL) {
    std::string message = "Failed to load " + filename;
    throw khSimpleException(message);
  }

  int buffer_count = 0;
  char buffer[256];
  unsigned int value_count = static_cast< unsigned int> (POLICY) + 2; // Note: should be +1 but
  // it turns out there's 42 fields in Ubuntu even though only 41 are documented.
  values_.reserve(value_count);
  while(true) {
    int c = fgetc(fp);
    bool done = (c == EOF);

    if (static_cast<char>(c) == ' ' || done) { // End current value
      buffer[buffer_count] = 0;
      std::string buffer_str(buffer);
      if (!buffer_str.empty()) {
        // only add non-empty values (i.e., ignore runs of spaces)
        values_.push_back(std::string(buffer));
      }
      buffer_count = 0;
    } else {
      buffer[buffer_count] = static_cast<char>(c);
      buffer_count++;
    }
    // Stop when end of file reached.
    if (done) {
      break;
    }
  }
  // Account for Ubuntu (42) and everyone else (41).
  // assert(values_.size() == value_count || values_.size()+1 == value_count);
  fclose(fp);

  // Get a few more stats from PS --- particularly cpu utilization
  {
    std::string ps_command("ps h -p " + pid_string + " -o pcpu,rss,size,vsize");
    fp = popen(ps_command.c_str(), "r");
    if (fp == NULL) {
      std::string message = "Failed to open pipe to 'ps' command: " + ps_command;
      throw khSimpleException(message);
    }
    fscanf(fp, "%f %ld %ld %ld", &cpu_percentage_, &resident_set_size_,
           &mem_size_megabytes_, &vmem_size_megabytes_);
    fclose(fp);
  }
  mem_size_megabytes_ /= 1024;
  vmem_size_megabytes_ /= 1024;
}

// Return the statistic for the specified key.
std::string ProcPidStats::GetStat(Keys key) const {
  return values_[static_cast< unsigned int> (key)];
}

// Return the statistic for the specified key.
// This assumes that the key is an integer...will assert for TCOMM which is a
// string.
long ProcPidStats::GetIntegerStat(Keys key) const {
  assert(key != TCOMM);
  const std::string& string_value = values_[static_cast< unsigned int> (key)];
  long value = strtol(string_value.c_str(), NULL, 10);
  return value;
}

// Handy Access methods

int ProcPidStats::CpuUsagePercentage() {
  return static_cast<int>(cpu_percentage_);
}

int ProcPidStats::MemoryUsageMegaBytes() const {
  return vmem_size_megabytes_;
}

long ProcPidStats::MajorPageFaults() const {
  long value = GetIntegerStat(MAJ_FLT);
  long child_value = GetIntegerStat(CMAJ_FLT);
  return value + child_value;
}
