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


#include "fusion/autoingest/sysman/khResourceManager.h"

#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>
#include <algorithm>
#include "fusion/autoingest/.idl/TaskLists.h"
#include "fusion/autoingest/Asset.h"
#include "fusion/autoingest/AssetVersion.h"
#include "fusion/autoingest/sysman/khSystemManager.h"
#include "fusion/autoingest/sysman/khAssetManager.h"
#include "fusion/autoingest/sysman/khTask.h"
#include "fusion/autoingest/sysman/khResourceProviderProxy.h"
#include "fusion/autoingest/sysman/AssetVersionD.h"
#include "fusion/autoingest/geAssetRoot.h"
#include "common/khFileUtils.h"
#include "common/performancelogger.h"
#include "fusion/config/gefConfigUtil.h"

// ****************************************************************************
// ***  global instances
// ****************************************************************************
khResourceManager theResourceManager;
khVolumeManager  &theVolumeManager(theResourceManager);

// How long to wait on communication with provider before assuming it's dead
// 60 seconds should be long enough to not give many (if any) false positives
// and short enough to still be effective. This timeout isn't used on requests
// that need to wait for results that may take time to calculate. It's used
// only on fire and forget notifications.
int SYSMAN_PROVIDER_TIMEOUT = 60;


void
khResourceManager::DumpWaitQueue(void)
{
  notify(NFY_NOTICE, "----- Task Queue -----");
  for (TaskWaitingQueue::iterator t = taskWaitingQueue.begin();
       t != taskWaitingQueue.end(); ++t) {
    notify(NFY_NOTICE, "%s", (*t)->verref().c_str());
  }
  notify(NFY_NOTICE, "----------------------");
}


// ****************************************************************************
// ***  khResourceManager
// ****************************************************************************
khResourceManager::khResourceManager(void)
{
}


// ****************************************************************************
// ***  ~khResourceManager
// ****************************************************************************
khResourceManager::~khResourceManager(void)
{
  // really nothing to do, main is long gone and it's too late for us to do
  // anything useful

  // even though providers may still be populated with a bunch of
  // allocated khResourceProviderProxy's, we're about to exit the app
  // so we'll let the system clean this up for us.
}


// ****************************************************************************
// ***  Init
// ****************************************************************************
void
khResourceManager::Init(void)
{
  std::string statedir =
    geAssetRoot::Dirname(AssetDefs::AssetRoot(), geAssetRoot::StateDir);

  khLockGuard lock(mutex);
  if(getenv("KH_NFY_LEVEL") == NULL)
  {
      Systemrc systemrc;
      LoadSystemrc(systemrc);
      std::uint32_t logLevel = systemrc.logLevel;
      notify(NFY_WARN, "system log level changed to: %s",
             khNotifyLevelToString(static_cast<khNotifyLevel>(logLevel)).c_str());
      setNotifyLevel(static_cast<khNotifyLevel>(logLevel));
  }

  // Find all the old task symlinks and tell the asset manager to resubmit
  // them. The symlinks have the form (taskid.task -> verref)
  DIR *dir = opendir(statedir.c_str());
  if (!dir) {
    notify(NFY_FATAL, "Unable to opendir(%s): %s", statedir.c_str(),
           khstrerror(errno).c_str());
  }
  struct dirent64 *entry;
  while ((entry = readdir64(dir))) {
    std::string dname = entry->d_name;

    if (dname == ".") continue;
    if (dname == "..") continue;
    if (!khHasExtension(dname, ".task")) continue;

    std::string child = statedir + "/" + dname;

    if (khSymlinkExists(child)) {
      std::uint32_t taskid = 0;
      FromString(khDropExtension(dname), taskid);
      if (taskid == 0) {
        notify(NFY_FATAL, "Unrecognized task symlink %s",
               child.c_str());
      }
      std::string verref;
      if (!khReadSymlink(child, verref)) {
        notify(NFY_FATAL, "Unable to process old tasks");
      }
      if (!khUnlink(child)) {
        notify(NFY_FATAL, "Unable to process old tasks");
      }

      // Tell the asset manager we lost this task
      // He'll re-submit it with all the details
      NotifyTaskLost(TaskLostMsg(verref, taskid));
    }
  }
  closedir(dir);
}


