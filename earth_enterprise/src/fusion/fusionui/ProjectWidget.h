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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_PROJECTWIDGET_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_PROJECTWIDGET_H_

#include "projectwidgetbase.h"
#include "fusion/fusionui/QValidatorExt.h"
#include <Qt/q3listview.h>

class ProjectLayerView;
class LayerItemBase;

class ProjectWidget : public ProjectWidgetBase {
 public:
  explicit ProjectWidget(QWidget* parent);

  ProjectLayerView* ListView() { return layer_listview; }
  void HideGroupButton();

 protected:
  // all derived classes must provide both methods of adding an asset
  // without args will be called when the "Add Layer" button is pressed
  // and with args is called when a valid asset is 'dropped' on the listview
  virtual LayerItemBase* NewLayerItem() = 0;
  virtual LayerItemBase* NewLayerItem(const QString& assetref) = 0;

  // Generic Checkbox (for things like preview layers)
  void HideGenericCheckbox();
  bool GetGenericCheckboxState() const;
  void SetGenericCheckboxState(bool state);
  void SetGenericCheckboxText(const QString& text);

  // Legend
  void HideLegend();

  // UUID Edit Box
  void HideUuid();
  void ShowUuid();

  // TimeMachine Checkbox
  bool GetTimeMachineCheckboxState() const;
  void SetTimeMachineCheckboxState(bool state);
  void HideTimeMachineCheckbox();
  void ShowTimeMachineCheckbox();

  // OverlayTerrain controls
  void HideOverlayTerrainControls();
  void ShowOverlayTerrainControls();
  void SetEnabledOverlayTerrainParametersControls(bool state);
  void BuildOverlayTerrainCheckboxToggled(bool state);

  bool GetBuildOverlayTerrainCheckboxState() const;
  void SetBuildOverlayTerrainCheckboxState(bool state);
  int  GetOverlayTerrainStartLevelSpinbox() const;
  void SetOverlayTerrainStartLevelSpinbox(int val);
  int  GetOverlayTerrainResourcesMinLevelSpinbox() const;
  void SetOverlayTerrainResourcesMinLevelSpinbox(int val);

 private:
  // inherited from ProjectWidgetBase
  virtual void AddLayer();
  virtual void AddGroup();
  virtual void DeleteLayer();
  virtual void MoveLayerUp();
  virtual void MoveLayerDown();
  virtual void SelectLayer();
  virtual void DropAsset(const QString& assetref);
  virtual void GenericCheckboxToggled(bool state);
  virtual void TimeMachineCheckboxToggled(bool state);
  virtual void ContextMenu(Q3ListViewItem* item, const QPoint& pt, int col);

  // Validator for overlay_terrain_start_level_spinbox.
  qt_fusion::EvenQIntValidator start_level_validator_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_PROJECTWIDGET_H_
