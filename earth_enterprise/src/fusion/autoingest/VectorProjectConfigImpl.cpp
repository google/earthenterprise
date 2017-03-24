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



#include <functional>
#include <algorithm>

#include <qstringlist.h>

#include "autoingest/.idl/storage/VectorProjectConfig.h"
#include "common/RuntimeOptions.h"
#include "common/khConstants.h"


AvailId
VectorProjectConfig::GetAvailChannelIds(void) const
{
  AvailId avail(ExpertBeginChannelId(), EndChannelId());

  // find the Ids I already have and exclude them from the list
  for (std::vector<LayerConfig>::const_iterator layerConfig = layers.begin();
       layerConfig != layers.end(); ++layerConfig) {
    if (layerConfig->channelId)
      avail.exclude(layerConfig->channelId);
  }
  // now exclude ExpertChannelIds if not already excluded
  // also make sure that we dont generate an exception
  uint32 expertId;
  for (expertId=ExpertBeginChannelId(); expertId<BeginChannelId(); expertId++) {
    avail.exclude(expertId, false);
  }

  return avail;
}


AvailId
VectorProjectConfig::GetAvailStyleIds(void) const
{
  std::vector<uint32> have;
  AvailId avail(BeginStyleId(), EndStyleId());

  // find the Ids I already have and exclude them from the list
  for (std::vector<LayerConfig>::const_iterator layerConfig
         = layers.begin();
       layerConfig != layers.end(); ++layerConfig) {
    // skip if it's not a streamed layer
    if (!layerConfig->IsStreamedLayer())
      continue;

    for (std::vector<DisplayRuleConfig>::const_iterator disp
           = layerConfig->displayRules.begin();
         disp != layerConfig->displayRules.end(); ++disp) {
      if (disp->feature.style.id)
        avail.exclude(disp->feature.style.id);
      if (disp->site.style.id)
        avail.exclude(disp->site.style.id);
    }
  }

  return avail;
}



void
VectorProjectConfig::AssignChannelIdsIfZero(void)
{
  // In addition to finding the available id's it will also
  // catch duplicates
  AvailId avail = GetAvailChannelIds();

  if (!RuntimeOptions::GoogleInternal()) {
    for (std::vector<LayerConfig>::iterator layerConfig
           = layers.begin();
         layerConfig != layers.end(); ++layerConfig) {
      if (layerConfig->channelId == 0) {
        layerConfig->channelId = avail.next();
      }
    }
  } else {
    // Google-internal vector processing workflow requires that all channel
    // ids be manually assigned so if we find a '0' channel id it's an error
    QStringList bad_layers;
    for (std::vector<LayerConfig>::iterator layerConfig
           = layers.begin();
         layerConfig != layers.end(); ++layerConfig) {
      if (layerConfig->channelId == 0) {
        bad_layers.push_back(layerConfig->DefaultNameWithPath());
      }
    }
    if (bad_layers.size() > 0) {
      QString msg = kh::tr("The following layer(s) need a channel id:\n") +
                    bad_layers.join("\n");
      throw khException(msg);
    }
  }
}


// Assign UUIDs to LayerConfigs that are not yet assigned.
void
VectorProjectConfig::AssignUuidsIfEmpty(void)
{
  std::for_each(layers.begin(), layers.end(),
                std::mem_fun_ref(&LayerConfig::AssignUuidIfEmpty));
}

void VectorProjectConfig::ResetIds(void)
{
  indexVersion = 1;
  std::for_each(layers.begin(), layers.end(),
                std::mem_fun_ref(&LayerConfig::ResetIds));
}

void
VectorProjectConfig::AssignStyleIdsIfZero(void)
{
  AvailId avail = GetAvailStyleIds();

  for (std::vector<LayerConfig>::iterator layerConfig
         = layers.begin();
       layerConfig != layers.end(); ++layerConfig) {
    // skip if it's not a streamed layer
    if (!layerConfig->IsStreamedLayer())
      continue;

    for (std::vector<DisplayRuleConfig>::iterator disp
           = layerConfig->displayRules.begin();
         disp != layerConfig->displayRules.end(); ++disp) {
      if ((disp->feature.enabled()) &&
          disp->feature.style.id == 0)
        disp->feature.style.id = avail.next();
      if (disp->site.enabled &&
          disp->site.style.id == 0)
        disp->site.style.id = avail.next();
    }
  }
}

