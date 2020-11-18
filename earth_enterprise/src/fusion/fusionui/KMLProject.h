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


#ifndef KHSRC_FUSION_FUSIONUI_KMLPROJECT_H__
#define KHSRC_FUSION_FUSIONUI_KMLPROJECT_H__
#include <Qt/q3listview.h>
#include "AssetWidgetBase.h"
#include "AssetDerived.h"
#include "ProjectWidget.h"
#include "AssetDisplayHelper.h"

class AssetBase;
class KMLProjectEditRequest;

class KMLProjectWidget : public ProjectWidget,
                         public AssetWidgetBase {
 public:
  KMLProjectWidget(QWidget *parent, AssetBase* base);

  void Prefill(const KMLProjectEditRequest &req);
  void AssembleEditRequest(KMLProjectEditRequest *request);

 protected:
  virtual LayerItemBase* NewLayerItem();
  virtual LayerItemBase* NewLayerItem(const QString& assetref);
};


class KMLProjectDefs {
 public:
  typedef KMLProjectEditRequest  Request;
  typedef KMLProjectWidget      Widget;
  typedef bool (*SubmitFuncType)(const Request&, QString &, int);

  static const AssetDisplayHelper::AssetKey kAssetDisplayKey =
      AssetDisplayHelper::Key_KMLProject;
  static SubmitFuncType        kSubmitFunc;
};



class KMLProject : public AssetDerived<KMLProjectDefs,KMLProject>,
                       public KMLProjectDefs {
 public:
  KMLProject(QWidget* parent);
  KMLProject(QWidget* parent, const Request& req);
};

#endif  // !KHSRC_FUSION_FUSIONUI_KMLPROJECT_H__
