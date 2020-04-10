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


#include <searchtabs/.idl/SearchTabs.h>


// ****************************************************************************
// ***  SearchTabDefinition
// ****************************************************************************
QString SearchTabDefinition::IsOk() {
  if (fields.size() == 0) {
    return QString("At least one label/key pair must be defined.");
  }

  if (url.isEmpty() && plugin_name.isEmpty()) {
    return QString("Either URL or Plugin Name must be defined.");
  }

  return QString();
}

SearchTabDefinition SearchTabDefinition::DefaultPOIDefinition(void) {
  SearchTabDefinition poi_tab;
  poi_tab.label = "POI Search";
  poi_tab.fields = std::vector<SearchTabDefinition::Field>
                   (1, SearchTabDefinition::Field("Search Term", "searchTerm"));
  poi_tab.plugin_name = "POISearch";
  poi_tab.query_params = "flyToFirstElement=true&displayKeys=searchTerm";
  poi_tab.is_poi = true;
  return poi_tab;
}

void SearchTabDefinition::ApplyLabelOverrides(
    const SearchTabLabelOverrides &overrides) {
  if (!overrides.label.isEmpty()) {
    label = overrides.label;
  }
  unsigned int count = std::min(fields.size(), overrides.fields.size());
  for (unsigned int i = 0; i < count; ++i) {
    if (!overrides.fields[i].isEmpty()) {
      fields[i].label = overrides.fields[i];
    }
  }
}



// ****************************************************************************
// ***  SearchTabLabelOverrides
// ****************************************************************************
SearchTabLabelOverrides::SearchTabLabelOverrides(
    const SearchTabDefinition &def) {
  label = def.label;
  fields.resize(def.fields.size());
  for (unsigned int i = 0; i < def.fields.size(); ++i) {
    fields[i] = def.fields[i].label;
  }
}
