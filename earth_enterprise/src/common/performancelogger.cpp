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

std::string ioPrefFile;
std::string timePrefFile;
std::string threadPrefFile;

void checkForLeadingZero(
        std::stringstream& io,
        std::stringstream& thread,
        std::stringstream& time,
        const int& val) {
    if (val > 9) return;
    std::string temp("0");
    io << temp;
    thread << temp;
    time << temp;
}

void printToSStream(
        std::stringstream& io,
        std::stringstream& thread,
        std::stringstream& time,
        const char& delimiter,
        const int& val) {
    io << val << delimiter;
    thread << val << delimiter;
    time << val << delimiter;
}

void createFileNames() {
    time_t t = time(0);
    tm date = *localtime(&t);
    std::stringstream io,time,thread;
    io << "io.";
    time << "time.";
    thread << "thread.";
    checkForLeadingZero(io,thread,time,date.tm_mon);
    printToSStream(io,thread,time,'-',date.tm_mon);
    checkForLeadingZero(io,thread,time,date.tm_mday);
    printToSStream(io,thread,time,'-',date.tm_mday);
    printToSStream(io,thread,time,'-',date.tm_year+1900);
    checkForLeadingZero(io,thread,time,date.tm_hour);
    printToSStream(io,thread,time,':',date.tm_hour);
    checkForLeadingZero(io,thread,time,date.tm_min);
    printToSStream(io,thread,time,':',date.tm_min);
    checkForLeadingZero(io,thread,time,date.tm_sec);
    printToSStream(io,thread,time,'.',date.tm_sec);
    io << "csv";
    thread << "csv";
    time << "csv";
    ioPrefFile = io.str();
    timePrefFile = time.str();
    threadPrefFile = thread.str();
}

void PerformanceLogger::doNotify(string message, string fileName)
{
    //implemented by Daniel
}

/*void PerformanceLogger::logIO(
        const string & operation,
        const string & object,
        const timespec startTime,
        const timespec endTime,
        const size_t size ) {
        //const size_t requests) {
    //output info
    stringstream message;
    const timespec duration = timespecDiff(endTime,startTime);
    double throughput = (timespecToDouble(duration)*size) / 1024;
    message.setf(ios_base::fixed, ios_base::floatfield);
    this->doNotify(message.str(),io_FnBase+ext);
    message << setprecision(9)
            << operation << ',' << object << ": "
            << ", start time: " << startTime
            << ", end time: " << endTime
            << ", duration: " << duration
            << ", buffer size: " << size;
    this->doNotify(message.str(),io_FnBase+ext);
}

void PerformanceLogger::logThread() {
//implemented by pavel
}*/

// Log a profiling message
void PerformanceLogger::logTiming(
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

  this->doNotify(message.str(),timePrefFile);
}
