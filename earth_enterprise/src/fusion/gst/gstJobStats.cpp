// Copyright 2017 Google Inc.
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


#include <gstJobStats.h>

#ifdef JOBSTATS_ENABLED

#include <notify.h>

std::vector<gstJobStats*>* gstJobStats::stat_objects = nullptr;

gstJobStats::gstJobStats(const std::string& group_name,
                         const JobName* names, int job_count)
    : group_name_(group_name),
      job_names_(names),
      job_count_(job_count),
      jobs_(new JobUnit[job_count]) {
  if (stat_objects == nullptr) {
    stat_objects = new std::vector<gstJobStats*>;
  }
  stat_objects->push_back(this);
  Reset();
}

gstJobStats::~gstJobStats() {
  delete [] jobs_;
}

void gstJobStats::Reset() {
  for (int j = 0; j < job_count_; ++j) {
    jobs_[j].total_time = 0.0;
    jobs_[j].total_run = 0;
  }
}

void gstJobStats::DumpStats() {
  for (int j = 0; j < job_count_; ++j) {
    notify(NFY_PROGRESS, "%s:  %s: %.6lf - %d", group_name_.c_str(),
           job_names_[j].name, jobs_[j].total_time, jobs_[j].total_run);
  }
}

void gstJobStats::Begin(int job_id) {
  jobs_[job_id].begin_time = khTimer::tick();
}

void gstJobStats::End(int job_id) {
  jobs_[job_id].total_time += khTimer::delta_s(
      jobs_[job_id].begin_time, khTimer::tick());
  ++jobs_[job_id].total_run;
}

void gstJobStats::DumpAllStats() {
  if (stat_objects != nullptr) {
    for (std::vector<gstJobStats*>::iterator it = stat_objects->begin();
         it != stat_objects->end(); ++it) {
      (*it)->DumpStats();
    }
  }
}

void gstJobStats::ResetAll() {
  if (stat_objects != nullptr) {
    for (std::vector<gstJobStats*>::iterator it = stat_objects->begin();
         it != stat_objects->end(); ++it) {
      (*it)->Reset();
    }
  }
}

#endif
