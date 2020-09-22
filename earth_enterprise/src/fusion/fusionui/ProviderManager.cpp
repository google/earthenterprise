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
#include <Qt/q3popupmenu.h>
#include <Qt/qmessagebox.h>
#include <Qt/q3header.h>
#include <notify.h>

#include <autoingest/.idl/gstProvider.h>
#include "khException.h"
#include "ProviderManager.h"
#include "ProviderEdit.h"
using QTableItem = Q3TableItem;
using QTable = Q3Table;
using QPopupMenu = Q3PopupMenu;

// used for the name column, but holds entire copy of the provider
class ProviderNameItem : public QTableItem {
 public:
  ProviderNameItem(QTable* table, const gstProvider &provider) :
      QTableItem(table, QTableItem::Never, provider.name),
      provider_(provider)
  {
  }

  void SetProvider(const gstProvider &provider) {
    provider_ = provider;
    setText(provider_.name);
  }

  gstProvider GetProvider(void) const {
    provider_.name = text();
    return provider_;
  }

 private:
  mutable gstProvider provider_;
};

ProviderManager::ProviderManager(QWidget* parent, bool modal, Qt::WFlags flags)
  : ProviderManagerBase(parent, 0, modal, flags) {
  providerTable->verticalHeader()->hide();
  providerTable->setLeftMargin(0);
  providerTable->setColumnStretchable(0, true);

  providerTable->setNumRows(0);

  if (!provider_set_.Load()) {
    QMessageBox::critical(this, kh::tr("Error"),
                          kh::tr("Unable to load providers\n") +
                          kh::tr("Check console for more information"),
                          kh::tr("OK"), 0, 0, 0);
  }

  for (unsigned int row = 0; row < provider_set_.items.size(); ++row) {
    SetRow(row, provider_set_.items[row]);
  }

  for (int col = 0; col < providerTable->numCols(); ++col)
    providerTable->adjustColumn(col);
}

void ProviderManager::SetRow(int row, const gstProvider& provider) {
  int num_rows = providerTable->numRows();
  if (row > (num_rows - 1))
    providerTable->setNumRows(row + 1);
  providerTable->setItem(row, 0, new ProviderNameItem(providerTable,
                                                      provider));
  providerTable->setText(row, 1, QString::fromUtf8(provider.key.c_str()));
  providerTable->setText(row, 2, provider.copyright);
  providerTable->adjustRow(row);
}

gstProvider ProviderManager::GetProviderFromRow(int row) {
  gstProvider p = static_cast<ProviderNameItem*>(
      providerTable->item(row, 0))->GetProvider();
  p.key = providerTable->text(row, 1).toUtf8().constData();
  p.copyright = providerTable->text(row, 2);
  return p;
}

void ProviderManager::accept() {
  gstProviderSet new_providers;
  (void)new_providers.Load();

  // see if the providers have changed since I loaded them
  // ignore the next_id and only check the actual providers
  new_providers.next_id = provider_set_.next_id;
  if (new_providers != provider_set_) {
    QMessageBox::critical(
        this, kh::tr("Error"),
        kh::tr("The provider list has changed on disk since this window was opened\n") +
        kh::tr("Refusing to save"),
        kh::tr("OK"), 0, 0, 0);
    return;
  } else {
    provider_set_.items.clear();
    for (int row = 0; row < providerTable->numRows(); ++row) {
      provider_set_.items.push_back(GetProviderFromRow(row));
    }
    if (!provider_set_.Save()) {
      QMessageBox::critical(
          this, kh::tr("Error"),
          kh::tr("Unable to save providers.\n"
             "Check console for more information."),
          kh::tr("OK"), 0, 0, 0);
      return;
    }
  }
  ProviderManagerBase::accept();
}

gstProvider ProviderManager::EditProvider(const gstProvider& current_provider) {
  gstProvider new_provider = current_provider;
  ProviderEdit edit(this);

  bool unique = true;
  do {
    unique = true;
    if (edit.configure(new_provider) != QDialog::Accepted)
      return current_provider;

    new_provider = edit.getProvider();
    for (int row = 0; row < providerTable->numRows(); ++row) {
      if (row == providerTable->currentRow())
        continue;
      QString key = providerTable->text(row, 1);
      if (key == QString::fromUtf8(new_provider.key.c_str()) ||
          new_provider.key == "") {
        unique = false;
        break;
      }
    }
    if (!unique) {
      QMessageBox::warning(this, "Error",
        kh::trUtf8("Please provide a unique provider key.\n\n"),
        QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
    }
  } while (!unique);
  return new_provider;
}

void ProviderManager::modifyProvider() {
  int row = providerTable->currentRow();
  if (row == -1)
    return;

  gstProvider current_provider = GetProviderFromRow(row);
  gstProvider new_provider = EditProvider(current_provider);
  if (new_provider != current_provider) {
    if ((new_provider.key != current_provider.key) &&
        (QMessageBox::warning(
            this, "Warning",
            kh::trUtf8("Are you sure you want to change the key for this provider?\n\n") +
            kh::trUtf8("Currently there is no check to confirm that an asset\n") +
            kh::trUtf8("does not reference this provider.  This may cause \n") +
            kh::trUtf8("assets to point to non-existent providers."),
            kh::tr("OK"), kh::tr("Cancel"), QString::null, 1, 1) != 0)) {
      return;
    }
    SetRow(row, new_provider);
    for (int col = 0; col < providerTable->numCols(); ++col)
      providerTable->adjustColumn(col);
  }
}

void ProviderManager::newProvider() {
  gstProvider new_provider = EditProvider(gstProvider());
  if (new_provider != gstProvider()) {
    new_provider.id = provider_set_.GetNextId();
    SetRow(providerTable->numRows(), new_provider);
  }
}

void ProviderManager::deleteProvider() {
  int row = providerTable->currentRow();
  if (row == -1)
    return;

  if (QMessageBox::warning(this, "Warning",
      kh::trUtf8("Are you sure you want to delete this provider?\n\n") +
      kh::trUtf8("Currently there is no check to confirm that an asset\n") +
      kh::trUtf8("does not reference this provider.  This may cause \n") +
      kh::trUtf8("assets to point to non-existent providers."),
      kh::tr("OK"), kh::tr("Cancel"), QString::null, 1, 1) == 0)
    providerTable->removeRow(row);
}

void ProviderManager::contextMenu(int row, int col, const QPoint& pos) {
  enum { NEW_PROVIDER, MODIFY_PROVIDER, DELETE_PROVIDER };

  QPopupMenu menu(this);
  menu.insertItem(kh::tr("&New Provider"), NEW_PROVIDER);
  if (row != -1) {
    menu.insertItem(kh::tr("&Modify Provider"), MODIFY_PROVIDER);
    menu.insertItem(kh::tr("&Delete Provider"), DELETE_PROVIDER);
  }

  switch (menu.exec(pos)) {
    case NEW_PROVIDER:
      newProvider();
      break;
    case MODIFY_PROVIDER:
      modifyProvider();
      break;
    case DELETE_PROVIDER:
      deleteProvider();
      break;
  }
}
