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

//

#include "WaitBaseManager.h"
#include "WaitBase.h"
#include <qstring.h>

namespace mttypes {

WaitBaseManager::WaitBaseManager(void) :
    wait_bases_(),
    mutex_(),
    aborted_(false)
{
}

WaitBaseManager::~WaitBaseManager(void) {
}

void WaitBaseManager::AddWaitBase(WaitBase *base) {
  khLockGuard lock(mutex_);
  wait_bases_.push_back(base);
  base->SetManager(this);
}

void WaitBaseManager::RemoveWaitBase(WaitBase *base) {
  khLockGuard lock(mutex_);
  wait_bases_.remove(base);
  base->ClearManager();
}

void WaitBaseManager::HandleAbortMessage(const QString &msg) {
  notify(NFY_WARN, "%s", (const char *)msg.utf8());
}

void WaitBaseManager::Abort(const QString &msg) {
  khLockGuard lock(mutex_);
  if (!aborted_) {
    aborted_= true;

    HandleAbortMessage(msg);

    // make a copy I can reference after I've released the lock
    std::vector<WaitBase*> to_abort;
    std::copy(wait_bases_.begin(), wait_bases_.end(),
              back_inserter(to_abort));

    // unlock first in case they try to call back here to me
    khUnlockGuard unlock(mutex_);
    for (std::vector<WaitBase*>::iterator i = to_abort.begin();
         i != to_abort.end(); ++i) {
      (*i)->AbortFromManager();
    }
  }
}


// ****************************************************************************
// ***  ManagedThread
// ****************************************************************************
ManagedThread::ManagedThread(WaitBaseManager &manager,
                             const khFunctor<void> &fun) :
    khThread(fun),
    manager_(manager)
{ }

void ManagedThread::thread_main(void) {
  try {
    fun();
  } catch (const khAbortedException &e) {
    // nothing to do. The code that triggered the exception has already
    // reported it.
    // just fall out the bottom
  } catch (const khException &e) {
    manager_.Abort(e.qwhat());
  } catch (const std::exception &e) {
    manager_.Abort(e.what());
  } catch (...) {
    manager_.Abort("Unknown exception");
  }
}



} // namespace mttypes
