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

// Note: need to keep this synced with MapDatabaseWidget.cpp

#include "fusion/fusionui/MercatorMapDatabaseWidget.h"

#include <string>
#include <vector>
#include <Qt/qframe.h>
#include <Qt/qlabel.h>
#include <Qt/qpushbutton.h>
#include <Qt/qgroupbox.h>
#include <Qt/qcheckbox.h>

#include <autoingest/.idl/AssetStorage.h>
#include <autoingest/plugins/MapProjectAsset.h>
#include "fusion/gst/gstAssetGroup.h"
#include "common/khConstants.h"

#include "fusion/fusionui/AssetBase.h"
#include "fusion/fusionui/AssetChooser.h"
#include "fusion/fusionversion.h"

QString MercatorMapDatabaseWidget::empty_text(kh::tr("<none>"));

MercatorMapDatabaseWidget::MercatorMapDatabaseWidget(QWidget* parent,
                                                     AssetBase* base)
  : MercatorMapDatabaseWidgetBase(parent), AssetWidgetBase(base) {
}

void MercatorMapDatabaseWidget::Prefill(const MapDatabaseEditRequest& request) {
  std::vector<std::string> projects;

  if (request.config.mapProject.size() != 0) {
    projects.push_back(request.config.mapProject);
    std::string san = shortAssetName(request.config.mapProject);
    map_project_label->setText(san.c_str());
  } else {
    map_project_label->setText(empty_text);
  }

  if (GetFusionProductType() == FusionLT) {
    EnableImageryProject(false);
  } else {
    if (request.config.imageryProject.size() != 0) {
      projects.push_back(request.config.imageryProject);
      SetImageryProject(QString(request.config.imageryProject.c_str()));
    } else {
      ClearImageryProject();
    }
  }

  use_google_imagery_check->setChecked(request.config.useGoogleImagery);
}

void MercatorMapDatabaseWidget::EnableImageryProject(bool state) {
  static QString save_imagery_project = empty_text;
  if (state == false) {
    save_imagery_project = imagery_project_path_;
    ClearImageryProject();
  } else {
    SetImageryProject(save_imagery_project);
  }

  imagery_project_label->setEnabled(state);
  clear_mercator_imagery_btn->setEnabled(state);
  choose_mercator_imagery_btn->setEnabled(state);
  imagery_pixmap->setEnabled(state);
  imagery_pre_label->setEnabled(state);
}

void MercatorMapDatabaseWidget::UseGoogleImagery(bool state) {
  // Google Maps and Fusion Mercator imagery are compatible...nothing to do.
}

void MercatorMapDatabaseWidget::AssembleEditRequest(
    MapDatabaseEditRequest* request) {
  if (map_project_label->text() != empty_text) {
    request->config.mapProject =
        map_project_label->text().latin1() + kMapProjectSuffix;
  } else {
    request->config.mapProject = std::string();
  }

  if (imagery_project_label->text() != empty_text) {
    request->config.imageryProject = GetImageryProjectPath();
  } else {
    request->config.imageryProject = std::string();
  }

  request->config.useGoogleImagery = use_google_imagery_check->isOn();
  request->config.is_mercator = true;
}

void MercatorMapDatabaseWidget::ChooseMapProject() {
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

void MercatorMapDatabaseWidget::ChooseImageryProject() {
  std::vector<AssetChooser::AssetCategoryDef> compatible_asset_defs;
  compatible_asset_defs.push_back(
      AssetChooser::AssetCategoryDef(AssetDefs::Imagery,
                                     kMercatorProjectSubtype));
  compatible_asset_defs.push_back(
      AssetChooser::AssetCategoryDef(AssetDefs::Imagery, kProjectSubtype));

  AssetChooser chooser(
      this, AssetChooser::Open,
      compatible_asset_defs,
      std::string("All Imagery Projects"),  // 'all compatible assets' text.
      std::string(""));  // no icon for 'all compatible assets' filter.
  if (chooser.exec() != QDialog::Accepted)
    return;

  QString newpath;
  if (!chooser.getFullPath(newpath))
    return;

  SetImageryProject(newpath);
}

void MercatorMapDatabaseWidget::ClearMapProject() {
  map_project_label->setText(empty_text);
}

void MercatorMapDatabaseWidget::ClearImageryProject() {
  imagery_project_path_ = "";
  imagery_project_label->setText(empty_text);
}

void MercatorMapDatabaseWidget::SetImageryProject(const QString& path) {
  if (path == empty_text) {
    ClearImageryProject();
  } else {
    imagery_project_path_ = path;
    std::string san = shortAssetName(imagery_project_path_);
    imagery_project_label->setText(san.c_str());
  }
}
