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


#include "AssetD.h"
#include <AssetThrowPolicy.h>

// ****************************************************************************
// ***  AssetImplD
// ****************************************************************************

khRefGuard<AssetImplD>
AssetImplD::Load(const std::string &boundref)
{
  khRefGuard<AssetImplD> result;

  // make sure the base class loader actually instantiated one of me
  // this should always happen, but there are no compile time guarantees
  result.dyncastassign(AssetImpl::Load(boundref));
  if (!result) {
    AssetThrowPolicy::FatalOrThrow(kh::tr("Internal error: AssetImplD loaded wrong type for ") + boundref);
  }

  return result;
}

void
AssetImplD::AddVersionRef(const std::string &verref)
{
  // add to beginning of my list of versions
  versions.insert(versions.begin(), verref);
}

void
AssetImplD::Modify(const std::vector<SharedString>& inputs_,
                   const khMetaData &meta_) {
  inputs = inputs_;
  meta = meta_;
}

bool
AssetImplD::InputsUpToDate(const AssetVersion &version,
                           const std::vector<AssetVersion> &cachedInputs) const
{
  if (cachedInputs.size() != version->inputs.size())
    return false;

  for (uint i = 0; i < cachedInputs.size(); ++i) {
    if (cachedInputs[i]->GetRef() != version->inputs[i])
      return false;
  }

  return true;
}

void
AssetImplD::UpdateInputs(std::vector<AssetVersion> &inputvers) const
{
  std::size_t inputs_count = inputs.size();

  inputvers.reserve(inputs_count);
  for (size_t idx = 0; idx < inputs.size(); ++idx) {
    const auto &i = inputs[idx];
    AssetVersionRef verref(i);
    if (verref.Version() == "current") {
      // we only need to update our input if it's an asset ref
      // or a version ref with "current"
      // if it points to a specific version there's nothing to do
      AssetD asset(verref.AssetRef());
      bool needed = false;
      inputvers.push_back(asset->Update(needed));
      notify(NFY_PROGRESS, "Updating asset input %lu (of %lu input%s).",
        idx + 1, inputs_count, inputs_count == 1 ? "" : "s");
    } else {
      inputvers.push_back(AssetVersion(verref));
    }
  }
  notify(NFY_PROGRESS, "Updating asset inputs complete.");
}
