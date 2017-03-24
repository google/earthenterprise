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



#include "SystemManager.h"
#include "SystemListener.h"
#include <autoingest/khAssetManagerProxy.h>
#include <notify.h>
#include <qapplication.h>
#include <qlistbox.h>
#include <qtooltip.h>
#include <qpainter.h>
#include <qpen.h>


class WaitingItem : public QListBoxText
{
 protected:
  virtual void paint(QPainter *painter) {
    if (!error.isEmpty()) {
      QPen oldpen = painter->pen();
      painter->setPen(QObject::red);
      QListBoxText::paint(painter);
      painter->setPen(oldpen);
    } else {
      QListBoxText::paint(painter);
    }
  }
 public:
  QString error;
  WaitingItem(QListBox *parent, const QString &text,
              const QString &err = QString()) :
      QListBoxText(parent, text), error(err) {
  }
};

class WaitingErrorTip : public QToolTip
{
 public:
  WaitingErrorTip(QListBox *listbox) : QToolTip(listbox) { }
  virtual ~WaitingErrorTip(void) { }
 protected:
  virtual void maybeTip(const QPoint &p) {
    QListBox *listbox = dynamic_cast<QListBox*>(parentWidget());
    if (listbox) {
      QListBoxItem *item = listbox->itemAt(p);
      if (item) {
        WaitingItem *witem = dynamic_cast<WaitingItem*>(item);
        if (witem && !witem->error.isEmpty()) {
          tip(listbox->itemRect(item), witem->error);
        }
      }
            
    }
  }
};

SystemManager::SystemManager( QWidget* parent, const char* name, bool modal, WFlags fl )
    : SystemManagerBase( parent, name, modal, fl ), taskTimer(this)
{
  connect(&taskTimer, SIGNAL(timeout()), this, SLOT(updateTasks()));

  waitingList->setSelectionMode(QListBox::NoSelection);
  activeList->setSelectionMode(QListBox::NoSelection);

  (void) new WaitingErrorTip(waitingList);
}


void
SystemManager::show()
{
  if ( !isVisible() ) {
    connect(SystemListener::instance,
            SIGNAL(assetsChanged(const AssetChanges &)),
            this,
            SLOT(assetsChanged(const AssetChanges &)));
    taskTimer.start( 5000 );
  }

  // don't wait for the timer to populate the tasks for the first time
  updateTasks();

  SystemManagerBase::show();
}

void
SystemManager::hide()
{
  SystemManagerBase::hide();
  disconnect(SystemListener::instance,
             SIGNAL(assetsChanged(const AssetChanges &)),
             this,
             SLOT(assetsChanged(const AssetChanges &)));
  taskTimer.stop();
}


void
SystemManager::assetsChanged(const AssetChanges &changes)
{
  static const uint maxitems = 1000;
  uint count = activityList->count() + changes.items.size();

  activityList->setUpdatesEnabled(false);

  // prune the log widget so it doesn't get too long
  while (count > maxitems) {
    activityList->removeItem(0);
    --count;
  }

  // add new log entries
  for (AssetChanges::CIterator i = changes.items.begin();
       i != changes.items.end(); ++i) {
    activityList->insertItem( i->ref + ": " + i->desc );
  }


  // make sure the last one is visible
  if (!activityList->itemVisible(count-1)) {
    activityList->setBottomItem(count-1);
  }

  activityList->setUpdatesEnabled(true);
  activityList->repaint();

  // use this event as a clue to update the tasklist
  updateTasks();
}

void
SystemManager::updateTasks(void)
{
  QString error;
  TaskLists taskLists;
  if (!khAssetManagerProxy::GetCurrTasks("dummy", taskLists, error)) {
    // Can't get TaskList...
    // clean top panes and display message about
    // being unable to connect to asset manager
    waitingList->clear();
    activeList->clear();
    (void) new WaitingItem(waitingList,
                           tr("--- Unable to contact System Manager ---"));
    activeList->insertItem(tr("--- Unable to contact System Manager ---"));

  } else {
    // Waiting list
    waitingList->clear();
    for (std::vector<TaskLists::WaitingTask>::const_iterator w =
           taskLists.waitingTasks.begin();
         w != taskLists.waitingTasks.end(); ++w) {
      (void) new WaitingItem(waitingList, w->verref,
                             w->activationError);
    }

    // ActiveList
    activeList->clear();
    for (std::vector<TaskLists::Provider>::const_iterator p =
           taskLists.providers.begin();
         p != taskLists.providers.end(); ++p) {
      for (std::vector<TaskLists::ActiveTask>::const_iterator a =
             p->activeTasks.begin();
           a != p->activeTasks.end(); ++a) {
        activeList->insertItem(p->hostname + ": " + a->verref);
      }
    }
  }
}
