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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_FIELDGENERATORCONTROLLER_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_FIELDGENERATORCONTROLLER_H_

#include "fusion/fusionui/WidgetControllers.h"
#include "fusion/gst/gstSourceManager.h"

#include <qstringlist.h>

class QComboBox;
class QLineEdit;
class FieldGenerator;

class FieldGeneratorController : public WidgetController {
  Q_OBJECT

 public slots:
  void modeChanged();
  void moreButtonClicked();

 public:
  virtual void SyncToConfig(void);

 protected:
  virtual void SyncToWidgetsImpl(void);

 public:
  static void Create(WidgetControllerManager &manager,
                     QComboBox *combo_,
                     QLineEdit *lineEdit_,
                     bool lineEditEmptyOk_,
                     QPushButton *button_,
                     FieldGenerator *config_,
                     const gstSharedSource &source_,
                     const QStringList &contextScripts_,
                     const QString &errorContext);

 private:
  FieldGeneratorController(WidgetControllerManager &manager,
                           QComboBox *combo_,
                           QLineEdit *lineEdit_,
                           bool lineEditEmptyOk_,
                           QPushButton *button_,
                           FieldGenerator *config_,
                           const gstSharedSource &source_,
                           const QStringList &contextScripts_,
                           const QString &errorContext_);
  void MySyncToWidgetsImpl(void);

  QComboBox      *combo;
  QLineEdit      *lineEdit;
  bool            lineEditEmptyOk;
  QPushButton    *button;
  FieldGenerator *config;
  gstSharedSource source;
  QStringList     contextScripts;
  QString         errorContext;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_FIELDGENERATORCONTROLLER_H_
