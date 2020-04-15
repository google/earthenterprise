// Copyright 2017 Google Inc.
// Copyright 2020 The Open GEE Contributors
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



#include "AssetVersionRef.h"

#include <khstrconv.h>
#include "Asset.h"
#include "AssetVersion.h"

AssetVersionRef::AssetVersionRef(const std::string &ref)
{
  std::string::size_type verstart = ref.rfind("?version=");
  assetRef = ref.substr(0, verstart);
  if (verstart != std::string::npos) {
    version = ref.substr(verstart+9, std::string::npos);
  } else {
    version = "current";
  }
}

AssetVersionRef::AssetVersionRef(const SharedString &ref) : AssetVersionRef(ref.toString())
{
}

AssetVersionRef::operator std::string() const
{
  if (version == "current") {
    return assetRef;
  } else {
    return assetRef + "?version=" + version;
  }
}

AssetVersionRef::AssetVersionRef(const std::string &assetRef_,
                                 const std::string &version_)
    : assetRef(assetRef_), version(version_) {
  if (version.size() == 0) {
    version = "current";
  }
}

AssetVersionRef::AssetVersionRef(const std::string &assetRef_, unsigned int vernum)
    : assetRef(assetRef_), version(ToString(vernum)) {
}

std::string
AssetVersionRef::Bind(void) const
{
  if (version == "current") {
    return Asset(assetRef)->CurrVersionRef();
  } else if (version == "lastgood") {
    return Asset(assetRef)->GetLastGoodVersionRef();
  } else {
    return *this;
  }
}
