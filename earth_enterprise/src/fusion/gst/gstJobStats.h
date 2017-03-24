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

#ifndef KHSRC_FUSION_GST_GSTJOBSTATS_H__
#define KHSRC_FUSION_GST_GSTJOBSTATS_H__

#include <enableJobStats.h>

#ifdef JOBSTATS_ENABLED
#include <string>
#include <vector>
#include <khTimer.h>
#include <common/base/macros.h>

class gstJobStats {
 public:
  struct JobName {
    int id_x;
    char* name;
  };

  struct JobUnit {
    khTimer_t begin_time;
    double total_time;
    int total_run;
    JobUnit() : total_time(0.0), total_run(0) {}
  };

  gstJobStats(const std::string& group_name,
              const JobName* names, int job_count);
  ~gstJobStats();

  class ScopedTimer {
   public:
    ScopedTimer(gstJobStats* stats, int job_id)
        : stats_(stats), job_id_(job_id) {
      stats_->Begin(job_id_);
    }
    ~ScopedTimer() {
      stats_->End(job_id_);
    }
   private:
    gstJobStats* stats_;
    int job_id_;
  };

  void Reset();
  void DumpStats();
  void Begin(int job_id);
  void End(int job_id);

  static void DumpAllStats();
  static void ResetAll();

 private:
  std::string group_name_;
  const JobName* job_names_;
  int job_count_;
  JobUnit* jobs_;

  static std::vector<gstJobStats*>* stat_objects;

  DISALLOW_COPY_AND_ASSIGN(gstJobStats);
};

#define JOBSTATS_BEGIN(obj, name) obj->Begin(name);
#define JOBSTATS_END(obj, name) obj->End(name);
#define JOBSTATS_DUMP(obj) obj->DumpStats();
#define JOBSTATS_SCOPED(obj, name) gstJobStats::ScopedTimer tmp##name(obj, name);
#define JOBSTATS_DUMPALL() gstJobStats::DumpAllStats();
#define JOBSTATS_RESETALL() gstJobStats::ResetAll();

#else

#define JOBSTATS_BEGIN(obj, name)
#define JOBSTATS_END(obj, name)
#define JOBSTATS_DUMP(obj)
#define JOBSTATS_SCOPED(obj, name)
#define JOBSTATS_DUMPALL()
#define JOBSTATS_RESETALL()

#endif

#endif  // !KHSRC_FUSION_GST_GSTJOBSTATS_H__