// ****************************************************************************
// ***  TaskCmdLoop thread
// ****************************************************************************
void
khResourceManager::ProcessTaskCmd(const TaskCmd &cmd) throw()
{
  try {
    cmd(this);
  } catch (const std::exception &e) {
    notify(NFY_WARN, "Caught exception processing task command: %s",
           e.what());
  } catch (...) {
    notify(NFY_WARN, "Caught unknown exception processing task command");
  }
}

void
khResourceManager::TaskCmdLoop(void) throw()
{
  while (1) {
    try {
      TaskCmd cmd = taskCmdQueue.pop();
      // don't check for WantExit here!
      // if we pop a command we MUST process it
      {
        khLockGuard lock(mutex);
        ProcessTaskCmd(cmd);
      }
      if (theSystemManager.WantExit()) {
        break;
      }
    } catch (const std::exception &e) {
      notify(NFY_WARN, "Caught exception popping assset command queue: %s",
             e.what());
    } catch (...) {
      notify(NFY_WARN, "Caught unknown exception popping assset command queue");
    }
  }
}


// ****************************************************************************
// ***  ProviderListenerLoop
// ****************************************************************************
void
khResourceManager::ProviderListenerLoop(void) throw()
{
  try {
    TCPListener serverSock(SockAddr(InetAddr::IPv4Any,
                                    FusionConnection::khResourceManagerPort));
    while (1) {
      try {
        FusionConnection::Handle provider
          (FusionConnection::AcceptClient(serverSock));
        if (theSystemManager.WantExit()) {
          break;
        }

        // get initial info about provider
        // will throw exception if times out
        ProviderConnectMsg connreq;
        provider->ReceiveNotify(connreq, SYSMAN_PROVIDER_TIMEOUT);

        {
          khLockGuard lock(mutex);

          // will link himself into my providers list
          // and will spawn own handler thread for reading
          (void) new khResourceProviderProxy(connreq, provider);
        }
      } catch (const std::exception &e) {
        notify(NFY_WARN,
               "Caught exception in resource manager server loop: %s",
               e.what());
      } catch (...) {
        notify(NFY_WARN, "Caught exception in resource manager server loop");
      }
    }
  } catch (const std::exception &e) {
    notify(NFY_WARN, "Unable to open server socket: %s", e.what());
  } catch (...) {
    notify(NFY_WARN, "Unable to open server socket");
  }
  theSystemManager.SetWantExit();
}


void
khResourceManager::InsertProvider(khResourceProviderProxy *proxy)
{
  std::string host = proxy->Host();
  if (providers.find(host) != providers.end()) {
    // collision
    throw khException(kh::tr("Already have resource provider named '%1'")
                      .arg(ToQString(host)));
  }

  providers.insert(std::make_pair(host, proxy));

  // instantiate the Volumes that this provider will manage
  std::vector<std::string> volnames;
  GetHostVolumes(host, volnames);
  for (const auto& vn : volnames) {
    if (volumes.find(vn) != volumes.end())
    {
      notify(NFY_WARN,"Volume %s already present in list of names", vn.c_str());
      delete volumes[vn];
      volumes.erase(vn);
     }
     volumes[vn] = new Volume(vn, proxy);
  }

  // wake up the activate thread
  activateCondVar.signal_one();
}


void
khResourceManager::EraseProvider(khResourceProviderProxy *proxy)
{
  std::string host = proxy->Host();
  providers.erase(host);

  // remove the Volumes that this provider used to manage
  std::vector<std::string> volnames;
  GetHostVolumes(host, volnames);
  for (const auto& vn : volnames) {
    delete volumes[vn];
    volumes.erase(vn);
  }
  if (volumes.empty())
  {
      volumes.clear();
  }
}


