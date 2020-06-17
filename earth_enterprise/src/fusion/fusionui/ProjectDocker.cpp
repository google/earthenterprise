// Copyright 2017-2020 Google Inc.
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


#include <Qt/q3vgroupbox.h>
using QVGroupBox = Q3VGroupBox;
#include <Qt/qlayout.h>

#include "ProjectDocker.h"
#include "ProjectManager.h"

ProjectDocker::ProjectDocker(Place p, QWidget* parent, const char* name)
    : QDockWindow(p, parent, name, 0) {
  setResizeEnabled(true);
  setCloseMode(QDockWindow::Always);
  setCaption(name);

  QVGroupBox* box = new QVGroupBox(this);
  box->layout()->setMargin(0);
  box->setMinimumSize(QSize(200, 200));

  setWidget(box);

  // Preview project
  preview_ = new ProjectManager(box, "Preview", ProjectManager::Preview);
}

void ProjectDocker::selectBox(const gstDrawState& draw_state,
                              Qt::ButtonState btn_state) {
  preview_->selectBox(draw_state, btn_state);
}

void ProjectDocker::enableAllLayers() {
  preview_->enableAllLayers(true);
}

void ProjectDocker::disableAllLayers() {
  preview_->enableAllLayers(false);
}

void ProjectDocker::expandAllLayers() {
  preview_->openAllLayers(true);
}

void ProjectDocker::collapseAllLayers() {
  preview_->openAllLayers(false);
}
