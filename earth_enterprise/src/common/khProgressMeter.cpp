// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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


#include "khProgressMeter.h"

#include <stdio.h>
#include <stdlib.h>
#include <cmath>
#include "procpidstats.h"

const double khProgressMeter::reportTotalChangedByFraction = 0.01; // 1%
const double khProgressMeter::reportFractionDoneInterval = 0.05;   // 5%
const double khProgressMeter::reportMSInterval = 5 * 60 * 1000;    // 5 minutes

khProgressMeter::khProgressMeter( std::int64_t totalSteps, const QString &desc_,
                                  const std::string& prefix) :
    desc(desc_),
    overallStats(totalSteps),
    countUntilNextCheck(1),
    finalized(false),
    progress_meter_prefix_(prefix)
{
  // create the 1st interval - it serves as a reference point for later
  // intervals.
  intervals.push_back(IntervalStats(totalSteps));

  if (totalSteps != 0)
    fprintf(stderr, "%sTotal %s to process: %lld\n",
            progress_meter_prefix_.c_str(), desc.latin1(),
            (long long)totalSteps);

  timer.start();
}


khProgressMeter::~khProgressMeter(void) {
  if (!finalized) {
    if (std::uncaught_exception()) {
      // The stack is being unwound as the result of an exception.
      // This means we haven't successfully finished processing, so
      // don't report the finish stats to the user

      // Do Nothing ...
    } else {
      finalize();
    }
  }
}


void
khProgressMeter::finalize(void) {
  finalized = true;

  // if necessary, make one last report
  if ((overallStats.processedCount() != prevStats().processedCount()) ||
      (overallStats.totalCount != prevStats().totalCount)) {
    overallStats.elapsedMS += (std::int64_t) timer.restart();
    DoReport();
  }

  if (overallStats.totalCount != 0) {
    fprintf(stderr, "%sProcessed %lld %s\n", progress_meter_prefix_.c_str(),
            (long long)overallStats.doneCount, desc.latin1());
    if (overallStats.skippedCount) {
      fprintf(stderr, "%sSkipped    %lld %s\n", progress_meter_prefix_.c_str(),
              (long long)overallStats.skippedCount, desc.latin1());
    }
    fprintf(stderr, "%sTotal time to process: %s\n",
            progress_meter_prefix_.c_str(),
            msToString(overallStats.elapsedMS).latin1());
    fprintf(stderr, "%sAverage %s per second: %.2lf\n",
            progress_meter_prefix_.c_str(),
            desc.latin1(),
            AverageTilesPerMS() * 1000);
  }
}


double
khProgressMeter::AverageTilesPerMS(void) const {
  return (double(overallStats.processedCount()) /
          double(overallStats.elapsedMS));;
}

double
khProgressMeter::RunningTilesPerMS(void) const {
  std::int64_t runningProcessed = overallStats.processedCount() -
                           intervals.front().processedCount();
  std::int64_t runningElapsedMS = overallStats.elapsedMS -
                           intervals.front().elapsedMS;

  return double(runningProcessed) / double(runningElapsedMS);
}


bool
khProgressMeter::NeedReport(void) const {
  // Don't report in the first second, we likely won't have enough samples
  // yet to give any reasonable averages
  if (overallStats.elapsedMS < 1000) {
    return false;
  }


  // Report if my total count has changed too much
  if (overallStats.totalCount != prevStats().totalCount) {
    std::int64_t diff = overallStats.totalCount - prevStats().totalCount;
    if (fabs(double(diff) / (double)prevStats().totalCount) >
        reportTotalChangedByFraction) {
      return true;
    }
  }

  // Report if enough time has elapsed since the last report
  if (overallStats.elapsedMS - prevStats().elapsedMS > reportMSInterval) {
    return true;
  }

  // Report if enough progess has been made since last report
  if (std::abs(overallStats.progressFraction() -
               prevStats().progressFraction()) > reportFractionDoneInterval) {
    return true;
  }

  return false;
}


