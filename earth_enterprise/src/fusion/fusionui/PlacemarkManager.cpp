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


#include <stdlib.h>
#include <Qt/qglobal.h>
#include <Qt/q3table.h>
#include <Qt/qpushbutton.h>
#include <Qt/qmessagebox.h>

#include <khFileUtils.h>
#include "khException.h"
#include "Preferences.h"
#include "PlacemarkManager.h"
#include "PlacemarkEdit.h"

PlacemarkManager::PlacemarkManager()
  : PlacemarkManagerBase(0, 0, false, 0) {
  placemark_table->verticalHeader()->hide();
  placemark_table->setLeftMargin(0);
  placemark_table->setColumnStretchable(0, true);

  if (khExists(Preferences::filepath("placemarks.xml").toUtf8().constData()))
    placemarks_.Load(Preferences::filepath("placemarks.xml").toUtf8().constData());
}

void PlacemarkManager::accept() {
  Save();
  PlacemarkManagerBase::accept();
}

void PlacemarkManager::Save() {
  if (!placemarks_.Save(Preferences::filepath("placemarks.xml").toUtf8().constData())) {
    QMessageBox::critical(
        this, kh::tr("Error"),
        kh::tr("Unable to save Favorites") +
        kh::tr("Check console for more information"),
        kh::tr("OK"), 0, 0, 0);
  }
}

void PlacemarkManager::reject() {
  placemarks_ = restore_placemarks_;
  PlacemarkManagerBase::reject();
}

QStringList PlacemarkManager::GetList() {
  QStringList list;
  std::vector<gstPlacemark>::iterator it = placemarks_.items.begin();
  std::vector<gstPlacemark>::iterator end = placemarks_.items.end();
  for (; it != end; ++it)
    list << it->name;
  return list;
}

void PlacemarkManager::Add(const gstPlacemark& pm) {
  placemarks_.items.push_back(pm);
  Save();
}

gstPlacemark PlacemarkManager::Get(int idx) const {
  return placemarks_.items[idx];
}

int PlacemarkManager::Manage() {
  restore_placemarks_ = placemarks_;

  placemark_table->setNumRows(0);

  std::vector<gstPlacemark>::iterator it = placemarks_.items.begin();
  std::vector<gstPlacemark>::iterator end = placemarks_.items.end();
  for ( int row = 0; it != end; ++it, ++row ) {
    placemark_table->setNumRows(row + 1);
    SetRow(row, *it);
  }

  for (int col = 0; col < placemark_table->numCols(); ++col)
    placemark_table->adjustColumn(col);

  if (placemark_table->numRows() != 0)
    placemark_table->selectRow(0);

  return PlacemarkManagerBase::exec();
}

void PlacemarkManager::SetRow(int row, const gstPlacemark& pm) {
  placemark_table->setText(row, 0, pm.name);
  placemark_table->setText(row, 1, QString("%1").arg(pm.latitude, 0, 'f', 8));
  placemark_table->setText(row, 2, QString("%1").arg(pm.longitude, 0, 'f', 8));
  placemark_table->setText(row, 3, QString("%1").arg(pm.level));
  placemark_table->adjustRow(row);
}

void PlacemarkManager::deletePlacemark() {
  int row = placemark_table->currentRow();
  if (row == -1)
    return;

  // get an iterator for current row
  std::vector<gstPlacemark>::iterator it = placemarks_.items.begin();
  for (int pos = 0; pos != row; ++pos, ++it)
    ;
  placemarks_.items.erase(it);

  for (int swap = row; swap != placemark_table->numRows() - 1; ++swap)
    placemark_table->swapRows(swap, swap + 1);

  placemark_table->setNumRows(placemark_table->numRows() - 1);

  if (placemark_table->numRows() == 0) {
    selectionChanged(-1, 0);
  } else {
    placemark_table->selectRow(row);
  }
}

void PlacemarkManager::modifyPlacemark() {
  int row = placemark_table->currentRow();
  if (row == -1)
    return;

  PlacemarkEdit edit(this, placemarks_.items[row]);

  if (edit.exec() != QDialog::Accepted)
    return;

  gstPlacemark pm = edit.getPlacemark();

  placemarks_.items[row] = pm;

  SetRow(row, pm);
}

void PlacemarkManager::moveDown() {
  int row = placemark_table->currentRow();
  if (row == -1 || row == placemark_table->numRows() - 1)
    return;

  gstPlacemark tmp = placemarks_.items[row];
  placemarks_.items[row] = placemarks_.items[row + 1];
  placemarks_.items[row + 1] = tmp;

  SetRow(row, placemarks_.items[row]);
  SetRow(row + 1, placemarks_.items[row + 1]);

  placemark_table->selectRow(row + 1);
}

void PlacemarkManager::moveUp() {
  int row = placemark_table->currentRow();
  if (row == -1 || row == 0)
    return;

  gstPlacemark tmp = placemarks_.items[row];
  placemarks_.items[row] = placemarks_.items[row - 1];
  placemarks_.items[row - 1] = tmp;

  SetRow(row, placemarks_.items[row]);
  SetRow(row - 1, placemarks_.items[row - 1]);

  placemark_table->selectRow(row - 1);
}

void PlacemarkManager::newPlacemark() {
  PlacemarkEdit edit(this, gstPlacemark());

  if (edit.exec() != QDialog::Accepted)
    return;

  gstPlacemark pm = edit.getPlacemark();
  Add(pm);

  int row = placemark_table->numRows();
  placemark_table->setNumRows(row + 1);
  SetRow(row, pm);
  placemark_table->selectRow(row);
}

void PlacemarkManager::selectionChanged(int row, int) {
  // none selected
  if (row == -1) {
    delete_btn->setEnabled(false);
    move_up_btn->setEnabled(false);
    move_down_btn->setEnabled(false);
  } else {
    delete_btn->setEnabled(true);
    move_up_btn->setEnabled(row != 0);
    move_down_btn->setEnabled(row != placemark_table->numRows() - 1);
    placemark_table->selectRow(row);
  }
}
