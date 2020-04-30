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

// Base class for RasterDbrootGenerator and VectorDbrootGenerator
// See header file for more complete description

#include "fusion/dbroot/proto_dbroot_generator.h"
#include "google/protobuf/stubs/strutil.h"  // for UnescapeCEscapeString
#include "common/khStringUtils.h"
#include "common/khstrconv.h"
#include "fusion/dbroot/proto_dbroot_context.h"

ProtoDbrootGenerator::ProtoDbrootGenerator(const ProtoDbrootContext &context,
                                           const QString &locale,
                                           const std::string &outfile,
                                           bool map_strings_to_ids) :
    locale_(locale),
    context_(context),
    outfile_(outfile),
    map_strings_to_ids_(map_strings_to_ids) {
}

void ProtoDbrootGenerator::SetProtoStringId(
    keyhole::dbroot::StringIdOrValueProto* proto,
    const std::string &string_to_set) {
  if (map_strings_to_ids_) {
    UsedStringMap::const_iterator found = used_strings_.find(string_to_set);
    std::uint32_t id;
    if (found != used_strings_.end()) {
      id = found->second;
    } else {
      id = used_strings_.size() + 1;  // we want to start with id==1
      used_strings_[string_to_set] = id;
    }
    proto->clear_value();
    proto->set_string_id(id);
  } else {
    proto->clear_string_id();
    proto->set_value(string_to_set);
  }
}

namespace {
inline float ToFloat(const std::string &str) {
  float tmp;
  FromString(str, tmp);
  return tmp;
}
}

bool ProtoDbrootGenerator::SetLookAtString(
    keyhole::dbroot::NestedFeatureProto* nested_feature,
    const std::string& look_at_string) {
  if (look_at_string.empty()) {
    return true;
  }

  std::vector<std::string> look_at;
  TokenizeString(look_at_string, look_at, "|");

  // Valid look_at strings will have at least a latitude and longitude.
  if (look_at.size() < 2) {
    return false;
  }

  keyhole::dbroot::LookAtProto* look_at_proto =
      nested_feature->mutable_look_at();
  look_at_proto->set_longitude(ToFloat(look_at[0]));
  look_at_proto->set_latitude(ToFloat(look_at[1]));

  if (look_at.size() >= 3)
    look_at_proto->set_range(ToFloat(look_at[2]));
  if (look_at.size() >= 4)
    look_at_proto->set_tilt(ToFloat(look_at[3]));
  if (look_at.size() >= 5)
    look_at_proto->set_heading(ToFloat(look_at[4]));

  return true;
}

void ProtoDbrootGenerator::AddStringTranslations(void) {
  for (UsedStringMap::const_iterator i = used_strings_.begin();
       i != used_strings_.end(); ++i) {
    keyhole::dbroot::StringEntryProto* string_entry =
        outproto_.add_translation_entry();
    string_entry->set_string_value(i->first);
    string_entry->set_string_id(i->second);
  }
}

void ProtoDbrootGenerator::AddUsedProviders(void) {
  for (std::set<std::uint32_t>::const_iterator id = used_provider_ids_.begin();
       id != used_provider_ids_.end(); ++id) {
    const gstProvider *provider = context_.GetProvider(*id);
    if (provider) {
      keyhole::dbroot::ProviderInfoProto* provider_proto =
          outproto_.add_provider_info();
      provider_proto->set_provider_id(provider->id);
      provider_proto->set_vertical_pixel_offset(
          provider->copyrightVerticalPos);

      // Provider copyright strings (and only copyright strings) allow
      // C-style escape sequences (e.g. \xC2\xA9 for the copyright symbol)
      // This is done to make it easier for customers to input characters
      // they can't get from their keyboard.
      // We want to just emit pure UTF8 in the protobuf, so we replace the
      // escape sequences here.

      // NOTE: UnescapeCEscapeString doesn't validate that the hex/oct
      // escape sequences are valid UTF-8. That is the responsibility of
      // the user.
      std::string output_copyright =
          google::protobuf::UnescapeCEscapeString(
              std::string(provider->copyright.utf8()));

      SetProtoStringId(provider_proto->mutable_copyright_string(),
                       output_copyright);
    }
  }
}

void ProtoDbrootGenerator::WriteProtobuf(
    geProtoDbroot::FileFormat output_format) {
  // Add the things that this class has been tracking
  AddUsedProviders();
  AddStringTranslations();

  outproto_.Write(outfile_, output_format);
}