void
khProgressMeter::CheckAndReportIfNecessary(void) {
  // get elapsed time and reset timer (atomic)
  overallStats.elapsedMS += timer.restart();

  if (NeedReport()) {
    DoReport();
  }

  // calculate next check delay
  // We don't want to check the timer each time (too slow).
  // So we calculate how many counts we're going to wait before
  // checking the timer.
  if ((overallStats.doneCount < 1) || (overallStats.elapsedMS < 1000)) {
    // we really don't have enough samples yet to trust any averages
    countUntilNextCheck = 1;
  } else {
    double tilesPerMS = RunningTilesPerMS();

    // how long until I need to report based on elapsed time?
    double msToElapsedReport =
      reportMSInterval - (overallStats.elapsedMS -
                          prevStats().elapsedMS);

    // how long until I need to report based on progress?
    double fractionToProgressReport = reportFractionDoneInterval -
                                      fabs(overallStats.progressFraction() -
                                           prevStats().progressFraction());
    std::int64_t countToProgressReport =
      std::int64_t(fractionToProgressReport * overallStats.totalCount);
    double msToProgressReport = countToProgressReport / tilesPerMS;

    // estimate next check as 1/2 of the smaller of the two
    // we use 1/2 because we don't want to be late. :-)
    double msUntilNextCheck = std::min(msToElapsedReport,
                                       msToProgressReport) / 2;

    // bound the check delay (using time)
    static const std::int32_t msMaxCheckDelay = 30 * 1000; // max 30 sec
    static const std::int32_t msMinCheckDelay = 1000;      // min 1 sec
    if (msUntilNextCheck > msMaxCheckDelay) {
      msUntilNextCheck = msMaxCheckDelay;
    } else if (msUntilNextCheck < msMinCheckDelay) {
      msUntilNextCheck = msMinCheckDelay;
    }

    countUntilNextCheck = std::int64_t(msUntilNextCheck * tilesPerMS);
  }
}


void
khProgressMeter::DoReport(void) {
  // Collect cpu and other interesting stats.
  ProcPidStats proc_pid_stats;

  double tilesPerMS = RunningTilesPerMS();
  std::int64_t remainingCount = overallStats.totalCount -
                         overallStats.processedCount();
  std::int64_t remainingMS = std::int64_t(remainingCount / tilesPerMS);


  fprintf(stderr,
          "%sCompleted %3.0lf%% (%lld/%lld) - %s/sec: %.2lf - time left: %s",
          progress_meter_prefix_.c_str(),
          overallStats.progressFraction() * 100,
          (long long)overallStats.processedCount(),
          (long long)overallStats.totalCount,
          desc.latin1(),
          tilesPerMS * 1000.0,
          (remainingMS >= 0) ? msToString(remainingMS).latin1() : "???");
  // Add more CPU stats for this process (useful for tracking performance issues).

  int cpu_percentage = proc_pid_stats.CpuUsagePercentage();
  int memory_usage_megabytes = proc_pid_stats.MemoryUsageMegaBytes();
  long page_faults = proc_pid_stats.MajorPageFaults();
  fprintf(stderr, " [CPU: %d Mem: %d MB PF: %ld]\n",
          cpu_percentage, memory_usage_megabytes, page_faults);

  // make a copy of the overallStats and save it in the intervals deque
  intervals.push_back(overallStats);
  if (intervals.size() > NumRunningAverageIntervals + 1) {
    intervals.pop_front();
  }
}


QString khProgressMeter::msToString( std::int64_t ms )
{
  bool negative = false;
  if (ms < 0) {
    negative = true;
    ms = -ms;
  }

  const float msPerSec = 1000.0;
  const float msPerMin = msPerSec * 60.0;
  const float msPerHour = msPerMin * 60.0;
  const float msPerDay = msPerHour * 24.0;

  float msLeft = float( ms );

  unsigned int days = uint( msLeft / msPerDay );
  msLeft -= days * msPerDay;

  unsigned int hours = uint( msLeft / msPerHour );
  msLeft -= hours * msPerHour;

  unsigned int mins = uint( msLeft / msPerMin );
  msLeft -= mins * msPerMin;

  unsigned int secs =  uint( ( msLeft / msPerSec ) + 0.5 );

  char timebuf[ 20 ];
  snprintf( timebuf, 20, "%02u:%02u:%02u", hours, mins, secs );

  QString timeStr;
  if ( days > 1 )
    timeStr = QString( "%1 Days, " ).arg( days );
  else if ( days == 1 )
    timeStr = QString( "%1 Day, " ).arg( days );

  timeStr += timebuf;

  return negative ? "- " + timeStr : timeStr;
}
