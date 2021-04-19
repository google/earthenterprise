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

#include <Qt/q3dragobject.h>
#include <assert.h>
#include <QtGui/QMouseEvent>
#include "AssetTableView.h"
#include "AssetDrag.h"
#include "AssetManager.h"
#include "AssetDisplayHelper.h"
#include "Preferences.h"
#include <Qt/q3mimefactory.h>
#include <Qt/qpalette.h>

using QMimeSourceFactory = Q3MimeSourceFactory;
using QImageDrag = Q3ImageDrag;
using QTableSelection = Q3TableSelection;
// -----------------------------------------------------------------------------

static QPixmap uic_load_pixmap(const QString& name) {
  const QMimeSource* m = QMimeSourceFactory::defaultFactory()->data(name);
  if (!m)
    return QPixmap();
  QPixmap pix;
  QImageDrag::decode(m, pix);
  return pix;
}

// -----------------------------------------------------------------------------

AssetTableItem::AssetTableItem(QTable* table, gstAssetHandle handle)
    : QTableItem(table, QTableItem::Never, QString::null),
      asset_handle_(handle) {
  Asset asset = handle->getAsset();
  AssetDisplayHelper a(asset->type, asset->PrettySubtype());
  setPixmap(a.GetPixmap());
  std::string san = shortAssetName(handle->getName());
  setText(san.c_str());
}

AssetTableItem::~AssetTableItem() {
}

gstAssetHandle AssetTableItem::GetAssetHandle() const {
  return asset_handle_;
}

void AssetTableItem::paint(QPainter* p, const QColorGroup& cg, const QRect& cr,
                           bool sel) {
  std::string txt(text().toUtf8().constData());
  QColorGroup ncg = AssetManager::GetStateDrawStyle(txt, p, cg);
  QTableItem::paint(p, ncg, cr, sel);
}

// -----------------------------------------------------------------------------

AssetStateItem::AssetStateItem(QTable* table, QString state)
    : QTableItem(table, QTableItem::Never, state) {
}

void AssetStateItem::paint(QPainter* p, const QColorGroup& cg, const QRect& cr,
                           bool sel) {
  std::string txt(text().toUtf8().constData());
  QColorGroup ncg = AssetManager::GetStateDrawStyle(txt, p, cg);
  QTableItem::paint(p, ncg, cr, sel);
}

// -----------------------------------------------------------------------------

AssetTableView::AssetTableView(QWidget* parent, const char* name)
    : QTable(parent, name),
      sort_column_(1),
      sort_ascending_(true) {
  setNumRows(0);
  setNumCols(0);
  setShowGrid(false);
  setReadOnly(true);
  setSorting(false);
  setSelectionMode(QTable::SingleRow);
  setDragEnabled(true);

  // enable multi-select
  setSelectionMode(QTable::MultiRow);

  setLeftMargin(0);
  verticalHeader()->hide();

  setColumnMovingEnabled(false);
  setRowMovingEnabled(false);
}

AssetTableView::~AssetTableView() {
}

AssetTableItem* AssetTableView::GetItem(int row) const {
  return static_cast<AssetTableItem*>(item(row, 0));
}

gstAssetHandle AssetTableView::GetAssetHandle(int row) const {
  AssetTableItem* assetItem = GetItem(row);

  if (assetItem == NULL) {
    return gstAssetHandleImpl::Create(gstAssetFolder(QString::null),
                                      QString::null);
  }

  return assetItem->GetAssetHandle();
}


void AssetTableView::contentsMousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton)
    drag_start_point_ = event->pos();
  QTable::contentsMousePressEvent(event);
}


void AssetTableView::contentsMouseMoveEvent(QMouseEvent* event) {
  if ((event->state() & Qt::LeftButton) && !drag_start_point_.isNull()) {
    // make sure drag has moved more than 3 pixels so just clicking doesn't
    // accidentally start a drag
    if (QABS(event->pos().x() - drag_start_point_.x()) >= 3 ||
        QABS(event->pos().y() - drag_start_point_.y()) >= 3) {
      AssetTableItem* item = GetItem(currentRow());
      if (item == NULL)
        return;

      AssetDrag* drag_object = new AssetDrag(this, item->GetAssetHandle()->getAsset());
      QString asset_group;
      for (int s = 0; s < numSelections(); ++s) {
        QTableSelection sel = selection(s);
        for (int row = sel.topRow(); row <= sel.bottomRow(); ++row) {
          AssetTableItem* item = GetItem(row);
          assert(item != NULL);
          if (!asset_group.isEmpty())
            asset_group += QChar(127);  // special character used as a separator
          asset_group += item->GetAssetHandle()->getAsset()->GetRef().toString().c_str();
        }
      }
      drag_object->setText(asset_group);
      drag_object->setPixmap(item->pixmap());
      drag_start_point_ = QPoint();
      drag_object->dragCopy();
    }
  } else {
    QTable::contentsMouseMoveEvent(event);
  }
}

void AssetTableView::columnClicked(int col) {
  if (col == sort_column_) {
    sort_ascending_ = !sort_ascending_;
  } else {
    sort_ascending_ = true;
    sort_column_ = col;
  }

  horizontalHeader()->setSortIndicator(col, sort_ascending_);
  sortColumn(sort_column_, sort_ascending_, true);
}
