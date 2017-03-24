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

// Note: need to keep this synced with MercatorImageryProject.h

#ifndef KHSRC_FUSION_FUSIONUI_IMAGERYPROJECT_H__
#define KHSRC_FUSION_FUSIONUI_IMAGERYPROJECT_H__

#include "AssetDerived.h"
#include "RasterProjectWidget.h"
#include "AssetDisplayHelper.h"
#include "AssetWidgetBase.h"

class AssetBase;
class RasterProjectEditRequest;

class ImageryProjectWidget : public RasterProjectWidget,
                             public AssetWidgetBase {
 public:
  ImageryProjectWidget(QWidget *parent, AssetBase* base);
};


class ImageryProjectDefs {
 public:
  typedef RasterProjectEditRequest  Request;
  typedef ImageryProjectWidget      Widget;
  typedef bool (*SubmitFuncType)(const Request&, QString &, int);

  static const AssetDisplayHelper::AssetKey kAssetDisplayKey =
      AssetDisplayHelper::Key_ImageryProject;
  static SubmitFuncType        kSubmitFunc;
};



class ImageryProject : public AssetDerived<ImageryProjectDefs,ImageryProject>,
                       public ImageryProjectDefs {
 public:
  ImageryProject(QWidget* parent);
  ImageryProject(QWidget* parent, const Request& req);

  static Request FinalMakeNewRequest(void);
};

#endif  // !KHSRC_FUSION_FUSIONUI_IMAGERYPROJECT_H__
