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


#ifndef KHSRC_FUSION_FUSIONUI_MAPPROJECT_H__
#define KHSRC_FUSION_FUSIONUI_MAPPROJECT_H__

#include "AssetWidgetBase.h"
#include "AssetDerived.h"
#include "ProjectWidget.h"
#include "AssetDisplayHelper.h"
#include <autoingest/.idl/storage/MapProjectConfig.h>
#include <Qt/qobject.h>
#include <Qt/qglobal.h>
class AssetBase;
class MapProjectEditRequest;
class QL3istViewItem;

class MapProjectWidget : public ProjectWidget,
                         public AssetWidgetBase {
  Q_OBJECT

 public:
  MapProjectWidget(QWidget *parent, AssetBase* base);

  void Prefill(const MapProjectEditRequest &req);
  void AssembleEditRequest(MapProjectEditRequest *request);

 public slots:
  void ModifyItem(Q3ListViewItem* item, const QPoint& pt, int col);

 private:
  // inherited from ProjectWidget
  virtual LayerItemBase* NewLayerItem();
  virtual LayerItemBase* NewLayerItem(const QString& assetref);
  virtual void ContextMenu(Q3ListViewItem* item, const QPoint& pt, int col);
};

class MapProjectDefs {
 public:
  typedef MapProjectEditRequest  Request;
  typedef MapProjectWidget      Widget;
  typedef bool (*SubmitFuncType)(const Request&, QString &, int);

  static const AssetDisplayHelper::AssetKey kAssetDisplayKey =
      AssetDisplayHelper::Key_MapProject;
  static SubmitFuncType        kSubmitFunc;
};


class MapProject : public AssetDerived<MapProjectDefs,MapProject>,
                       public MapProjectDefs {
 public:
  MapProject(QWidget* parent);
  MapProject(QWidget* parent, const Request& req);
};

#endif  // !KHSRC_FUSION_FUSIONUI_MAPPROJECT_H__
