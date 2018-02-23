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
#include <cstdlib>
#include "common/performancelogger.h"
#include "common/notify.h"
#include "common/timeutils.h"
#include <cassert>

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
      ioPrefFile << "operation,object,startTime,endTime,threadID,bufferSize,duration" << endl;
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
            - throughput: (size/1024) / duration = MbPS
    */
    ioPrefFile.close();
    fstream file(ioFileName.c_str(),fstream::in);
    string line;
    getline(file,line); //ignore header

    int requests=0;
    double durationTot=0, sizeTot=0;

    // loop through file, extract buffer size and duration
    while(getline(file,line)) {
      ++requests;
      size_t pos = line.rfind(',');
      size_t len = line.length();
      durationTot += strtod(line.substr(pos+1,len-1).c_str(),0);
      len = pos-1;
      pos = line.rfind(',',len);
      sizeTot += strtod(line.substr(pos+1,len-pos).c_str(),0);
    }

    // dont divide by 0
    assert(durationTot > 0);

    // calculate throughput
    double throughput = ((sizeTot/1024)/durationTot);

    // close and reopen for writing
    file.close();
    file.open(ioFileName.c_str(), fstream::out | fstream::app);
    file << "requests,throughput" << endl
         << requests << ',' << throughput << endl;
    file.close();
}

void closeFiles() {
    if (ioPrefFile.is_open()) ioPostProcess();
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
            << tid       << ','
            << size       << ','
            << duration;

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
  message << setprecision(9)
          << operation << ','
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
