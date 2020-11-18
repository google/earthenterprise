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


#include <algorithm>
#include <Qt/q3popupmenu.h>
#include <qcursor.h>
#include <khFileUtils.h>

#include "FileHistory.h"
using QPopupMenu = Q3PopupMenu;
FileHistory::FileHistory(QWidget* parent, const QString& path)
    : QPushButton(parent), filePath(path) {
  setText(trUtf8("Recent..."));

  // retrieve from disk
  if (khExists(filePath.latin1()))
    history.Load(filePath.latin1());

  connect(this, SIGNAL(clicked()), this, SLOT(chooseRecent()));
}

void FileHistory::chooseRecent() {
  QStringList recent = getFileList();

  if (recent.size() == 0)
    return;

  QPopupMenu menu(this);

  int p = 0;
  for (QStringList::Iterator it = recent.begin(); it != recent.end(); ++it, ++p)
    menu.insertItem((*it), p);
  menu.insertSeparator();
  menu.insertItem(trUtf8("Clear History"), 999);

  int pos = menu.exec(QCursor::pos());

  if (pos == 999) {
    clearFileHistory();
  } else if (pos != -1)  {
    emit selectFile(recent[pos]);
  }
}

QStringList FileHistory::getFileList() const {
  QStringList list;
  for (std::list<QString>::const_iterator it = history.files.begin();
       it != history.files.end(); ++it) {
    list << *it;
  }
  return list;
}

// add new filepath to our history
void FileHistory::addFile(const QString& fname) {
  // search list for this path in the current history
  std::list<QString>::iterator found = std::find(history.files.begin(),
                                                 history.files.end(), fname);

  // if the path was found, remove it as it will be added again at the top
  if (found != history.files.end())
    history.files.erase(found);

  history.files.push_front(fname);

  // clamp to maximum size
  if (history.files.size() > kMaxHistoryLength)
    history.files.resize(kMaxHistoryLength);

  history.Save(filePath.latin1());
}

void FileHistory::addFiles(const QStringList& flist) {
  for (QStringList::ConstIterator it = flist.begin(); it != flist.end(); ++it) {
    std::list<QString>::iterator found = std::find(history.files.begin(),
                                                   history.files.end(), *it);

    // if the path was found, remove it as it will be added again at the top
    if (found != history.files.end())
      history.files.erase(found);

    history.files.push_front(*it);
  }

  // clamp to maximum size
  if (history.files.size() > kMaxHistoryLength)
    history.files.resize(kMaxHistoryLength);

  history.Save(filePath.latin1());
}

void FileHistory::clearFileHistory() {
  history.files.clear();
  history.Save(filePath.latin1());
}
