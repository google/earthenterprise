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


#include "khTask.h"
#include <khstl.h>
#include <khFileUtils.h>
#include <notify.h>
#include "khResourceProviderProxy.h"
#include "AssetVersionD.h"
#include <autoingest/sysman/.idl/JobStorage.h>
#include <autoingest/geAssetRoot.h>
#include "common/performancelogger.h"


// ****************************************************************************
// ***  static members - must be protected by theResourceManager.mutex
// ****************************************************************************
khTask::TaskVerrefMap khTask::taskVerrefMap;


// ****************************************************************************
// ***  khTask
// ****************************************************************************
khTask::khTask(const SubmitTaskMsg &msg)
    : verref_(msg.verref), taskid_(msg.taskid),
      priority_(msg.priority), taskdef_(msg.taskdef),
      requirements(taskdef_, taskid_, verref_), // <-- can throw exception
      submitTime_(time(0)),
      beginTime_(0),
      progressTime_(0),
      progress_(0),
      jobHandler(0)
{
  // It's assumed that the caller already removed any existing
  // task for this verref

  // make our persistent copy (it's just a symlink)
  if (!khSymlink(verref_, TaskFilename())) {
    std::string msg { kh::tr("Unable to write ").toUtf8().constData() };
    msg += TaskFilename();
    throw khErrnoException(msg.c_str());
  }

  // now that we're persistent, add myself to the lists
  taskVerrefMap.insert(std::make_pair(verref_, this));
  theResourceManager.InsertWaitingTask(this);
}


// ****************************************************************************
// ***  ~khTask
// ****************************************************************************
khTask::~khTask(void) throw()
{
  assert(taskid_);

  if (jobHandler) {
    // stops the job
    // removes me from his list
    // takes ownership of my reservations (until job finished)
    jobHandler->StopTask(this);
  } else {
    theResourceManager.EraseWaitingTask(this);

    // Reservation destructors will release reservation and
    // remove the files they represent. This would happen
    // automatically at the end of this destructor. But I just felt like
    // being explicit
    reservations_.clear();
  }

  taskVerrefMap.erase(verref_);

  // remove my persistence
  if (!khUnlink(TaskFilename())) {
    notify(NFY_WARN, "Unable to unlink %s", TaskFilename().c_str());
  }

  // help to see if we're deleting the task twice
  taskid_ = 0;
}


// ****************************************************************************
// ***  Activated
// ****************************************************************************
void
khTask::Activated(const std::vector<Reservation> &res,
                  khResourceProviderProxy *handler) throw() {
  reservations_ = res;
  jobHandler = handler;
  theResourceManager.EraseWaitingTask(this);
}


// ****************************************************************************
// ***  Accessors
// ****************************************************************************
void
khTask::Progress(const JobProgressMsg &msg)
{
  beginTime_ = msg.beginTime;
  progressTime_ = msg.progressTime;
  progress_ = msg.progress;
}


std::string
khTask::LogFilename(void) const
{
  return LeafAssetVersionImpl::LogFilename(verref_);
}

bool
khTask::NewTask(const SubmitTaskMsg &msg) throw()
{
  try {
    // will add itself into taskVerrefMap
    (void) new khTask(msg);
    return true;
  } catch (const std::exception &e) {
    notify(NFY_WARN, "Exception during SubmitTask (NewTask): %s Error: %s",
           msg.verref.c_str(), e.what());
    LeafAssetVersionImplD::WriteFatalLogfile(msg.verref,
                                             "SubmitTask",
                                             e.what());
  } catch (...) {
    notify(NFY_WARN, "Unknown exception during SubmitTask (NewTask): %s",
           msg.verref.c_str());
    LeafAssetVersionImplD::WriteFatalLogfile(msg.verref,
                                             "SubmitTask",
                                             "Unknown error");
  }
  return false;
}

khTask*
khTask::FindTask(const std::string &verref) throw()
{
  TaskVerrefMap::iterator found = taskVerrefMap.find(verref);
  if (found != taskVerrefMap.end()) {
    return found->second;
  }
  return 0;
}


