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


#ifndef KHSRC_FUSION_FUSIONUI_ASSETTABLEVIEW_H__
#define KHSRC_FUSION_FUSIONUI_ASSETTABLEVIEW_H__

#include <Qt/q3table.h>
#include <Qt/qtableview.h>
#include <Qt/qpoint.h>
#include <gstAssetGroup.h>

class QDragObject;

// -----------------------------------------------------------------------------
using QTableItem = Q3TableItem;
using QTable = Q3Table;

class AssetTableItem : public QTableItem {
 public:
  AssetTableItem(QTable* table, gstAssetHandle);
  ~AssetTableItem();

  gstAssetHandle GetAssetHandle() const;

  // from QTableItem
  virtual void paint(QPainter* p, const QColorGroup& cg, const QRect& cr,
                     bool sel);

 private:
  gstAssetHandle asset_handle_;
};

// -----------------------------------------------------------------------------

class AssetStateItem : public QTableItem {
 public:
  AssetStateItem(QTable* table, QString state);

  // from QTableItem
  virtual void paint(QPainter* p, const QColorGroup& cg, const QRect& cr,
                     bool sel);
};

// -----------------------------------------------------------------------------

class AssetTableView : public QTable {
 public:
  AssetTableView(QWidget* parent = 0, const char* name = 0);
  ~AssetTableView();

  gstAssetHandle GetAssetHandle(int row) const;
  AssetTableItem* GetItem(int row) const;

 private:
  // from QTable
  virtual void contentsMousePressEvent(QMouseEvent* e);
  virtual void contentsMouseMoveEvent(QMouseEvent* e);
  virtual void columnClicked(int col);
  
  QPoint drag_start_point_;
  int sort_column_;
  bool sort_ascending_;
};

#endif  // !KHSRC_FUSION_FUSIONUI_ASSETTABLEVIEW_H__
