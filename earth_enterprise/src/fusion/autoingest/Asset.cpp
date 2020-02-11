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



#include "fusion/autoingest/Asset.h"
#include "fusion/autoingest/AssetVersion.h"

std::string AssetImpl::GetLastGoodVersionRef(void) const {
  for (VersionList::const_iterator i = versions.begin();
       i != versions.end(); ++i) {
    AssetVersion ver(*i);
    if (ver->state == AssetDefs::Succeeded) {
      return *i;
    }
  }
  return AssetVersionRef(GetRef(), 0);  // an invalid version ref
}

std::string AssetImpl::WorkingDir(const std::string &ref) {
  return ref + "/";
}


std::string AssetImpl::XMLFilename(const std::string &ref) {
  return AssetDefs::AssetPathToFilename(WorkingDir(ref) + "khasset.xml");
}


void AssetImpl::GetInputFilenames(std::vector<std::string> &out) const {
  for (const auto &i : inputs) {
    AssetVersion(i)->GetOutputFilenames(out);
  }
}
