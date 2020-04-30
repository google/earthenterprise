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

#ifndef __khResourceManager_h
#define __khResourceManager_h

#include <set>
#include <khVolumeManager.h>
#include <khFunctor.h>
#include <khMTTypes.h>
#include <autoingest/sysman/.idl/TaskStorage.h>
#include "khTask.h"

class khResourceProviderProxy;
class khAssetManager;
class TaskLists;

// How long to wait on communication with provider before assuming it's dead
extern int SYSMAN_PROVIDER_TIMEOUT;


class khResourceManager : public khVolumeManager
{
  friend class khSystemManager;

  class Volume {
    typedef std::map<std::string, std::uint64_t> Reservations;

    std::string  name;
    std::uint32_t       serial;
    std::uint64_t       avail;
    Reservations reservations;
    khResourceProviderProxy *provider;
   public:
    std::uint32_t serialnum(void) const { return serial; }

    std::string ReservationFilename(void) const;
    bool MakeReservation(const std::string &path, std::uint64_t size);
    void ReleaseReservation(const std::string &path);
    void CleanReservation(const std::string &path);
    void SetAvail(std::uint64_t a) {
      avail = a;
    }

    Volume(const std::string n, khResourceProviderProxy *p);
  };

  // active list of volumes with their avail & reservations
  std::map<std::string, Volume*> volumes;
  Volume* GetVolume(const std::string &volname) {
    std::map<std::string, Volume*>::const_iterator found =
      volumes.find(volname);
    if (found != volumes.end()) {
      return found->second;
    }
    return 0;
  }

 public:
  // redefined to avoid circular header dependencies w/ khAssetManager
  typedef khFunctor1<khAssetManager*, void> AssetCmd;

  typedef std::map<std::string, khResourceProviderProxy*> Providers;
  typedef khFunctor1<khResourceManager*, void> TaskCmd;
  typedef khMTQueue<TaskCmd>                   TaskCmdQueue;
  struct task_less :
      public std::binary_function<khTask*,khTask*,bool> {
    bool operator()(const first_argument_type a, second_argument_type b) {
      if (a->priority() == b->priority()) {
        if (a->submitTime() == b->submitTime()) {
          if (a->taskid() == b->taskid()) {
            if (a->verref() == b->verref()) {
              return a < b;
            } else {
              return a->verref() < b->verref();
            }
          } else {
            return a->taskid() < b->taskid();
          }
        } else {
          return a->submitTime() < b->submitTime();
        }
      } else {
        return a->priority() < b->priority();
      }
    }
  };
  typedef std::set<khTask*, task_less>  TaskWaitingQueue;
  typedef std::vector<Reservation> Reservations;


  khResourceManager(void);
  ~khResourceManager(void);
  void Init(void);

  void ScheduleProviderAbandonment(khResourceProviderProxy *proxy);
  void AbandonProvider(khResourceProviderProxy *proxy);
  void DoAbandonProvider(const std::string &host);


  // ***** stuff for TaskCmdLoop *****
  TaskCmdQueue taskCmdQueue;
  void ProcessTaskCmd(const TaskCmd &) throw();
  void TaskCmdLoop(void) throw();

  // ***** provider listener thread *****
  void ProviderListenerLoop(void) throw();

  // ***** activate thread *****
  void ActivateLoop(void) throw();
  bool TryActivate(void) throw();
  bool CheckVolumeHosts(khTask *task);
  bool MakeFixedVolumeReservations(khTask *task,
                                   Reservations &reservations);
  khResourceProviderProxy*
  FindSatisfyingProvider(khTask *task,
                         const Providers &availProviders,
                         Reservations &reservations);
  bool ProviderCanSatisfy(khTask *task,
                          khResourceProviderProxy *provider,
                          Reservations &reservations);


  // ***** resource mutex and the stuff it protects *****
  // this is no-destroy mutex because it needs to remain locked
  // even while the application exists
  khNoDestroyMutex   mutex;

  void DumpWaitQueue(void);
 private:
  Providers          providers;
  TaskWaitingQueue   taskWaitingQueue;
  khCondVar          activateCondVar;
  std::uint32_t             numActivateBlockers;

 public:
  // Various accessor routines - need to hold mutex
  void BumpUpBlockers(void) {
    ++numActivateBlockers;
  }
  void BumpDownBlockers(void) {
    --numActivateBlockers;
    if (numActivateBlockers == 0) {
      activateCondVar.signal_one();
    }
  }
  void InsertProvider(khResourceProviderProxy *);
  void EraseProvider(khResourceProviderProxy *);
  void InsertWaitingTask(khTask* task);
  void EraseWaitingTask(khTask* task);

  Reservation MakeVolumeReservation(const std::string &assetPath,
                                    const std::string &volume,
                                    const std::string &path,
                                    std::uint64_t size);
  void ReleaseVolumeReservation(const std::string &volume,
                                const std::string &path);
  void CleanVolumeReservation(const std::string &volume,
                              const std::string &path);

  Reservation MakeCPUReservation(khResourceProviderProxy *provider,
                                 const TaskRequirements::CPU &req);
  void ReleaseCPUReservation(const std::string &host, unsigned int num);
  void SetVolumeAvail(const VolumeAvailMsg &msg);


  // ***** commands *****
  void SubmitTask(const SubmitTaskMsg &msg);
  void DeleteTask(const std::string &verref);

  // routines to send notifications to khAssetManager
  void NotifyTaskLost(const TaskLostMsg &lostMsg);
  void NotifyTaskProgress(const TaskProgressMsg &progressMsg);
  void NotifyTaskDone(const TaskDoneMsg &doneMsg);

  // ***** misc *****
  void GetCurrTasks(TaskLists &ret);
  void ReloadConfig(void);
};

extern khResourceManager theResourceManager;


#endif /* __khResourceManager_h */
