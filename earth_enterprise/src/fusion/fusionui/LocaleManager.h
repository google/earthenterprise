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


#ifndef KHSRC_FUSION_FUSIONUI_LOCALEMANAGER_H__
#define KHSRC_FUSION_FUSIONUI_LOCALEMANAGER_H__

#include <Qt/qobjectdefs.h>
#include <autoingest/.idl/Locale.h>
#include <localemanagerbase.h>

class LocaleManager : public LocaleManagerBase {
 public:
  LocaleManager(QWidget* parent, bool modal, Qt::WFlags flags);

  // inherited from QDialog
  virtual void accept();

  // inherited from LocaleManagerBase
  virtual void NewLocale();
  virtual void DeleteLocale();
  virtual void ModifyLocale();

  static std::vector<QString> SupportedLocales();

 private:
  bool LocaleExists(const QString& text);

  LocaleSet localeset_;
};

#endif  // !KHSRC_FUSION_FUSIONUI_LOCALEMANAGER_H__
