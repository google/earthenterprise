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


#ifndef KHSRC_FUSION_FUSIONUI_PROJECTLAYERVIEW_H__
#define KHSRC_FUSION_FUSIONUI_PROJECTLAYERVIEW_H__

#include <Qt/qobjectdefs.h>
#include <string>
#include "AssetDrag.h"
#include <Qt/qwidget.h>
#include <Qt/q3listview.h>
#include <Qt/qevent.h>


class ProjectLayerView : public Q3ListView {
  Q_OBJECT

 public:
  ProjectLayerView(QWidget* parent = 0, const char* n = 0, Qt::WFlags f = 0);

  void EnableAssetDrops(AssetDefs::Type type, const std::string& subtype);
  void SelectItem(Q3ListViewItem* item);

 signals:
  void dropAsset(const QString& a);

 protected:
  // inherited from QScrollView
  virtual void contentsDragMoveEvent(QDragMoveEvent* e);
  virtual void contentsDropEvent(QDropEvent* e);

 private:
  AssetDefs::Type drag_asset_type_;
  std::string drag_asset_subtype_;
};

#endif  // !KHSRC_FUSION_FUSIONUI_PROJECTLAYERVIEW_H__
