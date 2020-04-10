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


#include <gstVectorProject.h>

#include <qmap.h>
#include <algorithm>

#include <gstFilter.h>
#include <gstSource.h>
#include <gstLayer.h>
#include <gstProgress.h>
#include <autoingest/khAssetManagerProxy.h>
#include <autoingest/plugins/VectorProjectAsset.h>

gstVectorProject::gstVectorProject(const std::string& name)
    : name_(name) {
  serverType(VectorProjectConfig::Standard);
  buildVersion(1);
}

gstVectorProject::~gstVectorProject() {
  for (std::vector<gstLayer*>::iterator it = layers_.begin();
       it != layers_.end(); ++it) {
    (*it)->unref();
  }
  layers_.clear();
}

void gstVectorProject::serverType(VectorProjectConfig::ServerType type) {
  if (type != config_.serverType) {
    config_.serverType = type;
  }
}

void gstVectorProject::buildVersion(unsigned int v) {
  if (v != config_.indexVersion) {
    config_.indexVersion = v;
  }
}

bool gstVectorProject::Prefill(const VectorProjectConfig& cfg) {
  QMap<QString, gstLayer*> group_map;
  config_ = cfg;
  config_.layers.clear();

  // add each layer
  for (std::vector<LayerConfig>::const_iterator it = cfg.layers.begin();
       it != cfg.layers.end(); ++it) {
    // check to see if this is a standard layer, or a layer group
    // empty assetref means it's a group
    if (it->assetRef.length() != 0) {
      // find asset
      Asset asset(AssetDefs::FilenameToAssetPath(it->assetRef));
      if (asset->type != AssetDefs::Vector ||
          asset->subtype != kProductSubtype) {
        notify(NFY_WARN, "Invalid product: %s ", it->assetRef.c_str());
        return false;
      }
      AssetVersion ver(asset->GetLastGoodVersionRef());
      if (!ver) {
        notify(NFY_WARN, "Unable to get good version of asset: %s",
               it->assetRef.c_str());
        return false;
      }
      gstSource* src = new gstSource(ver->GetOutputFilename(0).c_str());
      gstLayer* layer = new gstLayer(this, src, 0 /*srcLayerNum*/, *it);
      src->unref();    // source manager now owns this source
      layer->SetSortId(layers_.size());
      layers_.push_back(layer);
    } else {
      // must be a layer group
      QString fullPath = it->DefaultNameWithPath();
      if (!group_map.contains(fullPath)) {
        gstLayer* layer = new gstLayer(this, 0/*src*/, 0/*srcLayerNum*/, *it);
        layer->SetSortId(layers_.size());
        layers_.push_back(layer);
        group_map[fullPath] = layer;
      } else {
        notify(NFY_WARN, "Internal Error: Duplicate group name: %s",
               (const char*)fullPath.utf8());
      }
    }
  }

  for (std::vector<gstLayer*>::const_iterator it = layers_.begin();
       it != layers_.end(); ++it) {
    notify(NFY_DEBUG, "layer id = %d, name = \"%s\"", (*it)->Id(),
           (*it)->GetPath().latin1());
    if ((*it)->isGroup()) {
      notify(NFY_DEBUG, "  ** Group");
      continue;
    }
    for (unsigned int f = 0; f < (*it)->NumFilters(); ++f) {
      notify(NFY_DEBUG,
             "  filter %d, name = \"%s\": feature style = %d, site style = %d",
             f, (*it)->GetFilterById(f)->Name().latin1(),
             (*it)->GetFilterById(f)->FeatureConfigs().config.style.id,
             (*it)->GetFilterById(f)->Site().config.style.id);
    }
  }

  return true;
}

void gstVectorProject::AssembleEditRequest(VectorProjectEditRequest *request) {
  // Compose a VectorProjectConfig using my config plus the lyaer configs
  // of my gstLayer children.
  request->config = config_;
  request->config.layers.clear();
  for (std::vector<gstLayer*>::const_iterator it = layers_.begin();
       it != layers_.end(); ++it) {
    const LayerConfig& layerCfg = (*it)->GetConfig();
    request->config.AddLayer(layerCfg);
  }

#ifndef NDEBUG
  // This shouldn't happen, but code below it relies on it.
  // So let's just double check
  if (request->config.layers.size() != layers_.size()) {
    throw khException(kh::tr("Internal Error: Missing layer groups"));
  }
#endif

  // Fill in any missing channel and layer ids.
  request->config.AssignChannelIdsIfZero();
  request->config.AssignUuidsIfEmpty();
  request->config.AssignStyleIdsIfZero();

  // Now update all the configs in the gstLayers, small things may have
  // changed (ids assigned, names made unique, etc).
  for (unsigned int i = 0; i < layers_.size(); ++i) {
    layers_[i]->SetConfig(request->config.layers[i]);
  }
}


bool gstVectorProject::HasSearchFields(void) const {
  for (std::vector<gstLayer*>::const_iterator it = layers_.begin();
       it != layers_.end(); ++it) {
    if ((*it)->GetConfig().HasSearchFields()) {
      return true;
    }
  }
  return false;
}
