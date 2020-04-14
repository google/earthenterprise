/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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

#ifndef __khTask_h
#define __khTask_h

#include <map>
#include <autoingest/sysman/.idl/TaskStorage.h>
#include "Reservation.h"
#include "TaskRequirements.h"
#include <qstring.h>
#include "common/performancelogger.h"


class khResourceProviderProxy;
class JobProgressMsg;

class khTask
{
 private:
  typedef std::map<std::string, khTask*> TaskVerrefMap;
  static TaskVerrefMap taskVerrefMap;

  // information about the task
  std::string              verref_;
  std::uint32_t                   taskid_;
  std::int32_t                    priority_;
  TaskDef                  taskdef_;
  TaskRequirements         requirements;
  std::vector<std::string> boundOutfiles;
#if 0 && defined(SUPPORT_TASK_RELOCATE)
  std::vector<std::string> relocatedOutfiles;
  std::map<std::string, std::string> relocateMap;
#endif

  // when I was submitted
  time_t                   submitTime_;

  // info about running time
  time_t                   beginTime_;
  time_t                   progressTime_;
  double                   progress_;

  // The provider who's handling my job
  khResourceProviderProxy *jobHandler;

  // my active resource reservations
  std::vector<Reservation> reservations_;

 public:
  // reason I was passed over during activation
  QString activationError;

 protected:
  ~khTask(void) throw();
 private:
  khTask(const SubmitTaskMsg &);

  std::string TaskFilename(void) const throw();
 public:
  // so I can update my beginTime, progressTime & progress
  void Progress(const JobProgressMsg &msg);

  bool bindOutfiles(const std::vector<Reservation> &res);
  void buildCommands(std::vector<std::vector<std::string> > &commands,
                     const std::vector<Reservation> &res);
  std::string LogFilename(void) const;

  // only way to create a khTask object
  static bool NewTask(const SubmitTaskMsg &) throw();
  static khTask* FindTask(const std::string &verref) throw();

  // only way to destroy a khTask object
  static void DeleteTask(khTask *) throw();
  static void LoseTask(khTask *) throw();
  static void FinishTask(khTask *, bool success,
                         time_t beginTime, time_t endTime) throw();

  // accessors
  std::string     verref(void) const throw() { return verref_; }
  std::uint32_t          taskid(void) const throw() { return taskid_; }
  std::int32_t         priority(void) const throw() { return priority_; }
  const TaskDef &taskdef(void) const throw() { return taskdef_; }
  time_t      submitTime(void) const throw() { return submitTime_;}
  time_t      beginTime(void) const throw()  { return beginTime_;}
  time_t      progressTime(void) const throw() { return progressTime_;}
  double      progress(void) const throw()   { return progress_;}
  const std::vector<Reservation> &reservations(void) const throw() {
    return reservations_;
  }
  const TaskRequirements &GetRequirementsRef(void) const throw() {
    return requirements;
  }


  // Called to switch from waitingQueue to handler
  void Activated(const std::vector<Reservation> &res,
                 khResourceProviderProxy *handler) throw();
};


#endif /* __khTask_h */
