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


/******************************************************************************
File:        SystemListener.h
Description: 

-------------------------------------------------------------------------------
For history see CVS log (cvs log SystemListener.h -or- Emacs Ctrl-xvl).
******************************************************************************/
#ifndef __SystemListener_h
#define __SystemListener_h

#include <Qt/qobject.h>
#include <autoingest/AssetChangeListener.h>
#include <Qt/qobjectdefs.h>
#include <map>

class SystemListener : public QObject {
  Q_OBJECT

  khMutex lock;
  AssetChanges changes;
  void addChanges(const AssetChanges &changes_);

  class Listener : public AssetChangeListener {
    SystemListener &syslisten;
   protected:
    virtual void handleAssetChanges(const AssetChanges &changes) {
      syslisten.addChanges(changes);
    }
   public:
    Listener(SystemListener &syslisten_) : syslisten(syslisten_) { }
  } listener;

 public:
  static SystemListener *instance;

  SystemListener( void );
  ~SystemListener( void );
  virtual bool event(QEvent *);

  signals:
  void assetsChanged(const AssetChanges &changes);
};



class AssetWatcher;
class AssetWatcherManager : public QObject {
  Q_OBJECT

  typedef std::multimap<std::string, AssetWatcher*> WatchersType;
  typedef std::pair<WatchersType::iterator, WatchersType::iterator> WatcherRange;
  WatchersType watchers;
public slots:
void assetsChanged(const AssetChanges &changes);

 public:
  static AssetWatcherManager *instance;

  AssetWatcherManager(void);
  ~AssetWatcherManager(void);
  void AddWatcher(AssetWatcher *);
  void DropWatcher(AssetWatcher *);
};


class AssetWatcher {
 public:
  std::string ref;
  virtual void changed(void) { }

  AssetWatcher(const std::string &r) : ref(r) {
    AssetWatcherManager::instance->AddWatcher(this);
  }
  virtual ~AssetWatcher(void) {
    AssetWatcherManager::instance->DropWatcher(this);
  }
};


#endif /* __SystemListener_h */
