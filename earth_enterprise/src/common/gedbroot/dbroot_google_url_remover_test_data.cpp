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


#include <cstddef>
#include "common/gedbroot/dbroot_google_url_remover_test_data.h"


const char* TEST_DBROOT_BASE =
    "imagery_present: true\n"
    "provider_info {\n"
    "  provider_id: 2\n"
    "  copyright_string {\n"
    "    value: \"New Zealand Data\"\n"
    "  }\n"
    "}\n"
    "end_snippet {\n"
    "  model {\n"
    "    radius: 6371.009766\n"
    "  }\n"
    "  auth_server_url {\n"
    "    value: \"http://auth.keyhole.com\"\n"
    "  }\n"
    "  mfe_domains {\n"
    "    country_code: \"US\"\n"
    "    domain_name: \"maps.google.com\"\n"
    "    supported_features: GEOCODING\n"
    "    supported_features: LOCAL_SEARCH\n"
    "    supported_features: DRIVING_DIRECTIONS\n"
    "  }\n"
    "  mfe_domains {\n"
    "    country_code: \"DE\"\n"
    "    domain_name: \"maps.google.de\"\n"
    "    supported_features: GEOCODING\n"
    "    supported_features: LOCAL_SEARCH\n"
    "    supported_features: DRIVING_DIRECTIONS\n"
    "  }\n"
    // We want to make sure we aren't just deleting them all.
    "  mfe_domains {\n"
    "    country_code: \"XX\"\n"
    "    domain_name: \"nasa.gov\"\n"
    "    supported_features: GEOCODING\n"
    "    supported_features: LOCAL_SEARCH\n"
    "    supported_features: DRIVING_DIRECTIONS\n"
    "  }\n"
    "  mfe_domains {\n"
    "    country_code: \"GB\"\n"
    "    domain_name: \"maps.google.co.uk\"\n"
    "    supported_features: GEOCODING\n"
    "    supported_features: LOCAL_SEARCH\n"
    "    supported_features: DRIVING_DIRECTIONS\n"
    "  }\n"
    "}\n"
    "language: \".\"\n"
    "database_version {\n"
    "  quadtree_version: 3\n"
    "}\n"
    "proto_imagery: true\n"
    "database_name {\n"
    "  value: \"New Zealand Relief\"\n"
    "}\n";


const char* CANON_CLEANED_ALLFIELDS =
    // The fields that are "" have defaults that contain 'google'
    // (or 'keyhole'). We don't know how the client will react to these
    // nothing-filled fields, actually!
    "imagery_present: true\n"
    "provider_info {\n"
    "  provider_id: 2\n"
    "  copyright_string {\n"
    "    value: \"New Zealand Data\"\n"
    "  }\n"
    "}\n"
    // This required field was added for correctness; our remover doesn't know
    // it wasn't there all along, so it leaves it, as it should.
    "nested_feature {\n"
    "  channel_id: 0\n"
    "}\n"
    "end_snippet {\n"
    "  model {\n"
    "    radius: 6371.009766\n"
    "  }\n"
    "  disable_authentication: true\n"
    "  mfe_domains {\n"
    "    country_code: \"XX\"\n"
    "    domain_name: \"nasa.gov\"\n"
    "    supported_features: GEOCODING\n"
    "    supported_features: LOCAL_SEARCH\n"
    "    supported_features: DRIVING_DIRECTIONS\n"
    "  }\n"
    // We wrote to all fields, and don't touch these two permitted ones so,
    // they're here.
    "  free_license_url {\n"
    "    value: \"www.google.com\"\n"
    "  }\n"
    "  pro_license_url {\n"
    "    value: \"www.google.com\"\n"
    "  }\n"
    "  client_options {\n"
    "    js_bridge_request_whitelist: \"\"\n"
    "  }\n"
    // is_visible was added for correctness, it was left because it, properly,
    // was untouched by the remover.
    "  search_tab {\n"
    "    is_visible: false\n"
    "  }\n"
    // planetary_database is removed because, although we added a
    // required sibling, the url part was also required. Removing that
    // means we had to remove the whole planetary_database.
    "  autopia_options {\n"
    "    metadata_server_url: \"\"\n"
    "    depthmap_server_url: \"\"\n"
    "  }\n"
    "  search_info {\n"
    "    default_url: \"\"\n"
    "  }\n"
    "  elevation_service_base_url: \"\"\n"
    "}\n"
    "language: \".\"\n"
    "database_version {\n"
    "  quadtree_version: 3\n"
    "}\n"
    "proto_imagery: true\n"
    "database_name {\n"
    "  value: \"New Zealand Relief\"\n"
    "}\n";


