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

/**
 * @fileoverview Utilities shared by both the 3d globe and 2d map cutters.
 *
 */

// Need the static URL for absolute addresses
var geeVirtualServers = {};  // Virtual servers returned from fusion.
var geeIs2d = false; // Whether we are cutting a 2-d Map.

/**
 * Load the specified javascript.
 * @param {string}
 *          path the path relative to the current web page of the javascript.
 */
function geeLoadScript(path) {
  document.write('<script src="' + path +
                 '" type="text/javascript"><\/script>');
}

/**
 * Init the Google Earth plugin. This will contact the GEE Server for the GEE
 * Database specifics and will use those to initialize: 1) the earth plugin 2)
 * the layer panel 3) the search tabs
 */
function geeCutterInit() {
  // Set up the extent text.
  geeSetPolygonEntryType();

  // Gets data and call back function for setting up virtual server
  // pull-down menu.
  if (GEE_VIRTUAL_SERVERS.length == 0) {
    jQuery.getJSON('/cgi-bin/globe_cutter_app.py?cmd=SERVERS',
                  geeSetVirtualServerSelect);
  } else {
    geeSetVirtualServerSelect(GEE_VIRTUAL_SERVERS);
  }
}

/**
 * Decide whether the user has selected a 2d map or a 3d globe for cutting.
 */
function geeSelectGlobeOrMap() {
  // Gets data and call back function for setting up virtual server
  // pull-down menu.

  if (GEE_VIRTUAL_SERVERS.length == 0) {
    jQuery.getJSON('/cgi-bin/globe_cutter_app.py?cmd=SERVERS',
                   geeSelectDefaultGlobeOrMap);
  } else {
    geeSetVirtualServerSelect(GEE_VIRTUAL_SERVERS);
  }
}

/**
 * Change to globe from another virtual server.
 * @param {string} vs_name The name of virtual server to switch to.
 * @return {bool} false to indicate processing of event.
 */
function geeChangeGlobe(vs_name) {
  var url = GEE_BASE_URL;
  if (geeIsVirtualServer2d(vs_name + '/')) {
    document.location = url + '/cutter/map_cutter.html?server=' + vs_name;
  } else {
    document.location = url + '/cutter/globe_cutter.html?server=' + vs_name;
  }
  return false;
}

/**
 * Check if int is in the given range.
 * @param {string} int_entry String to be used as an int.
 * @param {number} min_value Value which entry should equal or exceed.
 * @param {number} max_value Value which entry should NOT exceed.
 * @return {bool} whether given entry is in given range.
 */
function geeCheckInt(int_entry, min_value, max_value) {
  var val = parseInt(int_entry.value);
  return (val >= min_value) && (val <= max_value);
}

/**
 * Convert altitude to zoom level.
 * @param {number} altitude Altitude to convert.
 * @return {number} zoom level for given altitude.
 */
function geeAltitudeToZoomLevel(altitude) {
  for (var zoomLevel = 1; zoomLevel < 31; ++zoomLevel) {
    if (altitude > geeZoomLevelToCameraAltitudeMap[zoomLevel]) {
      return zoomLevel;
    }
  }

  return zoomLevel;
}

/**
 * Get max level at which data is kept.
 * @return {number} Level at which polygon is rendered in qtnodes.
 */
function geePolygonLevel() {
  var select = gees.dom.get('polygon_zoom');
  var zoom = select.options[select.selectedIndex].value;
  if (zoom == undefined || zoom == '' || zoom == '--' || zoom == null) {
    zoom = 18;
  }
  return zoom;
}

/**
 * Get max level at which data is kept.
 * @return {number} Level above which no data is kept.
 */
function geeMaxLevel() {
  var select = gees.dom.get('outer_zoom');
  var zoom = select.options[select.selectedIndex].value;
  if (zoom == undefined || zoom == '' || zoom == '--' || zoom == null) {
    zoom = 1;
  }
  return zoom;
}

/**
 * Get level at or below which all data is kept.
 * @return {number} Level at or below which all data is kept.
 */
function geeDefaultLevel() {
  var select = gees.dom.get('inner_zoom');
  var zoom = select.options[select.selectedIndex].value;
  if (zoom == undefined || zoom == '' || zoom == '--' || zoom == null) {
    zoom = 1;
  }
  return zoom;
}

/**
 * Set polygon kml.
 * @param {string} polygonKml Polygon specifying ROI.
 */
