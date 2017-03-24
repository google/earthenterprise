/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef _khTimer_h_
#define _khTimer_h_

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

// Fake dependency for forcing a rebuild based on the build command switch.
#include <enableInternalUseFlag.h>

typedef unsigned long long khTimer_t;

/** khTimer is a microsecond resolution time wrapper around
 *  "gettimeofday". The "tick" method should not be called in
 *  inner loops of any consequence.
 *
 *  Originally written to be a: high resolution, low latency
 *  time stamper. We had to simplify it because of issues of the
 *  clock frequency changing within a run (due to power saving
 *  features).
 *
 *  TODO: replace this with ordinary timers. Note
 *  that the current class really should be static, but we are
 *  deferring this structural change until we switch to ordinary
 *  timers. */
class khTimer {
 private:
  khTimer();
  khTimer(const khTimer&);
  void operator=(const khTimer&);

 public:
  // Return the time of day as the current time in microseconds.
  static inline khTimer_t tick(void) {
    timeval tv;
    gettimeofday(&tv, NULL);
    const khTimer_t ts = (khTimer_t)tv.tv_sec * 1000000 + tv.tv_usec;
    return ts;
  }

  // seconds
  static inline double delta_s( const khTimer_t &t1, const khTimer_t &t2 ) {
    const khTimer_t delta = t2 - t1;
    // The tick values are in units of microseconds.
    return ((double)delta*1e-6);
  }

  // millisecond = one thousandth of a second = 1.0 / 1,000 seconds
  static inline double delta_m( const khTimer_t &t1, const khTimer_t &t2 ) {
    const khTimer_t delta = t2 - t1;
    // The tick values are in units of microseconds.
    return ((double)delta*1e-3);
  }

#ifdef GEE_INTERNAL_USE_ONLY
  // To be used for debugging and printing delta times.
  // Usage: khTimer_t timer = khTimer::tick();
  //        ... // some code lines
  //        PrintDelta(__LINE__, "a key to distinguish where", timer);
  //        The key can be __FILE__
  static void PrintDelta(int line, const char* key, khTimer_t& timer) {
    khTimer_t now = khTimer::tick();
    double elapsed = khTimer::delta_s(timer, now);
    fprintf(stderr, "Delta at %4d: %20s : %12.4f\n", line, key, elapsed);
    timer = now;
  }
#endif

};

#endif // ! _khTimer_h_