const char* HAND_DBROOTS[] = {
  // kActualExample
  "imagery_present: true\n"
  "provider_info {\n"
  "  provider_id: 2\n"
  "  copyright_string {\n"
  "    value: \"New Zealand Data\"\n"
  "  }\n"
  "}\n"
  "end_snippet {\n"
  "  model {\n"
  "    radius: 6371.009766\n"
  "  }\n"
  "  auth_server_url {\n"
  "    value: \"http://auth.keyhole.com\"\n"
  "  }\n"
  "  mfe_domains {\n"
  "    country_code: \"US\"\n"
  "    domain_name: \"maps.google.com\"\n"
  "    supported_features: GEOCODING\n"
  "    supported_features: LOCAL_SEARCH\n"
  "    supported_features: DRIVING_DIRECTIONS\n"
  "  }\n"
  "  mfe_domains {\n"
  "    country_code: \"DE\"\n"
  "    domain_name: \"maps.google.de\"\n"
  "    supported_features: GEOCODING\n"
  "    supported_features: LOCAL_SEARCH\n"
  "    supported_features: DRIVING_DIRECTIONS\n"
  "  }\n"
  "  mfe_domains {\n"
  "    country_code: \"GB\"\n"
  "    domain_name: \"maps.google.co.uk\"\n"
  "    supported_features: GEOCODING\n"
  "    supported_features: LOCAL_SEARCH\n"
  "    supported_features: DRIVING_DIRECTIONS\n"
  "  }\n"
  "  mfe_domains {\n"
  "    country_code: \"ES\"\n"
  "    domain_name: \"maps.google.es\"\n"
  "    supported_features: GEOCODING\n"
  "    supported_features: LOCAL_SEARCH\n"
  "    supported_features: DRIVING_DIRECTIONS\n"
  "  }\n"
  "  mfe_domains {\n"
  "    country_code: \"FR\"\n"
  "    domain_name: \"maps.google.fr\"\n"
  "    supported_features: GEOCODING\n"
  "    supported_features: LOCAL_SEARCH\n"
  "    supported_features: DRIVING_DIRECTIONS\n"
  "  }\n"
  "  mfe_domains {\n"
  "    country_code: \"IT\"\n"
  "    domain_name: \"maps.google.it\"\n"
  "    supported_features: GEOCODING\n"
  "    supported_features: LOCAL_SEARCH\n"
  "    supported_features: DRIVING_DIRECTIONS\n"
  "  }\n"
  "  mfe_domains {\n"
  "    country_code: \"JP\"\n"
  "    domain_name: \"maps.google.jp\"\n"
  "    supported_features: GEOCODING\n"
  "    supported_features: LOCAL_SEARCH\n"
  "    supported_features: DRIVING_DIRECTIONS\n"
  "  }\n"
  "  mfe_domains {\n"
  "    country_code: \"NL\"\n"
  "    domain_name: \"maps.google.nl\"\n"
  "    supported_features: GEOCODING\n"
  "    supported_features: LOCAL_SEARCH\n"
  "    supported_features: DRIVING_DIRECTIONS\n"
  "  }\n"
  "  mfe_domains {\n"
  "    country_code: \"CN\"\n"
  "    domain_name: \"maps.google.com\"\n"
  "    supported_features: GEOCODING\n"
  "    supported_features: LOCAL_SEARCH\n"
  "    supported_features: DRIVING_DIRECTIONS\n"
  "  }\n"
  "  reverse_geocoder_url {\n"
  "    value: \"http://maps.google.com/maps/api/earth/GeocodeService.Search\"\n"
  "  }\n"
  "  sky_database_url {\n"
  "    value: \"http://khmdb.google.com?db=sky\"\n"
  "  }\n"
  "  default_web_page_intl_url {\n"
  "    value: \"http://www.google.com/intl/%1/\"\n"
  "  }\n"
  "  start_up_tips_url {\n"
  "    value: \"http://earth.google.com/intl/%1/tips/v43/\"\n"
  "  }\n"
  "  user_guide_intl_url {\n"
  "    value: \"http://earth.google.com/intl/%1/userguide/v4/\"\n"
  "  }\n"
  "  support_center_intl_url {\n"
  "    value: \"http://earth.google.com/support/?hl=%1\"\n"
  "  }\n"
  "  business_listing_intl_url {\n"
  "    value: \"http://www.google.com/local/add/login?hl=%3&gl=%2\"\n"
  "  }\n"
  "  support_answer_intl_url {\n"
  "    value: \"http://earth.google.com/support/bin/answer.py?answer=%4&"
  // ct'd
  "hl=%1\"\n"
  "  }\n"
  "  support_topic_intl_url {\n"
  "    value: \"http://earth.google.com/support/bin/topic.py?topic=%4&hl=%1\"\n"
  "  }\n"
  "  support_request_intl_url {\n"
  "    value: \"http://earth.google.com/support/bin/request.py?hl=%1\"\n"
  "  }\n"
  "  earth_intl_url {\n"
  "    value: \"http://earth.google.com/intl/$[hl]/\"\n"
  "  }\n"
  "  add_content_url {\n"
  "    value: \"http://earth.google.com/ig/directory?pid=earth&synd=earth&"
  // ct'd
  "cat=featured&hl=$[hl]&gl=%2\"\n"
  "  }\n"
  "  sketchup_not_installed_url {\n"
  "    value: \"http://sketchup.google.com/intl/$[hl]/modeling.html\"\n"
  "  }\n"
  "  sketchup_error_url {\n"
  "    value: \"http://sketchup.google.com/intl/$[hl]/gemodelerror.html\"\n"
  "  }\n"
  "  num_pro_start_up_tips: 8\n"
  "  pro_start_up_tips_url {\n"
  "    value: \"http://earth.google.com/intl/%1/tips/v5/pro/\"\n"
  "  }\n"
  "}\n"
  "language: \".\"\n"
  "database_version {\n"
  "  quadtree_version: 3\n"
  "}\n"
  "proto_imagery: true\n"
  "database_name {\n"
  "  value: \"New Zealand Relief\"\n"
  "}\n",

  // kDisableAuthenticationRequirement
  "end_snippet {\n"
  "  disable_authentication: true\n"
  "}\n",

  // kLeavesOptionalSiblings
  "end_snippet {\n"
  "  reverse_geocoder_url {\n"
  "    string_id: 99\n"
  "    value: \"www.keyhole.com\"\n"
  "  }\n"
  "}\n",

  NULL
};


