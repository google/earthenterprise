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
 * Singleton class for profiling events. The programmer can use
 * this class to mark the beginning and end of events, and the timing of those
 * events will be written out to a log file. This class is intended for
 * performance debugging.
 */
class Profiler {
  public:
    static Profiler * instance() { return _instance; }

    inline void Begin(const std::string & operation, const std::string & object, const size_t size) {
      log(BEGIN, operation, object, size);
    }

    inline void End(const std::string & operation, const std::string & object) {
      log(END, operation, object);
    }

  private:
    enum ProfileEvent { BEGIN, END };

    static const double NSEC_TO_SEC = 1e-9;
    static Profiler * const _instance;

    Profiler() {}
    void log(ProfileEvent event, const std::string & operation, const std::string & object, const size_t size = 0);
    double getTime() const;
};

/*
 * A convenience class for profiling a block of code. Profiling begins when an
 * instance of this class is created and ends when the instance goes out of
 * scope.
 */
class BlockProfiler {
  public:
    BlockProfiler(const std::string & operation, const std::string & object, const size_t size) :
        operation(operation), object(object)
    {
      profiler->Begin(operation, object, size);
    }
    ~BlockProfiler() {
      profiler->End(operation, object);
    }
  private:
    static Profiler * const profiler;
    const std::string operation;
    const std::string object;
};

// Programmers should use these functions to profile code rather than the
// functions above. This makes it easy to use compile time flags to exclude
// the profiling code.
#define BEGIN_PROFILING(op, obj, size) { BlockProfiler prof(op, obj, size)
#define END_PROFILING() }

#endif // PROFILER_H