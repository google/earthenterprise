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
// limitations under the Licens
//

#ifndef GOOGLE3_KEYHOLE_TOOLS_DBROOT_DBROOT_V2_CONVERTER_H_
#define GOOGLE3_KEYHOLE_TOOLS_DBROOT_DBROOT_V2_CONVERTER_H_

#include "common/gedbroot/proto_dbroot.h"
#include "common/gedbroot/tools/eta_parser.h"

namespace keyhole {

// Converts an v1 dbroot protocol message to a dbrootv2 protocol
// message. This class will soon be deprecated since dbroots will at some point
// directly output to the V2 format. A lot of V2 features are simply not
// supported at all in the previous format.
class DbRootV2Converter {
 public:
  explicit DbRootV2Converter(geProtoDbroot *dbroot_proto);
  virtual ~DbRootV2Converter();

  // Fills internal DbRootV2 proto from contents in ETA format.
  bool ParseETADbRoot(const std::string& contents, const std::int32_t epoch);

 private:
  void AddTranslationTable();

  // Adds string in map to translation table, or returns existing Id for string.
  std::int32_t BuildTranslatedStringId(const std::string& str);

  // Utility function that converts a string into a StringIdOrValue proto.
  void AddStringToProto(const std::string& str,
                        dbroot::StringIdOrValueProto* proto);

  dbroot::NestedFeatureProto* NewLayer(std::int32_t channel_id,
                                       const std::string& layer_name,
                                       const std::string& parent_name);

  // ETA parsing functions.
  void ParseETANestedFeatures(const EtaDocument& eta_doc);
  void ParseETAChannelLODs(const EtaDocument& eta_doc);

  dbroot::NestedFeatureProto* GetNestedFeatureById(std::int32_t channel_id);
  void MaybeAddRequirements(const std::string& required_vram,
                            const std::string& required_version,
                            const std::string& probability,
                            const std::string& user_agent,
                            const std::string& client_caps,
                            dbroot::NestedFeatureProto* out);

  geProtoDbroot *const dbroot_v2_proto_;

  // Map that holds all translated string entries.
  typedef std::tr1::unordered_map<std::string, std::uint32_t> StringToIdMap;
  StringToIdMap string_entries_;

  // Map that associates a name (not stored in proto bufs any more) to a layer.
  // Map does *not* own the NestedFeatureProtoV2 objects.
  typedef std::tr1::unordered_map<std::string,
      dbroot::NestedFeatureProto*> NamedLayerMap;
  typedef std::tr1::unordered_map<int,
      dbroot::NestedFeatureProto*> IdLayerMap;
  NamedLayerMap name_layer_map_;  // All layers with a name.
  IdLayerMap id_layer_map_;       // All layers with an id.

  // Counter we increment to assign new ids to each new string.
  std::uint32_t last_id_;
};

}  // namespace keyhole

#endif  // GOOGLE3_KEYHOLE_TOOLS_DBROOT_DBROOT_V2_CONVERTER_H_
