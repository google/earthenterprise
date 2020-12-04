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

// Note: need to keep this synced with MapDatabaseWidget.h


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_MERCATORMAPDATABASEWIDGET_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_MERCATORMAPDATABASEWIDGET_H_

#include <autoingest/.idl/storage/MapDatabaseConfig.h>
#include "mercatormapdatabasewidgetbase.h"
#include "common/khRefCounter.h"
#include "fusion/fusionui/AssetWidgetBase.h"

class AssetBase;
class DatabaseSearchTabs;

class MercatorMapDatabaseWidget : public MercatorMapDatabaseWidgetBase,
                                  public AssetWidgetBase {
 public:
  MercatorMapDatabaseWidget(QWidget* parent, AssetBase* base);

  void Prefill(const MapDatabaseEditRequest& request);
  void AssembleEditRequest(MapDatabaseEditRequest* request);

  // inherited from MercatorMapDatabaseWidgetBase
  virtual void ChooseMapProject();
  virtual void ClearMapProject();
  virtual void ChooseImageryProject();
  virtual void ClearImageryProject();
  virtual void UseGoogleImagery(bool state);

 private:
  void EnableImageryProject(bool state);
  void SetImageryProject(const QString& path);
  inline std::string GetImageryProjectPath() const {
    return imagery_project_path_.toUtf8().constData();
  }

  static QString empty_text;
  QString imagery_project_path_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_MERCATORMAPDATABASEWIDGET_H_