void
khResourceManager::InsertWaitingTask(khTask* task)
{
  taskWaitingQueue.insert(task);
  activateCondVar.signal_one();
}

void
khResourceManager::EraseWaitingTask(khTask* task)
{
  taskWaitingQueue.erase(task);
}


Reservation
khResourceManager::MakeVolumeReservation(const std::string &assetPath,
                                         const std::string &volume,
                                         const std::string &path,
                                         std::uint64_t size)
{
  Volume *vol = GetVolume(volume);
  if (vol && vol->MakeReservation(path, size)) {
    return VolumeReservationImpl::Make(assetPath,
                                       khFusionURI(volume, path));
  }
  return Reservation();
}

void
khResourceManager::ReleaseVolumeReservation(const std::string &volume,
                                            const std::string &path)
{
  Volume *vol = GetVolume(volume);
  if (vol) {
    vol->ReleaseReservation(path);

    // wake up the activate thread
    activateCondVar.signal_one();
  }
}

void
khResourceManager::CleanVolumeReservation(const std::string &volume,
                                          const std::string &path)
{
  Volume *vol = GetVolume(volume);
  if (vol) {
    vol->CleanReservation(path);
  }
}


Reservation
khResourceManager::MakeCPUReservation(khResourceProviderProxy *provider,
                                      const TaskRequirements::CPU &req)
{
  unsigned int num = std::min(provider->AvailCPUs(), req.maxNumCPU);
  if (num >= req.minNumCPU) {

    provider->usedCPUs += num;
    PERF_CONF_LOGGING( "rmanager_reservation_used_numcpus", provider->host, provider->numCPUs );
    return CPUReservationImpl::Make(provider->Host(), num);
  }
  else {
    PERF_CONF_LOGGING( "rmanager_fail_reservation_insufficient_numcpus", provider->host, num );
  }
  return Reservation();
}


void
khResourceManager::ReleaseCPUReservation(const std::string &host, unsigned int num)
{
  Providers::iterator found = providers.find(host);
  if (found != providers.end()) {
    found->second->usedCPUs -= num;
    PERF_CONF_LOGGING( "rmanager_release_numcpus_reservation", host, found->second->usedCPUs );
    // wake up the activate thread
    activateCondVar.signal_one();
  }
}


void
khResourceManager::SetVolumeAvail(const VolumeAvailMsg &msg)
{
  Volume *vol = GetVolume(msg.volname);
  if (vol) {
    // don't listen to any avail messages that don't reflect
    // all my reservations
    if (vol->serialnum() == msg.serial) {
      vol->SetAvail(msg.avail);

      // wake up the activate thread
      activateCondVar.signal_one();
    }
  }
}


// ****************************************************************************
// ***  ActivateLoop
// ****************************************************************************
void
khResourceManager::ActivateLoop(void) throw()
{
  while (1) {
    if (theSystemManager.WantExit()) return;
    khLockGuard lock(mutex);
    if (theSystemManager.WantExit()) return;
    if (!TryActivate()) {
      if (theSystemManager.WantExit()) return;
      notify(NFY_DEBUG, "***** ActivateLoop going to sleep *****");
      activateCondVar.wait(mutex);
      notify(NFY_DEBUG, "***** ActivateLoop woke up *****");
    } else {
      notify(NFY_DEBUG, "***** ActivateLoop did something *****");
    }
  }
}


