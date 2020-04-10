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
//

// Imports an ETA dbroot and converts to a dbroot v2 protocol buffer.

#include "common/gedbroot/tools/dbroot_v2_converter.h"

#include <sstream>
#include <string>
#include <vector>

#include "common/gedbroot/tools/eta_dbroot_utils.h"

using std::string;
using std::vector;
using std::stringstream;

namespace {

const char kImageryLayerName[] = "imagery";
const char kTerrainLayerName[] = "terrain";

}  // anonymous namespace

namespace keyhole {

DbRootV2Converter::DbRootV2Converter(geProtoDbroot *dbroot_proto)
    : dbroot_v2_proto_(dbroot_proto),
      last_id_(1) {
  if (!dbroot_proto) {
    notify(NFY_FATAL, "Invalid dbroot_proto passed to DbRootV2Converter.");
  }
}

DbRootV2Converter::~DbRootV2Converter() {}

bool DbRootV2Converter::ParseETADbRoot(const string& contents,
                                       const std::int32_t epoch) {
  EtaDocument eta_doc;
  eta_doc.ParseFromString(contents);
  eta_utils::ParseProviders(eta_doc, dbroot_v2_proto_);
  eta_utils::ParseStyleAttributes(eta_doc, dbroot_v2_proto_);
  eta_utils::ParseStyleMaps(eta_doc, dbroot_v2_proto_);
  ParseETANestedFeatures(eta_doc);
  ParseETAChannelLODs(eta_doc);
  eta_utils::ParseEndSnippet(eta_doc, dbroot_v2_proto_);
  notify(NFY_INFO, "Final DbRoot proto sizes:\nNumber of providers: %d\n"
         "Number of style maps: %d", dbroot_v2_proto_->provider_info_size(),
         dbroot_v2_proto_->style_map_size() );
  dbroot_v2_proto_->mutable_database_version()->set_quadtree_version(epoch);

  if (dbroot_v2_proto_->HasContents()) {
    AddTranslationTable();
    return true;
  } else {
    notify(NFY_WARN, "No content extracted from ETA dbroot.");
    return false;
  }
}

void DbRootV2Converter::AddTranslationTable() {
    for (StringToIdMap::iterator it = string_entries_.begin();
         it != string_entries_.end(); ++it) {
      dbroot::StringEntryProto* string_entry =
          dbroot_v2_proto_->add_translation_entry();
    string_entry->set_string_value(it->first);
    string_entry->set_string_id(it->second);
  }
}

std::int32_t DbRootV2Converter::BuildTranslatedStringId(const string& str) {
  StringToIdMap::iterator it = string_entries_.find(str);
  if (it != string_entries_.end()) {
    return it->second;
  }
  std::int32_t new_index = last_id_++;
  string_entries_.insert(std::make_pair(str, new_index));
  return new_index;
}

void DbRootV2Converter::AddStringToProto(const string& str,
                                         dbroot::StringIdOrValueProto* proto) {
  std::uint32_t id = BuildTranslatedStringId(str);
  CHECK(!proto->has_value());
  proto->set_string_id(id);
}

// Returns a new layer based on the name of the layer to create and name of
// parent layer. If parent name is empty, a new layer is added at the root of
// the hierarchy. If the parent layer is found, a new layer is added as a
// child of that layer. If a name is specified, the newly created layer is
// added to the layer name map.
// Post-Conditions:
//  name_layer_map_: Entry added for (layer_name, return value), if the
//                   name is not empty.
//  id_layer_map_: Entry added for (channel_id, return value).
dbroot::NestedFeatureProto* DbRootV2Converter::NewLayer(
    std::int32_t channel_id,
    const string& layer_name,
    const string& parent_name) {
  dbroot::NestedFeatureProto* new_layer = NULL;
  NamedLayerMap::iterator it = name_layer_map_.end();

  // Only search parent if layer has a name and parent is defined.
  if (!parent_name.empty()) {
    it = name_layer_map_.find(parent_name);
    if (it == name_layer_map_.end()) {
      notify(NFY_FATAL,
             "parent layer specified but not created, hierarchy will not be "
             "created as expected.  Parent name: %s,  Layer ID: %d",
             parent_name.c_str(), channel_id);
    }
  }

  if (it == name_layer_map_.end()) {
    // Parent is not specified or parent layer not found, create layer.
    new_layer = dbroot_v2_proto_->add_nested_feature();
  } else {
    // Parent layer was found in map
    dbroot::NestedFeatureProto* parent_layer = it->second;
    new_layer = parent_layer->add_children();
  }
  CHECK(new_layer);
  new_layer->set_channel_id(channel_id);

  // Add layer to map if layer_name is specified.
  if (!layer_name.empty()) {
    name_layer_map_.insert(std::make_pair(layer_name, new_layer));
  }

  // Add layer to id_layer_map_.
  id_layer_map_.insert(std::make_pair(channel_id, new_layer));

  return new_layer;
}

// Reads nested layers from LODs from [export.nestedLayers].
void DbRootV2Converter::ParseETANestedFeatures(const EtaDocument& eta_doc) {
  vector<EtaStruct*> layers_list;
  if (!eta_utils::GetListFromDocument(eta_doc,
                                      "export.nestedlayers", "etNestedLayer",
                                      &layers_list)) {
    notify(NFY_WARN, "missing export.nestedlayers in dbroot");
    return;
  }

  for (vector<EtaStruct*>::const_iterator it = layers_list.begin();
       it != layers_list.end(); ++it) {
    const EtaStruct* eta_layer = *it;
    //     <etNestedLayer> [name] {
    //  0    <etInt>     [channelId]            -1
    //  1    <etString>  [assetUUID]            ""
    //  2    <etString>  [displayName]          ""
    //  3    <etString>  [parentName]           ""
    //  4    <etBool>    [isFolder]             false
    //  5    <etBool>    [isVisible]            false
    //  6    <etBool>    [isEnabled]            true
    //  7    <etBool>    [isChecked]            false
    //  8    <etBool>    [isExpandable]         true
    //  9    <etString>  [labels]               ""   (ignored)
    // 10    <etInt>     [preserveTextLevel]    30
    // 11    <etString>  [layerIcon]            ""
    // 12    <etString>  [description]          ""
    // 13    <etString>  [lookAt]               ""
    // 14    <etString>  [kmlUrl]               ""   (ignored)
    // 15    <etString>  [kmlLayerUrl]          ""
    // 16    <etBool>    [saveLocked]           true
    // 17    <etString>  [requiredVRAM]         ""
    // 18    <etString>  [requiredClientVer]    ""
    // 19    <etString>  [probability]          ""
    // 20    <etString>  [requiredUserAgent]    ""
    // 21    <etString>  [requiredClientCapabilities]   ""
    // 22    <etString>  [clientConfigScriptName]    ""
    //       # optional for diorama layers.
    // 23    <etInt>     [dioramaDataChannelBase]    -1
    // 24    <etInt>     [replicaDataChannelBase]    -1
    //     }
    const string& layer_name = eta_layer->name();
    eta_utils::EtaReader reader(eta_layer);
    // Special case if layer name is "imagery" or "terrain".
    if (layer_name == kImageryLayerName) {
      dbroot_v2_proto_->set_imagery_present(true);
    } else if (layer_name == kTerrainLayerName) {
      dbroot_v2_proto_->set_terrain_present(true);
    } else {
      string parent_name = reader.GetString("parentName", "");
      std::int32_t channel_id = reader.GetInt("channelId", -1);

      dbroot::NestedFeatureProto* nested = NewLayer(channel_id,
                                                    layer_name,
                                                    parent_name);
      nested->set_asset_uuid(reader.GetString("assetUUID", ""));
      AddStringToProto(reader.GetString("displayName", ""),
                       nested->mutable_display_name());
      nested->set_is_visible(reader.GetBool("isVisible", false));
      nested->set_is_enabled(reader.GetBool("isEnabled", true));
      nested->set_is_checked(reader.GetBool("isChecked", false));
      bool is_folder = reader.GetBool("isFolder", false);
      if (is_folder) {
        dbroot::FolderProto* folder = nested->mutable_folder();
        folder->set_is_expandable(reader.GetBool("isExpandable", true));
      }
      dbroot::LayerProto* layer = nested->mutable_layer();
      layer->set_preserve_text_level(reader.GetInt("preserveTextLevel", 30));
      nested->set_layer_menu_icon_path(reader.GetString("layerIcon", ""));
      string description = reader.GetString("description", "");
      if (!description.empty()) {
        AddStringToProto(description, nested->mutable_description());
      }
      eta_utils::ParseLookAtString(reader.GetString("lookAt", ""), nested);
      string kml_url = reader.GetString("kmlLayerUrl", "");
      if (!kml_url.empty()) {
        AddStringToProto(kml_url, nested->mutable_kml_url());
      }
      nested->set_is_save_locked(reader.GetBool("saveLocked", false));
      MaybeAddRequirements(reader.GetString("requiredVRAM", ""),
                           reader.GetString("requiredClientVer", ""),
                           reader.GetString("probability", ""),
                           reader.GetString("requiredUserAgent", ""),
                           reader.GetString("requiredClientCapabilities", ""),
                           nested);
      string script_name = reader.GetString("clientConfigScriptName", "");
      if (!script_name.empty()) {
        nested->set_client_config_script_name(script_name);
      }
      static const int kDioramaDataChannelBaseDefault = -1;
      int diorama_data_channel_base = reader.GetInt(
          "dioramaDataChannelBase", kDioramaDataChannelBaseDefault);
      if (diorama_data_channel_base != kDioramaDataChannelBaseDefault) {
        nested->set_diorama_data_channel_base(
            diorama_data_channel_base);
      }
      static const int kReplicaDataChannelBaseDefault = -1;
      int replica_data_channel_base = reader.GetInt(
          "replicaDataChannelBase", kReplicaDataChannelBaseDefault);
      if (replica_data_channel_base != kReplicaDataChannelBaseDefault) {
        nested->set_replica_data_channel_base(
            replica_data_channel_base);
      }
    }
  }
}

// Reads channel LODs from [export.channelLODs]
// In ETA, <etChannelLOD> has the following structure
//  <etTemplate> [<etChannelLOD>]
//  {
//      channelIndex lodFlags
//      level0  level1  level2  level3  level4  level5
//      level6  level7  level8  level9  level10 level11
//      level12 level13 level14 level15 level16 level17
//      level18 level19 level20 level21 level22 level23
//  }
void DbRootV2Converter::ParseETAChannelLODs(const EtaDocument& eta_doc) {
  vector<EtaStruct*> lod_list;
  if (!eta_utils::GetListFromDocument(eta_doc,
                                      "export.channelLODs", "etChannelLOD",
                                      &lod_list)) {
    notify(NFY_WARN, "missing export.channelLODs in dbroot");
  }

  for (vector<EtaStruct*>::const_iterator it = lod_list.begin();
       it != lod_list.end(); ++it) {
    const EtaStruct* eta_lod = *it;
    eta_utils::EtaReader reader(eta_lod);
    if (eta_lod->num_children() < 26) {
      notify(NFY_WARN, "unexpected num children for lod %s: %d",
             eta_lod->name().c_str(), eta_lod->num_children());
      continue;
    }
    std::int32_t channel_id = reader.GetInt("channelIndex", -1);
    dbroot::NestedFeatureProto* nested = GetNestedFeatureById(channel_id);
    if (!nested) {
      notify(NFY_WARN, "could not find layer id %d", channel_id);
      continue;
    }
    std::int32_t lod_flags = reader.GetInt("lodFlags", 0);

    static const int kPolygonFlag = 0x7;
    if (lod_flags == kPolygonFlag) {
      nested->set_feature_type(dbroot::NestedFeatureProto::TYPE_POLYGON_Z);
    }

    static const int kLodIndexMax = 32;
    for (int lod_index = 0; lod_index < kLodIndexMax; ++lod_index) {
      stringstream lod_stream;
      lod_stream << "level" << lod_index;
      const keyhole::EtaStruct* eta_lod_child = eta_lod->GetChildByName(
          lod_stream.str(), "etFloat");
      if (eta_lod_child) {
        // We read the LOD as int even though it is an etFloat.
        int level_lod = eta_lod_child->GetInt32Value();
        if (level_lod != 30 && level_lod > 0) {
          dbroot::ZoomRangeProto* zoom_range =
              nested->mutable_layer()->add_zoom_range();
          zoom_range->set_min_zoom(lod_index);
          zoom_range->set_max_zoom(level_lod);
        }
      }
    }
  }
}

dbroot::NestedFeatureProto* DbRootV2Converter::GetNestedFeatureById(
    std::int32_t channel_id) {
  IdLayerMap::iterator found = id_layer_map_.find(channel_id);
  if (found != id_layer_map_.end())
    return found->second;
  else
    return NULL;
}

void DbRootV2Converter::MaybeAddRequirements(const string& required_vram,
                                             const string& required_version,
                                             const string& probability,
                                             const string& user_agent,
                                             const string& client_caps,
                                             dbroot::NestedFeatureProto* out) {
  if (!required_vram.empty()) {
    out->mutable_requirement()->set_required_vram(required_vram);
  }
  if (!required_version.empty()) {
    out->mutable_requirement()->set_required_client_ver(required_version);
  }
  if (!probability.empty()) {
    out->mutable_requirement()->set_probability(probability);
  }
  if (!user_agent.empty()) {
    out->mutable_requirement()->set_required_user_agent(user_agent);
  }
  if (!client_caps.empty()) {
    out->mutable_requirement()->set_required_client_capabilities(client_caps);
  }
}


}  // namespace keyhole
