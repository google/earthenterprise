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
File:        AssetVersionActions.h
Description:

-------------------------------------------------------------------------------
For history see CVS log (cvs log AssetVersionActions.h -or- Emacs Ctrl-xvl).
******************************************************************************/
#ifndef __AssetVersionActions_h
#define __AssetVersionActions_h


#include <qaction.h>
#include <qobject.h>
#include <Qt/q3popupmenu.h>
using QPopupMenu = Q3PopupMenu;

class AssetVersionActions : public QObject
{
  Q_OBJECT

  QWidget *parent;
  std::string verref;
  enum { REBUILD_ACTION = 1000,
         CANCEL_ACTION,
         CLEAN_ACTION,
         SETBAD_ACTION,
         CLEARBAD_ACTION };

 public slots:
 void parentDestroyed();

 public:
  void rebuild(void);
  void cancel(void);
  void clean(void);
  void setbad(void);
  void clearbad(void);

  AssetVersionActions(QWidget *parent, const std::string &ref);
  bool Contribute(QPopupMenu *menu);
  void handle(int item);
};

#endif /* __AssetVersionActions_h */
