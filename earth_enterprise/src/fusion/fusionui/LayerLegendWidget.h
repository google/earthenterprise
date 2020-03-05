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

//

#ifndef FUSION_FUSIONUI_LAYERLEGENDWIDGET_H__
#define FUSION_FUSIONUI_LAYERLEGENDWIDGET_H__

#include <Qt/qobjectdefs.h>
#include <Qt/qwidget.h>
#include <Qt/q3table.h>

class QVBoxLayout;
class QCheckBox;
class EndEditTable;
using QTable = Q3Table;
class LayerLegendWidget : public QWidget {
  Q_OBJECT

 public:
  LayerLegendWidget( QWidget* parent, const char* name = 0, Qt::WFlags fl = 0 );
  ~LayerLegendWidget(void);

  QVBoxLayout*  vlayout;
  QCheckBox*    filter_check;

  QTable* GetTable();
  void EndEdit();

 protected slots:
    virtual void languageChange();

 private:
  EndEditTable* table;
};




#endif // FUSION_FUSIONUI_LAYERLEGENDWIDGET_H__
