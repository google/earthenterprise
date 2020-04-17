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
#include <Qt/qpushbutton.h>
#include <Qt/qlistview.h>
#include <Qt/qcheckbox.h>
#include <Qt/qgroupbox.h>
#include <Qt/qlayout.h>
#include <Qt/qlabel.h>
#include <Qt/qlineedit.h>
#include <Qt/qspinbox.h>
#include "fusion/fusionui/LayerItemBase.h"
#include "fusion/fusionui/ProjectLayerView.h"
#include "common/khTileAddrConsts.h"

ProjectWidget::ProjectWidget(QWidget* parent)
    : ProjectWidgetBase(parent),
      start_level_validator_(StartTmeshLevel, MaxTmeshLevel) {
  // Set overlay_terrain_start_level_spinbox.
  // Qt4 accepts integer values by defaul
  //overlay_terrain_start_level_spinbox->setValidator(&start_level_validator_);

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
  Q3ListViewItem* item = layer_listview->selectedItem();
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
                                QPixmap("timemachine-on.png"));
  } else {
    timemachine_pixmap->setPixmap(
                                QPixmap("timemachine-off.png"));
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

void ProjectWidget::ContextMenu(
    Q3ListViewItem* item, const QPoint& pt, int col) {
  // do nothing, let derived classes override
}
