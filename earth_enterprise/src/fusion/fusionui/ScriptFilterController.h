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

#ifndef FUSION_FUSIONUI_SCRIPTFILTERCONTROLLER_H__
#define FUSION_FUSIONUI_SCRIPTFILTERCONTROLLER_H__

#include <WidgetControllers.h>
#include <gstSourceManager.h>

class QLineEdit;

class ScriptFilterController : public WidgetController
{
  Q_OBJECT

 public slots:
  void moreButtonClicked();

 public:
  virtual void SyncToConfig(void);
 protected:
  virtual void SyncToWidgetsImpl(void);
 public:
  static void Create(WidgetControllerManager &manager,
                     QLineEdit *lineEdit_,
                     QPushButton *button_,
                     QString *config_,
                     const gstSharedSource &source_,
                     const QStringList &contextScripts_,
                     const QString &errorContext);

 private:
  ScriptFilterController(WidgetControllerManager &manager,
                         QLineEdit *lineEdit_,
                         QPushButton *button_,
                         QString *config_,
                         const gstSharedSource &source_,
                         const QStringList &contextScripts_,
                         const QString &errorContext_);
  void MySyncToWidgetsImpl(void);

  QLineEdit      *lineEdit;
  QPushButton    *button;
  QString        *config;
  gstSharedSource source;
  QStringList     contextScripts;
  QString         errorContext;
};

#endif // FUSION_FUSIONUI_FIELDGENERATORCONTROLLER_H__
