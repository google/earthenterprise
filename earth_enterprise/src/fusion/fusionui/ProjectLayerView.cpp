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
#include <Qt/q3header.h>
#include <Qt/qstringlist.h>
#include <Qt/qobject.h>
#include "ProjectLayerView.h"
#include "Preferences.h"
using QListView = Q3ListView;


ProjectLayerView::ProjectLayerView(QWidget* parent, const char* n, Qt::WFlags f)
    : QListView(parent, n, f) {
  header()->setStretchEnabled(true);
  header()->hide();
  addColumn(QObject::trUtf8("Name"));
  // disable sorting.  the item positions will be managed directly
  setSorting(-1);
}

void ProjectLayerView::EnableAssetDrops(AssetDefs::Type type,
                                        const std::string& subtype) {
  drag_asset_type_ = type;
  drag_asset_subtype_ = subtype;
  setAcceptDrops(true);
  viewport()->setAcceptDrops(true);
}

void ProjectLayerView::SelectItem(Q3ListViewItem* item) {
  Q3ListViewItem* child = firstChild();
  while (child) {
    setSelected(child, false);
    child = child->nextSibling();
  }
  setCurrentItem(item);
  setSelected(item, true);
}

void ProjectLayerView::contentsDragMoveEvent(QDragMoveEvent* e) {
  // QScrollView::contentsDragMoveEvent seems to be more reliable than
  // QScrollView::contentsDragEnterEvent so use it even though it will
  // continue to be called as the drag is moved around the widget
  e->accept(AssetDrag::canDecode(dynamic_cast<QMimeSource*>(e), drag_asset_type_, drag_asset_subtype_));
}

void ProjectLayerView::contentsDropEvent(QDropEvent* e) {
  if (AssetDrag::canDecode(e, drag_asset_type_, drag_asset_subtype_)) {
    QString text;
    AssetDrag::decode(e, text);

    QStringList assets = QStringList::split(QChar(127), text);
    for (QStringList::Iterator it = assets.begin(); it != assets.end(); ++it) {
      emit dropAsset(*it);
    }
  }
}
