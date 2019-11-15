#!/usr/bin/env python2.7
#
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Defaults of fields not available for editing.

Every field in here is suppressed when the snippet fields are shown to the user.
When any set of snippets is written to a dbroot, the fields are set to
the value on the right.

Note: Hard-masked snippets list may only contain one item of repeated snippet.
Repeated element of snippet path is specified w/out index.
e.g. filmstrip_config.
"""

hard_masked_snippets = {
    # [2] active.
    "end_snippet.auth_server_url.value":
        "",

    # [3] active.
    "end_snippet.disable_authentication":
        True,

    # [4] unnecessary.
    # DEPRECATED in Earth 6.1 and higher in favor of search_config.
    # "end_snippet.mfe_domains.domain_name": "",

    # [9] active.
    "end_snippet.sky_database_is_available":
        False,
    # [10] active.
    "end_snippet.sky_database_url.value":
        "",

    # [13] unnecessary. DEPRECATED as of version 7.0.
    # "end_snippet.start_up_tips_url.value": "",

    # [52] unnecessary. DEPRECATED as of version 7.0.
    # "end_snippet.pro_start_up_tips_url.value": "",

    # [16] active.
    "end_snippet.business_listing_intl_url.value":
        "",

    # [21] active
    "end_snippet.add_content_url.value":
        "",

    # [22] unnecessary. DEPRECATED.
    # "end_snippet.sketchup_not_installed_url.value": "",

    # [23] unnecessary. DEPRECATED.
    # "end_snippet.sketchup_error_url.value": "",

    # [24] active.
    "end_snippet.free_license_url.value":
        "",

    # [25] active.
    "end_snippet.pro_license_url.value":
        "",

    # [28] active.
    "end_snippet.diorama_description_url_base.value":
        "",

    # [30] client_options snippet.
    # unnecessary. DEPRECATED after Earth 5.1.
    # "end_snippet.client_options.disable_embedded_browser_vista": False,
    # active.
    "end_snippet.client_options.use_protobuf_quadtree_packets":
        False,
    # active  [default = true]
    "end_snippet.client_options.use_extended_copyright_ids":
        False,
    # client_options.precipitations_options snippet.
    # active.
    "end_snippet.client_options.precipitations_options.image_url":
        "",
    # active.
    "end_snippet.client_options.precipitations_options.clouds_layer_url":
        "",
    # active.
    # Controls whether or not the "Show in maps button" should be shown. On by
    # default, typically set to false for EC databases. [default = true]
    "end_snippet.client_options.show_2d_maps_icon":
        False,
    # not active. [default = false].
    # "end_snippet.client_options.disable_internal_browser": False,
    # not active.
    # "end_snippet.client_options.internal_browser_blacklist": "",
    # not active. [default = "*"]
    # "end_snippet.client_options.internal_browser_origin_whitelist": "*",
    # active. This should be set to an empty string for Fusion dbroots.
    "end_snippet.client_options.js_bridge_request_whitelist":
        "",

    # TODO: move to masked_snippets (make it available for user)?
    # [31] The fetching_options snippet.
    # active.
    # List of domains for which the client should use the HTTPS protocol
    # instead of standard HTTP, when client option is enabled (supported as
    # of Earth 7.1).
    "end_snippet.fetching_options.domains_for_https":
        "",
    # active. It is empty string by default.
    # List of hosts for which the client should use regular HTTP instead of
    # HTTPS.  This is effectively a list of exceptions to the domains_for_https
    # field above.  Supported as of Earth 7.1.1.x for x > 1888.
    "end_snippet.fetching_options.hosts_for_http":
        "",

    # [38] not active.
    # Url of deauthentication server (for paying customers only)
    # "end_snippet.deauth_server_url.value": "",

    # [41] active
    # If not specified, default values in the client will be used.
    "end_snippet.data_error_server_info.name.value":
        "",
    "end_snippet.data_error_server_info.base_url.value":
        "",
    "end_snippet.data_error_server_info.post_wizard_path.value":
        "",
    "end_snippet.data_error_server_info.file_submit_path.value":
        "",

    # [43] The log_server snippet.
    # unnecessary. [default = false].
    # "end_snippet.log_server.enable": False,
    # unnecessary. [default = 1].
    # "end_snippet.log_server.throttling_factor": 1,
    # active.
    "end_snippet.log_server.url.value":
        "",

    # [44] The autopia_options snippet.
    # Note: Candidate for moving to masked_snippets, but only if we can verify
    # support in GEE. (See http://b/4364808).
    # active.
    "end_snippet.autopia_options.metadata_server_url":
        "",
    # active.
    "end_snippet.autopia_options.depthmap_server_url":
        "",

    # TODO: if we add the coverage_overlay_url snippet,
    # EC 7.0.3.8542(linux), EC 7.1.1888(OS X) crash while opening database.
    # not active. [default = ""].
    # "end_snippet.autopia_options.coverage_overlay_url": "",

    # unnecessary, have no defaults.
    # "end_snippet.autopia_options.max_imagery_qps": 0,
    # "end_snippet.autopia_options.max_metadata_depthmap_qps": 0,

    # unnecessary: DEPRECATED and unused since 6.1.
    # "end_snippet.search_info.default_url": "",

    # [55] active.
    "end_snippet.pro_upgrade_url.value":
        "",

    # [56] active.
    "end_snippet.earth_community_url.value":
        "",

    # [58] active.
    "end_snippet.sharing_url.value":
        "",

    # [60] active. [default = false].
    "end_snippet.do_gplus_user_check":
        False,

    # [61] active.
    "end_snippet.rocktree_data_proto.url.value":
        "",

    # [62] filmstrip_config
    # not active.
    # "end_snippet.filmstrip_config.requirements.probability": "",
    # "end_snippet.filmstrip_config.requirements.required_client_capabilities": "",
    # "end_snippet.filmstrip_config.requirements.required_client_ver": "",
    # "end_snippet.filmstrip_config.requirements.required_user_agent": "",
    # "end_snippet.filmstrip_config.requirements.required_vram": "",
    "end_snippet.filmstrip_config.alleycat_url_template.value":
        "",
    "end_snippet.filmstrip_config.fallback_alleycat_url_template.value":
        "",
    # unnecessary. DEPRECATED.
    # "end_snippet.filmstrip_config.metadata_url_template.value": "",
    # unnecessary. DEPRECATED.
    # "end_snippet.filmstrip_config.thumbnail_url_template.value": "",
    # unnecessary. DEPRECATED.
    # "end_snippet.filmstrip_config.kml_url_template.value": "",
    # active.
    "end_snippet.filmstrip_config.featured_tours_url.value":
        "",
    # active.
    "end_snippet.filmstrip_config.enable_viewport_fallback":
        False,
    # unnecessary. DEPRECATED.
    # "end_snippet.filmstrip_config.viewport_fallback_distance": 0,
    # not active
    # "end_snippet.filmstrip_config.imagery_type.imagery_type_id": 0,
    # not active
    # "end_snippet.filmstrip_config.imagery_type.imagery_type_label": "",
    "end_snippet.filmstrip_config.imagery_type.metadata_url_template.value":
        "",
    "end_snippet.filmstrip_config.imagery_type.thumbnail_url_template.value":
        "",
    "end_snippet.filmstrip_config.imagery_type.kml_url_template.value":
        "",

    # [65] active.
    "end_snippet.pro_measure_upsell_url.value":
        "",

    # [66] active.
    "end_snippet.pro_print_upsell_url.value":
        "",

    # [67] active.
    "end_snippet.star_data_proto.url.value":
        "",

    # [68] active.
    "end_snippet.feedback_url.value":
        ""
}
