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


#ifndef _DataViewTable_h_
#define _DataViewTable_h_
#include <Qt/q3table.h>
#include <Qt/q3intdict.h>
#include <gstValue.h>

class gstSelector;

using QTableItem = Q3TableItem;
using QTable = Q3Table;

class DataViewTable : public QTable {
 public:
  DataViewTable(QWidget* parent = 0, const char* name = 0);
  ~DataViewTable();

  virtual void sortColumn(int col, bool ascending, bool wholeRows);
  virtual void columnClicked(int col);

  virtual void resizeData(int sz) {}
  virtual QTableItem* item(int r, int c) const { return NULL; }
  virtual void setItem(int r, int c, QTableItem* i) {}
  virtual void clearCell(int r, int c) {}

  virtual QWidget* createEditor(int r, int col, bool initFromCell) const {
    return 0;
  }
  virtual void clearCellWidget(int r, int c) {}
  virtual void insertWidget(int r, int c, QWidget* w) {}

  virtual QWidget* cellWidget(int r, int c) const { return NULL; }

  virtual void paintCell(QPainter *p, int r, int c,
                         const QRect& cr, bool sel, const QColorGroup& cg);

  void setSelector(gstSelector* sel);

 private:
  int sort_column_;
  bool sort_ascending_;

  gstSelector *selector_;
};

#endif  // !_DataViewTable_h_
