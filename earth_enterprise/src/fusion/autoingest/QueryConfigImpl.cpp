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

#include <autoingest/.idl/storage/QueryConfig.h>
#include <autoingest/.idl/storage/MapSubLayerConfig.h>
#include <autoingest/.idl/storage/LayerConfig.h>

#include <qstringlist.h>

// Create a QueryConfig with the necessary pieces from a LayerConfig
QueryConfig::QueryConfig(const LayerConfig &layerConfig,
                         const QStringList &contextScripts) :
    allowFeatureDuplication(layerConfig.allowFeatureDuplication),
    contextScript(),
    allowEmptyLayer(layerConfig.allowEmptyLayer)
{
  if (layerConfig.HasFilterJS()) {
    if (!contextScripts.empty()) {
      for (QStringList::const_iterator cs = contextScripts.begin();
           cs != contextScripts.end(); ++cs) {
        if (!(*cs).isEmpty()) {
          contextScript += *cs;
          contextScript += "\n";
        }
      }
    }
    contextScript += layerConfig.layerContextScript;
  }

  for (std::vector<DisplayRuleConfig>::const_iterator display
         = layerConfig.displayRules.begin();
       display != layerConfig.displayRules.end(); ++display) {
    filters.push_back(display->filter);
  }
}


// Create a QueryConfig with the necessary pieces from a MapSubLayerConfig
QueryConfig::QueryConfig(const MapSubLayerConfig &mapSubLayerConfig,
                         const QStringList &contextScripts) :
    allowFeatureDuplication(mapSubLayerConfig.allowFeatureDuplication),
    contextScript(),
    allowEmptyLayer(mapSubLayerConfig.allowEmptyLayer)
{
  if (mapSubLayerConfig.HasFilterJS()) {
    if (!contextScripts.empty()) {
      for (QStringList::const_iterator cs = contextScripts.begin();
           cs != contextScripts.end(); ++cs) {
        if (!(*cs).isEmpty()) {
          contextScript += *cs;
          contextScript += "\n";
        }
      }
    }
    contextScript += mapSubLayerConfig.contextScript;
  }

  for (std::vector<MapDisplayRuleConfig>::const_iterator display
         = mapSubLayerConfig.display_rules.begin();
       display != mapSubLayerConfig.display_rules.end(); ++display) {
    filters.push_back(display->filter);
  }
}
