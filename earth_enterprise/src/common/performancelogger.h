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
 * instance of this class is created and ends when the instance goes out of
 * scope.
 */
template <class PerfLoggerCls>
class BlockPerformanceLogger {
  public:
    BlockPerformanceLogger(
        const std::string & operation,
        const std::string & object,
        const size_t size = 0) :
      perfLogger(PerfLoggerCls::instance()),
      operation(operation),
      object(object),
      size(size),
      startTime(getime::getMonotonicTime())
    {
      // Nothing to do - just initialize class members above
    }
    ~BlockPerformanceLogger() {
      const timespec endTime = getime::getMonotonicTime();
      perfLogger->log(operation, object, startTime, endTime, size);
    }
  private:
    PerfLoggerCls * const perfLogger;
    const std::string operation;
    const std::string object;
    const size_t size;
    const timespec startTime;
};

// Programmers should use the macros below to time code instead of using the
// classes above directly. This makes it easy to use compile time flags to
// exclude the time code.

// For all of the macros, the variadic arguments include the object name
// (required) and the size (optional).

// This macro will time a block of code. The call to this macro should be the
// first statement in the block you want to time - it will not time anything
// that comes before it. It will continue timinguntil it goes out of scope.
#define PERF_LOG_BLOCK(op, ...) \
  BlockPerformanceLogger<PerformanceLogger> _block_prof_inst(op, __VA_ARGS__)

// You can use these macros if you want to time a part of a block of code. If
// you use multiple perf loggin statements in the same block you must give each
// one a unique name.
#define BEGIN_PERF_LOGGING(name, op, ...) \
  BlockPerformanceLogger<PerformanceLogger> * name = \
      new BlockPerformanceLogger<PerformanceLogger>(op, __VA_ARGS__)
#define END_PERF_LOGGING(name) delete name

#endif // PERFORMANCELOGGER_H