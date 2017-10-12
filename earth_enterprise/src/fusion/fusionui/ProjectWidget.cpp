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


#include "fusion/fusionui/ProjectWidget.h"
#include "fusion/autoingest/.idl/storage/PacketLevelConfig.h"
#include "fusion/autoingest/sysman/plugins/PacketLevelAssetD.h"
#include "Preferences.h"

#include <qpushbutton.h>
#include <qlistview.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <notify.h>

#include "fusion/fusionui/LayerItemBase.h"
#include "fusion/fusionui/ProjectLayerView.h"

#include "common/khTileAddrConsts.h"
//#include "common/khException.h"
#include "khException.h"

//bool DecimationThreshold::decimate = true;
//double DecimationThreshold::decimation_threshold = 0.09;

//bool ProjectWidget::decimate = true;
//double DecimationThreshold::decimation_threshold = 0.09;

ProjectWidget::ProjectWidget(QWidget* parent)
    : ProjectWidgetBase(parent),
      start_level_validator_(StartTmeshLevel, MaxTmeshLevel) {
  // Set overlay_terrain_start_level_spinbox.
  overlay_terrain_start_level_spinbox->setValidator(&start_level_validator_);

  HideUuid();
  HideTimeMachineCheckbox();
  HideOverlayTerrainControls();
  SelectLayer();
}

void ProjectWidget::AddLayer() {
  LayerItemBase* item = NewLayerItem();
  if (item) {
    ListView()->SelectItem(item);
  }
}

void ProjectWidget::DropAsset(const QString& assetref) {
  LayerItemBase* item = NewLayerItem(assetref);
  if (item) {
    ListView()->SelectItem(item);
  }
}

void ProjectWidget::HideGroupButton() {
  add_group_btn->hide();
}

void ProjectWidget::AddGroup() {
}

void ProjectWidget::DeleteLayer() {
  LayerItemBase* item =
      static_cast<LayerItemBase*>(layer_listview->currentItem());
  if (item == NULL)
    return;

  LayerItemBase* select_after_delete = NULL;

  if (item->Next()) {
    select_after_delete = item->Next();
  } else if (item->Previous()) {
    select_after_delete = item->Previous();
  }

  delete item;

  layer_listview->setSelected(select_after_delete, true);
}

void ProjectWidget::MoveLayerUp() {
  LayerItemBase* item =
      static_cast<LayerItemBase*>(layer_listview->currentItem());
  if (item == NULL)
    return;

  LayerItemBase* prev = item->Previous();
  if (prev == NULL)
    return;

  item->SwapPosition(prev);
  ListView()->SelectItem(item);
}

void ProjectWidget::MoveLayerDown() {
  LayerItemBase* item =
      static_cast<LayerItemBase*>(layer_listview->currentItem());
  if (item == NULL)
    return;

  LayerItemBase* next = item->Next();
  if (next == NULL)
    return;

  item->SwapPosition(next);
  ListView()->SelectItem(item);
}

void ProjectWidget::SelectLayer() {
  QListViewItem* item = layer_listview->selectedItem();
  LayerItemBase* layer_item = static_cast<LayerItemBase*>(item);
  if (layer_item) {
    delete_layer_btn->setEnabled(true);
    move_down_btn->setEnabled(layer_item->CanMoveDown());
    move_up_btn->setEnabled(layer_item->CanMoveUp());
    layer_listview->setSelected(item, true);
  } else {
    delete_layer_btn->setEnabled(false);
    move_down_btn->setEnabled(false);
    move_up_btn->setEnabled(false);
  }
}

void ProjectWidget::HideGenericCheckbox() {
  generic_check->hide();
}

void ProjectWidget::HideLegend() {
  legend_group_box->hide();
}

void ProjectWidget::HideUuid() {
  uuid_label->hide();
  uuid_edit->hide();
}

void ProjectWidget::ShowUuid() {
  uuid_label->show();
  uuid_edit->show();
}


bool ProjectWidget::GetTimeMachineCheckboxState() const {
  return is_timemachine_checkbox->isChecked();
}

void ProjectWidget::SetTimeMachineCheckboxState(bool state) {
  is_timemachine_checkbox->setChecked(state);
  if (state) {
    timemachine_pixmap->setPixmap(
                                QPixmap::fromMimeSource("timemachine-on.png"));
  } else {
    timemachine_pixmap->setPixmap(
                                QPixmap::fromMimeSource("timemachine-off.png"));
  }
}

void ProjectWidget::HideTimeMachineCheckbox() {
  timemachine_pixmap->hide();
  is_timemachine_checkbox->hide();
}

void ProjectWidget::ShowTimeMachineCheckbox() {
  timemachine_pixmap->show();
  is_timemachine_checkbox->show();
}

void ProjectWidget::TimeMachineCheckboxToggled(bool state) {
  // do nothing, let derived classes override
}

// OverlayTerrain controls.
void ProjectWidget::HideOverlayTerrainControls() {
  build_overlay_terrain_checkbox->hide();
  overlay_terrain_resources_min_level_label->hide();
  overlay_terrain_resources_min_level_spinbox->hide();
  overlay_terrain_start_level_label->hide();
  overlay_terrain_start_level_spinbox->hide();
}

void ProjectWidget::ShowOverlayTerrainControls() {
  bool enabled = GetBuildOverlayTerrainCheckboxState();
  SetEnabledOverlayTerrainParametersControls(enabled);

  build_overlay_terrain_checkbox->show();
  overlay_terrain_resources_min_level_label->show();
  overlay_terrain_resources_min_level_spinbox->show();
  overlay_terrain_start_level_label->show();
  overlay_terrain_start_level_spinbox->show();
}

