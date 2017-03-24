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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_SERVERCOMBINATIONEDIT_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_SERVERCOMBINATIONEDIT_H_

#include <servercombinationeditbase.h>
#include <autoingest/.idl/ServerCombination.h>

class ServerCombinationEdit : public ServerCombinationEditBase {
 public:
  ServerCombinationEdit(QWidget* parent, const ServerCombination& c);
  ServerCombination GetCombination() const;
  virtual void accept();
  virtual void QueryStreamServer() {
    return QueryServer();
  }

  virtual void StreamUrlTextChanged(const QString& url_text);
  virtual void CacertTextChanged();
  virtual void InsecureSslCheckboxToggled(bool state);
  virtual void ChooseCacertFile();

 private:
  void QueryServer();
  void UpdateSslOptionControls(bool enabled);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_SERVERCOMBINATIONEDIT_H_
