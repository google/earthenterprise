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


#include "fusion/portableglobe/createmetadbroot.h"

// for printf, ::remove
#include <cstdio>

#include <string>
#include <algorithm>
#include <vector>
#include <sstream>  // NOLINT(readability/streams)
#include <istream>  // NOLINT(readability/streams)
#include <fstream>  // NOLINT(readability/streams)
// for std::runtime_error
#include <stdexcept>

#include "common/khConstants.h"
#include "common/khStringUtils.h"
#include "common/khFileUtils.h"
#include "common/notify.h"

#include <google/protobuf/text_format.h>  // NOLINT(*)

namespace fusion_portableglobe {

LayerInfo::AssetType LayerInfo::kVector = "VECTOR";
LayerInfo::AssetType LayerInfo::kImage = "IMAGE";
LayerInfo::AssetType LayerInfo::kBuilding = "BUILDING";
LayerInfo::AssetType LayerInfo::kKml = "KML";
LayerInfo::AssetTypes* LayerInfo::asset_types;

void LayerInfo::init_asset_types() {
  asset_types = new AssetTypes;
  // A raw vector data file (e.g., CSV, KML, SHP, ...).
  asset_types->insert(kVector);
  asset_types->insert(kImage);
  asset_types->insert(kBuilding);
  asset_types->insert(kKml);
}

bool LayerInfo::is_known_layer_type(const std::string& name) {
  AssetTypes::const_iterator itFound = asset_types->find(name);
  return itFound != asset_types->end();
}

std::string MetaDbRootWriter::UrlFactoryBase::
getProjectEarthSubLayerDbRelativeUrl(int layerInProject) {
  std::ostringstream oss;
  oss << "." << kSeparator << layerInProject << kSeparator << kKh << kSeparator;
  return oss.str();
}

std::string MetaDbRootWriter::UrlFactoryBase::
getKmlLayerInProjectRelativeUrl(int layerInProject) {
  std::ostringstream oss;
  oss << "." << kSeparator  << layerInProject << kSeparator << kKmlLayer
      << kSeparator << kKmlLayerRoot;
  return oss.str();
}

const char* MetaDbRootWriter::UrlFactoryBase::kSeparator = "/";
const char* MetaDbRootWriter::UrlFactoryBase::kKmlLayerRoot = "root.kml";

const char* MetaDbRootWriter::kKh = "kh";
const char* MetaDbRootWriter::kKmlLayer = "kmllayer";

// TODO: -- May not need this.
const char* MetaDbRootWriter::kDefaultDbrootBaseContents =
    "end_snippet {\n"
    "  disable_authentication: true\n"
    "}\n";


// A layer line looks like:
//   base_globe /data/gray_marble.glb IMAGE 1 0
enum InfoLayerIndices {
  kDisplayNameIndex   = 0,
  kLayerGlxPath       = 1,
  kLayerTypenameIndex = 2,
  kSelfLayerIndex     = 3,
  kParentLayerIndex   = 4,
};
const int kNumInfoLayerFields = 5;

bool IntFromText(const std::string& str, int* out_i) {
  if (!out_i) {
    notify(NFY_FATAL, "Valid int pointer required");
  }
  std::istringstream iss(str);
  iss >> *out_i;
  // whether there were enough leading digits to convert to an int.
  return !iss.fail();
}

// Adds string to proto field.
static
void AddStringToProto(const std::string& str,
                      keyhole::dbroot::StringIdOrValueProto* proto_field) {
  proto_field->clear_string_id();
  proto_field->set_value(str);
}

void MetaDbRootWriter::AddLayersToDbRoot(const VisibleLayers& layers,
                                         geProtoDbroot::DbRootProto* dbroot) {
  assert(dbroot);

  for (VisibleLayers::const_iterator it = layers.begin(), end = layers.end();
       it != end; ++it) {
    keyhole::dbroot::NestedFeatureProto* nested_feature =
        dbroot->add_nested_feature();
    AddStringToProto(it->second.display_name,
                     nested_feature->mutable_display_name());
    int layer_index = it->first;
    nested_feature->set_channel_id(layer_index);
    if (it->second.asset_type == LayerInfo::kBuilding ||
        it->second.asset_type == LayerInfo::kImage) {
      // protocol stuff
      std::string db_url = UrlFactoryBase::
          getProjectEarthSubLayerDbRelativeUrl(layer_index);
      nested_feature->set_database_url(db_url);
    } else if (it->second.asset_type == LayerInfo::kVector) {
      // Add the URL for the KML layer as a nested layer in the project's dboot.
      // TODO: Add better control of LHP naming.
      std::string kml_url = UrlFactoryBase::
          getKmlLayerInProjectRelativeUrl(layer_index);
      AddStringToProto(kml_url, nested_feature->mutable_kml_url());
    } else if (it->second.asset_type == LayerInfo::kKml) {
      AddStringToProto(it->second.path, nested_feature->mutable_kml_url());
    } else {
      printf("Found project child asset with unexpected drawable type: %s",
             it->second.asset_type.c_str());
      exit(1);
    }
  }
}

void MetaDbRootWriter::AddSearchToDbRoot(geProtoDbroot::DbRootProto* dbroot) {
  assert(dbroot);
  // Add search to end snippet.
  keyhole::dbroot::EndSnippetProto* end_snippet =
      dbroot->mutable_end_snippet();
  // Ensure deprecated search tabs are clear.
  end_snippet->clear_search_tab();
  keyhole::dbroot::EndSnippetProto::SearchConfigProto* search_config =
      end_snippet->mutable_search_config();
  keyhole::dbroot::EndSnippetProto::SearchConfigProto::SearchServer*
      search_server = search_config->add_search_server();
  // No supplemental UI allowed in Portable.
  search_server->clear_supplemental_ui();
  // Only allow the one-box search for Portable.
  AddStringToProto(kPortableSearchUrl, search_server->mutable_url());
}

// More leaf-ward layers supercede their parents, and only actual
// /leaves/ are visible.
// A child of a layer is one that has that layer as its parent (layers
// know about their parents, not about their children).  We find all
// layers which are parents of something, but, only when all have been
// checked do we remove them. We do this because we assume no order
// for the layers, and a layer which is superceded early may
// nonetheless itself supercede a layer that appears later.
void VisibleLayers::RemoveParentLayers() {
  typedef std::vector<LayersByIndex::iterator> Marked;
  Marked marked_for_deletion;
  // O(n^2) but, small n.
  for (LayersByIndex::iterator itLayer = layers_by_index_.begin(),
       endLayer = layers_by_index_.end(); itLayer != endLayer; ++itLayer) {
    int layer = itLayer->second.layer;
    // scan for children (layers with <layer> as a parent)
    for (const_iterator itMaybeChild = layers_by_index_.begin();
         itMaybeChild != layers_by_index_.end(); ++itMaybeChild) {
      if (itMaybeChild->second.parent_layer == layer) {
        marked_for_deletion.push_back(itLayer);
        break;
      }
    }
  }
  // remove all marked
  for (Marked::const_iterator itMarked = marked_for_deletion.begin();
       itMarked != marked_for_deletion.end(); ++itMarked) {
    layers_by_index_.erase(*itMarked);
  }
}

bool MetaDbRootWriter::CreateMetaDbRoot(
    bool has_base_imagery,
    bool has_proto_imagery,
    bool has_terrain,
    std::istream& in_layers,
    const std::string& user_base_dbroot_fname,
    geProtoDbroot* outDbRoot) {
  if (!outDbRoot) {
    throw std::runtime_error("Must have a outDbRoot address");
  }
  Layers layers;

  VisibleLayers visible_layers;
  try {
    for (int i = 0; in_layers.good(); ++i) {
      std::string line;
      getline(in_layers, line);
      if (line.empty())
        continue;
      std::vector<std::string> tokens;
      TokenizeString(line, tokens, " ");

      if (static_cast<int>(tokens.size()) < kNumInfoLayerFields) {
        notify(NFY_WARN, "Line has too few fields (%d) - at least %d expected",
               static_cast<int>(tokens.size()),
               static_cast<int>(kNumInfoLayerFields));
      } else {
        LayerInfo::AssetType layer_type = tokens[kLayerTypenameIndex];
        bool is_known_layer =
            LayerInfo::is_known_layer_type(layer_type);
        if (!is_known_layer) {
          notify(NFY_DEBUG, "layer type %s unknown and ignored",
                 layer_type.c_str());
          // Although we may not display unknown layers, one may still supercede
          // one that we do display, so we keep processing.
        }
        int layer_index;
        int parent_layer_index;
        bool isint = IntFromText(tokens[kSelfLayerIndex], &layer_index);
        if (!isint) {
          // Probably the wrong kind of file. However when used in
          // other than a utility, don't want it to be fatal.
          notify(NFY_WARN, "Value %s of field %d is not an int",
                 tokens[kSelfLayerIndex].c_str(),
                 static_cast<int>(kSelfLayerIndex));
          return false;
        }
        // layers 0 and 1 are special and we don't pay attention to them.
        if (layer_index == 0 || layer_index == 1) {
          notify(NFY_WARN,
                 "Layer %s is omitted since layer indexes 0 and 1 are special.",
                 tokens[kDisplayNameIndex].c_str());
          continue;
        }
        isint = IntFromText(tokens[kParentLayerIndex], &parent_layer_index);
        if (!isint) {
          notify(NFY_WARN, "Value %s of field %d is not an int",
                 tokens[kSelfLayerIndex].c_str(),
                 static_cast<int>(kSelfLayerIndex));
          return false;
        }

        LayerInfo layer_info = { tokens[kDisplayNameIndex],
                                 tokens[kLayerGlxPath],
                                 layer_type,
                                 layer_index,
                                 parent_layer_index };
        visible_layers.insert(layer_info);
      }
    }
    // ('catch<nospace>(' to satisfy google-lint which thinks it's a function.)
  } catch(const std::ios_base::failure& e) {
    std::cerr << "fail or bad: mismatch of data or similar"
              << e.what() << std::endl;
    throw;
  } catch(const std::exception& e) {
    std::cerr << "Non-fail,bad std:: fault: " << e.what() << std::endl;
    throw;
  } catch(...) {
    std::cerr << "Unknown exception" << std::endl;
    throw;
  }

  visible_layers.RemoveParentLayers();

  geProtoDbroot layersDbRoot;
  AddLayersToDbRoot(visible_layers, &layersDbRoot);
  AddSearchToDbRoot(&layersDbRoot);
  layersDbRoot.set_imagery_present(has_base_imagery);
  layersDbRoot.set_proto_imagery(has_proto_imagery);
  layersDbRoot.set_terrain_present(has_terrain);

  std::string baseFilename;
  bool using_default = user_base_dbroot_fname.empty();
  if (!using_default) {
    baseFilename = user_base_dbroot_fname;
  } else {
    baseFilename = khTmpFilename("/tmp/deleteme_");
    std::ofstream out(baseFilename.c_str());
    out << kDefaultDbrootBaseContents;
  }

  // pull in the base version.
  outDbRoot->Combine(baseFilename, geProtoDbroot::kTextFormat);
  bool success = outDbRoot->IsInitialized();
  if (using_default) {
    ::remove(baseFilename.c_str());
  }
  if (success) {
    outDbRoot->MergeFrom(layersDbRoot);
    success = outDbRoot->IsInitialized();
  }
  return success;
}

bool MetaDbRootWriter::SaveTo(const std::string& dbroot_fname,
                              geProtoDbroot::FileFormat file_format) {
  LayerInfo::init_asset_types();
  bool success = CreateMetaDbRoot(has_base_imagery_,
                                  has_proto_imagery_,
                                  has_terrain_,
                                  in_layers_,
                                  base_dbroot_fname_,
                                  &combinedDbRoot_);

  if (!success) {
    return false;
  }

  // write a local copy of our modified dbroot
  combinedDbRoot_.Write(dbroot_fname, file_format);
  return true;
}

}  // namespace fusion_portableglobe
