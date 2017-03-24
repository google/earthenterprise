/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_CREATEMETADBROOT_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_CREATEMETADBROOT_H_

#include <string>
#include <iosfwd>  // NOLINT(readability/streams)
#include <vector>
#include <set>
// for std::pair
#include <utility>


#include "common/gedbroot/proto_dbroot.h"

namespace fusion_portableglobe {

class LayerInfo {
 public:
  typedef std::string AssetType;

  std::string display_name;
  std::string path;
  AssetType asset_type;
  int layer;
  int parent_layer;

  static void init_asset_types();
  static bool is_known_layer_type(const std::string& name);

  static AssetType kVector;
  static AssetType kImage;
  static AssetType kBuilding;
  static AssetType kKml;

 private:
  typedef std::set<AssetType> AssetTypes;
  static AssetTypes* asset_types;
};

// Used in the unit test.
struct LayerInfoIsLower {
  bool operator()(const LayerInfo& a, const LayerInfo& b) const {
    return a.layer < b.layer;
  }
};


class VisibleLayers {
 public:
  // std::map in case the layers are sparse. Also, being out-of-order,
  // map is more convenient than vector.
  typedef std::map<int, LayerInfo> LayersByIndex;

  void RemoveParentLayers();

  typedef LayersByIndex::const_iterator const_iterator;
  const_iterator begin() const { return layers_by_index_.begin(); }
  const_iterator end() const { return layers_by_index_.end(); }
  size_t size() const { return layers_by_index_.size(); }

  const_iterator find(int layer_no) const {
    return layers_by_index_.find(layer_no);
  }
  void insert(const LayerInfo& layer_info) {
    layers_by_index_[layer_info.layer] = layer_info;
  }

 private:
  LayersByIndex layers_by_index_;
};


class MetaDbRootWriter {
 public:
  MetaDbRootWriter(bool has_base_imagery,
                   bool has_proto_imagery,
                   bool has_terrain,
                   std::istream& in_layers,
                   const std::string& base_dbroot_fname
                   )
      : has_base_imagery_(has_base_imagery),
        has_proto_imagery_(has_proto_imagery),
        has_terrain_(has_terrain),
        in_layers_(in_layers),
        base_dbroot_fname_(base_dbroot_fname)
  {}

  // Defaults to TextFormat
  bool SaveTo(const std::string& out_dbroot_fname) {
    return SaveTo(out_dbroot_fname, geProtoDbroot::kTextFormat);
  }
  bool SaveTo(const std::string& out_dbroot_fname,
              geProtoDbroot::FileFormat fileFormat);

 protected:
  static bool CreateMetaDbRoot(bool has_base_imagery,
                               bool has_proto_imagery,
                               bool has_terrain,
                               std::istream& in_layers,
                               const std::string& base_dbroot_fname,
                               geProtoDbroot* outDbRoot);

  bool CreateMetaDbRoot() {
    return CreateMetaDbRoot(has_base_imagery_,
                            has_proto_imagery_,
                            has_terrain_,
                            in_layers_,
                            base_dbroot_fname_,
                            &combinedDbRoot_);
  }

  // Not private, only for testing
  geProtoDbroot combinedDbRoot_;

 private:
  static const char* kKh;
  static const char* kKmlLayer;
  typedef std::vector<LayerInfo> Layers;

  static void AddLayersToDbRoot(const VisibleLayers& layers,
                                geProtoDbroot::DbRootProto* dbRoot);

  static void AddSearchToDbRoot(geProtoDbroot::DbRootProto* dbRoot);

  static bool LayerTypeFrom(const std::string& name,
                            LayerInfo::AssetType* outAssetType);

  bool has_base_imagery_;
  bool has_proto_imagery_;
  bool has_terrain_;
  std::istream& in_layers_;
  VisibleLayers layers_;
  std::string base_dbroot_fname_;

  static const char* kLayerTypes[];
  static const char* kLayerUrlString[];

  // Shouldn't contain any layer-specific stuff of course, unless
  // you know what you're doing.
  static const char* kDefaultDbrootBaseContents;

  struct UrlFactoryBase {
    static std::string getProjectEarthSubLayerDbRelativeUrl(int layerInProject);
    static std::string getKmlLayerInProjectRelativeUrl(int layerInProject);
    static const char* kKmlLayerRoot;
    static const char* kSeparator;
  };
};

}  // namespace fusion_portableglobe

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_PORTABLEGLOBE_CREATEMETADBROOT_H_