bool
khResourceManager::TryActivate(void) throw()
{
  notify(NFY_DEBUG, "===== TryActivate =====");
  notify(NFY_DEBUG, "     num blockers  = %d", numActivateBlockers);
  notify(NFY_DEBUG, "     tasks waiting = %lu",
         static_cast<long unsigned>(taskWaitingQueue.size()));
  notify(NFY_DEBUG, "     num providers = %lu",
         static_cast<long unsigned>(providers.size()));

  // do some quick short circuit tests to see if we need to check
  // in more detail
  if ((numActivateBlockers == 0) &&
      taskWaitingQueue.size() &&
      providers.size()) {
    // figure out which providers have CPU resources to spare
    Providers availProviders;
    for (const auto& p : providers)
    {
        if (p.second->usedCPUs < p.second->numCPUs)
        {
            availProviders.insert(p);
        }
    }
    notify(NFY_DEBUG, "     avail providers = %lu",
           static_cast<long unsigned>(availProviders.size()));
    if (availProviders.size() != 0) {
      // walk through the waiting tasks (in order) trying to find
      // a processor to satisfy their needs
      for (TaskWaitingQueue::iterator t = taskWaitingQueue.begin();
           t != taskWaitingQueue.end(); ++t) {
        notify(NFY_DEBUG, "     trying \"%s\"",
               (*t)->verref().c_str());
        if (CheckVolumeHosts(*t)) {
          Reservations fixedReservations;
          if (MakeFixedVolumeReservations(*t, fixedReservations)) {
            Reservations localReservations;
            khResourceProviderProxy *provider =
              FindSatisfyingProvider(*t, availProviders,
                                     localReservations);
            if (provider) {
              // clear any previous activation error
              (*t)->activationError = QString();

              // combine fixed and local reservations
              std::copy(localReservations.begin(),
                        localReservations.end(),
                        back_inserter(fixedReservations));
              localReservations.clear();

              if ((*t)->bindOutfiles(fixedReservations)) {
                // this will either Start the task or
                // send a failed
                provider->StartTask(*t, fixedReservations);

                // we must exit here since *t has been removed
                // from taskWaitingQueue and the iterator could
                // now be invalid.
                // return true so the caller will come right
                // back in here and try some more
                return true;
              } else {
                (*t)->activationError =
                  kh::tr("Unable to bind outfiles.");
                notify(NFY_WARN, "    %s",
                       (*t)->activationError.latin1());
              }
            } else {
              (*t)->activationError =
                kh::tr("Unable to find a suitable resource provider:"
                       " no CPU(s) is available to start the task %1.")
                .arg(ToQString((*t)->verref()));
              notify(NFY_DEBUG, "    %s",
                     (*t)->activationError.latin1());
            }

            fixedReservations.clear();
          }
        }
      }
    }
  }

  return false;
}


bool
khResourceManager::CheckVolumeHosts(khTask *task)
{
  const TaskRequirements& req(task->GetRequirementsRef());

  // check to see that I have all the providers necessary for my
  // required volumes
  // NOTE: this checks all the providers that are connected, not just
  // those with avail CPUs
  for (std::set<std::string>::const_iterator rvh =
         req.requiredVolumeHosts.begin();
       rvh != req.requiredVolumeHosts.end(); ++rvh) {
    if (providers.find(*rvh) == providers.end()) {
      // missing a volume provider - can't process this task
      task->activationError = kh::tr("Volume host '%1' unavailable")
                              .arg(ToQString(*rvh));
      notify(NFY_DEBUG, "    %s",
             task->activationError.latin1());
      return false;
    }
  }

  return true;
}


