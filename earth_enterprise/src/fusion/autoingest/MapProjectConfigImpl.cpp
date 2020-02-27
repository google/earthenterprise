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



#include <autoingest/.idl/storage/MapProjectConfig.h>
#include <autoingest/plugins/MapLayerAsset.h>

void
MapProjectConfig::AddLayer(const LayerItem& layer_item)
{
  // check if we already have it
  for (std::vector<LayerItem>::const_iterator layer = layers.begin();
       layer != layers.end(); ++layer) {
    if (layer->assetRef == layer_item.assetRef) {
      throw khException(kh::tr("Layer '%1' already exists in project")
                        .arg(layer_item.assetRef.c_str()));
    }
  }

  layers.push_back(layer_item);
}

void
MapProjectConfig::DeleteLayer(const LayerItem& layer_item)
{
  // find the one we want to delete
  for (std::vector<LayerItem>::iterator layer = layers.begin();
       layer != layers.end(); ++layer) {
    if (layer->assetRef == layer_item.assetRef) {
      layers.erase(layer);
      return;
    }
  }
  throw khException(kh::tr("Layer '%1' not in project").arg(
      layer_item.assetRef.c_str()));
}

bool
MapProjectConfig::HasSearchFields(void) const
{
  for (std::vector<LayerItem>::const_iterator l = layers.begin();
       l != layers.end(); ++l) {

    MapLayerAssetVersion query((*l).assetRef);
    if (!query) {
      notify(NFY_WARN, "Unable to get good version of asset %s.\n",(*l).assetRef.c_str());
      continue;
    }

    if (query->config.HasSearchFields()) return true;
  }
  return false;
}

// ****************************************************************************
// ***  MapFeatureConfig
// ****************************************************************************
VectorDefs::FeatureReduceMethod
MapFeatureConfig::ReduceMethod(void) const {
#if 1
  if (WantDrawAsRoads()) {
    return VectorDefs::ReduceFeatureRoads;
  }
  switch (displayType) {
    case VectorDefs::PointZ:
    case VectorDefs::IconZ:
      return VectorDefs::ReduceFeatureNone;
    case VectorDefs::LineZ:
      return VectorDefs::ReduceFeaturePolylines;
    case VectorDefs::PolygonZ:
      return VectorDefs::ReduceFeaturePolygons;
  }
  // keep compiler happy
#endif
  return VectorDefs::ReduceFeatureNone;
}