const char* CANON_CLEANED_HAND_DBROOTS[] = {
  // kActualExample
  "imagery_present: true\n"
  "provider_info {\n"
  "  provider_id: 2\n"
  "  copyright_string {\n"
  "    value: \"New Zealand Data\"\n"
  "  }\n"
  "}\n"
  "end_snippet {\n"
  "  model {\n"
  "    radius: 6371.009766\n"
  "  }\n"
  "  disable_authentication: true\n"
  "  client_options {\n"
  "    js_bridge_request_whitelist: \"\"\n"
  "  }\n"
  "  autopia_options {\n"
  "    metadata_server_url: \"\"\n"
  "    depthmap_server_url: \"\"\n"
  "  }\n"
  "  search_info {\n"
  "    default_url: \"\"\n"
  "  }\n"
  "  elevation_service_base_url: \"\"\n"
  "  num_pro_start_up_tips: 8\n"
  "}\n"
  "language: \".\"\n"
  "database_version {\n"
  "  quadtree_version: 3\n"
  "}\n"
  "proto_imagery: true\n"
  "database_name {\n"
  "  value: \"New Zealand Relief\"\n"
  "}\n",

  // kDisableAuthenticationRequirement
  "end_snippet {\n"
  // the point of the test
  "  disable_authentication: true\n"
  "  client_options {\n"
  "    js_bridge_request_whitelist: \"\"\n"
  "  }\n"
  "  autopia_options {\n"
  "    metadata_server_url: \"\"\n"
  "    depthmap_server_url: \"\"\n"
  "  }\n"
  "  search_info {\n"
  "    default_url: \"\"\n"
  "  }\n"
  "  elevation_service_base_url: \"\"\n"
  "}\n",

  // kLeavesOptionalSiblings
  "end_snippet {\n"
  "  disable_authentication: true\n"
  // That reverse_geocoder_url & string_id are left is the point.
  "  reverse_geocoder_url {\n"
  "    string_id: 99\n"
  "  }\n"
  "  client_options {\n"
  "    js_bridge_request_whitelist: \"\"\n"
  "  }\n"
  "  autopia_options {\n"
  "    metadata_server_url: \"\"\n"
  "    depthmap_server_url: \"\"\n"
  "  }\n"
  "  search_info {\n"
  "    default_url: \"\"\n"
  "  }\n"
  "  elevation_service_base_url: \"\"\n"
  "}\n",

  NULL
};