bool
khResourceManager::MakeFixedVolumeReservations(khTask *task,
                                               Reservations &reservations)
{
  const TaskDef& taskdef(task->taskdef());
  const TaskRequirements& req(task->GetRequirementsRef());

  // Try to make all the output reservations for the outputs with fixed
  // volumes
  for (unsigned int i = 0; i < taskdef.outputs.size(); ++i) {
    const TaskDef::Output *defo          = &taskdef.outputs[i];
    const TaskRequirements::Output *reqo = &req.outputs[i];
    if (reqo->volume != "*anytmp*") {
      Reservation newres =
        MakeVolumeReservation(defo->path,
                              reqo->volume, reqo->path,
                              reqo->size);
      if (!newres) {
        // unable to acquire reservation
        task->activationError =
          kh::tr("Not enough disk space to make reservation for output files."
                 "Task %1: unable to make reservation %2/%3:%4")
          .arg(ToQString(task->verref()))
          .arg(ToQString(reqo->volume))
          .arg(ToQString(reqo->path))
          .arg(ToQString(reqo->size));
        notify(NFY_DEBUG, "    %s",
               task->activationError.latin1());
        reservations.clear();
        return false;
      } else {
        reservations.push_back(newres);
      }
    }
  }

  return true;
}

khResourceProviderProxy*
khResourceManager::FindSatisfyingProvider(khTask *task,
                                          const Providers &availProviders,
                                          Reservations &reservations)
{
  const TaskRequirements& req(task->GetRequirementsRef());

  // determine which build providers to try (and in what order)
  std::vector<khResourceProviderProxy*> tryProviders;
  if (req.requiredBuildHost.size()) {
    Providers::const_iterator found =
      availProviders.find(req.requiredBuildHost);
    if (found != availProviders.end()) {
      tryProviders.push_back(found->second);
    }
  } else if (req.preferredBuildHost.size()) {
    Providers::const_iterator found =
      availProviders.find(req.preferredBuildHost);
    if (found != availProviders.end()) {
      tryProviders.push_back(found->second);
    }
    for (Providers::const_iterator p = availProviders.begin();
         p != availProviders.end(); ++p) {
      if (p != found) {
        tryProviders.push_back(p->second);
      }
    }
  } else {
    for (Providers::const_iterator p = availProviders.begin();
         p != availProviders.end(); ++p) {
      tryProviders.push_back(p->second);
    }
  }

  // try each tryProvider to see if the requirements can be met
  for (std::vector<khResourceProviderProxy*>::const_iterator p =
         tryProviders.begin();
       p != tryProviders.end(); ++p) {
    khResourceProviderProxy *provider = (*p);
    if (ProviderCanSatisfy(task, provider, reservations)) {
      return provider;
    }
  }

  return 0;
}


// ****************************************************************************
// ***  ProviderCanSatisfy
// ****************************************************************************
bool
khResourceManager::ProviderCanSatisfy(khTask *task,
                                      khResourceProviderProxy *provider,
                                      Reservations &reservations)
{
  const TaskDef& taskdef(task->taskdef());
  const TaskRequirements& req(task->GetRequirementsRef());

  // reserve a CPU on the provider
  Reservation cpures = MakeCPUReservation(provider, req.cpu);
  if (!cpures) {
    reservations.clear();   // clear any fixed reservations already made
    return false;
  }
  reservations.push_back(cpures);

  // try to make reservations for outputs with volumes set to *anytmp*
  for (unsigned int o = 0; o < taskdef.outputs.size(); ++o) {
    const TaskDef::Output *defo          = &taskdef.outputs[o];
    const TaskRequirements::Output *reqo = &req.outputs[o];
    if (reqo->volume == "*anytmp*") {
      // get a list of possible tmpVolumes
      std::vector<std::string> tmpVolumes;
      if (reqo->localToJob == TaskRule::Must) {
        GetLocalTmpVolumes(provider->Host(), tmpVolumes);
      } else {
        // we treat prefer and dontcare the same here
        // we give local preference
        GetLocalTmpVolumes(provider->Host(), tmpVolumes);
        GetRemoteTmpVolumes(provider->Host(), tmpVolumes);
      }

      // prune and order tmpVolumes based on inputs that must be or
      // prefer to be on a different volume
      for (unsigned int i = 0; i < req.inputs.size(); ++i) {
        const TaskRequirements::Input *reqi = &req.inputs[i];

        // sone inputs don't have volumes (namely URI's)
        if (reqi->volume.size()) {
          if (reqo->differentVolumes[i] == TaskRule::Must) {
            std::vector<std::string>::iterator found =
              std::find(tmpVolumes.begin(), tmpVolumes.end(),
                        reqi->volume);
            if (found != tmpVolumes.end()) {
              // remove this volume
              tmpVolumes.erase(found);
            }
          } else if (reqo->differentVolumes[i] == TaskRule::Prefer) {
            std::vector<std::string>::iterator found =
              std::find(tmpVolumes.begin(), tmpVolumes.end(),
                        reqi->volume);
            if (found != tmpVolumes.end()) {
              // move this volume at the back
              tmpVolumes.erase(found);
              tmpVolumes.push_back(reqi->volume);
            }
          }
        }
      }

      // now see if one of the remaining tmpVolumes will work
      bool satisfied = false;
      for (std::vector<std::string>::const_iterator tv =
             tmpVolumes.begin();
           tv != tmpVolumes.end(); ++tv) {
        Reservation newres = MakeVolumeReservation(defo->path,
                                                   *tv, reqo->path,
                                                   reqo->size);
        if (newres) {
          reservations.push_back(newres);
          satisfied = true;
          break;
        }
      }
      if (!satisfied) {
        reservations.clear();
        return false;
      }
    }
  }

  return true;
}

