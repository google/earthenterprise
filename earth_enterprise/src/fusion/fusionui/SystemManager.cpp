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

#include "SystemManager.h"
#include "SystemListener.h"
#include <autoingest/khAssetManagerProxy.h>
#include <notify.h>
#include <Qt/qapplication.h>
#include <Qt/q3listbox.h>
#include <Qt/qtooltip.h>
#include <Qt/qpainter.h>
#include <Qt/qpen.h>
using QListBoxText = Q3ListBoxText;
using QListBoxItem = Q3ListBoxItem;
using QListBox = Q3ListBox;

class WaitingItem : public QListBoxText
{
 protected:
  virtual void paint(QPainter *painter) {
    if (!error.isEmpty()) {
      QPen oldpen = painter->pen();
      painter->setPen(Qt::red);
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

/* TODO: Momentarily disabled

class WaitingErrorTip // : public QToolTip
{

 public:
  WaitingErrorTip(QListBox *listbox) // : QToolTip(listbox) { }
  {
      QToolTip::add(listbox, QString());
  }
  virtual ~WaitingErrorTip(void) { }
 protected:
  virtual void maybeTip(const QPoint &p) {
    QListBox *listbox = dynamic_cast<QListBox*>(parentWidget());
    if (listbox) {
      QListBoxItem *item = listbox->itemAt(p);
      if (item) {
        WaitingItem *witem = dynamic_cast<WaitingItem*>(item);
        if (witem && !witem->error.isEmpty()) {
          // the original QT3 call to display a tooltop popup was to:
          //    tip(listbox->itemRect(item), witem->error);
          QToolTip::showText(listbox->itemRect(item).topLeft(),witem->error);
        }
      }

    }
  }
};*/

SystemManager::SystemManager( QWidget* parent, const char* name, bool modal, Qt::WFlags fl )
    : SystemManagerBase( parent, name, modal, fl ), taskTimer(this)
{
  connect(&taskTimer, SIGNAL(timeout()), this, SLOT(updateTasks()));

  waitingList->setSelectionMode(QListBox::NoSelection);
  activeList->setSelectionMode(QListBox::NoSelection);

  //(void) new WaitingErrorTip(waitingList);
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
  static const unsigned int maxitems = 1000;
  unsigned int count = activityList->count() + changes.items.size();

  activityList->setUpdatesEnabled(false);

  // prune the log widget so it doesn't get too long
  while (count > maxitems) {
    activityList->removeItem(0);
    --count;
  }

  // add new log entries
  for (AssetChanges::CIterator i = changes.items.begin();
       i != changes.items.end(); ++i) {
    std::string itemname { i->ref.toString() };
    itemname += ": " + i->desc;
    activityList->insertItem(QString(itemname.c_str()));
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
    QString errorMsg;
    std::string temp_msg { "GetCurrTasks: "};
    temp_msg += sysManBusyMsg;
    static const QString BUSY_MSG = temp_msg.c_str();
    if (error.compare(BUSY_MSG) == 0)
      errorMsg = kh::tr("--- System Manager is busy ---");
    else
      errorMsg = kh::tr("--- Unable to contact System Manager ---");

    // Can't get TaskList...
    // clean top panes and display message about
    // being unable to connect to asset manager
    waitingList->clear();
    activeList->clear();
    (void) new WaitingItem(waitingList, errorMsg);
    activeList->insertItem(errorMsg);

  } else {
    // Waiting list
    waitingList->clear();
    for (std::vector<TaskLists::WaitingTask>::const_iterator w =
           taskLists.waitingTasks.begin();
         w != taskLists.waitingTasks.end(); ++w) {
      (void) new WaitingItem(waitingList, w->verref.c_str(),
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
        QString itemname = p->hostname.c_str();
        itemname += "; ";
        itemname += a->verref.c_str();
        activeList->insertItem(itemname);
      }
    }
  }
}
