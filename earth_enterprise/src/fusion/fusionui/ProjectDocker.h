/*
 * Copyright 2017-2020 Google Inc.
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


#ifndef KHSRC_FUSION_FUSIONUI_PROJECTDOCKER_H__
#define KHSRC_FUSION_FUSIONUI_PROJECTDOCKER_H__

#include <Qt/q3dockwindow.h>
#include <Qt/qobjectdefs.h>
using QDockWindow = Q3DockWindow;
class QPushButton;
class ProjectManager;
class gstDrawState;

class ProjectDocker : public QDockWindow {
  Q_OBJECT

 public:
  ProjectDocker(Place p, QWidget* parent, const char* name);

  ProjectManager* Preview() const { return preview_; }

 protected slots:
  void selectBox(const gstDrawState& s, Qt::ButtonState bs);

  void enableAllLayers();
  void disableAllLayers();

  void expandAllLayers();
  void collapseAllLayers();

 private:
  ProjectManager* preview_;
};

#endif  // !KHSRC_FUSION_FUSIONUI_PROJECTDOCKER_H__
