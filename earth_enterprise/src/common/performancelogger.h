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
#include <cassert>
#include "common/timeutils.h"

enum EnvelopeType { PERFORMANCE, IO, NUM_ENVELOPE_TYPE_ENUMS };

/*
    Data envelope: used as a struct since there is essentially no
    logic included for the adt, outside of toString(), which is a
    proto-strategy pattern. All sub envelopes should base themselves on
    perfEnvelope and encapsulate their own log string logic on top.
    Takes the place of the parameter list for PerformanceLogger::log()
*/

struct perfEnvelope {
    std::string operation;
    std::string object;
    timespec startTime;
    timespec endTime;
    size_t size;
    virtual std::string toString();
    virtual ~perfEnvelope() {};
};

struct ioEnvelope : perfEnvelope {
    size_t requests;
    size_t throughput;
    std::string toString();
};

/*
 * Singleton class for logging event performance. This class is intended for
 * performance debugging.
 */
class PerformanceLogger {
  public:
    static PerformanceLogger * const instance() { return _instance; }
    void log(
        perfEnvelope * envelope); // the envelope
  private:
    static PerformanceLogger * const _instance;
    PerformanceLogger() {  }
    void doNotify(const std::string & message);
};

const int max_types = 2; //TODO: enumeration
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
        const EnvelopeType & eType,
        const size_t size = 0) :
      operation(operation),
      object(object),
      eType(eType),
      size(size),
      startTime(getime::getMonotonicTime()),
      ended(false) {}
    void end(EnvelopeType type) { //TODO: turn into enum
      if (!ended) {
        assert(type > 0 && type < NUM_ENVELOPE_TYPE_ENUMS);
        ended = true;
        perfEnvelope * envelope;
        const timespec endTime = getime::getMonotonicTime();
        if (type == PERFORMANCE) {
            envelope = new perfEnvelope;
            envelope->operation = operation;
            envelope->object = object;
            envelope->startTime = startTime;
            envelope->endTime = endTime;
            envelope->size = size;
        } else if (type == IO) {
            ioEnvelope* temp = new ioEnvelope;
            envelope = temp;
        }
        PerfLoggerCls::instance()
            ->log(envelope);//operation, object, startTime, endTime, size);
          delete envelope;
      }
    }
    void setType (EnvelopeType type) { eType = type; }
    ~BlockPerformanceLogger() {
      end(eType);
    }
  private:
    const std::string operation;
    const std::string object;
    const EnvelopeType eType;
    const size_t size;
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
  BlockPerformanceLogger<PerformanceLogger> name(op, __VA_ARGS__);
#define CREATE_PERF_ENVELOPE
#define END_PERF_LOGGING(name) name.end(PERF)


#endif // PERFORMANCELOGGER_H
