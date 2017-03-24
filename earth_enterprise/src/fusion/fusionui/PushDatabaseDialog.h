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

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_PUSHDATABASEDIALOG_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_PUSHDATABASEDIALOG_H_

#include <string>
#include <vector>

#include "fusion/autoingest/Asset.h"
#include "pushdatabasedialogbase.h"


class PushDatabaseDialog : public PushDatabaseDialogBase {
 public:
  PushDatabaseDialog(QWidget* parent, const Asset& asset,
                     const std::vector<QString>& nicknames);

  QString GetSelectedNickname();
  int GetSelectedCombination();
  bool HasValidVersions();
  std::string GetSelectedVersion();

 private:
  std::vector<std::string> valid_versions_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_PUSHDATABASEDIALOG_H_