// ****************************************************************************
// ***  command - SubmitTask
// ****************************************************************************
void
khResourceManager::SubmitTask(const SubmitTaskMsg &msg)
{
  assert(!mutex.trylock());

  // just in case another task has already been
  // submitted for this verref
  DeleteTask(msg.verref);

  notify(NFY_DEBUG, "SubmitTask %s", msg.verref.c_str());

  if (!khTask::NewTask(msg)) {
    time_t now = time(0);
    TaskDoneMsg doneMsg(msg.verref, msg.taskid,
                        false, now /* beginTime */, now /* endTime */);
    NotifyTaskDone(doneMsg);
  }

#if 0
  DumpWaitQueue();
#endif
}


// ****************************************************************************
// ***  command - DeleteTask
// ****************************************************************************
void
khResourceManager::DeleteTask(const std::string &verref)
{
  assert(!mutex.trylock());

  notify(NFY_DEBUG, "DeleteTask %s", verref.c_str());

  khTask *found = khTask::FindTask(verref);
  if (found) {
    khTask::DeleteTask(found);
  }
}



// ****************************************************************************
// ***  Routines to send notifications to khAssetManager
// ****************************************************************************
void
khResourceManager::NotifyTaskLost(const TaskLostMsg &lostMsg)
{
  // assert that we're already locked
  assert(!mutex.TryLock());

  notify(NFY_DEBUG, "NotifyTaskLost %s", lostMsg.verref.c_str());

  // tell asset manager it's lost
  theAssetManager.assetCmdQueue.push
    (AssetCmd(std::mem_fun(&khAssetManager::TaskLost), lostMsg));

  // tell me to wait until asset manager knows it's done
  BumpUpBlockers();
}

void
khResourceManager::NotifyTaskProgress(const TaskProgressMsg &progressMsg)
{
  // assert that we're already locked
  assert(!mutex.TryLock());

  // tell asset manager about the progress
  theAssetManager.assetCmdQueue.push
    (AssetCmd(std::mem_fun(&khAssetManager::TaskProgress), progressMsg));
}

void
khResourceManager::NotifyTaskDone(const TaskDoneMsg &doneMsg)
{
  // assert that we're already locked
  assert(!mutex.TryLock());

  notify(NFY_DEBUG, "NotifyTaskDone %s", doneMsg.verref.c_str());

  // tell asset manager it's done
  theAssetManager.assetCmdQueue.push
    (AssetCmd(std::mem_fun(&khAssetManager::TaskDone), doneMsg));

  // tell me to wait until asset manager knows it's done
  BumpUpBlockers();
}


