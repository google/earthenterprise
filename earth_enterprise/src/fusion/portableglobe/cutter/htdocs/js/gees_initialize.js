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
var gees = gees || {};
var GEE_HOST = window.location.host;
var GEE_BASE_URL = window.location.protocol + '//' + GEE_HOST;

// We use indexOf a bit, which is not supported by <=IE8.
// This simulates indexOf where it is not supported.
(function() {
    if (!Array.prototype.indexOf) {
        Array.prototype.indexOf = function(searchElement /*, fromIndex */) {
            'use strict';
            if (this == null) {
                throw new TypeError();
            }
            var t = Object(this);
            var len = t.length >>> 0;
            if (len === 0) {
                return -1;
            }
            var n = 0;
            if (arguments.length > 1) {
                n = Number(arguments[1]);
                if (n != n) { // shortcut for verifying if it's NaN
                    n = 0;
                } else if (n != 0 && n != Infinity && n != -Infinity) {
                    n = (n > 0 || -1) * Math.floor(Math.abs(n));
                }
            }
            if (n >= len) {
                return -1;
            }
            var k = n >= 0 ? n : Math.max(len - Math.abs(n), 0);
            for (; k < len; k++) {
                if (k in t && t[k] === searchElement) {
                    return k;
                }
            }
            return -1;
        };
    }
  })()

/**
 * Initializes functions and variables for the GEE front-end environment.
 */
gees.initialize = {

  // Ping all the available services and make sure there are no errors.
  pingServices: function(errorMsg, serviceIndex) {

    // List of available services.
    var serviceList = ['Publish',
                       'StreamPush',
                       'SearchPublish',
                       'Snippets'];

    errorMsg = errorMsg || '';
    serviceIndex = serviceIndex || 0;

    // Ping a service, or deliver an error message if one exists.
    if (serviceIndex < serviceList.length) {

      // Create service specific url request.
      var pingUrl =
          GEE_BASE_URL + '/geserve/' + serviceList[serviceIndex] + '?Cmd=Ping';
      gees.requests.asyncGetRequest(pingUrl, function(pingResponse) {
        var responseCode = pingResponse.split('Gepublish-StatusCode:')[1];
        if (responseCode == -1) {

          // If a problem occurs, add the error response to the message.
          errorMsg +=
              pingResponse.split('Gepublish-StatusMessage:')[1] + '<br>';
        }

        // Increment counter and ping additional services.
        serviceIndex++;
        gees.initialize.pingServices(errorMsg, serviceIndex);
      });
    } else {

      // If an error message exists, notify the user.
      if (errorMsg != '') {
        gees.tools.errorBar(errorMsg);
      }
    }
  },

  databases: function() {
    // Create a list of all databases and portables.
    gees.assets.setItems();
    // Get and return the source Json.
    var response = jQuery.parseJSON(gees.requests.getDatabases());
    if (response.status_code != 0) {
      var msg = 'Unable to load list of Databases.';
      gees.tools.errorBar(msg);
      return;
    }
    var dbList = response.results;
    for (var i = 0; i < dbList.length; i++) {
      var db = dbList[i];
      db = gees.tools.setAdditionalDatabaseDetails(db);
      // Build a list of Registered Database and Portables.
      if (db.registered) {
        gees.assets.databases.push(db);
        if (gees.tools.isCuttableDb(db)) {
          gees.assets.cutterItems.push(db);
        }
      }
      var undiscovered = gees.assets.tempMemory.indexOf(db.name) == -1;
      if (!gees.tools.isFusionDb(db) && undiscovered) {
        gees.assets.portables.push(db);
      }
      gees.tools.collectDashboardData(db);
    }
  },

  searchDefs: function() {
    gees.assets.searchDefs = [];
    var response = jQuery.parseJSON(gees.requests.getSearchDefs());
    if (response.status_code != 0) {
      var msg = 'Unable to load list of Search Definitions.';
      gees.tools.errorBar(msg);
      return;
    }
    var searchDefsList = response.results;
    for (var i = 0; i < searchDefsList.length; i++) {

      // Get Search Definition properties.
      var item = searchDefsList[i].search_def_content;
      var name = searchDefsList[i].search_def_name;

      // Query and config can be null, in which case we will force it
      // be be at least ''. '&amp;' will not encode correctly.  Change
      // to '&' if found.
      var query = item.additional_query_param || '';
      var config = item.additional_config_param || '';
      query = query.replace(/&amp;/g, '&');
      config = config.replace(/&amp;/g, '&');

      // Create an object for the Search Definition.
      var searchDef = {};
      searchDef['name'] = name;
      searchDef['sortName'] = name.toLowerCase();
      searchDef['label'] = item.label;
      searchDef['url'] = item.service_url;
      searchDef['query'] = query;
      searchDef['config'] = config;
      searchDef['fields'] = item.fields;
      searchDef['isSystem'] = gees.tools.isSystemTab(name);
      searchDef['html_transform_url'] = item.html_transform_url;
      searchDef['kml_transform_url'] = item.kml_transform_url;
      searchDef['suggest_server'] = item.suggest_server;
      searchDef['result_type'] = item.result_type;

      // Append Search Def object to the list of Search Defs.
      gees.assets.searchDefs.push(searchDef);
    }
  },

  snippets: function() {
    // Get a list of available snippet profiles.
    gees.assets.snippets = [];
    var response = jQuery.parseJSON(gees.requests.getSnippets());
    if (response.status_code == 0) {
      gees.assets.snippets = response.results;
    } else {
      var msg = 'Unable to load list of Snippets.';
      gees.tools.errorBar(msg);
      return;
    }
    for (var item in gees.assets.snippets) {
      gees.assets.snippets[item]['sortName'] =
          String(gees.assets.snippets[item].set_name).toLowerCase();
    }
    gees.assets.snippets.sort(gees.tools.sort.asc('sortName'));
  },

  virtualServers: function() {
    // Get a list of available virtual servers.
    gees.assets.virtualServers = [];
    var string = gees.requests.getVirtualServers();
    var array = string.split('Gepublish-VsName:');
    for (var i = 0; i < array.length; i++) {
      var host = array[i].split(/\n/)[0];
      if (host != '') {
        gees.assets.virtualServers.push(host);
      }
    }
  },

  missingPortables: function() {
    // Report an error message if there are missing portables.
    if (gees.assets.missingItems.length > 0) {
      var missingPortableGlobeErrorMsg = 'Missing portable globe:<br>' +
          gees.assets.missingItems.slice(0, 10).join(',<br>');
      if (gees.assets.missingItems.length > 10) {
        missingPortableGlobeErrorMsg += '<br>...';
      } else {
        missingPortableGlobeErrorMsg += '.';
      }

      missingPortableGlobeErrorMsg +=
          '<br>The portable globes might be deleted or device is unmounted.' +
          '<br>To cleanup registration information run: ' +
          'geserveradmin --portable_cleanup.';
      gees.tools.errorBar(missingPortableGlobeErrorMsg);
    }
  },

  init: function() {
    this.pingServices();
    this.searchDefs();
    this.snippets();
    this.virtualServers();
    this.missingPortables();
    return this;
  }

}

// Invoke this function every time this script is loaded.
gees.initialize.databases();
