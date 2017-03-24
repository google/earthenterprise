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


#ifndef KHSRC_FUSION_FUSIONUI_TERRAINPROJECT_H__
#define KHSRC_FUSION_FUSIONUI_TERRAINPROJECT_H__

#include "AssetWidgetBase.h"
#include "AssetDerived.h"
#include "RasterProjectWidget.h"
#include "AssetDisplayHelper.h"

class AssetBase;
class RasterProjectEditRequest;

class TerrainProjectWidget : public RasterProjectWidget,
                             public AssetWidgetBase {
 public:
  TerrainProjectWidget(QWidget *parent, AssetBase* base);
};


class TerrainProjectDefs {
 public:
  typedef RasterProjectEditRequest  Request;
  typedef TerrainProjectWidget      Widget;
  typedef bool (*SubmitFuncType)(const Request&, QString &, int);

  static const AssetDisplayHelper::AssetKey kAssetDisplayKey =
      AssetDisplayHelper::Key_TerrainProject;
  static SubmitFuncType        kSubmitFunc;
};



class TerrainProject : public AssetDerived<TerrainProjectDefs,TerrainProject>,
                       public TerrainProjectDefs {
 public:
  TerrainProject(QWidget* parent);
  TerrainProject(QWidget* parent, const Request& req);

  static Request FinalMakeNewRequest(void);
};



#endif  // !KHSRC_FUSION_FUSIONUI_TERRAINPROJECT_H__
