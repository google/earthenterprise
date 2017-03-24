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

#include "khConstants.h"
#include "TerrainProject.h"
#include "AssetDerivedImpl.h"

class AssetBase;

// ****************************************************************************
// ***  TerrainProjectDefs
// ****************************************************************************
TerrainProjectDefs::SubmitFuncType TerrainProjectDefs::kSubmitFunc =
     &khAssetManagerProxy::RasterProjectEdit;


// ****************************************************************************
// ***  TerrainProjectWidget
// ****************************************************************************
TerrainProjectWidget::TerrainProjectWidget(QWidget *parent, AssetBase* base) :
    RasterProjectWidget(parent, AssetDefs::Terrain, kProductSubtype),
    AssetWidgetBase(base)
{ }


// ****************************************************************************
// ***  ImagerProject
// ****************************************************************************
TerrainProject::TerrainProject(QWidget* parent) :
    AssetDerived<TerrainProjectDefs, TerrainProject>(parent)
{
  // We never preserve the indexVersion. That's up to the system manager
  saved_edit_request_.config.indexVersion = 1;
}

TerrainProject::TerrainProject(QWidget* parent, const Request& req) :
    AssetDerived<TerrainProjectDefs, TerrainProject>(parent, req)
{
  // We never preserve the indexVersion. That's up to the system manager
  saved_edit_request_.config.indexVersion = 1;
}

TerrainProjectDefs::Request TerrainProject::FinalMakeNewRequest(void) {
  return Request(AssetDisplayHelper::AssetType(kAssetDisplayKey));
}



// Explicit instantiation of base class
template class AssetDerived<TerrainProjectDefs, TerrainProject>;
