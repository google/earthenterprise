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
#include <vector>
#include "common/performancelogger.h"
#include "common/notify.h"
#include "common/timeutils.h"

using namespace std;
using namespace getime;

std::ofstream ioPrefFile;
std::string ioFileName;
std::ofstream timePrefFile;
std::ofstream threadPrefFile;

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

void openFiles() {
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
    // if not open, open and add header
    if (!ioPrefFile.is_open()) { 
      ioPrefFile.open(io.str().c_str());
      ioFileName = io.str();
      ioPrefFile << "operation,object,startTime,endTime,bufferSize" << endl;
    }
    if (!timePrefFile.is_open()) {
      timePrefFile.open(time.str().c_str());
      timePrefFile << "operation,object,startTime,endTime,duration,threadID,objectSize" << endl;
    }
    if (!threadPrefFile.is_open()) {
      threadPrefFile.open(thread.str().c_str());
      //TODO: pavel fill in header line
    }
}

void ioPostProcess()
{
    /*
        Do IO post processing and output:
            - total number of write requests (number of lines in file)
            - throughput: (size/duration) * 1024 = MbPS

        Also look into a simple CVS parser. I have used https://github.com/ben-strasser/fast-cpp-csv-parser
        but not sure about licensing, as this is BSD license. I can write my own, but I'd prefer to
        not re-invent the wheel.
    */

}

void closeFiles() {
    if (ioPrefFile.is_open()) ioPrefFile.close();
    if (threadPrefFile.is_open()) threadPrefFile.close();
    if (timePrefFile.is_open()) timePrefFile.close();
}

PerformanceLogger& PerformanceLogger::instance() {
    static PerformanceLogger _instance;
    return _instance;
}

void PerformanceLogger::doNotify(string message, ostream& fileName)
{
    //implemented by Daniel
}

void PerformanceLogger::logIO(
        const string & operation,
        const string & object,
        const timespec startTime,
        const timespec endTime,
        const size_t size ) {
        //const size_t requests) {
    //output info
    stringstream message;
    const timespec duration = timespecDiff(endTime,startTime);
    const pid_t tid = syscall(SYS_gettid);
    //double throughput = (timespecToDouble(duration)*size) / 1024;
    message.setf(ios_base::fixed, ios_base::floatfield);
    message << operation << ','
            << object    << ','
            << startTime << ','
            << endTime   << ','
            << duration  << ','
            << tid       << ','
            << size;

    this->doNotify(message.str(),ioPrefFile);
}

/*void PerformanceLogger::logThread() {
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
  message << setprecision(9)/*
          << operation << " " << object << ": "
          << "start time: " << startTime
          << ", "
          << "end time: " << endTime
          << ", "
          << "duration: " << duration
          << ", "
          << "thread: " << tid*/;
  message << operation << ','
          << object    << ','
          << startTime << ','
          << endTime   << ','
          << duration  << ','
          << tid       << ',';

  if (size > 0) {
    message << size;
  }

  this->doNotify(message.str(),timePrefFile);
}
