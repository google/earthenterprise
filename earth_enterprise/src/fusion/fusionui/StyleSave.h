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

#ifndef STYLESAVE_H
#define STYLESAVE_H
#include <Qt/qobjectdefs.h>
#include <Qt/qpushbutton.h>
#include <Qt/qwidget.h>
#include <Qt/q3button.h>
#include <Qt/q3buttongroup.h>
#include <Qt/q3combobox.h>
#include "TextPreviewLabel.h"

class QDragEnterEvent;
class QDropEvent;
class QDragLeaveEvent;

/*
The purpose of this file is to prevent circular dependency errors which occur
in textstylebase.h and TextStyle.h

Only the declaration appears in this file, the definition will stay in TextStyle.cpp
as was originally written
*/


// ****************************************************************************
// ***  Style Save Button
// ****************************************************************************

class StyleSaveButton : public QPushButton {
  Q_OBJECT

 public:
  StyleSaveButton(QWidget* parent, const char* name);
  StyleSaveButton(Q3ButtonGroup*);

  // inherited from QWidget
  // support drop
  virtual void dragEnterEvent(QDragEnterEvent* event);
  virtual void dropEvent(QDropEvent* event);
  virtual void dragLeaveEvent(QDragLeaveEvent* event);

 signals:
  void StyleChanged(QWidget* btn);
};


#endif // STYLESAVE_H
