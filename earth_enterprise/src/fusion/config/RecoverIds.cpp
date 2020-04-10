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


#include "RecoverIds.h"


#include <autoingest/AssetTraverser.h>
#include <autoingest/plugins/RasterProjectAsset.h>
#include <autoingest/plugins/RasterProductAsset.h>
#include <autoingest/plugins/MapLayerAsset.h>
#include <autoingest/sysman/.idl/FusionUniqueId.h>
#include <config/gePrompt.h>



namespace {

class MyTraverser : public AssetTraverser {
 public:
  MyTraverser(void) :
      AssetTraverser(),
      max_channel_id_(0),
      max_resource_id_(0)
  {
    RequestType(AssetDefs::Imagery, kProjectSubtype);
    RequestType(AssetDefs::Terrain, kProjectSubtype);
    RequestType(AssetDefs::Imagery, kProductSubtype);
    RequestType(AssetDefs::Terrain, kProductSubtype);
    RequestType(AssetDefs::Map, kLayer);
  }

  FusionUniqueId GetUniqueIds(void) {
    Traverse();
    FusionUniqueId unique_ids;
    if (max_channel_id_ > 0) {
      unique_ids.next_ids[FusionUniqueId::Channel] = max_channel_id_ + 1;
    }
    if (max_resource_id_ > 0) {
      unique_ids.next_ids[FusionUniqueId::Resource] = max_resource_id_ + 1;
    }
    return unique_ids;
  }
 protected:
  virtual void HandleAsset(Asset &asset) {
    if ((asset->type == AssetDefs::Imagery ||
         asset->type == AssetDefs::Terrain) &&
        asset->subtype == kProjectSubtype) {
      RasterProjectAsset raster_proj = asset;
      if (raster_proj->config.fuid_channel_ > max_channel_id_) {
        max_channel_id_ = raster_proj->config.fuid_channel_;
      }
    } else if ((asset->type == AssetDefs::Imagery ||
                asset->type == AssetDefs::Terrain) &&
               asset->subtype == kProductSubtype) {
      RasterProductAsset raster_prod = asset;
      if (raster_prod->config.fuid_resource_ > max_resource_id_) {
        max_resource_id_ = raster_prod->config.fuid_resource_;
      }
    } else if (asset->type == AssetDefs::Map &&
               asset->subtype == kLayer) {
      MapLayerAsset map_layer = asset;
      if (map_layer->config.fuid_channel_ > max_channel_id_) {
        max_channel_id_ = map_layer->config.fuid_channel_;
      }
    }
  }

  std::uint32_t max_channel_id_;
  std::uint32_t max_resource_id_;
};

} // namespace



void RecoverUniqueIds(void) {
  MyTraverser traverser;
  FusionUniqueId unique_ids = traverser.GetUniqueIds();
  unique_ids.OverwriteSave_Dangerous(AssetDefs::AssetRoot());
}



void PromptUserAndFixUniqueIds(bool noprompt) {
  if (!noprompt) {
    // confirm with user that it's OK to chown
    QString msg = kh::tr(
    "The unique id file in %1 is missing or corrupt.\n"
    "To fix it, this tool must scan your asset root to determine which ids\n"
    "are in use.\n"
    "Depending on the size of your asset root, this could take a while.\n")
                    .arg(AssetDefs::AssetRoot().c_str());
    fprintf(stderr, "%s", (const char *)msg.utf8());
    if (!geprompt::confirm(kh::tr("Proceed with scan"), 'Y')) {
      throw khException(kh::tr("Aborted by user"));
    }
  }
  RecoverUniqueIds();
}
