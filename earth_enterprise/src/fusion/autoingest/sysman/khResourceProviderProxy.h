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

#ifndef __khResourceProviderProxy_h
#define __khResourceProviderProxy_h

#include <FusionConnection.h>
#include <khThread.h>
#include "Reservation.h"
#include <map>
#include <autoingest/sysman/ResourceProviderProxy.h>
#include "Reservation.h"
#include "khResourceManager.h"

class khTask;
class TaskLists;

class khResourceProviderProxy
{
  friend class khResourceManager;

  class DeadJob {
    std::uint32_t                   taskid;
    std::vector<Reservation> reservations;
   public:
    DeadJob(std::uint32_t tid, const std::vector<Reservation> &res)
        : taskid(tid), reservations(res) { }
  };

  typedef std::map<std::uint32_t, khTask*> ActiveMap;
  typedef std::map<std::uint32_t, DeadJob> DeadMap;

  std::string host;
  unsigned int        numCPUs;
  unsigned int        usedCPUs;
 public:
  ResourceProviderProxy providerProxy;
 private:
  ActiveMap     activeMap;
  DeadMap       deadMap;
  bool          wantAbandon;
  bool          cleaned;

  // private and unimplemented
  khResourceProviderProxy(const khResourceProviderProxy &);
  khResourceProviderProxy& operator=(const khResourceProviderProxy &);
 public:
  khResourceProviderProxy(const ProviderConnectMsg &conn,
                          FusionConnection::Handle &providerHandle);
  ~khResourceProviderProxy(void);

  void Cleanup(void);

  bool WantAbandon(void) const { return wantAbandon; }
  bool ConnectionLost(void) const { return wantAbandon || cleaned; }

  void HandleProviderLoop(void);

  void StartTask(khTask *, const std::vector<Reservation> &) throw();
  void StopTask(khTask *) throw();

  // Handle socket messages from real resource provider
  void JobProgress(const JobProgressMsg &);
  void JobDone(const JobDoneMsg &);
  void SetVolumeAvail(const VolumeAvailMsg &);

  std::string Host(void) const { return host; }

  void AddToTaskLists(TaskLists &ret);
  unsigned int AvailCPUs(void) const { return numCPUs - usedCPUs; }

 private:
  // routines implemented in khResourceProviderProxyDispatch.cpp
  void DispatchNotify(const FusionConnection::RecvPacket &);
  void DispatchRequest(const FusionConnection::RecvPacket &, std::string &replyPayload);

  // routines called by the Dispatch functions
  // they acquire the appropriate locks and call the supplied function
  template <class Fun>
  void DispatchThunk(Fun fun,
                     typename Fun::second_argument_type arg) {
    // so I can call khTaskManager routines
    khLockGuard lock(theResourceManager.mutex);

    fun(this, arg);
  }
};

#endif /* __khResourceProviderProxy_h */
