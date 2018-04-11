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


#include "khResourceProviderProxy.h"
#include "khResourceManager.h"
#include "khSystemManager.h"
#include "khTask.h"
#include "AssetVersionD.h"

#include <khnet/SocketException.h>
#include <autoingest/.idl/VolumeStorage.h>
#include <autoingest/.idl/TaskLists.h>
#include <autoingest/sysman/.idl/JobStorage.h>
#include <fusionversion.h>


// ****************************************************************************
// ***  khResourceProviderProxy
// ****************************************************************************
khResourceProviderProxy::khResourceProviderProxy
(const ProviderConnectMsg &conn,
 FusionConnection::Handle &providerHandle) :
    host(conn.host), numCPUs(conn.numCPUs), usedCPUs(0),
    providerProxy(providerHandle),
    wantAbandon(false), cleaned(false)
{
  PERF_CONF_LOGGING( "connect_resource_vcpus", host, numCPUs  );
  // Verify fusion version - bail out if mismatch
  if (strcmp(conn.fusionVer.c_str(), GEE_VERSION) != 0) {
    throw khException(kh::tr("Incorrect version: %1 instead of %2")
                      .arg(ToQString(conn.fusionVer)).arg(ToQString(GEE_VERSION)));
  }
  // add myself to the resource manager
  // this will send a message to the provider for
  // each volume he's supposed to maintain
  // I'm not in my listen loop yet, so any immediate avail msgs
  // will block. That's why the resource provider has a separate send queue
  theResourceManager.InsertProvider(this);

  // now launch my handler thread
  RunInDetachedThread
    (khFunctor<void>(std::mem_fun
                     (&khResourceProviderProxy::HandleProviderLoop),
                     this));
}


void
khResourceProviderProxy::Cleanup(void)
{
  if (cleaned) return;
  cleaned = true;

  // clear my activeMap
  for (ActiveMap::iterator t = activeMap.begin();
       t != activeMap.end(); ++t) {
    khTask::LoseTask(t->second);
  }
  activeMap.clear();

  // clear my deadMap
  deadMap.clear();

  // remove myself from the resource manager
  // do this last since it will remove my volumes and we want them to stay
  // around long enough to have their reservations cancelled
  theResourceManager.EraseProvider(this);
}


khResourceProviderProxy::~khResourceProviderProxy(void)
{
  Cleanup();
}


// ****************************************************************************
// ***  HandleProviderLoop
// ****************************************************************************
void
khResourceProviderProxy::HandleProviderLoop(void)
{
  // NOTE: MT parallelization
  // this thread reads from providerProxy while the thread holding
  // theResourceManager.mutex can write the same socket. This should not
  // be a problem. TCP sockets are designed to work that way.
  try {
    while (1) {
      FusionConnection::RecvPacket msg;
      providerProxy.connection()->ReceiveMsg(msg);
      if (theSystemManager.WantExit()) {
        break;
      }
      if (ConnectionLost()) {
        break;
      }
      std::string cmdname = msg.cmdname();
      // because we can't write to this socket, we only support
      // the dispatch of NotifyMsg's
      if (msg.msgType == FusionConnection::NotifyMsg) {
        notify(NFY_DEBUG, "Handling notify %s", cmdname.c_str());
        try {
          DispatchNotify(msg);
        } catch (const std::exception &e) {
          notify(NFY_WARN, "%s: Error: %s", cmdname.c_str(),
                 e.what());
        } catch (...) {
          notify(NFY_WARN, "%s: Unknown error", cmdname.c_str());
        }
      } else {
        notify(NFY_WARN, "Unsupported message type: %d",
               msg.msgType);
        // we can't recover from this since the provider may be
        // waiting for a reply of some sort
        throw khException(kh::tr("Unrecognized message"));
      }
    }
  } catch (const SocketException &e) {
    if (e.errnum() == EPIPE) {
      notify(NFY_DEBUG, "Provider closed connection");
      // the provider has closed its side of the connection
      // No big deal, just fall off the bottom of this handler thread
    } else {
      notify(NFY_WARN, "Provider socket error: %s", e.what());
    }
  } catch (const std::exception &e) {
    notify(NFY_WARN, "Error handling provider messages: %s", e.what());
  } catch (...) {
    notify(NFY_WARN, "Unknown error handling provider messages");
  }

  // delete myself, but only after we get the resource mutex
  // If we already WantExit, then this mutex will never be available
  // and we will block here until the thread evaporates at the end of main
  khLockGuard lock(theResourceManager.mutex);
  delete this;
}


