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

// Note: need to keep this synced with MapDatabase.h

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_MERCATORMAPDATABASE_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_MERCATORMAPDATABASE_H_

#include "AssetDerived.h"
#include "AssetDisplayHelper.h"

class MapDatabaseEditRequest;
class MercatorMapDatabaseWidget;

class MercatorMapDatabaseDefs {
 public:
  typedef MapDatabaseEditRequest    Request;
  typedef MercatorMapDatabaseWidget Widget;
  typedef bool (*SubmitFuncType)(const Request&, QString &, int);

  static const AssetDisplayHelper::AssetKey kAssetDisplayKey =
      AssetDisplayHelper::Key_MercatorMapDatabase;
  static SubmitFuncType        kSubmitFunc;
};

class MercatorMapDatabase
    : public AssetDerived<MercatorMapDatabaseDefs, MercatorMapDatabase>,
      public MercatorMapDatabaseDefs {
 public:
  explicit MercatorMapDatabase(QWidget* parent);
  MercatorMapDatabase(QWidget* parent, const Request& req);
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_MERCATORMAPDATABASE_H_