void
khTask::DeleteTask(khTask *task) throw()
{
  // I've been explictly asked to delete this task
  // So the asset manager already knows I'm going away
  // no need to notify anybody of anything
  delete task;
}


void
khTask::LoseTask(khTask *task) throw()
{
  // This task has been lost (for whatever reason)
  // let the asset manager know
  theResourceManager.NotifyTaskLost(TaskLostMsg(task->verref_,
                                                task->taskid_));

  // if it's been lost there's no reason to try to have the handler
  // try to stop the task
  task->jobHandler = 0;

  delete task;
}


void
khTask::FinishTask(khTask *task, bool success,
                   time_t beginTime, time_t endTime) throw()
{
  // This task has finished
  // let the asset manager know
  TaskDoneMsg doneMsg(task->verref_, task->taskid_,
                      success, beginTime, endTime,
#if 0 && defined(SUPPORT_TASK_RELOCATE)
                      task->relocatedOutfiles
#else
                      task->boundOutfiles
#endif
                      );
  if (success) {
    // release reservations so the files can persist
    for (std::vector<Reservation>::iterator r =
           task->reservations_.begin();
         r != task->reservations_.end(); ++r) {
      (*r)->Release();
    }
  }

  theResourceManager.NotifyTaskDone(doneMsg);

  // if it's finished there's no reason to try to have the handler
  // try to stop the task
  task->jobHandler = 0;

  delete task;
}


// ****************************************************************************
// ***  TaskFilename
// ****************************************************************************
std::string
khTask::TaskFilename(void) const throw()
{
  return khComposePath(geAssetRoot::Dirname(AssetDefs::AssetRoot(),
                                            geAssetRoot::StateDir),
                       ToString(taskid_) + ".task");
}



// ****************************************************************************
// ***  buildCommands
// ****************************************************************************

static std::string
passthrough(const std::string &s)
{
  return s;
}


bool
khTask::bindOutfiles(const std::vector<Reservation> &res)
{
  unsigned int numOutputs = taskdef_.outputs.size();
  boundOutfiles.clear();
  boundOutfiles.reserve(numOutputs);
#if 0 && defined(SUPPORT_TASK_RELOCATE)
  relocatedOutfiles.clear();
  relocatedOutfiles.reserve(numOutputs);
  relocateMap.clear();
#endif
  for (unsigned int o = 0; o < numOutputs; ++o) {
    const TaskDef::Output &defout = taskdef_.outputs[o];
#if 0 && defined(SUPPORT_TASK_RELOCATE)
    const TaskRequirements::Output &reqout = requirements.outputs[o];
#endif
    std::string bound;
    for (std::vector<Reservation>::const_iterator r = res.begin();
         r != res.end(); ++r) {
      if ((*r)->BindFilename(defout.path, bound)) {
        break;
      }
    }
    if (bound.empty()) {
      boundOutfiles.clear();
      return false;
    }
    boundOutfiles.push_back(bound);
#if 0 && defined(SUPPORT_TASK_RELOCATE)
    if (reqout.relocateURI.Valid()) {
      std::string relocPath(reqout.relocateURI.NetworkPath());
      relocatedOutfiles.push_back(relocPath);
      relocateMap[bound] = relocPath;
    } else {
      relocatedOutfiles.push_back(bound);
    }
#endif
  }
  return true;
}


