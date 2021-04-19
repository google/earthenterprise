// Copyright 2017 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


#include <autoingest/khAssetManagerProxy.h>
#include <qlabel.h>
#include <VectorLayer.h>
#include "AssetDerivedImpl.h"
#include <gstAssetGroup.h>

// ****************************************************************************
// ***  VectorLayerDefs
// ****************************************************************************
VectorLayerDefs::SubmitFuncType VectorLayerDefs::kSubmitFunc =
    &khAssetManagerProxy::VectorLayerXEdit;


// ****************************************************************************
// ***  MapLayerWidget
// ****************************************************************************

VectorLayerWidget::VectorLayerWidget(QWidget* parent, AssetBase* base)
  : VectorLayerWidgetBase(parent),
    AssetWidgetBase(base) {}

VectorLayerWidget::~VectorLayerWidget() {}

QString empty_text(kh::tr("<none>"));

void VectorLayerWidget::Prefill(const VectorLayerXEditRequest& req) {
  if (req.config.vectorResource.empty()) {
    vector_resource_label->setText(empty_text);
  } else {
    std::string san = shortAssetName(req.config.vectorResource);
    vector_resource_label->setText(san.c_str());
  }
}

void VectorLayerWidget::AssembleEditRequest(VectorLayerXEditRequest* req) {
  if (vector_resource_label->text() != empty_text) {
    req->config.vectorResource =
        vector_resource_label->text().latin1() +
        AssetDefs::FileExtension(AssetDefs::Vector, kProductSubtype);
  } else {
    req->config.vectorResource = std::string();
  }
}

void VectorLayerWidget::chooseVectorResource() {
  AssetChooser chooser(this, AssetChooser::Open, AssetDefs::Vector,
                       kProductSubtype);
  if (chooser.exec() != QDialog::Accepted)
    return;

  QString newpath;
  if (!chooser.getFullPath(newpath))
    return;

  std::string san = shortAssetName(newpath);
  vector_resource_label->setText(san.c_str());
}

// ****************************************************************************
// ***  VectorLayer
// ****************************************************************************
VectorLayer::VectorLayer(QWidget* parent)
  : AssetDerived<VectorLayerDefs, VectorLayer>(parent) {
}

VectorLayer::VectorLayer(QWidget* parent, const Request& req)
  : AssetDerived<VectorLayerDefs, VectorLayer>(parent, req) {
}