void ProjectWidget::SetEnabledOverlayTerrainParametersControls(bool enabled) {
  overlay_terrain_resources_min_level_label->setEnabled(enabled);
  overlay_terrain_resources_min_level_spinbox->setEnabled(enabled);
  overlay_terrain_start_level_label->setEnabled(enabled);
  overlay_terrain_start_level_spinbox->setEnabled(enabled);
}

void ProjectWidget::BuildOverlayTerrainCheckboxToggled(bool state) {
  SetEnabledOverlayTerrainParametersControls(state);
}

bool ProjectWidget::GetBuildOverlayTerrainCheckboxState() const {
  return build_overlay_terrain_checkbox->isChecked();
}

void ProjectWidget::SetBuildOverlayTerrainCheckboxState(bool state) {
  build_overlay_terrain_checkbox->setChecked(state);
}

int ProjectWidget::GetOverlayTerrainStartLevelSpinbox() const {
  return overlay_terrain_start_level_spinbox->value();
}

void ProjectWidget::SetOverlayTerrainStartLevelSpinbox(int val) {
  overlay_terrain_start_level_spinbox->setValue(val);
}

int ProjectWidget::GetOverlayTerrainResourcesMinLevelSpinbox() const {
  return overlay_terrain_resources_min_level_spinbox->value();
}

void ProjectWidget::SetOverlayTerrainResourcesMinLevelSpinbox(int val) {
  overlay_terrain_resources_min_level_spinbox->setValue(val);
}

bool ProjectWidget::GetGenericCheckboxState() const {
  return generic_check->isChecked();
}

void ProjectWidget::SetGenericCheckboxState(bool state) {
  generic_check->setChecked(state);
}

void ProjectWidget::GenericCheckboxToggled(bool state) {
  // do nothing, let derived classes override
}

void ProjectWidget::SetGenericCheckboxText(const QString& text) {
  generic_check->setText(text);
}


///////////////////////////////////////////////////////////////////////////// temporary
#define MAX_DECIMATION 100
#include <stdio.h>
// this is bananas, i shouldn't have to initialize these again here since i already do it somewhere else, 
// but if i don't, this won't build, and if i don't also initialize them in PacketLevel.src it doesn't link. 
// I can initialize them in Preferences.cpp or main.cpp instead of here, but PacketLevelAssetVersionImplD still doesn't
// understand it unless i initialize therem there as well, even though they are members in its own class. 
// There are static things in preferences (class PrefsConfig), but i wasn't able to get PacketLevel to link against it.
// Tried fixing the includes (like ../../../fusion/fusionui/Preferences.h or whatever it was) but then that file had an include
// path issue against another file that I couldn't figure out how to correct. All the library stuff is very strange. One file
// might be included like <thing.h> in one place and "../../../something/thing.h" somewhere else and <something/thing.h> in a
// third place. I can use relative paths from the autogen code to the fusion code, but not the other way around, without 
// getting some information from the build scripts themselves (because otherwise won't know the name of the build directory,
// like NATIVE-${etc}).   I even tried putting values in preferences.idl, which is autogen code in the fusionui directory
// (and therefore conceptually somewhere in between the two parts), but that didn't work out either. Also tried in 
// autoingest/storage/PacketLevelConfig.idl since that's where some other relevant things are. 
//bool PacketLevelAssetVersionImplD::decimate = true;
//double PacketLevelAssetVersionImplD::decimation = 0.09;

void ProjectWidget::SetDecimation(const QString& decimation) { 

  bool status = false;
  double decimation_val = -1; 
  decimation_val = decimation.toDouble(); //decimation->text().toDouble(&status);
  //if(decimation_val == PrefsConfig::decimation_threshold) return;
  if(decimation_val == PacketLevelAssetVersionImplD::decimation) return;

  if(decimation_val >= 0 &&
     decimation_val <= MAX_DECIMATION) {
    status = true;
  }

  //printf("In %s %d and prior to setting, config.decimation is %lf\n", __FUNCTION__, __LINE__, config.decimation);
  if(!status) {
    std::string decimation_error;
    decimation_error += "Bad value for decimation threshold '";
    // decimation_error += decimation.text().latin1();
    decimation_error += (std::string)decimation_error;
    decimation_error += "'\n";
    //notify(NFY_WARN, decimation_error.c_str());
    notify(NFY_DEBUG, "bad decimation value\n");
    throw khException(decimation_error); 
  }

  else {
     //config.decimation = decimation_val;
     //PrefsConfig::decimate = true;
     //PrefsConfig::decimation_threshold = decimation_val;
     //decimate = true;
     //decimation_threshold = decimation_val;
    PacketLevelAssetVersionImplD::decimate = true;
    PacketLevelAssetVersionImplD::decimation = decimation_val;
     notify(NFY_DEBUG, "in SetDecimation line %d and decimation is %lf\n", __LINE__, decimation_val);
     //printf("In %s %d and after setting, decimation is %lf, input decimation was %lf\n", __FUNCTION__, __LINE__, PrefsConfig::decimation_threshold, decimation_val);
     printf("In %s %d and after setting, decimation is %lf, input decimation was %lf\n", __FUNCTION__, __LINE__, PacketLevelAssetVersionImplD::decimation, decimation_val);
  }
}


void ProjectWidget::ContextMenu(
    QListViewItem* item, const QPoint& pt, int col) {
  // do nothing, let derived classes override
}