void
khTask::buildCommands(std::vector<std::vector<std::string> > &outcmds,
                      const std::vector<Reservation> &res)
{
  // My outputs have already been bound, now just do the variable
  // substitution in the command line
  // $OUTPUT          -> alias for $OUTPUTS[0]
  // $OUTPUTS[<num>]
  // $OUTPUTS
  // $INPUT           -> alias for $INPUTS[0]
  // $INPUTS[<num>]
  // $INPUTS
  // also handle :basename, :dirname, :sansext
  // extensions on variable names
  // $OUTPUT:sansext
  // $INPUTS[3]:dirname

  // Other possible substitutions
  // $NUMCPU  -> number of CPUs assigned to the task


  std::vector<std::string> inputs;
  inputs.reserve(taskdef_.inputs.size());
  for (std::vector<TaskDef::Input>::const_iterator i =
         taskdef_.inputs.begin();
       i != taskdef_.inputs.end(); ++i) {

    inputs.push_back(i->path);
  }

  unsigned int numcmds = taskdef_.commands.size();
#if 0 && defined(SUPPORT_TASK_RELOCATE)
  outcmds.resize(numcmds + relocateMap.size());
#else
  outcmds.resize(numcmds);
#endif
  for (unsigned int cmdnum = 0; cmdnum < numcmds; ++cmdnum) {
    std::vector<std::string> &outcmd(outcmds[cmdnum]);

    for (std::vector<std::string>::const_iterator arg =
           taskdef_.commands[cmdnum].begin();
         arg != taskdef_.commands[cmdnum].end(); ++arg) {
      if (arg->empty()) continue;
      unsigned int len = arg->size();
      bool used = false;
      if ((*arg)[0] == '$') {
        const std::vector<std::string> *vec = 0;
        unsigned int i = 0;
        if (StartsWith(*arg, "$OUTPUT")) {
          vec = &boundOutfiles;
          i = 7;
        } else if (StartsWith(*arg, "$INPUT")) {
          vec = &inputs;
          i = 6;
        } else if (*arg == "$NUMCPU") {
          unsigned int numcpu = 0;
          for (std::vector<Reservation>::const_iterator r = res.begin();
               r != res.end(); ++r) {
            const CPUReservationImpl *cpuimpl =
              dynamic_cast<const CPUReservationImpl*>(&**r);
            if (cpuimpl) {
              numcpu = cpuimpl->num();
              break;
            }
          }
          if (numcpu == 0) {
            throw khException(kh::tr("Internal error: can find CPU reservation"));
          }
          outcmd.push_back(ToString(numcpu));
          used = true;
        }

        if (vec) {
          int index = 0;
          if ((i < len) && ((*arg)[i] == 'S')) {
            ++i;
            if ((i < len) && ((*arg)[i] == '[')) {
              ++i;
              unsigned int j = i;
              for (; j < len && ((*arg)[j] != ']'); ++j) ;
              if (j-i) {
                index = atoi(arg->substr(i, j-i).c_str());
              }
              i = j+1;
            } else {
              // no subscript, we want them all
              index = -1;
            }
          }
          std::string (*adapter)(const std::string &) = &passthrough;
          if ((i+1 < len) && ((*arg)[i] == ':')) {
            std::string subcmd = arg->substr(i+1, std::string::npos);
            if (subcmd == "basename")
              adapter = &khBasename;
            else if (subcmd == "dirname")
              adapter = &khDirname;
            else if (subcmd == "sansext")
              adapter = &khDropExtension;
            else
              adapter = 0;
          }
          if (adapter) {
            used = true;
            if (index >= 0) {
              if (index < (int)vec->size()) {
                outcmd.push_back((*adapter)((*vec)[index]));
              } else {
                throw khException(kh::tr("Invalid commandline arg: %1")
                                  .arg(ToQString(*arg)));
              }
            } else {
              std::transform(vec->begin(), vec->end(),
                             back_inserter(outcmd),
                             adapter);
            }
          }
        }
      }
      if (!used) {
        outcmd.push_back(*arg);
      }
    }
  } /* for cmdnum */

#if 0 && defined(SUPPORT_TASK_RELOCATE)
  unsigned int cmdnum = numcmds;
  for (std::map<std::string, std::string>::const_iterator
         reloc = relocateMap.begin();
       reloc != relocateMap.end(); ++reloc) {
    std::vector<std::string> &outcmd(outcmds[cmdnum]);
    outcmd.resize(3);
    outcmd[0] = "khrelocate";
    outcmd[1] = reloc->first;
    outcmd[2] = reloc->second;
    ++cmdnum;
  }
#endif
}