void
VectorProjectConfig::EnsureFolderExists(const QString &folder) {
  // top level - always exists
  if (folder.isEmpty()) return;

  // see if we can find a match
  for (std::vector<LayerConfig>::const_iterator l = layers.begin();
       l != layers.end(); ++l) {
    if (l->DefaultNameWithPath() == folder) {
      if (l->IsFolder()) {
        // we found the folder we're looking for
        return;
      } else {
        // we found a layer with the name that we need for the folder
        throw khException(kh::tr("Cannot add folder %1. Layer exists with that name.").arg(folder));
      }
    }
  }

  // didn't find a match, let's try to make one
  QString delim = QString("|");
  QString parent;
  if (folder.find(delim) != -1) {
    parent = folder.section(delim, 0, -2);
    EnsureFolderExists(parent);
  }
  LayerConfig newLayer;
  newLayer.legend = parent;
  newLayer.defaultLocale.name_ = folder.section(delim, -1);
  newLayer.defaultLocale.icon_ = IconReference(IconReference::Internal,
                                               kDefaultIconName);
  layers.push_back(newLayer);
}


void
VectorProjectConfig::AddLayer(const LayerConfig &layer)
{
  // first make sure we have the necessary parent folders
  EnsureFolderExists(layer.legend);

  uint num = 1;
  bool again;
  QString checkname;
  do {
    again = false;
    checkname = num > 1 ?
                layer.defaultLocale.name_.GetValue() +
                " (" + ToQString(num) + ")" :
                layer.defaultLocale.name_.GetValue();
    for (std::vector<LayerConfig>::const_iterator l = layers.begin();
         l != layers.end(); ++l) {
      if (l->defaultLocale.name_.GetValue() == checkname &&
          l->legend == layer.legend) {
        ++num;
        again = true;
        break;
      }
    }
  } while (again);
  // now add it and give it the name we want
  layers.push_back(layer);
  layers.back().defaultLocale.name_ = checkname;
}

void
VectorProjectConfig::CheckIds(void) const
{
  try {
    AvailId ids = GetAvailChannelIds();
  } catch (const std::exception &e) {
    notify(NFY_WARN, "Bad channel IDs: %s", e.what());
  } catch (...) {
    notify(NFY_WARN, "Bad channel IDs: Unknown error");
  }
  try {
    AvailId ids = GetAvailStyleIds();
  } catch (const std::exception &e) {
    notify(NFY_WARN, "Bad style IDs: %s", e.what());
  } catch (...) {
    notify(NFY_WARN, "Bad style IDs: Unknown error");
  }
}

void
VectorProjectConfig::AfterLoad(const VectorProjectConfig::DeprecatedMembers &)
{
  // add roads and borders layers in the config
  bool roadsAdded = false;
  bool bordersAdded = false;
  for (std::vector<LayerConfig>::iterator l = layers.begin();
       l != layers.end(); ++l) {
    if (l->defaultLocale.name_.GetValue() == "roads")
      roadsAdded = true;
    if (l->defaultLocale.name_.GetValue() == "borders")
      bordersAdded = true;

    if (l->legend == "roads" && !roadsAdded) {
      LayerConfig roads;
      roads.defaultLocale.name_ = "roads";
      roads.channelId = 0;
      roads.defaultLocale.icon_=
        IconReference(IconReference::Internal, kDefaultIconName);
      l = layers.insert(l, roads);
      ++l;
      roadsAdded = true;
    }
    if (l->legend == "borders" && !bordersAdded) {
      LayerConfig borders;
      borders.defaultLocale.name_ = "borders";
      borders.channelId = 0;
      borders.defaultLocale.icon_ =
        IconReference(IconReference::Internal, kDefaultIconName);
      l = layers.insert(l, borders);
      ++l;
      bordersAdded = true;
    }
  }
}

bool
VectorProjectConfig::HasJS(void) const
{
  for (std::vector<LayerConfig>::const_iterator l = layers.begin();
       l != layers.end(); ++l) {
    if (l->HasJS())
      return true;
  }
  return false;
}

bool
VectorProjectConfig::HasSearchFields(void) const
{
  for (std::vector<LayerConfig>::const_iterator l = layers.begin();
       l != layers.end(); ++l) {
    if (l->HasSearchFields())
      return true;
  }
  return false;
}
