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

//

#ifndef COMMON_MTTYPES_WAITBASEMANAGER_H__
#define COMMON_MTTYPES_WAITBASEMANAGER_H__

#include <list>
#include <khThread.h>
#include <khAbortedException.h>
#include <common/khFunctor.h>

class QString;

namespace mttypes {

class WaitBase;

class WaitBaseManager {
 public:
  WaitBaseManager(void);
  virtual ~WaitBaseManager(void);

  void AddWaitBase(WaitBase *base);
  void RemoveWaitBase(WaitBase *base);
  void Abort(const QString &msg);
  bool IsAborted(void) const { return aborted_; }

 protected:
  virtual void HandleAbortMessage(const QString &msg);

 private:
  std::list<WaitBase*> wait_bases_;
  khMutex mutex_;
  bool    aborted_;
};


class ManagedThread : public khThread {
 private:
  WaitBaseManager &manager_;

 protected:
  // wraps dispatch of thread function in try/catch that calls abort on manager
  virtual void thread_main(void);

 public:
  ManagedThread(WaitBaseManager &manager, const khFunctor<void> &fun);
};



} // namespace mttypes

#endif // COMMON_MTTYPES_WAITBASEMANAGER_H__
