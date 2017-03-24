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

// Note: need to keep this synced with MapDatabase.cpp

#include <autoingest/khAssetManagerProxy.h>

#include "MercatorMapDatabase.h"
#include "MercatorMapDatabaseWidget.h"
#include "AssetDerivedImpl.h"

// ****************************************************************************
// ***  MercatorMapDatabaseDefs
// ****************************************************************************
MercatorMapDatabaseDefs::SubmitFuncType MercatorMapDatabaseDefs::kSubmitFunc =
    &khAssetManagerProxy::MercatorMapDatabaseEdit;

// ****************************************************************************
// ***  MercatorMapDatabase
// ****************************************************************************
MercatorMapDatabase::MercatorMapDatabase(QWidget* parent)
  : AssetDerived<MercatorMapDatabaseDefs, MercatorMapDatabase>(parent)
{ }

MercatorMapDatabase::MercatorMapDatabase(QWidget* parent, const Request& req)
  : AssetDerived<MercatorMapDatabaseDefs, MercatorMapDatabase>(parent, req)
{ }



// Explicit instantiation of base class
template class AssetDerived<MercatorMapDatabaseDefs, MercatorMapDatabase>;
