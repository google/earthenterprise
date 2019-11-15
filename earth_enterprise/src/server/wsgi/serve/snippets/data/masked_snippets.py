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

"""End snippets defaults if they were untouched by the user.

We want to do this to mask out protobuf-defined defaults which will be in effect
in that case. A common case is a URL that points to a Google server.
Another one is that authentication is on by default, which is undesirable.
We do allow users to see and set these fields, they can point urls at Google,
but it's usually the user who doesn't want that.
"""


masked_snippets = {
    # [7] active.
    "end_snippet.reverse_geocoder_url.value":
        "",

    # TODO: move to hard_masked_snippets (make not available
    # for user)?
    # [11] active. empty string by default.
    "end_snippet.default_web_page_intl_url.value":
        "",

    # TODO: move to hard_masked_snippets (make not available
    # for user)?
    # [14] active.
    "end_snippet.user_guide_intl_url.value":
        "",

    # TODO: move to hard_masked_snippets (make not available
    # for user)?
    # [15] active.
    "end_snippet.support_center_intl_url.value":
        "",

    # TODO: move to hard_masked_snippets (make not available
    # for user)?
    # [17] active.
    "end_snippet.support_answer_intl_url.value":
        "",

    # TODO: move to hard_masked_snippets (make not available
    # for user)?
    # [18] active.
    "end_snippet.support_topic_intl_url.value":
        "",

    # TODO: move to hard_masked_snippets (make not available
    # for user)?
    # [19] active.
    "end_snippet.support_request_intl_url.value":
        "",

    # TODO: move to hard_masked_snippets (make not available
    # for user)?
    # [20] active.
    "end_snippet.earth_intl_url.value":
        "",

    # [32] The time machine options.
    # Note: option is not available to user, but located in masked snippets,
    # since it may be set by Fusion. So, we mask time machine option if it is
    # not set.
    # active
    "end_snippet.time_machine_options.server_url":
        "",

    # active
    "end_snippet.time_machine_options.is_timemachine":
        False,

    # [40] active.
    # If not specified, default values in the client will be used.
    "end_snippet.bbs_server_info.name.value":
        "",
    "end_snippet.bbs_server_info.base_url.value":
        "",
    "end_snippet.bbs_server_info.post_wizard_path.value":
        "",
    "end_snippet.bbs_server_info.file_submit_path.value":
        "",

    # [46] active.
    # If empty, service will be unavailable.
    # This should be set to empty for EC clients to disable connection to google
    # services.
    "end_snippet.elevation_service_base_url":
        "",

    # [47] unnecessary. [default = 500].
    # "end_snippet.elevation_profile_query_delay": 500,

    # TODO: move to hard_masked_snippets (make not available
    # for user)?
    # [48] active.
    # If not specified, this URL is built from user_guide_intl_url as
    # user_guide_intl_url + "tutorials/index.html".
    "end_snippet.tutorial_url.value":
        "",

    # TODO: move to hard_masked_snippets (make not available
    # for user)?
    # [49] active.
    # If not specified, this URL is built from user_guide_intl_url as
    # user_guide_intl_url + "ug_keyboard.html".
    "end_snippet.keyboard_shortcuts_url.value":
        "",

    # TODO: move to hard_masked_snippets (make not available
    # for user)?
    # [50] active.
    # If not specified, this URL is built from support_answer_intl_url
    "end_snippet.release_notes_url.value":
        "",

    # [54] active. URL of a page that will be shown when a KML search is
    # performed.
    # Note: for GEE, set to local path in order to override default settings.
    # Note: for GEE, set by default to kmlrender since we don't need to support
    # any searchlets or oneboxes.
    "end_snippet.search_config.kml_search_url.value":
        "/earth/client/kmlrender/index_$[hl].html",

    # [54] active. URL of a page that will be shown when KML is rendered in
    # the search panel.
    # Note: for GEE, set to local path in order to override default settings.
    "end_snippet.search_config.kml_render_url.value":
        "/earth/client/kmlrender/index_$[hl].html",

    # [54] active. URL of a page that will be displayed if a network error or
    # other local error occurs while performing a search.
    "end_snippet.search_config.error_page_url.value":
        "about:blank",

    # [54] active. URL of a page that will be shown when
    # the search history is requested.
    "end_snippet.search_config.search_history_url.value":
        "about:blank",

    # [57] active.
    # This should be set to empty for EC clients to disable connection to
    # google services. If nothing is specified, the client uses
    # "http://maps.google.com/".
    "end_snippet.google_maps_url.value":
        "",

    # TODO: move to hard_masked_snippets (make not available
    # for user)?
    # [59] active.
    "end_snippet.privacy_policy_url.value":
        "",

    # [63] active.
    "end_snippet.show_signin_button":
        False,

    # TODO: move to hard_masked_snippets (make not available
    # for user)?
    # [64] active.
    "end_snippet.startup_tips_intl_url.value":
        "",
}
