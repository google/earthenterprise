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


#ifndef KHSRC_FUSION_FUSIONUI_DATABASE_H__
#define KHSRC_FUSION_FUSIONUI_DATABASE_H__

#include "AssetDerived.h"
#include "AssetDisplayHelper.h"

class DatabaseEditRequest;
class DatabaseWidget;

class DatabaseDefs {
 public:
  typedef DatabaseEditRequest  Request;
  typedef DatabaseWidget       Widget;
  typedef bool (*SubmitFuncType)(const Request&, QString &, int);

  static const AssetDisplayHelper::AssetKey kAssetDisplayKey =
      AssetDisplayHelper::Key_Database;
  static SubmitFuncType        kSubmitFunc;
};

class Database : public AssetDerived<DatabaseDefs, Database>,
                 public DatabaseDefs {
                     
 public:
  Database(QWidget* parent);
  Database(QWidget* parent, const Request& req);
};


#endif  // !KHSRC_FUSION_FUSIONUI_DATABASE_H__
