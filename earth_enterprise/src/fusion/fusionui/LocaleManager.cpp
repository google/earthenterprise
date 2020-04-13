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

#include <Qt/q3table.h>
using QTable = Q3Table;
#include <Qt/qmessagebox.h>
#include <Qt/qinputdialog.h>
#include <notify.h>

#include "LocaleManager.h"

LocaleManager::LocaleManager(QWidget* parent, bool modal, Qt::WFlags flags)
  : LocaleManagerBase(parent, 0, modal, flags) {
  locale_table->verticalHeader()->hide();
  locale_table->setLeftMargin(0);
  locale_table->setNumRows(0);
  locale_table->setColumnStretchable(0, true);

  localeset_.Load();
  for (unsigned int row = 0; row < localeset_.supportedLocales.size(); ++row) {
    locale_table->setNumRows(row + 1);
    locale_table->setText(row, 0, localeset_.supportedLocales[row]);
    locale_table->adjustRow(row);
  }
  locale_table->adjustColumn(0);
}

std::vector<QString> LocaleManager::SupportedLocales() {
  LocaleSet locale_set;
  locale_set.Load();
  return locale_set.supportedLocales;
}

void LocaleManager::accept() {
  localeset_.supportedLocales.clear();

  for (int row = 0; row < locale_table->numRows(); ++row) {
    localeset_.supportedLocales.push_back(locale_table->text(row, 0));
  }

  if (!localeset_.Save()) {
    QMessageBox::critical(
        dynamic_cast<QWidget*>(this), tr("Error"),
        tr("Unable to save locales.\n"
           "Check console for more information."),
        tr("OK"), QString(), QString(),
        0, 0);
  } else {
    LocaleManagerBase::accept();
  }
}

bool LocaleManager::LocaleExists(const QString& text) {
  for (int row = 0; row < locale_table->numRows(); ++row) {
    if (text == locale_table->text(row, 0))
      return true;
  }
  return false;
}

void LocaleManager::NewLocale() {
  QString new_locale;
  while (true) {
    bool ok;
    new_locale = QInputDialog::getText(
                          tr("Support New Locale"),
                          tr("Locale name (eg. fr, de, it, fr_CA):"),
                          QLineEdit::Normal,
                          new_locale, &ok, this);
    new_locale = new_locale.stripWhiteSpace();
    if (!ok || new_locale.isEmpty())
      return;

    if (LocaleExists(new_locale)) {
      QMessageBox::warning(this, "Error",
                           trUtf8("Please provide a unique locale.\n\n"),
                           QMessageBox::Ok, QMessageBox::NoButton,
                           QMessageBox::NoButton);
    } else {
      break;
    }
  }

  int row = locale_table->numRows();
  locale_table->setNumRows(row + 1);
  locale_table->setText(row, 0, new_locale);
  locale_table->adjustRow(row);
}

void LocaleManager::ModifyLocale() {
  int row = locale_table->currentRow();
  if (row == -1)
    return;

  QString orig_locale = locale_table->text(row, 0);
  QString new_locale = orig_locale;
  while (true) {
    bool ok;
    new_locale = QInputDialog::getText(
                          tr("Modify Existing Locale"),
                          tr("Locale name:"),
                          QLineEdit::Normal,
                          new_locale, &ok, this);
    new_locale = new_locale.stripWhiteSpace();

    if (!ok || new_locale.isEmpty() || new_locale == orig_locale)
      return;

    if (LocaleExists(new_locale)) {
      QMessageBox::warning(this, "Error",
                           trUtf8("Please provide a unique locale.\n\n"),
                           QMessageBox::Ok, QMessageBox::NoButton,
                           QMessageBox::NoButton);
    } else {
      break;
    }
  }

  if (QMessageBox::warning(
          this, "Warning",
          tr("Are you sure you want to change this locale from \"%1\" to \"%2\"?\n\n"
             "Currently there is no check to confirm that this locale"
             " is currently not in use.").arg(orig_locale).arg(new_locale),
          tr("OK"), tr("Cancel"), QString::null, 1, 1) != 0) {
    return;
  } else {
    locale_table->setText(row, 0, new_locale);
    locale_table->adjustRow(row);
  }
}

void LocaleManager::DeleteLocale() {
  int row = locale_table->currentRow();
  if (row == -1)
    return;

  if (QMessageBox::warning(this, "Warning",
         trUtf8("Are you sure you want to delete this locale?\n\n"
                "Currently there is no check to confirm that a layer\n"
                "does not reference this Locale.  This may cause \n"
                "layers to contain non-existent locales."),
         tr("OK"), tr("Cancel"), QString::null, 1, 1) == 0)
    locale_table->removeRow(row);
}

