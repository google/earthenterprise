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



#ifndef _SystemManager_h_
#define _SystemManager_h_
#include <Qt/qobjectdefs.h>
#include "systemmanagerbase.h"
#include <vector>
#include <string>
#include <Qt/qtimer.h>

class AssetChanges;

class SystemManager : public SystemManagerBase
{
  Q_OBJECT

  std::vector<std::string> waiting;
  std::vector<std::string> active;
  QTimer taskTimer;
    
 public:
  SystemManager( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, Qt::WFlags fl = 0 );

  // overloaded so I can connect & disconnect my slot
  virtual void show();
  virtual void hide();

private slots:
void updateTasks();
  void assetsChanged(const AssetChanges &);
};

#endif // !_SystemManager_h_
