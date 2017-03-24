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


#include <set>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qimage.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qtextedit.h>
#include <qmessagebox.h>
#include <qtabwidget.h>

#include <gstLayer.h>
#include <gstAssetGroup.h>
#include <gstRecordJSContext.h>
#include "Preferences.h"
#include "LayerProperties.h"
#include "LocaleDetails.h"
#include "PixmapManager.h"
#include "SiteIcons.h"
#include "ScriptEditor.h"
#include "SearchFieldDialog.h"

static QString kDefaultLocaleName = QObject::tr("Default");

LayerProperties::LayerProperties(QWidget* parent, const LayerConfig& config,
                                 gstLayer* layer)
  : LayerPropertiesBase(parent, 0, false, 0),
    WidgetControllerManager(parent),
    layer_config_(config),
    layer_(layer) {
  idSpinBox->setValue(layer_config_.channelId);

  layer_config_.AssignUuidIfEmpty();

  uuidEdit->setText(layer_config_.asset_uuid_);
  assetNameLabel->setText(shortAssetName(layer_config_.assetRef));
  preserveTextSpin->setValue(layer_config_.preserveTextLevel);
  isVisibleCheck->setChecked(layer_config_.isVisible);

  LocaleDetails::Create(*this,
                        layer_legend,
                        LocaleDetails::LayerMode,
                        &layer_config_.defaultLocale,
                        &layer_config_.locales);

  scriptEdit->setText(layer_config_.layerContextScript);

  // Populate the search fields.
  fields_table->verticalHeader()->hide();
  fields_table->setLeftMargin(0);
  // fields_table->setColumnStretchable(1, true);
  for (uint row = 0; row < config.searchFields.size(); ++row) {
    fields_table->setNumRows(row + 1);
    fields_table->setText(row, 0, config.searchFields[row].name);
    fields_table->setText(row, 1,
        SearchField::UseTypeToString(config.searchFields[row].use_type));
    fields_table->adjustRow(row);
  }
  for (int col = 0; col < fields_table->numCols(); ++col)
    fields_table->adjustColumn(col);

  search_style_edit->setTextFormat(Qt::PlainText);
  search_style_edit->setText(config.searchStyle);
  // hidden for now
  // when we re-enable it here, we must also re-enable it
  // in EmitSearchFileHeader() -- fusion/tools/gevectorpoi.cpp
  search_style_box->hide();

  TextWidgetMetaFieldController<QTextEdit>::Create(*this, notes_edit,
                                                   &layer_config_.meta,
                                                   "notes");
  TimestampInserterController<QTextEdit>::Create(*this,
                                                 timestamp_btn, notes_edit);

  TextWidgetMetaFieldController<QTextEdit>::Create(*this, tc_notes_edit,
                                                   &layer_config_.meta,
                                                   "tc_notes");

  if (!Preferences::ExpertMode) {
    expertBox->hide();
  }
  if (!Preferences::GoogleInternal) {
    isVisibleCheck->hide();

    tab_widget->removePage(transconsole_tab);
    assert(transconsole_tab->parent() != 0);
  }

  SyncToWidgets();
  UpdateButtons();
}

LayerConfig LayerProperties::GetConfig() {
  SyncToConfig();

  layer_config_.channelId = static_cast<uint>(idSpinBox->value());
  layer_config_.asset_uuid_ = uuidEdit->text().ascii();
  layer_config_.preserveTextLevel = static_cast<uint>(
      preserveTextSpin->value());
  layer_config_.isVisible = isVisibleCheck->isChecked();

  layer_config_.layerContextScript = scriptEdit->text();

  layer_config_.searchFields.clear();
  for (int row = 0; row < fields_table->numRows(); ++row) {
    layer_config_.searchFields.push_back(
      SearchField(fields_table->text(row, 0),
                  SearchField::UseTypeFromString(fields_table->text(row, 1))));
  }

  layer_config_.searchStyle = search_style_edit->text();

  return layer_config_;
}

void LayerProperties::editScript() {
  QString script = scriptEdit->text();
  QStringList contextScripts = layer_->GetExternalContextScripts();
  if (ScriptEditor::Run(this,
                        layer_->GetSharedSource(),
                        script, ScriptEditor::StatementBlock,
                        contextScripts)) {
    scriptEdit->setText(script);
  }
}

void LayerProperties::compileAndAccept() {
  QString newScript = scriptEdit->text();
  if (newScript.isEmpty()) {
    accept();
    return;
  }

  gstHeaderHandle recordHeader = layer_->GetSourceAttr();

  QStringList contextScripts = layer_->GetExternalContextScripts();
  QString compilationError;
  if (gstRecordJSContextImpl::CompileCheck(recordHeader,
                                           contextScripts,
                                           newScript, compilationError)) {
    accept();
  } else {
    QMessageBox::critical(this, QObject::tr("JavaScript Error"),
                          QObject::tr("JavaScript Error:\n%1")
                          .arg(compilationError),
                          QObject::tr("OK"), 0, 0, 0);
  }
}

void LayerProperties::AddSearchField() {
  SearchFieldDialog dialog(this, AvailableAttributes());
  if (dialog.exec() == QDialog::Accepted) {
    int row = fields_table->numRows();
    fields_table->setNumRows(row + 1);
    fields_table->setText(row, 0, dialog.GetFieldName());
    fields_table->setText(row, 1, dialog.GetFieldUse());
    fields_table->adjustRow(row);
  }
  for (int col = 0; col < fields_table->numCols(); ++col)
    fields_table->adjustColumn(col);
  UpdateButtons();
}

void LayerProperties::DeleteSearchField() {
  int row = fields_table->currentRow();
  if ( row == -1 )
    return;

  if (QMessageBox::warning(this, "Warning",
                           QObject::trUtf8("Confirm delete.\n\n"),
                           QObject::tr("OK"), QObject::tr("Cancel"),
                           QString::null, 1, 1) == 0)
    fields_table->removeRow(row);
  UpdateButtons();
}

QStringList LayerProperties::AvailableAttributes() {
  QStringList existing_fields;
  for (int row = 0; row < fields_table->numRows(); ++row)
    existing_fields.append(fields_table->text(row, 0));

  QStringList remaining_fields;
  const gstHeaderHandle &record_header = layer_->GetSourceAttr();
  if (record_header && record_header->numColumns() != 0) {
    for (uint col = 0; col < record_header->numColumns(); ++col) {
      if (existing_fields.find(record_header->Name(col)) ==
          existing_fields.end())
        remaining_fields.append(record_header->Name(col));
    }
  }

  return remaining_fields;
}

void LayerProperties::MoveFieldUp() {
  int curr_row = fields_table->currentRow();
  SwapRows(curr_row - 1, curr_row);
  fields_table->selectRow(curr_row - 1);
}

void LayerProperties::MoveFieldDown() {
  int curr_row = fields_table->currentRow();
  SwapRows(curr_row, curr_row + 1);
  fields_table->selectRow(curr_row + 1);
}

void LayerProperties::SwapRows(int row0, int row1) {
  for (int col = 0; col < 2; ++col) {
    QString tmp = fields_table->text(row0, col);
    fields_table->setText(row0, col, fields_table->text(row1, col));
    fields_table->setText(row1, col, tmp);
  }
}

void LayerProperties::UpdateButtons() {
  add_field_btn->setEnabled(!AvailableAttributes().isEmpty());
  int row = fields_table->currentRow();
  delete_field_btn->setEnabled(row != -1);
  move_field_up_btn->setEnabled(row > 0);
  move_field_down_btn->setEnabled(row != -1 &&
                                  row < (fields_table->numRows() - 1));
}
