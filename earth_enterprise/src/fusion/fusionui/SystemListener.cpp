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


/******************************************************************************
File:        SystemListener.cpp
Description: 

-------------------------------------------------------------------------------
For history see CVS log (cvs log SystemListener.cpp -or- Emacs Ctrl-xvl).
******************************************************************************/
#include "SystemListener.h"
#include <notify.h>
#include <Qt/qapplication.h>
#include "AssetManager.h"


SystemListener *SystemListener::instance = 0;

SystemListener::SystemListener(void)
    : listener(*this)
{
  if (instance) {
    notify(NFY_FATAL, "Internal Error: Attempted to start a second SystemListener");
  }
  instance = this;

  if (!listener.run()) {
    notify(NFY_WARN, "Unable to start listener");
  }

}

SystemListener::~SystemListener(void)
{
  if (listener.needJoin()) {
    listener.setWantAbort();
    listener.join();
  }
}

bool
SystemListener::event(QEvent *event)
{
  if (event->type() == QEvent::User) {
    AssetChanges tosend;
    {
      // copy the changes we need to send so we don't need to hold
      // the mutex for the duration of the slot handlers
      khLockGuard guard(lock);
      tosend = changes;
      changes.items.clear();
    }
    if (tosend.items.size()) {
      emit assetsChanged(tosend);
    }
    return true;
  } else {
    return false;
  }
}


void
SystemListener::addChanges(const AssetChanges &changes_)
{
  // this runs in the listener thread. Don't do any GUI work here
  {
    // add the message to the list
    khLockGuard guard(lock);
    std::copy(changes_.items.begin(), changes_.items.end(),
              back_inserter(changes.items));
  }
  // post an event to the QUI thread
  QApplication::postEvent(this, new QCustomEvent(QEvent::User));

}



// ****************************************************************************
// ***  AssetWatcherManager
// ****************************************************************************
AssetWatcherManager* AssetWatcherManager::instance = 0;


void
AssetWatcherManager::assetsChanged(const AssetChanges &changes)
{
  for (AssetChanges::CIterator c = changes.items.begin();
       c != changes.items.end(); ++c) {
    WatcherRange found = watchers.equal_range(c->ref);
    for (WatchersType::iterator w = found.first; w != found.second; ++w) {
      w->second->changed();
    }
  }
  AssetManager::self->refresh();
}

AssetWatcherManager::AssetWatcherManager(void)
{
  if (instance) {
    notify(NFY_FATAL, "Internal Error: Attempted to start a second AssetWatcherManager");
  }
  instance = this;

  connect(SystemListener::instance,
          SIGNAL(assetsChanged(const AssetChanges &)),
          this,
          SLOT(assetsChanged(const AssetChanges &)));
}

AssetWatcherManager::~AssetWatcherManager(void)
{
  disconnect(SystemListener::instance,
             SIGNAL(assetsChanged(const AssetChanges &)),
             this,
             SLOT(assetsChanged(const AssetChanges &)));

  instance = 0;
}


void
AssetWatcherManager::AddWatcher(AssetWatcher *watcher)
{
  watchers.insert(std::make_pair(watcher->ref, watcher));
}

void
AssetWatcherManager::DropWatcher(AssetWatcher *watcher)
{
  WatcherRange found = watchers.equal_range(watcher->ref);
  for (WatchersType::iterator w = found.first; w != found.second; ++w) {
    if (w->second == watcher) {
      watchers.erase(w);
      break;
    }
  }
}
