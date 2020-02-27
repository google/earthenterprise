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


#ifndef _SelectionView_h_
#define _SelectionView_h_

#include <Qt/qobjectdefs.h>
#include <Qt/q3dockwindow.h>
using QDockWindow = Q3DockWindow;
#include "selectionviewbase.h"
#include <gstBBox.h>

class gstSelector;

class SelectionView : public SelectionViewBase {
  Q_OBJECT

 public:
  SelectionView(QWidget* parent = 0, const char* name = 0, Qt::WFlags fl = 0);

protected slots:
void configure(gstSelector* s);
  void openContextMenu(int row, int col, const QPoint& pos);

  signals:
  void zoomToBox(const gstBBox& b);

 private:
  gstSelector* selector_;

  void SaveColumns(int col = -1);
  void ExportSelectedFeatures();
};

class SelectionViewDocker : public QDockWindow {
 public:
  SelectionViewDocker(Place p = InDock, QWidget* parent = 0,
                      const char* name = 0, Qt::WFlags f = 0, bool mode = FALSE);
  ~SelectionViewDocker();

  SelectionView* selectionView() const { return selection_view_; }

 private:
  SelectionView* selection_view_;
};

#endif  // !_SelectionView_h_
