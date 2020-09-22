/*
 * Copyright 2017 Google Inc.
 * Copyright 2020 The Open GEE Contributors 
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


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_VECTOR_DBROOT_GENERATOR_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_VECTOR_DBROOT_GENERATOR_H_

#include <map>
#include "fusion/dbroot/proto_dbroot_generator.h"

class VectorDbrootContext;

class VectorDbrootGenerator : public ProtoDbrootGenerator {
 private:
  class LOD {
   public:
    QString name;
    unsigned int    channelId;
    unsigned int    lodFlags;
    std::vector< unsigned int>  states;

    LOD(const std::string &name_, unsigned int id, unsigned int lodFlags_) :
        name(name_.c_str()),
        channelId(id),
        lodFlags(lodFlags_) { }
  };

  class StyleIdMap {
   public:
    std::int32_t   map_id_;
    std::int32_t   normal_style_id_;
    std::int32_t   highlight_style_id_;

    StyleIdMap(std::int32_t map_id,
               std::int32_t normal_style_id,
               std::int32_t highlight_style_id) :
        map_id_(map_id),
        normal_style_id_(normal_style_id),
        highlight_style_id_(highlight_style_id) { }
  };


  VectorDbrootContext             *vector_context_;

  std::map<QString, QString> uniqueNameMap;
  std::set<QString>     usedNames;

  // used when building up nested proto structures from flat ones
  typedef std::map<std::string,
                   keyhole::dbroot::NestedFeatureProto*> NamedLayerMap;
  typedef std::map<std::int32_t,
                   keyhole::dbroot::NestedFeatureProto*> IdLayerMap;
  NamedLayerMap name_layer_map_;
  IdLayerMap    id_layer_map_;

  std::vector<StyleIdMap> style_id_maps_;

  QString GetUniqueName(const QString& layerPath);

  void EmitNestedLayers(void);
  void EmitStyleAttrs(void);
  void EmitStyleMaps(void);
  void EmitLODs(void);
  keyhole::dbroot::NestedFeatureProto* MakeProtoLayer(
      std::int32_t channel_id,
      const std::string &layer_name,
      const std::string &parent_name);

 public:
  VectorDbrootGenerator(VectorDbrootContext *vector_context,
                        const QString &locale,
                        const std::string &outfile);
  void Emit(geProtoDbroot::FileFormat output_format);
};


#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_VECTOR_DBROOT_GENERATOR_H_