// ****************************************************************************
// ***  khResourceManager::Volume
// ****************************************************************************
bool
khResourceManager::Volume::MakeReservation(const std::string &path,
                                           std::uint64_t size) {
  if (!provider->ConnectionLost() && (avail > size)) {
    Reservations::const_iterator found =
      reservations.find(path);
    if (found == reservations.end()) {
      avail -= size;
      reservations[path] = size;

      // every time we make a reservation we need to bump up our
      // serial number. That way we can recognize the out-of-date
      // VolumeAvailMsg's
      ++serial;

      // try to make the reservation persistent
      VolumeReservations storage(name, serial, reservations);
      if (!storage.Save(ReservationFilename())) {
        --serial;
        avail += size;
        reservations.erase(path);
        return false;
      }

      // let the provider know about the new reservations list
      QString error;
      if (provider->providerProxy.ChangeVolumeReservations
          (storage, error, SYSMAN_PROVIDER_TIMEOUT)) {
        notify(NFY_DEBUG, "Volume::MakeReservation %s, %llu, %s",
               name.c_str(), static_cast<long long unsigned>(size),
               path.c_str());
        return true;
      } else {
        // schedule abandon - can't do now because we're inside a
        // volume object that will get cleaned when the provider
        // is abandoned.
        theResourceManager.ScheduleProviderAbandonment(provider);
        return false;
      }
    }
  }
  return false;
}

void
khResourceManager::Volume::ReleaseReservation(const std::string &path) {
  Reservations::iterator found =
    reservations.find(path);
  notify(NFY_DEBUG, "Volume::ReleaseReservation %s, %s",
         name.c_str(), path.c_str());
  if (!provider->ConnectionLost() &&
      (found != reservations.end())) {
    // don't adjust avail, wait for the provider to update it
    // only he knows the actual size of the files remaining

    reservations.erase(found);

    // try to make the reservation persistent
    // there's not much we can do if it fails, a stranded reservation
    // will take up some disk space, but won't hurt much else
    // plus, the next time we lose connection to this provider
    // we'll ignore all the reservations anyway
    VolumeReservations storage(name, serial, reservations);
    (void) storage.Save(ReservationFilename());

    // let the provider know about the new reservations list
    QString error;
    if (!provider->providerProxy.ChangeVolumeReservations
        (storage, error, SYSMAN_PROVIDER_TIMEOUT)) {
      // schudule abandon - can't do now because we're inside a
      // volume object that will get cleaned when the provider
      // is abandoned.
      theResourceManager.ScheduleProviderAbandonment(provider);
    }
  }
}

void
khResourceManager::Volume::CleanReservation(const std::string &path) {
  notify(NFY_DEBUG, "Volume::CleanReservation %s, %s",
         name.c_str(), path.c_str());
  Reservations::iterator found = reservations.find(path);
  if (!provider->ConnectionLost() && (found != reservations.end())) {
    QString error;
    if (!provider->providerProxy.CleanPath
        (khFusionURI(name, path).LocalPath(), error,
         SYSMAN_PROVIDER_TIMEOUT)) {
      // schedule abandon - can't do now because we're inside a
      // volume object that will get cleaned when the provider
      // is abandoned.
      theResourceManager.ScheduleProviderAbandonment(provider);
    }
  }
}

std::string
khResourceManager::Volume::ReservationFilename(void) const
{
  return khComposePath(geAssetRoot::Dirname(AssetDefs::AssetRoot(),
                                            geAssetRoot::StateDir),
                       name + ".reservations");
}


