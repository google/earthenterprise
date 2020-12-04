/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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


#ifndef _ObjectDetail_h_
#define _ObjectDetail_h_

#include <Qt/qobjectdefs.h>
#include <Qt/qobject.h>
#include <Qt/q3listview.h>
using QListViewItem = Q3ListViewItem;
#include <khGuard.h>
#include <gstVertex.h>
#include <gstGeode.h>
#include <gstRecord.h>

#include "objectdetailbase.h"

class gstRecord;
class gstDrawState;

// -----------------------------------------------------------------------------

class ObjectDetail : public ObjectDetailBase {
  Q_OBJECT

 public:
  ObjectDetail(QWidget* parent, unsigned int id, gstGeodeHandle g, gstRecordHandle rec);
  ~ObjectDetail();

  class VertexItem : public QListViewItem {
   public:
    VertexItem(QListViewItem* parent, QListViewItem* after, const QString& l,
               const gstVertex& v);
    const gstVertex& getVertex() const { return vertex; }

   private:
    gstVertex vertex;

  };

  signals:
  void redrawPreview();

protected slots:
void drawVectors(const gstDrawState& s);
  virtual void selectionChanged(QListViewItem* item);

 private:
  gstGeodeHandle geode_handle_;
  bool drawVertex;
  gstVertex currentVertex;
};

#endif // !_ObjectDetail_h_
