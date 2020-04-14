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

// Base class for RasterDbrootGenerator and VectorDbrootGenerator
// Logic that is identical between raster and vector dbroots lives here.
// (e.g. converting individual strings into string table indexes, emitting
// the list of used providers)


#ifndef GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_PROTO_DBROOT_GENERATOR_H_
#define GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_PROTO_DBROOT_GENERATOR_H_

#include <set>
#include <map>
#include <string>
#include <qstring.h>  // NOLINT
//#include "common/khTypes.h"
#include <cstdint>
#include "common/gedbroot/proto_dbroot.h"

class ProtoDbrootContext;

namespace keyhole {
namespace dbroot {
class StringIdOrValueProto;
}  // namespace dbroot
}  // namespace keyhole

class ProtoDbrootGenerator {
 private:
  typedef std::set<std::uint32_t>              UsedProviderSet;
  typedef std::map<std::string, std::uint32_t> UsedStringMap;


 protected:
  ProtoDbrootGenerator(const ProtoDbrootContext &context,
                       const QString &locale,
                       const std::string &outfile,
                       bool map_strings_to_ids);

  // Set the supplied string into the given StrignIdOrValueProto message.
  // If this class was constructed with map_strings_to_ids set to true, the
  // string is added to used_strings_ and the id is placed into the
  // message. if map_strings_to_ids is false, the string is added directly
  // into the message.
  void SetProtoStringId(keyhole::dbroot::StringIdOrValueProto* proto,
                        const std::string &string_to_set);

  // Take the fusion-encoded look_at_string and explode it into the proto
  // structure used by the proto buf.
  bool SetLookAtString(keyhole::dbroot::NestedFeatureProto* nested_feature,
                       const std::string& look_at_string);

  // output the generated dbroot using the specified format
  void WriteProtobuf(geProtoDbroot::FileFormat output_format);

  // Mark the supplied provider id as being used by this dbroot.
  void AddProvider(std::uint32_t id) { used_provider_ids_.insert(id); }

 private:
  // called internally before writing
  // takes used_strings_ and emits translation_entry records into the dbroot
  void AddStringTranslations(void);

  // called internally before writing
  // takes used_provider_ids_ and emits provider_entry records into the dbroot
  void AddUsedProviders(void);


 protected:
  const QString             locale_;
  geProtoDbroot             outproto_;

 private:
  const ProtoDbrootContext &context_;
  const std::string         outfile_;
  UsedProviderSet           used_provider_ids_;
  UsedStringMap             used_strings_;
  bool                      map_strings_to_ids_;
};

#endif  // GEO_EARTH_ENTERPRISE_SRC_FUSION_DBROOT_PROTO_DBROOT_GENERATOR_H_
