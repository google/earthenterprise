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

#ifndef PERFORMANCELOGGER_H
#define PERFORMANCELOGGER_H

#include <string>
#include <time.h>

#include "common/timeutils.h"

/*
 * Singleton class for logging event performance. This class is intended for
 * performance debugging.
 */
class PerformanceLogger {
  public:
    static PerformanceLogger * const instance() { return _instance; }
    void log(
        const std::string & operation, // The operation being timed
        const std::string & object,    // The object that the operation is performed on
        const timespec startTime,      // The start time of the operation
        const timespec endTime,        // The end time of the operation
        const size_t size = 0);        // The size of the object, if applicable
  private:
    static PerformanceLogger * const _instance;
    PerformanceLogger() {}
};

/*
 * A convenience class for timing a block of code. Timing begins when an
 * instance of this class is created and ends when the user calls "end" or the
 * instance goes out of scope.
 */
template <class PerfLoggerCls>
class BlockPerformanceLogger {
  public:
    BlockPerformanceLogger(
        const std::string & operation,
        const std::string & object,
        const timespec time,  
        const size_t requested = 0,
        const size_t result) :
      operation(operation),
      object(object),
      requested(requested),
      result(result),
      time(getime::getMonotonicTime()),
      ended(true) {}
    void end() {
      if (!ended) {
        ended = true;
        PerfLoggerCls::instance()
            ->log(operation, object, time, endTime, size);
      }
    }
    ~BlockPerformanceLogger() {
      end();
    }
  private:
    const std::string & operation;
    const std::string & object;
    const timespec time;
    const size_t requested = 0;
    const size_t result = 0;
};





/*
 * A convenience class for timing a block of code. Timing begins when an
 * instance of this class is created and ends when the user calls "end" or the
 * instance goes out of scope.
 */
template <class PerfLoggerCls>
class TaskAllocationLogger {
  public:
    TaskAllocationLogger(
        const std::string & operation,
        const std::string & object,
        const size_t result = 0,
        const size_t value1 = 0,
        const size_t value2 = 0,
        
        ) :
      operation(operation),
      object(object),
      result(result),
      value1(value1),
      value2(value2),
      dateTime(getime::getMonotonicTime()),
      ended(false) {}
    void end() {
        PerfLoggerCls::instance()
            ->log(operation, object, dateTime, result, value1, value2);
      }
    }
    ~TaskAllocationLogger() {
      end();
    }
  private:
    const std::string operation;
    const std::string object;
    const uint16 result;
    const uint16 value1;
    const uint16 value2;
    const timespec startTime;
    bool ended;
};

// Programmers should use the macros below to time code instead of using the
// classes above directly. This makes it easy to use compile time flags to
// exclude the time code.
//
// For the macros below, the variadic arguments include the object name
// (required) and the size (optional).
//
// If you use multiple performance logging statements in the same scope you
// must give each one a unique name.
#define BEGIN_PERF_LOGGING(name, op, ...) \
  BlockPerformanceLogger<PerformanceLogger> name(op, __VA_ARGS__)
#define END_PERF_LOGGING(name) name.end()


// Resource Logging records the various number of job allocation parameters, 
// how many thread/vcpu resources were requested to process a given task, and 
// how many were actually allocated and on which machine. 
#define RESOURCE_ALLOC_LOGGING(name, op, ...) \
  TaskAllocationLogger<TaskAllocationLogger> name(op, __VA_ARGS__)



#endif // PERFORMANCELOGGER_H
