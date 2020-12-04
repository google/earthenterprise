/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors
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

//

#ifndef FUSION_FUSIONUI_RASTERPROJECTWIDGET_H__
#define FUSION_FUSIONUI_RASTERPROJECTWIDGET_H__
#include <Qt/qobjectdefs.h>
#include "ProjectWidget.h"
#include <string>
#include <autoingest/.idl/storage/AssetDefs.h>
#include <autoingest/.idl/storage/LayerLegend.h>
#include <gstBBox.h>
#include <WidgetControllers.h>

class RasterProjectEditRequest;
class gstDrawState;
class LayerItemBase;

class RasterProjectWidget : public ProjectWidget {
 Q_OBJECT

 public:
  RasterProjectWidget(QWidget *parent, AssetDefs::Type asset_type,
                      const std::string& sub_type);
  ~RasterProjectWidget();

  void Prefill(const RasterProjectEditRequest &req);
  void AssembleEditRequest(RasterProjectEditRequest *request);

 protected slots:
  void DrawFeatures(const gstDrawState& state);

 signals:
  void ZoomToSource(const gstBBox& b);
  void RedrawPreview();

 private:
  inline unsigned int ProductToDisplayLevel(unsigned int product_level) const {
    return product_level - level_diff_;
  }
  inline unsigned int DisplayToProductLevel(unsigned int display_level) const {
    return display_level + level_diff_;
  }

  // Checks whether the specified imagery resource has a valid acquisition
  // date, returning true if it is valid.
  // If not, it will display a warning and return false.
  // if resource is empty, then all resources in the project are tested.
  // Assumes: dialog list of Layers is populated already.
  bool CheckForValidDates(const std::string& resource);
  // Helper to check all resources for valid acquisition dates.
  // returns true if valid, false if not. If false it will display a warning
  // message.
  bool CheckForValidDates() {
    std::string empty_string("");
    return CheckForValidDates(empty_string);
  }
  // inherited from ProjectWidget
  virtual void ContextMenu(Q3ListViewItem* item, const QPoint& pt, int col);
  virtual LayerItemBase* NewLayerItem();
  virtual LayerItemBase* NewLayerItem(const QString& assetref);
  virtual void GenericCheckboxToggled(bool state);
  virtual void TimeMachineCheckboxToggled(bool state);

  AssetDefs::Type asset_type_;
  const std::string sub_type_;
  const bool is_mercator_;
  unsigned int level_diff_;
  LayerLegend legend_config_;
  WidgetControllerManager legendManager;
};

#endif // FUSION_FUSIONUI_RASTERPROJECTWIDGET_H__
