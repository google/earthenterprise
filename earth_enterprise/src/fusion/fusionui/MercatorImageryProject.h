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

// Note: need to keep this synced with ImageryProject.h

#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_MERCATORIMAGERYPROJECT_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_MERCATORIMAGERYPROJECT_H_

#include "AssetDerived.h"
#include "RasterProjectWidget.h"
#include "AssetDisplayHelper.h"
#include "AssetWidgetBase.h"

class MercatorImageryProjectWidget : public RasterProjectWidget,
                                     public AssetWidgetBase {
 public:
  MercatorImageryProjectWidget(QWidget *parent, AssetBase* base);
};

class MercatorImageryProjectDefs {
 public:
  typedef RasterProjectEditRequest  Request;
  typedef MercatorImageryProjectWidget Widget;
  typedef bool (*SubmitFuncType)(const Request&, QString &, int);

  static const AssetDisplayHelper::AssetKey kAssetDisplayKey =
      AssetDisplayHelper::Key_MercatorImageryProject;
  static SubmitFuncType        kSubmitFunc;
};


class MercatorImageryProject
    : public AssetDerived<MercatorImageryProjectDefs, MercatorImageryProject>,
      public MercatorImageryProjectDefs {
 public:
  explicit MercatorImageryProject(QWidget* parent);
  MercatorImageryProject(QWidget* parent, const Request& req);

  static Request FinalMakeNewRequest(void);
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_FUSIONUI_MERCATORIMAGERYPROJECT_H_
