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
#include "common/notify.h"
#include "common/timeutils.h"
#include "common/performancelogger.h"
#include <cassert>

using namespace std;
using namespace getime;

// make sure static members get initialized
string PerformanceLogger::ioFileName = "";
string PerformanceLogger::threadFileName = "";
string PerformanceLogger::timeFileName = "";

void PerformanceLogger::generateFileNames() {

    // needs mutex lock
    time_t t = time(0);
    tm date = *localtime(&t);
    char buf[256];
    if (!ioFileName.size()) {
        strftime(buf, sizeof(buf), "io_stats.%m-%d-%Y-%H:%M:%S.csv", &date);
        ioFileName = buf;
    }
    if (!timeFileName.size()) {
        strftime(buf, sizeof(buf), "time_stats.%m-%d-%Y-%H:%M:%S.csv", &date);
        timeFileName = buf;
    }
    if (!threadFileName.size()) {
        strftime(buf, sizeof(buf), "thread_stats.%m-%d-%Y-%H:%M:%S.csv", &date);
        threadFileName = buf;
    }
}

/*
    Do IO post processing and output:
        - total number of write requests (number of lines in file)
        - throughput: (size/1024) / duration = MbPS

    WILL THIS BE DONE IN C++ OR WILL IT BE ANALYZED AFTER VIA ANOTHER
    METHOD?

    As for right now, this is not being called
*/

void ioPostProcess()
{
    /*fstream file(ioFileName.c_str(),fstream::in);
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
    file.close();*/
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

    assert(ioFileName.size());
    //Daniel: perhaps move open/close to doNotify?
    fstream ioPrefFile(ioFileName.c_str(), fstream::out | fstream::app);
    doNotify(message.str(),ioPrefFile);
    ioPrefFile.close();
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
          << tid       << ','
          << size;
  assert(timeFileName.size());
  //Daniel: perhaps move open/close to doNotify?
  fstream timePrefFile(timeFileName.c_str(), fstream::out | fstream::app);
  doNotify(message.str(),timePrefFile);
  timePrefFile.close();
}
