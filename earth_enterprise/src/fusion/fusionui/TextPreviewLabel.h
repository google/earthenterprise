/*
 * Copyright 2020 Google Inc. Copyright 2019 the Open GEE Contributors.
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

#ifndef TEXTPREVIEWLABEL_H
#define TEXTPREVIEWLABEL_H

#include <Qt/qlabel.h>
#include <Qt/qwidget.h>
#include <Qt/qobjectdefs.h>
#include <autoingest/.idl/storage/MapSubLayerConfig.h>
#include <autoingest/.idl/MapTextStyle.h>
class QMouseEvent;
class MapTextStyleEvent;

/*
The purpose of this file is to prevent circular dependency errors which occur
in textstylebase.h and TextStyle.h

Only the declaration appears in this file, the definition will stay in TextStyle.cpp
as was originally written
*/

// ****************************************************************************
// ***  Preview Label
// ****************************************************************************
class TextPreviewLabel : public QLabel {
  Q_OBJECT

 public:
  TextPreviewLabel(QWidget* parent, const char* name);
  TextPreviewLabel(QDialog*);

  // inherited from QWidget
  // support drag
  virtual void mousePressEvent(QMouseEvent* event);
  virtual void mouseMoveEvent(QMouseEvent* event);

  void UpdateConfig(const MapTextStyleConfig& config);

 private:
  bool dragging_;
  MapTextStyleConfig config_;
};

#endif // TEXTPREVIEWLABEL_H
