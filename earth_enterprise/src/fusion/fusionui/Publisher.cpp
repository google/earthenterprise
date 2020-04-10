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


#include "fusion/fusionui/Publisher.h"

#include <assert.h>

#include <Qt/q3table.h>
#include <Qt/qmessagebox.h>
#include <Qt/q3header.h>
#include <Qt/qpushbutton.h>
#include <Qt/qurl.h>
#include "khException.h"
#include "fusion/fusionui/ServerCombinationEdit.h"

Publisher::Publisher(QWidget* parent, bool modal, Qt::WFlags flags)
  : PublisherBase(parent, 0, modal, flags) {
  server_combination_table->verticalHeader()->hide();
  server_combination_table->setLeftMargin(0);
  server_combination_table->setColumnStretchable(0, true);

  server_combination_table->setNumRows(0);

  if (!server_combination_set_.Load()) {
    QMessageBox::critical(this, kh::tr("Error"),
                          kh::tr("Unable to load existing Server Associations\n") +
                          kh::tr("Check console for more information"),
                          kh::tr("OK"), 0, 0, 0);
  }

  for (unsigned int row = 0; row < server_combination_set_.combinations.size(); ++row) {
    SetRow(row, server_combination_set_.combinations[row]);
  }

  for (int col = 0; col < server_combination_table->numCols(); ++col)
    server_combination_table->adjustColumn(col);

  if (server_combination_table->numRows() != 0)
    server_combination_table->selectRow(0);
}

void Publisher::SetRow(int row, const ServerCombination& c) {
  int num_rows = server_combination_table->numRows();
  if (row > (num_rows - 1))
    server_combination_table->setNumRows(row + 1);
  server_combination_table->setText(row, 0, c.nickname);
  server_combination_table->setText(row, 1, c.stream.url.c_str());
  QUrl url(c.stream.url.c_str());
  bool is_https = (url.protocol() == QString("https"));
  // Display SSL options only for URLs with https-scheme.
  if (is_https) {
    server_combination_table->setText(row, 2, c.stream.cacert_ssl.c_str());
    server_combination_table->setText(
        row, 3, c.stream.insecure_ssl ? "True" : "False");
  } else {
    assert(c.stream.cacert_ssl.empty());
    assert(!c.stream.insecure_ssl);
  }
  server_combination_table->adjustRow(row);
}

ServerCombination Publisher::GetCombinationFromRow(int row) {
  ServerCombination c;
  c.nickname = server_combination_table->text(row, 0);
  c.stream.url = server_combination_table->text(row, 1).toUtf8().constData();
  c.stream.cacert_ssl = server_combination_table->text(row, 2).toUtf8().constData();
  c.stream.insecure_ssl = (server_combination_table->text(row, 3) == "True");
  return c;
}

ServerCombination Publisher::EditCombination(
    const ServerCombination& current_combination) {
  bool unique = true;
  ServerCombination new_combination = current_combination;
  do {
    unique = true;
    ServerCombinationEdit edit(this, new_combination);
    if (edit.exec() != QDialog::Accepted)
      return current_combination;

    new_combination = edit.GetCombination();
    for (int row = 0; row < server_combination_table->numRows(); ++row) {
      // If we are in the "Edit" mode then we don't want to check the nickname
      // against the same entry in the table.
      if (!current_combination.nickname.isEmpty() &&
          row == server_combination_table->currentRow())
        continue;
      QString nickname = server_combination_table->text(row, 0);
      if (nickname == new_combination.nickname ||
          new_combination.nickname.isEmpty()) {
        unique = false;
        break;
      }
    }
    if (!unique) {
      QMessageBox::warning(this, "Error",
        trUtf8("Please provide a unique nickname.\n\n"),
        QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
    }
  } while (!unique);
  return new_combination;
}

void Publisher::EditCombination() {
  int row = server_combination_table->currentRow();
  if (row == -1)
    return;

  ServerCombination current_combination = GetCombinationFromRow(row);
  ServerCombination new_combination = EditCombination(current_combination);
  if (new_combination != current_combination)
    SetRow(row, new_combination);
}

void Publisher::AddCombination() {
  ServerCombination c;
  ServerCombination new_combination = EditCombination(c);
  if (new_combination != c) {
    SetRow(server_combination_table->numRows(), new_combination);
  }
}

void Publisher::DeleteCombination() {
  int row = server_combination_table->currentRow();
  if (row == -1)
    return;

  if (QMessageBox::warning(this, "Confirm Delete",
      trUtf8("Confirm Delete."),
      kh::tr("OK"), kh::tr("Cancel"), QString::null, 1, 1) == 0)
    server_combination_table->removeRow(row);
}

void Publisher::MoveDown() {
  int row = server_combination_table->currentRow();
  if (row == -1 || row == server_combination_table->numRows() - 1)
    return;

  SwapRows(row, row + 1);

  server_combination_table->selectRow(row + 1);
}

void Publisher::MoveUp() {
  int row = server_combination_table->currentRow();
  if (row == -1 || row == 0)
    return;

  SwapRows(row - 1, row);

  server_combination_table->selectRow(row - 1);
}

void Publisher::SwapRows(int a, int b) {
  ServerCombination tmp = GetCombinationFromRow(a);
  SetRow(a, GetCombinationFromRow(b));
  SetRow(b, tmp);
}

void Publisher::CurrentChanged(int row, int col) {
  // none selected
  if (row == -1) {
    delete_btn->setEnabled(false);
    move_up_btn->setEnabled(false);
    move_down_btn->setEnabled(false);
  } else {
    delete_btn->setEnabled(true);
    move_up_btn->setEnabled(row != 0);
    move_down_btn->setEnabled(row != server_combination_table->numRows() - 1);
    server_combination_table->selectRow(row);
  }
}

void Publisher::accept() {
  server_combination_set_.combinations.clear();
  for (int row = 0; row < server_combination_table->numRows(); row++) {
    server_combination_set_.combinations.push_back(GetCombinationFromRow(row));
  }

  if (!server_combination_set_.Save()) {
    QMessageBox::critical(
        this, kh::tr("Error"),
        kh::tr("Unable to save associations") +
        kh::tr("Check console for more information"),
        kh::tr("OK"), 0, 0, 0);
  }
  PublisherBase::accept();
}