function geeSetPolygon(polygonKml) {
  document.globe_cutter_form.polygon.value = polygonKml;
}

/**
 * Get polygon kml.
 * @return {string} Polygon specifying ROI.
 */
function geePolygon() {
  return gees.dom.get('kml_field').value;
}

/**
 * Get name to be given to the globe (base directory).
 * @return {string} Name for globe being built.
 */
function geeGlobeName() {
  return gees.dom.get('name_field').value;
}

/**
 * Checks flag for allowing overwriting of existing globe.
 * @return {bool} whether existing globe can be overwritten.
 */
function geeAllowOverwrite() {
  return gees.dom.isChecked('overwrite_radio');
}

/**
 * Checks flag for using alternate method of gathering tiles.
 * @return {bool} whether alternate method should be used.
 */
function geeUseAlternateMethod() {
  return gees.dom.isChecked('useAlternateMethod_radio');
}

/**
 * Checks flag for including historical imagery in cut globe.
 * @return {bool} whether historical imagery should be included.
 */
function geeIncludeHistoricalImagery() {
  return gees.dom.isChecked('includeHistorical_radio');
}

/**
 * Get description of the globe.
 * @return {string} Description of the globe.
 */
function geeGlobeDescription() {
  return gees.dom.get('text_field').value;
}

/**
 * Get base url for source server of the globe being cut.
 * @return {string} Url of globe being cut.
 */
function geeSource() {
  var select = gees.dom.get('GlobeIdHolder');
  var targetPath = select.options[select.selectedIndex].value;
  return gees.tools.getServerUrl(targetPath);
}

/**
 * Set response from server or other messages to the user.
 * @param {string} response Test to set in status.
 */
function geeSetResponse(response) {
  gees.dom.show('ResponseMessage');
  gees.dom.show('BuildResponseDiv');
  gees.dom.setHtml('ResponseMessage', response);
}

/**
 * Clear downloadable portable globe info.
 */
function geeClearGlobeInfo() {
  gees.dom.clearHtml('globe');
}

/**
 * Select a globe or a map cutting page.
 *
 * @param {map} virtual_servers Virtual servers info from Json.
 */
function geeSelectDefaultGlobeOrMap(virtual_servers) {
 geeVirtualServers = virtual_servers;
 if (virtual_servers && virtual_servers.length > 0) {
   geeChangeGlobe(virtual_servers[0].url);
 } else {
   document.location = GEE_BASE_URL + '/cutter/globe_cutter.html';
 }
}

/**
 * Returns whether virtual server is serving a 2d map.
 *
 * @param {string} vs Virtual server being checked.
 * @return {boolean} whether server is serving a 2d map.
 */
function geeIsVirtualServer2d(vs) {
  if (geeVirtualServers.length == 0) {
    return false;
  }
  if (vs == '') {
    return geeVirtualServers[0].is_2d;
  } else {
    for (var i = 0; i < geeVirtualServers.length; i++) {
      if (geeVirtualServers[i].url + '/' == vs) {
        return geeVirtualServers[i].is_2d;
      }
    }
  }
  // Unknown virtual server.
  return false;
}


/**
 * Creates pull-down menu of available virtual servers (ge globes).
 * @param {string} virtual_servers Returned JSON object with list of
 *                                 virtual servers.
 */
function geeSetVirtualServerSelect(virtual_servers) {
  geeVirtualServers = virtual_servers;
  if (geeVirtualServers.length == 0) {
    alert('No published maps or globes detected.');
    return;
  }

  var select_div = gees.dom.get('server_select');
  var globe_select = '<select name="server"';
  globe_select += ' onChange="geeChangeGlobe(' +
                  'document.globe_connection_form.server.value)">';
  // If coming in blind, reload using the first globe or map.
  // By doing this we ensure the proper map or globe cutter page.
  if (GEE_SERVER_URL == '') {
    geeChangeGlobe(virtual_servers[0].url);
  }

  // Create pull-down and select the current map or globe.
  for (var i = 0; i < geeVirtualServers.length; i++) {
    globe_select += '<option value="' + geeVirtualServers[i].url + '"';
    if (geeVirtualServers[i].url + '/' == GEE_SERVER_URL) {
      globe_select += '" SELECTED';
      geeIs2d = geeVirtualServers[i].is_2d;
    } else {
      globe_select += '"';
    }

    globe_select += '>' + geeVirtualServers[i].name;
  }

  globe_select += '<\/select>';
  gees.dom.setHtml(select_div, globe_select);

  // Correct version should be loaded for if this is 2d Map or a 3d globe.
  geeInsertIntoDiv();
}

