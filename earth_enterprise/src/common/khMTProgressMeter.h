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

#ifndef __khMTProgressMeter_h
#define __khMTProgressMeter_h

#include "khProgressMeter.h"
#include "khThread.h"


// ****************************************************************************
// ***  khMTProgressMeter - MT safe progress meter
// ****************************************************************************

class khMTProgressMeter {
 private:
  khProgressMeter impl;
  khMutex         lock;

 public:
  khMTProgressMeter(std::int64_t totalSteps, const QString &desc_ = "tiles")
      : impl(totalSteps, desc_)
  { }

  inline void incrementDone(std::int64_t delta = 1) {
    khLockGuard guard(lock);
    impl.incrementDone(delta);
  }
  inline void incrementSkipped(std::int64_t delta = 1) {
    khLockGuard guard(lock);
    impl.incrementSkipped(delta);
  }
  inline void incrementTotal(std::int64_t delta) {
    khLockGuard guard(lock);
    impl.incrementTotal(delta);
  }

  // convenience for backward compatibility
  inline void increment(void) { incrementDone(); }
};


#endif /* __khMTProgressMeter_h */
