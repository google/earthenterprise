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

// Note: need to keep this synced with MercatorImageryProject.cpp

#include <autoingest/khAssetManagerProxy.h>

#include "khConstants.h"
#include "ImageryProject.h"
#include "AssetDerivedImpl.h"

class AssetBase;

// ****************************************************************************
// ***  ImageryProjectDefs
// ****************************************************************************
ImageryProjectDefs::SubmitFuncType ImageryProjectDefs::kSubmitFunc =
     &khAssetManagerProxy::RasterProjectEdit;


// ****************************************************************************
// ***  ImageryProjectWidget
// ****************************************************************************
ImageryProjectWidget::ImageryProjectWidget(QWidget *parent, AssetBase* base) :
    RasterProjectWidget(parent, AssetDefs::Imagery, kProductSubtype),
    AssetWidgetBase(base)
{ }


// ****************************************************************************
// ***  ImagerProject
// ****************************************************************************
ImageryProject::ImageryProject(QWidget* parent) :
    AssetDerived<ImageryProjectDefs, ImageryProject>(parent)
{
  // We never preserve the indexVersion. That's up to the system manager
  saved_edit_request_.config.indexVersion = 1;
}

ImageryProject::ImageryProject(QWidget* parent, const Request& req) :
    AssetDerived<ImageryProjectDefs, ImageryProject>(parent, req)
{
  // We never preserve the indexVersion. That's up to the system manager
  saved_edit_request_.config.indexVersion = 1;
}
    
ImageryProjectDefs::Request ImageryProject::FinalMakeNewRequest(void) {
  return Request(AssetDisplayHelper::AssetType(kAssetDisplayKey));
}



// Explicit instantiation of base class
template class AssetDerived<ImageryProjectDefs, ImageryProject>;
