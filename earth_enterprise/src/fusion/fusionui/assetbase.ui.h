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

/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/

#include "AssetNotes.h"

bool AssetBase::Save() {
    return false;
}

void AssetBase::Close() {
}

void AssetBase::EditNotes() {
  AssetNotes asset_notes(this, meta_.GetValue("notes"));
  if (asset_notes.exec() == QDialog::Accepted) {
      meta_.SetValue("notes", asset_notes.GetText());
  }
}


AssetDefs::Type AssetBase::AssetType() const {
    return AssetDefs::Invalid;
}

std::string AssetBase::AssetSubtype() const {
    return std::string("");
}

QWidget* AssetBase::BuildMainWidget(QWidget*) {
  return NULL;
}

void AssetBase::InstallMainWidget() {
  QWidget* main = BuildMainWidget(main_frame);
  main_frameLayout->addWidget(main, 0, 0);
}

void AssetBase::closeEvent( QCloseEvent* event) {
  event->ignore();
  Close();
}

void AssetBase::SetName(const QString& text) {
    asset_name_label->setText(text);
}

QString AssetBase::Name() {
  return asset_name_label->text();
}


bool AssetBase::IsModified() {
  return false;
}


std::string AssetBase::AssetPrettyName() {
  return std::string("");
}
