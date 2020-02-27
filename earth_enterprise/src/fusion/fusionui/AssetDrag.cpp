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


#include "AssetDrag.h"

const char kSep = ':';

// create an asset drag object by using a QTexDrag
// - set the main text to be the assetref
// - the subType is formed by combining the string
//   representation of the asset type and the asset subtype
//
// For example:
//   Vector Product asset named boundary.kvasset
//   QTextDrag::setText("boundary.kvasset")
//   QTextDrag::setSubtype("Vector:Product")

AssetDrag::AssetDrag(QWidget* drag_source, Asset asset)
  : QTextDrag(drag_source)
{
  setText(asset->GetRef().toString().c_str());
  std::string subtype = ToString(asset->type) + kSep + asset->subtype;
  setSubtype(subtype.c_str());
}

bool AssetDrag::canDecode(QMimeSource *e,
                          AssetDefs::Type asset_type,
                          const std::string& asset_subtype) {
  std::string subtype = "text/" + ToString(asset_type) + kSep + asset_subtype;
  return e->provides(subtype.c_str());
}
