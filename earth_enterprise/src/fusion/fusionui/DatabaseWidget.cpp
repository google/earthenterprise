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


#include <qframe.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qpushbutton.h>

#include <gstAssetGroup.h>
#include "AssetChooser.h"
#include "DatabaseWidget.h"
#include <fusionversion.h>

class AssetBase;

QString DatabaseWidget::empty_text(tr("<none>"));

DatabaseWidget::DatabaseWidget(QWidget* parent, AssetBase* base)
  : DatabaseWidgetBase(parent), AssetWidgetBase(base) {
}

void DatabaseWidget::Prefill(const DatabaseEditRequest& request) {
  std::vector<std::string> projects;

  if (request.config.vectorProject.size() != 0) {
    projects.push_back(request.config.vectorProject);
    std::string san = shortAssetName(request.config.vectorProject);
    vector_project_label->setText(san.c_str());
  } else {
    vector_project_label->setText(empty_text);
  }

  if (GetFusionProductType() == FusionLT) {
    imagery_project_label->setText(empty_text);
    terrain_project_label->setText(empty_text);
    pro_frame->setEnabled(false);
  } else {
    if (request.config.imageryProject.size() != 0) {
      projects.push_back(request.config.imageryProject);
      std::string san = shortAssetName(request.config.imageryProject);
      imagery_project_label->setText(san.c_str());
    } else {
      imagery_project_label->setText(empty_text);
    }

    if (request.config.terrainProject.size() != 0) {
      projects.push_back(request.config.terrainProject);
      std::string san = shortAssetName(request.config.terrainProject);
      terrain_project_label->setText(san.c_str());
    } else {
      terrain_project_label->setText(empty_text);
    }
  }
}

void DatabaseWidget::AssembleEditRequest(DatabaseEditRequest* request) {
  if (vector_project_label->text() != empty_text) {
    request->config.vectorProject =
        vector_project_label->text().latin1() + kVectorProjectSuffix;
  } else {
    request->config.vectorProject = std::string();
  }

  if (imagery_project_label->text() != empty_text) {
    request->config.imageryProject =
        imagery_project_label->text().latin1() + kImageryProjectSuffix;
  } else {
    request->config.imageryProject = std::string();
  }

  if (terrain_project_label->text() != empty_text) {
    request->config.terrainProject =
        terrain_project_label->text().latin1() + kTerrainProjectSuffix;
  } else {
    request->config.terrainProject = std::string();
  }

}

void DatabaseWidget::ChooseVectorProject() {
  AssetChooser chooser(this, AssetChooser::Open, AssetDefs::Vector,
                       kProjectSubtype);
  if (chooser.exec() != QDialog::Accepted)
    return;

  QString newpath;
  if (!chooser.getFullPath(newpath))
    return;

  std::string san = shortAssetName(newpath);
  vector_project_label->setText(san.c_str());
}

void DatabaseWidget::ChooseImageryProject() {
  AssetChooser chooser(this, AssetChooser::Open, AssetDefs::Imagery,
                       kProjectSubtype);
  if (chooser.exec() != QDialog::Accepted)
    return;

  QString newpath;
  if (!chooser.getFullPath(newpath))
    return;

  std::string san = shortAssetName(newpath);
  imagery_project_label->setText(san.c_str());
}

void DatabaseWidget::ChooseTerrainProject() {
  AssetChooser chooser(this, AssetChooser::Open, AssetDefs::Terrain,
                       kProjectSubtype);
  if (chooser.exec() != QDialog::Accepted)
    return;

  QString newpath;
  if (!chooser.getFullPath(newpath))
    return;
  std::string san = shortAssetName(newpath);
  terrain_project_label->setText(san.c_str());
}

void DatabaseWidget::ClearVectorProject() {
  vector_project_label->setText(empty_text);
}

void DatabaseWidget::ClearImageryProject() {
  imagery_project_label->setText(empty_text);
}

void DatabaseWidget::ClearTerrainProject() {
  terrain_project_label->setText(empty_text);
}
