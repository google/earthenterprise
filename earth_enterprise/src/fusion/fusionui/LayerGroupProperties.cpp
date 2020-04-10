// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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
#include <Qt/qobjectdefs.h>
#include <Qt/qlineedit.h>
#include <Qt/q3combobox.h>
#include <Qt/qspinbox.h>
#include <Qt/qlabel.h>
#include <Qt/qpixmap.h>
#include <Qt/qpushbutton.h>
#include <Qt/qimage.h>
#include <Qt/qcheckbox.h>
#include <Qt/qgroupbox.h>
#include <Qt/qtabwidget.h>
#include <Qt/q3textedit.h>
#include <gstLayer.h>

#include "Preferences.h"
#include "LayerGroupProperties.h"
#include "PixmapManager.h"
#include "SiteIcons.h"
#include "LocaleDetails.h"

using QTextEdit = Q3TextEdit;


static QString kDefaultLocaleName = QObject::tr("Default");

LayerGroupProperties::LayerGroupProperties(
    QWidget* parent, const LayerConfig& config)
    : LayerGroupPropertiesBase(parent, 0, false, 0),
      WidgetControllerManager(parent),
      layer_config_(config) {

  idSpinBox->setValue(layer_config_.channelId);
  layer_config_.AssignUuidIfEmpty();
  uuidEdit->setText(layer_config_.asset_uuid_.c_str());

  isVisibleCheck->setChecked(layer_config_.isVisible);
  isExpandableCheck->setChecked(layer_config_.isExpandable);
  isExpandableCheck->setEnabled(layer_config_.isVisible);

  LocaleDetails::Create(*this, layer_legend,
                        LocaleDetails::FolderMode,
                        &layer_config_.defaultLocale,
                        &layer_config_.locales);

  TextWidgetMetaFieldController<QTextEdit>::Create(*this, notes_edit,
                                                   &layer_config_.meta,
                                                   "notes");
  TimestampInserterController<QTextEdit>::Create(*this, timestamp_btn,
                                                 notes_edit);

  TextWidgetMetaFieldController<QTextEdit>::Create(*this, tc_notes_edit,
                                                   &layer_config_.meta,
                                                   "tc_notes");

  if (!Preferences::ExpertMode) {
    expertBox->hide();
    tab_widget->removePage(advanced_tab);
    assert(advanced_tab->parent() != 0);
  }

  if (!Preferences::GoogleInternal) {
    isVisibleCheck->hide();

    tab_widget->removePage(transconsole_tab);
    assert(transconsole_tab->parent() != 0);
  }

  SyncToWidgets();
}

LayerConfig LayerGroupProperties::GetConfig() {
  SyncToConfig();

  layer_config_.isVisible = isVisibleCheck->isChecked();
  layer_config_.isExpandable = isExpandableCheck->isEnabled() &&
                               isExpandableCheck->isChecked();
  layer_config_.channelId = static_cast< unsigned int> (idSpinBox->value());
  layer_config_.asset_uuid_ = uuidEdit->text().ascii();

  return layer_config_;
}

void LayerGroupProperties::toggleIsVisible(bool state) {
  isExpandableCheck->setEnabled(state);
}