/**
 * Allows user to toggle between a description of how to use the
 * embedded tools for defining a polygon and a textarea for entering
 * the polygon directly as KML.
 */
function geeSetPolygonEntryType() {
  var embedded_tool = gees.dom.get('embedded_tool_polygon');
  var kml_tool = gees.dom.get('kml_tool_polygon');
  if (gees.dom.isChecked(document.globe_cutter_form.kml_for_polygon)) {
    gees.dom.hide(embedded_tool);
    gees.dom.show(kml_tool);
  } else {
    gees.dom.show(embedded_tool);
    gees.dom.hide(kml_tool);
  }
}

/**
 * Clean up after finishing a sequence.
 */
function gee_cleanUp() {
  if (GEE_interval != '') {
    clearInterval(GEE_interval);
    GEE_interval = '';
  }
  // If overwriting was allowed on cut, remind user to re-register
  // any new/overwritten portables.
  if (geeAllowOverwrite()) {
    var msg = 'Remember to re-register any portables you may have overwritten';
    gees.tools.butterBar(msg, true);
  }
  button = gees.dom.get('CutButtonBlue');
  gees.dom.setClass(button, 'button blue');
  button = gees.dom.get('CancelButton');
  gees.dom.setClass(button, 'button standard Muted');
  buildBar = gees.dom.get('ProgressBar');
  gees.dom.addClass(buildBar, 'buildCompleted');
  var buildMsg = 'Build complete<a href="javascript:void(0)"' +
      ' onclick="hideBuildResponse()">-</a>';
  gees.dom.setHtml(buildBar, buildMsg);
  gees.dom.get('globe').innerHTML += '<br><a href="javascript:void(0)"' +
      ' onclick="hideBuildComplete()">Close this dialog</a>';
  GEE_running_get_data_sequence = false;
  GEE_clearUid();
}

/**
 * Get size of globe built so far.
 */
function gee_showGlobeSize() {
  if (GEE_interval == '') {
    alert('Got called but GEE interval is empty.');
  }

  if (GEE_running_get_data_sequence) {
    var url = '/cgi-bin/globe_cutter_app.py?cmd=BUILD_SIZE&uid=' + GEE_uid +
              '&globe_name=' + GEE_globe_name + GEE_force_arguments;
    jQuery.get(url, function(size) {
          div = gees.dom.get('globe');
          if ((div.innerHTML.indexOf('Your globe') < 0) &&
              (div.innerHTML.indexOf('Your map') < 0)) {
            gees.dom.setHtml(div, 'Working... ' + size);
          }
        });
  }
}

/**
 * Check if task is done.
 */
function gee_checkTaskDone() {
  if (GEE_interval == '' && GEE_cancelled == 0) {
    alert('Got called but GEE interval is empty.');
  }

  if (GEE_delay < 6) {
    // If you are cutting only small globes, you might want to
    // just set this to 1 or even 0.
    GEE_delay += 1;
  }

  if (GEE_running_get_data_sequence) {
    var url = '/cgi-bin/globe_cutter_app.py?cmd=BUILD_DONE&uid=' + GEE_uid +
              '&globe_name=' + GEE_globe_name + '&delay=' + GEE_delay +
              '&is_2d=' + GEE_is_2d + GEE_force_arguments;
    jQuery.get(url, function(is_running) {
          // If still running, keep checking until the task is done.
          if (is_running.substr(0, 1) == 't') {
            gee_checkTaskDone();
          // Once it is done, go to the next task in the sequence.
          } else {
            GEE_delay = 0;
            if (is_running.indexOf('FAILED') >= 0) {
              GEE_appendStatus(is_running);
              GEE_cleanUp();
            } else {
              packet_info = is_running.split(' ');
              msg = '<br/>';
              if (GEE_is_2d != 't') {
                msg += packet_info[0] + ' image packets<br/>';
                msg += packet_info[1] + ' terrain packets<br/>';
                msg += packet_info[2] + ' vectors packets';
              } else {
                msg += ' map tiles read<br/>';
              }
              GEE_appendStatus(msg);
              GEE_runNextAjaxSequenceItem();
           }
          }
        });
  }
}