// Note that these don't include mfe_domains. This is because by good
// luck, these are all pretty simple to create, but, the mfe_domains,
// the domain part having a sibling that is also required, isn't. So
// we deal with that separately.
const char* GOOGLEFREE_NON_DEFAULT_FIELD_PATHS[] = {
  // has a required sibling field, channel_id. We hardwire handling it.
  "nested_feature[].kml_url.value",
  "nested_feature[].database_url",
  // has default: end_snippet.client_options.js_bridge_request_whitelist[http://*.google.com/*]
  "end_snippet.auth_server_url.value",
//  "end_snippet.mfe_domains[].country_code!",
  "end_snippet.mfe_domains[].domain_name!",
  "end_snippet.ads_url_patterns",
  "end_snippet.reverse_geocoder_url.value",
  "end_snippet.sky_database_url.value",
  "end_snippet.default_web_page_intl_url.value",
  "end_snippet.start_up_tips_url.value",
  "end_snippet.pro_start_up_tips_url.value",
  "end_snippet.user_guide_intl_url.value",
  "end_snippet.support_center_intl_url.value",
  "end_snippet.business_listing_intl_url.value",
  "end_snippet.support_answer_intl_url.value",
  "end_snippet.support_topic_intl_url.value",
  "end_snippet.support_request_intl_url.value",
  "end_snippet.earth_intl_url.value",
  "end_snippet.add_content_url.value",
  "end_snippet.sketchup_not_installed_url.value",
  "end_snippet.sketchup_error_url.value",
//  "end_snippet.free_license_url.value",
//  "end_snippet.pro_license_url.value",
  "end_snippet.tutorial_url.value",
  "end_snippet.keyboard_shortcuts_url.value",
  "end_snippet.release_notes_url.value",
  "end_snippet.diorama_description_url_base.value",
  "end_snippet.diorama_blacklist_url.value",
  "end_snippet.client_options.precipitations_options.image_url",
  "end_snippet.client_options.precipitations_options.clouds_layer_url",
  "end_snippet.time_machine_options.server_url",
  "end_snippet.search_tab[].base_url",
  "end_snippet.cobrand_info[].logo_url!",
  "end_snippet.valid_database[].database_url!",
  "end_snippet.deauth_server_url.value",
  "end_snippet.bbs_server_info.base_url.value",
  "end_snippet.data_error_server_info.base_url.value",
  "end_snippet.planetary_database[].url!.value",
  "end_snippet.log_server.url.value",
  // has default: end_snippet.autopia_options.metadata_server_url[http://cbk0.google.com/cbk]
  // has default: end_snippet.autopia_options.depthmap_server_url[http://cbk0.google.com/cbk]
  "end_snippet.autopia_options.coverage_overlay_url",
  "end_snippet.search_config.search_server[].url.value",
  "end_snippet.search_config.search_server[].html_transform_url.value",
  "end_snippet.search_config.search_server[].kml_transform_url.value",
  "end_snippet.search_config.search_server[].supplemental_ui.url.value",
  "end_snippet.search_config.search_server[].searchlet[].url.value",
  "end_snippet.search_config.onebox_service[].service_url.value",
  "end_snippet.search_config.kml_search_url.value",
  "end_snippet.search_config.kml_render_url.value",
  "end_snippet.search_config.search_history_url.value",
  "end_snippet.search_config.error_page_url.value",
  // has default: end_snippet.search_info.default_url[http://maps.google.com/maps]
  // has default: end_snippet.elevation_service_base_url[http://maps.google.com/maps/api/elevation/]
  "end_snippet.pro_upgrade_url.value",
  "end_snippet.earth_community_url.value",
  "end_snippet.google_maps_url.value",
  "end_snippet.sharing_url.value",
  "end_snippet.privacy_policy_url.value",
  "dbroot_reference[].url!",
  NULL
};

const char* GOOGLISH_NON_DEFAULT_FIELD_PATHS[] = {
  "end_snippet.free_license_url.value",
  "end_snippet.pro_license_url.value",
  NULL
};

// Fields with defaults, that have to be 'plugged' / masked with emptiness,
// to prevent the default showing through.
const char* GOOGLE_DEFAULTED_FIELD_PATHS[] = {
  // [http://*.google.com/*]
  "end_snippet.client_options.js_bridge_request_whitelist",
  // [http://cbk0.google.com/cbk]
  "end_snippet.autopia_options.metadata_server_url",
  // [http://cbk0.google.com/cbk]
  "end_snippet.autopia_options.depthmap_server_url",
  // [http://maps.google.com/maps]
  "end_snippet.search_info.default_url",
  // [http://maps.google.com/maps/api/elevation/]
  "end_snippet.elevation_service_base_url",
  NULL
};
