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

#ifndef __khResourceProvider_h
#define __khResourceProvider_h

#include <khMTTypes.h>
#include <khThread.h>
#include <khThreadPool.h>
#include <autoingest/FusionConnection.h>
#include <autoingest/.idl/VolumeStorage.h>
#include <autoingest/sysman/.idl/JobStorage.h>

class khResourceManagerProxy;

class khResourceProvider
{

  class Job {
   public:
    uint32 jobid;
    FILE*  logfile;
    pid_t  pid;
    time_t beginTime;


    Job(uint32 jobid) : jobid(jobid), logfile(0), pid(0), beginTime(0) { }
  };

  // routines implemented in khResourceProviderDispatch.cpp
  void DispatchNotify(const FusionConnection::RecvPacket &);
  void DispatchRequest(const FusionConnection::RecvPacket &, std::string &replyPayload);

  // routines called by the Dispatch functions
  // they acquire the AssetGuard and call the supplied function
  template <class Fun>
  void DispatchThunk(Fun fun,
                     typename Fun::second_argument_type arg) {
    khLockGuard lock(mutex);
    if (wantexit) return;
    fun(this, arg);
  }

  // ***** signal loop *****
  void SignalLoop(void);

  // ***** stuff for SendLoop *****
  typedef khFunctor1<khResourceManagerProxy*, void> SendCmd;
  typedef khMTQueue<SendCmd> SendQueue;
  SendQueue *sendQueue;
  void SendLoop(FusionConnection::Handle manager);

  // ***** stuff for DispatchLoop *****
  typedef khMTQueue<FusionConnection::RecvPacket> DispatchQueue;
  DispatchQueue *dispatchQueue;
  void DispatchLoop(void);

  // ***** stuff for pruneLoop *****
  typedef khMTQueue<std::string> PruneQueue;
  PruneQueue *pruneQueue;
  void PruneLoop(void);


  // ***** stuff for handling jobs *****
  khThreadPool     *jobThreads;
  void JobLoop(StartJobMsg start); // pass by value because thread func
  bool ExecCmdline(Job *job, const std::vector<std::string> &cmdline);
  void SendProgress(uint32 jobid, double progress, time_t progressTime);
#if 0
  void ReadProgress(int readfd, uint32 jobid);
#endif
  void DeleteJob(std::vector<Job>::iterator which,
                 bool success = false,
                 time_t beginTime = 0, time_t endTime = 0);

  // ***** stuff for CheckVolumeAvailLoop *****
  void CheckVolumeAvailLoop(void);

  // ***** mutex and the stuff it protects *****
  // this is no-destroy mutex because it needs to remain locked
  // even while the application exists
  khNoDestroyMutex mutex;
  bool             wantexit;
  std::vector<Job> jobs;
  typedef std::map<std::string, VolumeReservations> VolResMap;
  VolResMap volResMap;


  Job* FindJobById(uint32 jobid, std::vector<Job>::iterator &found);
  bool WantExit(void) {
    khLockGuard lock(mutex);
    return wantexit;
  }

  // ***** command routines *****
  void StartJob(const StartJobMsg &start);
  void StopJob(const StopJobMsg &stop);
  void CleanupVolume(const VolumeReservations &res);
  void ChangeVolumeReservations(const VolumeReservations &res);
  void CleanPath(const std::string &path);

  // ***** helpers *****
  void CleanJobsAndVolumes(bool sigkill);

 public:
  khResourceProvider(void);
  ~khResourceProvider(void);
  void Run(void);
};

extern khResourceProvider theResourceProvider;

#endif /* __khResourceProvider_h */
