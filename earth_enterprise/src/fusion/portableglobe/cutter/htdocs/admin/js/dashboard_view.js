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

// TODO: High-level file comment.
/*
  This file creates the UI for the Dashboard mode of the GEE Server
  Admin UI.  The purpose of the Dashboard view is to give the user some
  data regarding the usage of their Server account.  There are three other
  files for the different modes of the Server UI, all in this folder:
    - database_view.js
    - search_view.js
    - snippet_view.js
*/

gees.admin.dashboardView = {

  populate: function() {
    gees.dom.show('DashboardPanel');
    // Update Fusion information.
    gees.dom.setHtml('PublishedFusion', gees.assets.totalPublishedFusion);
    gees.dom.setHtml('TotalFusion', gees.assets.fusionDatabaseCount);
    gees.dom.setHtml(
        'FusionData', gees.tools.getDisplaySize(gees.assets.fusionData, true));
    // Update Portable information.
    gees.dom.setHtml('PublishedPortables', gees.assets.totalPublishedPortables);
    gees.dom.setHtml('TotalPortables', gees.assets.totalPortableDatabases);
    gees.dom.setHtml('PortableData', gees.tools.getDisplaySize(
        gees.assets.portableData, true));
    // Update other server details.
    gees.dom.setHtml('TotalSearchTabs', gees.assets.searchDefs.length);
    gees.dom.setHtml('CutterStatus', gees.admin.cutterStatus);
    gees.dom.setHtml('TotalSnippetProfiles', gees.assets.snippets.length || 0);
  },

  init: function() {
    gees.admin.mode = gees.admin.modes.dashboard;
    gees.admin.switchViewHelper();
    gees.admin.setHeaderLinks();
    this.populate();
  }
};