khResourceManager::Volume::Volume(const std::string n,
                                  khResourceProviderProxy *p)
    : name(n), serial(0), avail(0), provider(p)
{
  QString error;
  std::string resname = ReservationFilename();
  if (!provider->ConnectionLost()) {
    if (khExists(resname)) {
      VolumeReservations storage;
      if (storage.Load(resname)) {
        if (!provider->providerProxy.CleanupVolume
            (storage, error, SYSMAN_PROVIDER_TIMEOUT)) {
          // schudule abandon - can't do now because we're inside a
          // volume object that will get cleaned when the provider
          // is abandoned.
          theResourceManager.ScheduleProviderAbandonment(provider);
          return;
        }
      }
      (void)khUnlink(resname);
    }
  }
  if (!provider->ConnectionLost()) {
    if (!provider->providerProxy.ChangeVolumeReservations
        (VolumeReservations(name, serial),
         error, SYSMAN_PROVIDER_TIMEOUT)) {
      // schudule abandon - can't do now because we're inside a
      // volume object that will get cleaned when the provider
      // is abandoned.
      theResourceManager.ScheduleProviderAbandonment(provider);
    }
  }
}


void
khResourceManager::GetCurrTasks(TaskLists &ret)
{
  assert(!mutex.TryLock());
  for (TaskWaitingQueue::iterator t = taskWaitingQueue.begin();
       t != taskWaitingQueue.end(); ++t) {
    khTask *task = *t;
    ret.waitingTasks.push_back
      (TaskLists::WaitingTask(task->verref(),
                              task->taskid(),
                              task->priority(),
                              task->submitTime(),
                              task->activationError));
  }

  for (Providers::iterator p = providers.begin();
       p != providers.end(); ++p) {
    p->second->AddToTaskLists(ret);
  }

  // Get total numbers of assets and asset's versions cached.
  ret.num_assets_cached = Asset::CacheSize();
  ret.num_assetversions_cached = AssetVersion::CacheSize();
  // Get total amount of memory used by the objects in asset and asset version caches.
  ret.asset_cache_memory = Asset::CacheMemoryUse();
  ret.version_cache_memory = AssetVersion::CacheMemoryUse();
  // Get the number of strings in SharedString string store.
  ret.str_store_size = SharedString::StoreSize();
}

void
khResourceManager::ReloadConfig(void)
{
  assert(!mutex.TryLock());

  // Someday reload volume definitions too...
  // This will require a fair amount of work.
  //   - what happens if a volume is removed?
  //   - what happens if a localpath changes?
  //   - what heppens to existing reservations in the above two cases?
  //   - how do I send new volume information to slave providers so they
  //         can know what to check for free space?

  TaskRequirements::LoadFromFiles();

  // "Lose" all waiting tasks. This will cause them to come back in,
  // rediscover their requirements and attempt to be kicked off again.
  TaskWaitingQueue::iterator t = taskWaitingQueue.begin();
  while (t != taskWaitingQueue.end()) {
    khTask::LoseTask(*t);
    t = taskWaitingQueue.begin();
  }
}


void
khResourceManager::ScheduleProviderAbandonment(khResourceProviderProxy *proxy)
{
  notify(NFY_WARN,
         "Unable to communicate with resource provider on host %s.\n"
         "    Assuming host is lost, scheduling connection termination.",
         proxy->Host().c_str());
  proxy->wantAbandon = true;
  taskCmdQueue.push_front
    (TaskCmd(std::mem_fun(&khResourceManager::DoAbandonProvider),
             proxy->Host()));
}

void
khResourceManager::AbandonProvider(khResourceProviderProxy *proxy)
{
  notify(NFY_WARN,
         "Unable to communicate with resource provider on host %s.\n"
         "    Assuming host is lost.",
         proxy->Host().c_str());
  proxy->wantAbandon = true;
  DoAbandonProvider(proxy->Host());
}

void
khResourceManager::DoAbandonProvider(const std::string &host)
{
  Providers::iterator found = providers.find(host);
  if ((found != providers.end()) && (found->second->WantAbandon())) {
    notify(NFY_WARN, "Terminating connection to host %s.", host.c_str());
    found->second->Cleanup();
  }
}
