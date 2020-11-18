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



#include <autoingest/.idl/storage/SearchTabReference.h>
#include <autoingest/.idl/SearchTabSet.h>
#include <autoingest/plugins/VectorProjectAsset.h>
#if 0
  #include <autoingest/plugins/RasterProjectAsset.h>
#endif
#include <autoingest/plugins/MapProjectAsset.h>
#include <khConstants.h>


// ****************************************************************************
// ***  SearchTabReference
// ****************************************************************************
SearchTabDefinition SearchTabReference::Bind(void) const {
  if (project_ref_.empty()) {
    SearchTabSet global_tabset;
    if (!global_tabset.Load()) {
      throw kh::tr("Unable to load seach tab definitions");
    }
    return global_tabset.FindByIdOrThrow(tab_id_);
  } else {
    Asset proj(project_ref_);
    if (!proj) {
      throw khException(kh::tr("Invalid asset reference: %1")
                        .arg(project_ref_.c_str()));
    }
    if (proj->subtype != kProjectSubtype) {
      throw khException(kh::tr("Asset reference is not a project: %1 %2")
                        .arg(project_ref_.c_str()).arg(proj->type));
    }
    if (proj->type == AssetDefs::Vector) {
      // for now only a single search tab is supported from projects,
      // so ignore the tab_id_
      VectorProjectAsset vproj(proj);
      if (vproj->config.HasSearchFields()) {
        SearchTabDefinition poi_tab =
          SearchTabDefinition::DefaultPOIDefinition();
        poi_tab.ApplyLabelOverrides(
            vproj->config.poi_search_tab_overrides);
        return poi_tab;
      } else {
        throw khException(kh::tr("Project has no search fields: %1")
                          .arg(project_ref_.c_str()));
      }
    } else if (proj->type == AssetDefs::Map) {
      MapProjectAsset mproj(proj);
      if (mproj->config.HasSearchFields()) {
        SearchTabDefinition poi_tab =
          SearchTabDefinition::DefaultPOIDefinition();
        poi_tab.ApplyLabelOverrides(mproj->config.poi_search_tab_overrides);
        return poi_tab;
      } else {
        throw khException(kh::tr("Project has no search fields: %1")
                                  .arg(project_ref_.c_str()));
      }

    } else {
      throw khException(kh::tr("Unsupported project type: %1")
                        .arg(proj->subtype.c_str()));
    }
  }

  // unreached, but silences compiler warnings
  return SearchTabDefinition();
}
