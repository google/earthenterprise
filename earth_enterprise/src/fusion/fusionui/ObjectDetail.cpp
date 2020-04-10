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


#include "fusion/fusionui/ObjectDetail.h"
#include "khException.h"
#include <Qt/qlabel.h>
#include <Qt/q3table.h>
#include <Qt/q3header.h>
#include <Qt/qlistview.h>
#include <Qt/q3listview.h>
#include "fusion/fusionui/Preferences.h"

#include "fusion/fusionui/GfxView.h"
#include "fusion/gst/gstFeature.h"

#include "fusion/gst/gstTypes.h"
using QListViewItem = Q3ListViewItem;
namespace {

template<typename ParentView>
void FillInGeodeData(ParentView *parent_view, const gstGeode *geode) {
  QListViewItem* lastPartItem = 0;
  for (unsigned int part = 0; part < geode->NumParts(); ++part) {
    QListViewItem* partItem =
        new QListViewItem(parent_view, lastPartItem,
                          QString("Part %1 (%2)").arg(part).
                          arg(geode->VertexCount(part)));
    lastPartItem = partItem;

    ObjectDetail::VertexItem* lastVertItem = 0;
    for (unsigned int v = 0; v < geode->VertexCount(part); ++v) {
      gstVertex vert = geode->GetVertex(part, v);
      ObjectDetail::VertexItem* vertItem =
          new ObjectDetail::VertexItem(partItem, lastVertItem,
                                       QString::number(v), vert);
      lastVertItem = vertItem;
    }
  }
}

}  // namespace

// -----------------------------------------------------------------------------

ObjectDetail::VertexItem::VertexItem(QListViewItem* parent,
                                     QListViewItem* after,
                                     const QString& l,
                                     const gstVertex& v)
    : QListViewItem(parent, after, l), vertex(v) {
  setText(1, QString("%1, %2, %3").
          arg(v.y * 360 - 180, 0, 'f', 10).
          arg(v.x * 360 - 180, 0, 'f', 10).
          arg(v.z));
}

// -----------------------------------------------------------------------------

ObjectDetail::ObjectDetail(QWidget* parent, unsigned int id, gstGeodeHandle geode,
                           gstRecordHandle attrib)
    : ObjectDetailBase(parent , 0, false, static_cast<Qt::WindowFlags>(Qt::WA_DeleteOnClose)),
      geode_handle_(geode),
      drawVertex(false) {
  setCaption(QString("Feature %1 Detail").arg(id));
  featureTypeLabel->setText(PrettyPrimType(geode_handle_->PrimType()).c_str());
  featurePartsLabel->setText(QString::number(geode_handle_->NumParts()));
  featureVertexesLabel->setText(
      QString::number(geode_handle_->TotalVertexCount()));

  //
  // fill in attributes
  //
  attributeTable->setSorting(false);    // disable sorting
  if (attrib && !attrib->IsEmpty()) {
    attributeTable->setNumRows(attrib->NumFields());
    for (unsigned int row = 0; row < attrib->NumFields(); ++row) {
      attributeTable->setText(row, 0, attrib->Header()->Name(row));
      attributeTable->setText(row, 1, attrib->Field(row)->ValueAsUnicode());
      attributeTable->adjustRow(row);
    }
    attributeTable->adjustColumn(0);
    attributeTable->adjustColumn(1);
  } else {
    attributeTable->setNumRows(0);
  }

  //
  // fill in vertex data
  //
  vertexListView->setSorting(-1);       // disable sorting
  if (Preferences::GlobalEnableAll)
    vertexListView->header()->setLabel(1, kh::tr("X, Y"));

  if (geode_handle_->PrimType() == gstMultiPolygon ||
      geode_handle_->PrimType() == gstMultiPolygon25D ||
      geode_handle_->PrimType() == gstMultiPolygon3D) {
    QListViewItem* lastMultiPartItem = 0;

    const gstGeodeCollection *multi_geode =
        static_cast<const gstGeodeCollection*>(&(*geode_handle_));
    for (unsigned int pp = 0; pp < geode_handle_->NumParts(); ++pp) {
      const gstGeode *geode =
          static_cast<const gstGeode*>(&(*multi_geode->GetGeode(pp)));

      QListViewItem* multiPartItem =
          new QListViewItem(vertexListView, lastMultiPartItem,
                            QString("Polygon %1 (%2)").arg(pp).
                            arg(geode->NumParts()));
      lastMultiPartItem = multiPartItem;

      FillInGeodeData(multiPartItem, geode);
    }
  } else {
    const gstGeode *geode = static_cast<gstGeode*>(&(*geode_handle_));
    FillInGeodeData(vertexListView, geode);
  }
  connect(GfxView::instance, SIGNAL(drawVectors(const gstDrawState &)),
          this, SLOT(drawVectors(const gstDrawState &)));
  connect(this, SIGNAL(redrawPreview()), GfxView::instance, SLOT(updateGL()));
}

ObjectDetail::~ObjectDetail() {
  disconnect(GfxView::instance, SIGNAL(drawVectors(const gstDrawState &)),
             this, SLOT(drawVectors(const gstDrawState &)));
  disconnect(this, SIGNAL(redrawPreview()),
             GfxView::instance, SLOT(updateGL()));
}

void ObjectDetail::drawVectors(const gstDrawState& state) {
  gstDrawState selectState = state;
  selectState.mode |= DrawSelected;

  geode_handle_->Draw(selectState, gstFeaturePreviewConfig());
  if (drawVertex)
    gstGeodeImpl::DrawSelectedVertex(selectState, currentVertex);
}

void ObjectDetail::selectionChanged(QListViewItem* item) {
  VertexItem* vert_item = dynamic_cast<VertexItem*>(item);

  if (vert_item == NULL) {
    drawVertex = false;
  } else {
    currentVertex = vert_item->getVertex();
    drawVertex = true;
  }

  emit redrawPreview();
}
