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


#ifndef _khProgressMeter_h_
#define _khProgressMeter_h_

#include <stdlib.h>
#include <string>
#include <qdatetime.h>
#include <deque>
#include "khTypes.h"

namespace std {
  template<class T>
  T abs(const T& val) {
    return (val > 0 ? val : -val);
  }
}


class khProgressMeter
{
  class IntervalStats {
   public:
    int64 totalCount;
    int64 doneCount;
    int64 skippedCount;
    int64 elapsedMS;

    inline int64 processedCount(void) const {
      return doneCount + skippedCount;
    }
    inline double progressFraction(void) const {
      return double(processedCount()) / double(totalCount);
    }

    IntervalStats(int64 total,
                  int64 done = 0, int64 skipped = 0,
                  int64 elapsed = 0) :
        totalCount(total),
        doneCount(done),
        skippedCount(skipped),
        elapsedMS(elapsed)
    { }
  };


  // private and unimplemented - illegal to copy
  khProgressMeter(const khProgressMeter&);
  khProgressMeter& operator=(const khProgressMeter &);
 public:
  khProgressMeter( int64 totalSteps, const QString &desc_ = "tiles", const std::string& progress_meter_prefix = "");
  ~khProgressMeter(void);

  inline void incrementDone(int64 delta = 1);
  inline void incrementSkipped(int64 delta = 1);
  inline void incrementTotal(int64 delta);

  // convenience for backward compatibility
  inline void increment(void) { incrementDone(1); }

  // call for explicit finalization, otherwise will finalize in destructor
  // prints summary statistics
  void finalize(void);

  // just a little utility, for myself and others
  static QString msToString( int64 ms );
 private:
  QString desc;

  // use signed rather than unsigned so logic doesn't need to worry
  // about wrapping if dropping below 0
  IntervalStats overallStats;

  // List of previous report intervals. Always contains at least one.
  // If you want a running average over 3 intervals, the deque will actually
  // contain up to 4 records. The 1st entry gives you the starting numbers
  // for the remaining intervals.
  static const uint NumRunningAverageIntervals = 5;
  std::deque<IntervalStats> intervals;
  const IntervalStats& prevStats(void) const { return intervals.back(); }


  int64 countUntilNextCheck;
  QTime timer;
  bool finalized;
  const std::string progress_meter_prefix_;

  static const double reportTotalChangedByFraction;
  static const double reportFractionDoneInterval;
  static const double reportMSInterval;

  double RunningTilesPerMS(void) const;
  double AverageTilesPerMS(void) const;
  bool NeedReport(void) const;
  void CheckAndReportIfNecessary(void);
  void DoReport(void);
};


inline void khProgressMeter::incrementDone(int64 delta) {
  overallStats.doneCount += delta;
  countUntilNextCheck -= std::abs(delta);
  if (countUntilNextCheck <= 0) {
    CheckAndReportIfNecessary();
  }
}

inline void khProgressMeter::incrementSkipped(int64 delta) {
  overallStats.skippedCount += delta;
  countUntilNextCheck -= std::abs(delta);
  if (countUntilNextCheck <= 0) {
    CheckAndReportIfNecessary();
  }
}

inline void khProgressMeter::incrementTotal(int64 delta) {
  overallStats.totalCount += delta;
  countUntilNextCheck -= std::abs(delta);
  if (countUntilNextCheck <= 0) {
    CheckAndReportIfNecessary();
  }
}


#endif // ! _khProgressMeter_h_
