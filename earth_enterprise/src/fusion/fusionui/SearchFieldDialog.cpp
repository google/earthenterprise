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


#include <qcombobox.h>
#include <qmessagebox.h>

#include "SearchFieldDialog.h"
#include <autoingest/.idl/storage/SearchField.h>

// create a static string to facilitate comparison
// when the combobox is selected
static QString InsertFieldTxt(QObject::tr("Insert Field"));

SearchFieldDialog::SearchFieldDialog(QWidget* parent,
                                     const QStringList available_fields)
  : SearchFieldDialogBase(parent, 0, false, 0) {
  field_use_combo->insertItem(
      SearchField::UseTypeToString(SearchField::SEARCH_ONLY));

  field_use_combo->insertItem(
      SearchField::UseTypeToString(SearchField::DISPLAY_ONLY));
  field_use_combo->insertItem(
      SearchField::UseTypeToString(SearchField::SEARCH_DISPLAY));
  field_use_combo->setCurrentItem(2);

  for (QStringList::const_iterator it = available_fields.begin();
       it != available_fields.end(); ++it) {
    field_name_combo->insertItem(*it);
  }
}

QString SearchFieldDialog::GetFieldName() {
  return field_name_combo->currentText();
}

QString SearchFieldDialog::GetFieldUse() {
  return field_use_combo->currentText();
}
