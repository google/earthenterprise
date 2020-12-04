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


#ifndef KHSRC_FUSION_FUSIONUI_ASSETICONVIEW_H__
#define KHSRC_FUSION_FUSIONUI_ASSETICONVIEW_H__

#include <Qt/q3iconview.h>
#include <Qt/qimage.h>
#include <Qt/qdir.h>
#include <gstAssetGroup.h>
#include <Qt/qnamespace.h>
#include <Qt/q3listview.h>
#include <Qt/qwidget.h>


// -----------------------------------------------------------------------------

using QIconView = Q3IconView;
using QIconViewItem = Q3IconViewItem;


class AssetIcon : public QIconViewItem {
 public:
  AssetIcon(QIconView* parent, gstAssetHandle handle, int sz);
  ~AssetIcon();

  void resize(int sz);
  static QImage* defaultImage;
  const QImage* image() const { return image_; }
  gstAssetHandle getAssetHandle() const { return asset_handle_; }

 private:
  QImage* image_;
  gstAssetHandle asset_handle_;
};

// -----------------------------------------------------------------------------

class AssetIconView : public QIconView {
 public:
  AssetIconView(QWidget* parent = 0, const char* name = 0, Qt::WFlags f = 0);

  QString currentLocation();

 protected:
  void startDrag();
};

#endif  // !KHSRC_FUSION_FUSIONUI_ASSETICONVIEW_H__
