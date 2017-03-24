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
#include "RasterAssetWidget.h"
#include "TerrainAsset.h"
#include "AssetDerivedImpl.h"

// ****************************************************************************
// ***  TerrainAssetDefs
// ****************************************************************************
TerrainAssetDefs::SubmitFuncType TerrainAssetDefs::kSubmitFunc =
     &khAssetManagerProxy::RasterProductImport;

// ****************************************************************************
// ***  TerrainAsset
// ****************************************************************************
TerrainAsset::TerrainAsset(QWidget* parent) :
    AssetDerived<TerrainAssetDefs, TerrainAsset>(parent)
{ }

TerrainAsset::TerrainAsset(QWidget* parent, const Request& req) :
    AssetDerived<TerrainAssetDefs, TerrainAsset>(parent, req)
{ }
    
TerrainAssetDefs::Request TerrainAsset::FinalMakeNewRequest(void) {
  return Request(AssetDisplayHelper::AssetType(kAssetDisplayKey));
}

// Explicit instantiation of base class
template class AssetDerived<TerrainAssetDefs, TerrainAsset>;
