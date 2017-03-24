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


#ifndef KHSRC_FUSION_FUSIONUI_VECTORLAYER_H__
#define KHSRC_FUSION_FUSIONUI_VECTORLAYER_H__

#include <autoingest/.idl/storage/VectorLayerXConfig.h>

#include "AssetWidgetBase.h"
#include "AssetChooser.h"
#include "AssetDerived.h"
#include "AssetDisplayHelper.h"
#include "vectorlayerwidgetbase.h"

class AssetBase;
class QColor;

class VectorLayerWidget : public VectorLayerWidgetBase,
                          public AssetWidgetBase {
  Q_OBJECT

 public:
  VectorLayerWidget(QWidget* parent, AssetBase* base);
  ~VectorLayerWidget();

  void Prefill(const VectorLayerXEditRequest& req);
  void AssembleEditRequest(VectorLayerXEditRequest* req);

 private:
  // inherited from VectorLayerWidgetBase
  virtual void chooseVectorResource();
};

class VectorLayerDefs {
 public:
  typedef VectorLayerXEditRequest  Request;
  typedef VectorLayerWidget       Widget;
  typedef bool (*SubmitFuncType)(const Request&, QString &, int);

  static const AssetDisplayHelper::AssetKey kAssetDisplayKey =
      AssetDisplayHelper::Key_VectorLayer;
  static SubmitFuncType        kSubmitFunc;
};

class VectorLayer : public AssetDerived<VectorLayerDefs,VectorLayer>,
                    public VectorLayerDefs {
 public:
  VectorLayer(QWidget* parent);
  VectorLayer(QWidget* parent, const Request& req);
};

#endif  // !KHSRC_FUSION_FUSIONUI_VECTORLAYER_H__
