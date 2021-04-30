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

// Note: need to keep this synced with MercatorMapDatabaseWidget.cpp

#include "fusion/fusionui/MapDatabaseWidget.h"

#include <qframe.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qcheckbox.h>

#include <autoingest/.idl/AssetStorage.h>
#include <autoingest/plugins/MapProjectAsset.h>
#include "fusion/gst/gstAssetGroup.h"
#include "common/khConstants.h"

#include "fusion/fusionui/AssetBase.h"
#include "fusion/fusionui/AssetChooser.h"
#include "fusion/fusionversion.h"

QString MapDatabaseWidget::empty_text(tr("<none>"));

MapDatabaseWidget::MapDatabaseWidget(QWidget* parent, AssetBase* base)
  : MapDatabaseWidgetBase(parent), AssetWidgetBase(base) {
}

void MapDatabaseWidget::Prefill(const MapDatabaseEditRequest& request) {
  std::vector<std::string> projects;
  std::string san;

  if (request.config.mapProject.size() != 0) {
    projects.push_back(request.config.mapProject);
    san = shortAssetName(request.config.mapProject);
    map_project_label->setText(san.c_str());
  } else {
    map_project_label->setText(empty_text);
  }

  if (GetFusionProductType() == FusionLT) {
    EnableImageryProject(false);
  } else {
    if (request.config.imageryProject.size() != 0) {
      projects.push_back(request.config.imageryProject);
      san = shortAssetName(request.config.imageryProject);
      imagery_project_label->setText(san.c_str());
    } else {
      imagery_project_label->setText(empty_text);
    }
  }
}

void MapDatabaseWidget::EnableImageryProject(bool state) {
  static QString save_imagery_project = empty_text;
  if (state == false) {
    save_imagery_project = imagery_project_label->text();
    imagery_project_label->setText(empty_text);
  } else {
    imagery_project_label->setText(save_imagery_project);
  }

  imagery_project_label->setEnabled(state);
  clear_imagery_btn->setEnabled(state);
  choose_imagery_btn->setEnabled(state);
  imagery_pixmap->setEnabled(state);
  imagery_pre_label->setEnabled(state);
}

void MapDatabaseWidget::AssembleEditRequest(MapDatabaseEditRequest* request) {
  if (map_project_label->text() != empty_text) {
    request->config.mapProject =
        map_project_label->text().toUtf8().constData();
    request->config.mapProject += kMapProjectSuffix;
  } else {
    request->config.mapProject = std::string();
  }

  if (imagery_project_label->text() != empty_text) {
    request->config.imageryProject =
        imagery_project_label->text().toUtf8().constData();
    request->config.imageryProject +=  std::string(kImageryProjectSuffix);
  } else {
    request->config.imageryProject = std::string();
  }

  request->config.useGoogleImagery = false;
  request->config.is_mercator = false;
}

void MapDatabaseWidget::ChooseMapProject() {
  AssetChooser chooser(this, AssetChooser::Open, AssetDefs::Map,
                       kProjectSubtype);
  if (chooser.exec() != QDialog::Accepted)
    return;

  QString newpath;
  if (!chooser.getFullPath(newpath))
    return;

  std::string san = shortAssetName(newpath);
  map_project_label->setText(san.c_str());
}

void MapDatabaseWidget::ChooseImageryProject() {
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

void MapDatabaseWidget::ClearMapProject() {
  map_project_label->setText(empty_text);
}

void MapDatabaseWidget::ClearImageryProject() {
  imagery_project_label->setText(empty_text);
}
