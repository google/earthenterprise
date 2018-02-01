// Copyright 2018 the Open GEE Contributors
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

#ifndef PROFILER_H
#define PROFILER_H

#include <string>

/*
 * Singleton class for profiling events. The programmer can use this class
 * to log the timing of operations. This class is intended for performance
 * debugging.
 */
class Profiler {
  public:
    static Profiler * instance() { return _instance; }
    void log(
        const std::string & operation, // The operation being timed
        const std::string & object,    // The object that the operation is performed on
        const double startTime,        // The start time of the operation
        const double endTime,          // The end time of the operation
        const size_t size = 0);        // The size of the object, if applicable
  private:
    static Profiler * const _instance;
    Profiler() {}
};

/*
 * A convenience class for profiling a block of code. Profiling begins when an
 * instance of this class is created and ends when the instance goes out of
 * scope.
 */
class BlockProfiler {
  public:
    BlockProfiler(
        const std::string & operation,
        const std::string & object,
        const size_t size);
    ~BlockProfiler();
  private:
    static Profiler * const profiler;
    const std::string operation;
    const std::string object;
    const size_t size;
    const double startTime;
};

// Programmers should use these macros to profile code instead of using the
// classes above directly. This makes it easy to use compile time flags to
// exclude the profiling code.
// The call to this macro should be the first statement in the block you want
// to profile - it will not profile anything that comes before it.
#define PROFILE_BLOCK(op, obj) BlockProfiler _prof_instance(op, obj)
#define PROFILE_BLOCK_SIZE(op, obj, size) BlockProfiler _prof_instance(op, obj, size)

#endif // PROFILER_H