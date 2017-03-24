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

// Note: need to keep this synced with ImageryAsset.cpp

#include <autoingest/khAssetManagerProxy.h>
#include "RasterAssetWidget.h"
#include "MercatorImageryAsset.h"
#include "AssetDerivedImpl.h"

// ****************************************************************************
// ***  MercatorImageryAssetDefs
// ****************************************************************************
MercatorImageryAssetDefs::SubmitFuncType MercatorImageryAssetDefs::kSubmitFunc =
     &khAssetManagerProxy::MercatorRasterProductImport;

// ****************************************************************************
// ***  MercatorImageryAsset
// ****************************************************************************
MercatorImageryAsset::MercatorImageryAsset(QWidget* parent)
  : AssetDerived<MercatorImageryAssetDefs, MercatorImageryAsset>(parent)
{ }

MercatorImageryAsset::MercatorImageryAsset(QWidget* parent, const Request& req)
  : AssetDerived<MercatorImageryAssetDefs, MercatorImageryAsset>(parent, req)
{ }

MercatorImageryAssetDefs::Request MercatorImageryAsset::FinalMakeNewRequest(
    void) {
  MercatorImageryAssetDefs::Request ret_val =
      Request(AssetDisplayHelper::AssetType(kAssetDisplayKey));
  ret_val.config.useMercatorProjection = true;
  return ret_val;
}

// Explicit instantiation of base class
template class AssetDerived<MercatorImageryAssetDefs, MercatorImageryAsset>;