// ****************************************************************************
// ***  StartTask
// ****************************************************************************
void
khResourceProviderProxy::StartTask
(khTask *task, const std::vector<Reservation> &res) throw()
{
  assert(!theResourceManager.mutex.trylock());

  try {
    std::vector<std::vector<std::string> > commands;
    task->buildCommands(commands, res);

    QString error;
    if (providerProxy.StartJob(StartJobMsg(task->taskid(),
                                           task->LogFilename(),
                                           commands),
                               error, SYSMAN_PROVIDER_TIMEOUT)) {
      activeMap.insert(std::make_pair(task->taskid(), task));
      task->Activated(res, this);
    } else {
      notify(NFY_WARN,
             "Unable to send StartJob for %s: %s.\n"
             "    Will send a TaskLost message.",
             task->verref().c_str(), error.latin1());
      khTask::LoseTask(task);

      // If I can't send a message, assume the resource provider I'm
      // connected to is dead.
      theResourceManager.AbandonProvider(this);
    }
  } catch (const std::exception &e) {
    notify(NFY_WARN,
           "Caught exception trying to StartTask %s: %s.\n"
           "    Will fabricate a TaskFailed message.",
           task->verref().c_str(), e.what());
    LeafAssetVersionImplD::WriteFatalLogfile(task->verref(),
                                             "StartTask", e.what());
    khTask::FinishTask(task,
                       false,  // success
                       0, 0);  // beginTime, endTime
  } catch (...) {
    notify(NFY_WARN,
           "Caught unknown exception trying to StartTask %s.\n"
           "    Will fabricate a TaskFailed message.",
           task->verref().c_str());
    LeafAssetVersionImplD::WriteFatalLogfile(task->verref(),
                                             "StartTask", "Unknown error");
    khTask::FinishTask(task,
                       false,  // success
                       0, 0);  // beginTime, endTime
  }
}


// ****************************************************************************
// ***  StopTask
// ****************************************************************************
void
khResourceProviderProxy::StopTask(khTask *task) throw()
{
  assert(!theResourceManager.mutex.trylock());

  ActiveMap::iterator found = activeMap.find(task->taskid());
  if (found != activeMap.end()) {
    activeMap.erase(found);
    deadMap.insert(std::make_pair(task->taskid(),
                                  DeadJob(task->taskid(),
                                          task->reservations())));
    if (!ConnectionLost()) {
      QString error;
      if (!providerProxy.StopJob(StopJobMsg(task->taskid()),
                                 error, SYSMAN_PROVIDER_TIMEOUT)) {
        notify(NFY_WARN, "Unable to send StopJob for %d: %s.",
               task->taskid(), error.latin1());

        // If I can't send a message, assume the resource provider I'm
        // connected to is dead.
        theResourceManager.AbandonProvider(this);
      }
    }
  }
}


// ****************************************************************************
// ***  JobProgress
// ****************************************************************************
void
khResourceProviderProxy::JobProgress(const JobProgressMsg &msg)
{
  assert(!theResourceManager.mutex.trylock());

  ActiveMap::iterator found = activeMap.find(msg.jobid);
  if (found != activeMap.end()) {
    found->second->Progress(msg);
    theResourceManager.NotifyTaskProgress(TaskProgressMsg
                                          (found->second->verref(),
                                           msg.jobid,
                                           msg.beginTime,
                                           msg.progressTime,
                                           msg.progress));
  }
}


// ****************************************************************************
// ***  JobDone
// ****************************************************************************
void
khResourceProviderProxy::JobDone(const JobDoneMsg &msg)
{
  assert(!theResourceManager.mutex.trylock());

  ActiveMap::iterator active = activeMap.find(msg.jobid);
  if (active != activeMap.end()) {
    khTask *task = active->second;
    activeMap.erase(active);
    khTask::FinishTask(task, msg.success, msg.beginTime, msg.endTime);
    return;
  }

  DeadMap::iterator dead = deadMap.find(msg.jobid);
  if (dead != deadMap.end()) {
    deadMap.erase(dead);
    return;
  }
}


// ****************************************************************************
// ***  VolumeAvail
// ****************************************************************************
void
khResourceProviderProxy::SetVolumeAvail(const VolumeAvailMsg &msg)
{
  theResourceManager.SetVolumeAvail(msg);
}


// ****************************************************************************
// ***  AddToTaskLists
// ****************************************************************************
void
khResourceProviderProxy::AddToTaskLists(TaskLists &ret)
{
  ret.providers.push_back
    (TaskLists::Provider(host, numCPUs,
                         std::vector<TaskLists::ActiveTask>()));

  for (ActiveMap::iterator t = activeMap.begin();
       t != activeMap.end(); ++t) {
    khTask *task = t->second;
    ret.providers.back().activeTasks.push_back
      (TaskLists::ActiveTask(task->verref(),
                             task->taskid(),
                             task->beginTime(),
                             task->progressTime(),
                             task->progress()));
  }
}
