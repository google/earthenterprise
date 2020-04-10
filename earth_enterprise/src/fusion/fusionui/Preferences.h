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


#ifndef KHSRC_FUSION_FUSIONUI_PREFERENCES_H__
#define KHSRC_FUSION_FUSIONUI_PREFERENCES_H__

#include <string>

#include "preferencesbase.h"
#include <fusionui/.idl/preferences.h>
#include <fusionui/.idl/layoutpersist.h>

class Preferences : public PreferencesBase {
 public:
  Preferences(QWidget* parent);
  static void init();
  static QString empty_path;

  static QString filepath(const QString& path);
  static PrefsConfig& getConfig();
  static PrefsConfig getDefaultConfig();

  static bool GlobalEnableAll;
  static bool ExpertMode;
  static bool ExperimentalMode;
  static bool GoogleInternal;

  static QString CaptionText();

  static std::string prefsDir;

  static QString DefaultVectorPath();
  static QString DefaultImageryPath();
  static QString DefaultTerrainPath();

  // Return the publish server name corresponding to the most recent
  // publish of the named database.
  // Returns the empty string if no mapping is found.
  static std::string PublishServerName(const std::string& database_name);

  // Update the publish server mapping for the specified database.
  // This will record it in the active preferences and save these preferences
  // to file.
  static void UpdatePublishServerDbMap(const std::string& database_name,
                                       const std::string& server_name);

 private:
  // inherited from QDialog
  virtual void accept();

  // inherited from PreferencesBase
  virtual void ChooseImageryProject();
  virtual void ChooseImageryIndex();
  virtual void ChooseImageryProjectMercator();
  virtual void ChooseImageryIndexMercator();
  virtual void ChooseSelectOutline();
  virtual void ChooseSelectFill();
  virtual void ChooseDefaultVectorPath();
  virtual void ChooseDefaultImageryPath();
  virtual void ChooseDefaultTerrainPath();

  static bool initOnce;
  QString EmptyCheck(const QString& txt);

  std::vector< unsigned int>  selectOutline;
  std::vector< unsigned int>  selectFill;
};

#endif  // !KHSRC_FUSION_FUSIONUI_PREFERENCES_H__
