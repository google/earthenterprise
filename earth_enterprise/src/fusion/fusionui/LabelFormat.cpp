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


#include <qstringlist.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qtextedit.h>
#include <qpushbutton.h>
#include <gstRecord.h>

#include "LabelFormat.h"

// create a static string to facilitate comparison
// when the combobox is selected
static QString InsertFieldTxt(QObject::tr("Insert Field"));

LabelFormat::LabelFormat(QWidget* parent, const gstHeaderHandle &record_header,
                         const QString& txt, LineMode mode)
    : LabelFormatBase(parent, 0, false, 0), mode_(mode) {

  if (record_header) {
    if (record_header->numColumns() != 0) {
      insert_field_combo->insertItem(InsertFieldTxt);
      for (unsigned int col = 0; col < record_header->numColumns(); ++col)
        insert_field_combo->insertItem(record_header->Name(col));
    } else {
      insert_field_combo->setEnabled(false);
    }
  }

  if (mode_ == SingleLine) {
    auto_populate_btn->hide();
    textEdit->hide();
    lineEdit->setText(txt);
    lineEdit->show();
    layout()->activate();
    setFixedSize(minimumSizeHint());
  } else {
    auto_populate_btn->show();
    lineEdit->hide();
    textEdit->setText(txt);
    textEdit->show();
  }
}

void LabelFormat::insertField(const QString &str) {
  if (str == InsertFieldTxt)
    return;

  insert_field_combo->blockSignals(true);
  insert_field_combo->setCurrentItem(0);
  insert_field_combo->blockSignals(false);

  if (mode_ == SingleLine) {
    lineEdit->blockSignals(true);
    lineEdit->insert(QString("«%1»").arg(str));
    lineEdit->setFocus();
    lineEdit->blockSignals(false);
  } else {
    textEdit->blockSignals(true);
    textEdit->insert(QString("«%1»").arg(str));
    textEdit->setFocus();
    textEdit->blockSignals(false);
  }
}

void LabelFormat::AutoPopulate() {
  textEdit->blockSignals(true);
  // skip first line which is InsertFieldTxt
  for (int item = 1; item < insert_field_combo->count(); ++item) {
    QString field = insert_field_combo->text(item);
    textEdit->insert(QString("%1: \xAB%2\xBB<br>\n").arg(field).arg(field));
  }
  textEdit->blockSignals(false);
}

void LabelFormat::accept() {
  if (mode_ == SingleLine) {
    label_text_ = lineEdit->text();
  } else {
    label_text_ = textEdit->text();
  }

  LabelFormatBase::accept();
}
