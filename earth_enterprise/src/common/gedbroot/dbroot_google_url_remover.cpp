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


#include "common/gedbroot/dbroot_google_url_remover.h"

#include <ctype.h>
#include <cassert>
#include <cstring>

#include <set>
#include <string>

#include "common/gedbroot/proto_dbroot.h"
#include "common/gedbroot/protobuf_walker.h"
#include "common/notify.h"
#include "common/khStringUtils.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/message.h"

namespace PBuf = google::protobuf;
using std::string;
using std::set;


#define ARRAY_SIZE(SOME_ARRAY) (sizeof(SOME_ARRAY) / sizeof(SOME_ARRAY[0]))

// Policy - fields that contain urls or domains with 'google' or
// 'keyhole' by default, which we wish to hide.
const char* GOOGLE_DEFAULTED_FIELDS[] = {
  // The sole field path without 'url' or 'domain' in its name.
  // [default = "http://*.google.com/*"]
  "end_snippet.client_options.js_bridge_request_whitelist",
  // [default = "http://cbk0.google.com/cbk"];
  "end_snippet.autopia_options.metadata_server_url",
  // [default = "http://cbk0.google.com/cbk"];
  "end_snippet.autopia_options.depthmap_server_url",
  // [default = "http://maps.google.com/maps"];
  "end_snippet.search_info.default_url",
  // [default = "http://maps.google.com/maps/api/elevation/"];
  "end_snippet.elevation_service_base_url"
};


// Callbacks that check given fields for given domain patterns; the
// 'default masking' value is a parameter as well (if you clear a
// field with a default, a request for that specific field will
// produce the default).
class UnwantedStringRemovingCallbacks : public ProtobufWalkerCallbacks {
  typedef set<string> StringSet;

 public:
  UnwantedStringRemovingCallbacks(
      char const* google_free_fields[], int google_free_fields_size,
      const char* masking_value)
      : masking_value_(masking_value) {
    Init(google_free_fields, google_free_fields_size);
  }

  void Init(char const* google_free_fields[], int google_free_fields_size) {
    InitGoogleFreeFieldset(google_free_fields, google_free_fields_size);
  }

  // Our 'event handler' that gets called on every string.
  virtual void on_STRING(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_STRING>::Type>&
      field) {
    // One of the field paths we're interested in?
    if (fieldset_.find(field.Fieldpath()) != fieldset_.end()) {
      if (!field.HasDefault()) {
        notify(NFY_WARN, "Field '%s' has no default; programming error or,"
               " wrong kind of protobuf.\n(No action taken.)",
               field.Fieldpath().c_str());
      } else if (!field.field_info().HasNonDefaultData()) {
        field.set(masking_value_);
      }
    }
  }

  virtual void on_BOOL(
      FieldAccessor_T<cpp_type<PBuf::FieldDescriptor::TYPE_BOOL>::Type>&
      field) {
    if (field.field_info().fieldpath() ==
        "end_snippet.disable_authentication") {
      field.set(true);
    }
  }

 private:
  // Convert fieldnames from array to <set>.
  void InitGoogleFreeFieldset(const char* google_free_fields[],
                              int google_free_fields_size) {
    for (const char** pfield = google_free_fields,
             **end = google_free_fields + google_free_fields_size;
         pfield < end; ++pfield) {
      fieldset_.insert(*pfield);
    }
  }

  StringSet fieldset_;
  const char* masking_value_;
};

bool RemoveUnwantedDomains(geProtoDbroot* dbroot,
                           const char* fieldpaths[],
                           int fieldpaths_size,
                           const char* mask_for_defaults) {
  bool dbroot_is_valid;
  if (!dbroot->IsInitialized()) {
    dbroot_is_valid = false;
  } else {
    UnwantedStringRemovingCallbacks url_removing_callbacks(
        fieldpaths, fieldpaths_size,
        mask_for_defaults);
    dbroot_is_valid = EditProtobuf("", dbroot, &url_removing_callbacks);
    if (dbroot_is_valid) {
      dbroot_is_valid = dbroot->IsInitialized();
    }
  }
  return dbroot_is_valid;
}

// and keyhole.*, as it happens.
// Returns whether the dbroot is still valid.
bool RemoveUnwantedGoogleUrls(geProtoDbroot* dbroot) {
  bool is_valid = RemoveUnwantedDomains(
      dbroot,
      GOOGLE_DEFAULTED_FIELDS, ARRAY_SIZE(GOOGLE_DEFAULTED_FIELDS),
      "");
  return is_valid;
}

#undef ARRAY_SIZE
