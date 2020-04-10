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

// Simple class to implement the combining of multiple proto dbroots
// It almost a simple protobuf combine. But since Combine() just
// concatenates repeated fields, we need to go in a remove any duplicate
// providers.

#include "fusion/dbroot/proto_dbroot_combiner.h"
#include <cstdint>

ProtoDbrootCombiner::ProtoDbrootCombiner(void) {
}

ProtoDbrootCombiner::~ProtoDbrootCombiner(void) {
  // define in .cc to reduce linker dependencies
}

void ProtoDbrootCombiner::AddDbroot(const std::string &filename,
                                    geProtoDbroot::FileFormat input_format) {
  outproto_.Combine(filename, input_format);
}

void ProtoDbrootCombiner::RemoveDuplicateProviders(void) {
  // Because the vector, imagery, & terrain dbroots are all emitted
  // independently from each other. They each might have a provider entry
  // for the same provider. We only want to send each provider once.
  // This routine looks for duplicates and keeps the "best".
  typedef std::map<std::int32_t, keyhole::dbroot::ProviderInfoProto> ProviderMap;
  ProviderMap keep_providers;

  // loop through all providers in the output protodbroot
  for (int i = 0; i < outproto_.provider_info_size(); ++i) {
    const ::keyhole::dbroot::ProviderInfoProto &curr =
        outproto_.provider_info(i);

    // see if we already have one with this id
    ProviderMap::iterator found = keep_providers.find(curr.provider_id());
    if (found == keep_providers.end()) {
      // none found, add this new one
      keep_providers[curr.provider_id()] = curr;
    } else {
      // we have one, let's see if the new one is better
      if (curr.has_copyright_string()) {
        if (!found->second.has_copyright_string()) {
          // existing one doesn't have a copyright string, take the new one
          keep_providers[curr.provider_id()] = curr;
        } else if (!found->second.copyright_string().has_string_id() &&
                   curr.copyright_string().has_string_id()) {
          // new one uses string table, old one doesn't, take new one
          keep_providers[curr.provider_id()] = curr;
        }
      }
    }
  }

  // now clear out the old providers from the dbroot proto
  outproto_.clear_provider_info();

  // and copy back in the ones we want
  if (keep_providers.size()) {
    for (ProviderMap::const_iterator prov_iterator = keep_providers.begin();
         prov_iterator != keep_providers.end(); ++prov_iterator) {
      *outproto_.mutable_provider_info()->Add() = prov_iterator->second;
    }
  }
}

void ProtoDbrootCombiner::Write(const std::string &filename,
                                geProtoDbroot::FileFormat output_format) {
  RemoveDuplicateProviders();

  outproto_.Write(filename, output_format);
}
