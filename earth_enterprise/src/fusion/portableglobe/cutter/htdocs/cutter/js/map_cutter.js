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
 * Code for UI elements for Map Cutter and for sending the sequence of commands
 * needed to cut a new map.
 *
 */
// Time between updates of size when cutting a map.
var GEE_SIZE_UPDATE_TIME = 1500;  // 1.5 seconds seems OK
var overwriting = false;

// Example:
// var FORCE_ARGUMENTS = '&FORCE_PORTABLE_PORT=8778' +
//                       '&FORCE_PORTABLE_TMP=' +
//                           '/usr/local/google/portable_server/tmp';
var FORCE_ARGUMENTS = '';

/**
 * Check that we have all of the needed values for the map cutter.
 * @return {string} Error messages or empty string if parameters are ok.
 */

function geeCheckMapParameters() {
  var msg = '';
  var formName = gees.dom.get('name_field').value;
  var formDescription = gees.dom.get('text_field').value;
  var formInnerZoom = geeDefaultLevel();
  var formOuterZoom = geeMaxLevel();
  var formPolygon = gees.dom.get('kml_field').value;

  if (formName == '') {
    msg += 'Please enter a <u>name</u>.<br>';
  } else {
    result = gees.tools.isValidName(formName);
    if (!result[0]) {
      msg += result[1];
    } else {
      gees.dom.get('name_field').value = result[1];
    }
  }
  if (formDescription == '') {
    msg += 'Please enter a <u>description</u><br>';
  }
  if (formInnerZoom == '--') {
    msg += 'Please choose an <u>World level</u><br>';
  }
  if (formPolygon != '' && formOuterZoom == '--') {
    msg += 'Please choose an <u>Region level</u><br>';
  }
  // TODO: validate formPolygon.  If there are no KML coordinates,
  // warn the user that their cut will be using viewport, and may be large,
  // etc.  Mock, then get appropriate verbage from Sean & Bret.
  return msg;
}

/**
 * Perform check before building a map.  Determine if an item will be
 * overwritten and, if so, if the existing item is registered.  If registered,
 * overwriting will not be allowed and the user will be notified.
 */
function checkAndBuild() {
  var allowOverwriting = geeAllowOverwrite();
  var useAlternateMethod = geeUseAlternateMethod();
  var includeHistorical = geeIncludeHistoricalImagery();

  // Refresh the list of databases to be sure information is up to date.
  gees.initialize.databases();

  if (allowOverwriting) {

    // Assemble a file name to test against, eg CutName.glm.
    var fileExt = isServing == '2D' ? '.glm' : '.glb';
    var overwritingName = geeGlobeName() + fileExt;

    // Determine if an item with this name exists.
    for (var i = 0; i < gees.assets.databases.length; i++) {
      if (gees.assets.databases[i].name == overwritingName) {

        // Notify user and prevent map from being built.
        overwritingPublishedItemsMessage();
        return false;
      }
    }
  }

  // If no published items are being overwritten, build the map.
  geeBuildMap(allowOverwriting, useAlternateMethod, includeHistorical);
}

/**
 * Notify the user that they cannot overwrite registered items.
 */
function overwritingPublishedItemsMessage() {
  popupTitle = 'Cannot overwrite registered item';
  popupContents = 'You are attempting to overwrite a database that is' +
    ' registered. Registered databases are ineligible to be overwritten.' +
    ' Please unregister the database (and unpublish first if necessary) in' +
    ' order to overwrite it.';
  popupButtons = '<a href="javascript:void(0);closeAllPopups()"' +
      ' class="button standard blue" onclick="closeAllPopups()">Ok</a>';
  gees.tools.toggleNotification(popupTitle, popupContents, popupButtons);
}

/**
 * Use a series of AJAX calls to cut a portable Map and make it available
 * for downloading. The AJAX calls provide some feedback along the way.
 */
function geeBuildMap(allowOverwriting, useAlternateMethod, includeHistorical) {
  hideBuildComplete();
  geeSetResponse('Building portable map ...');
  var msg = geeCheckMapParameters();
  if (msg != '') {
    errorProgressHeader();
    geeSetResponse(msg +
        '<a href="#" onclick="hideBuildComplete()"' +
        ' class="closeBuildDialog">Close this dialog</a>');
    return;
  }

  var useAlternateMethodStr = '';
  var includeHistoricalStr = '';
  var url = '/cgi-bin/globe_cutter_app.py?cmd=UID&globe_name=' + geeGlobeName();

  if (isServing == '2D') {
    url += '&is_2d=t';
  }

  if (allowOverwriting) {
    url += '&allow_overwrite=t';
  }

  if (useAlternateMethod) {
    useAlternateMethodStr = '&ignore_imagery_depth=t';
  }

  if (includeHistorical) {
    includeHistoricalStr = '&include_historical_imagery=t';
  }

  jQuery.get(url,
             function(uid_msg) {
                 var new_globe_info = uid_msg.split(' ');
                 var uid = new_globe_info[0];
                 GEE_setUid(uid);
                 if (isServing == '2D') {
                  GEE_setIs2d();
                 }
                 GEE_setForceArguments(FORCE_ARGUMENTS);
                 var new_globe_name = new_globe_info[1];
                 GEE_setGlobeName(new_globe_name);
                 if (allowOverwriting == true) {
                   geeSetResponse('Overwriting <i>' +
                                  new_globe_name + '</i> ...');
                 } else {
                   geeSetResponse('Building <i>' + new_globe_name +
                                  '</i> ...');
                 }
                 var mercator_flag = '';
                 if (isServing == '2D') {
                   if (geeServerDefs.projection == 'mercator') {
                     mercator_flag = '&is_mercator=t';
                   }
                 }
                 var polygon_level = 'polygon_level=' + geePolygonLevel();
                 var polygon = 'polygon=' +
                        gees.tools.geEncodeURIComponent(geePolygon());
                 var default_level = 'default_level=' + geeDefaultLevel();
                 var max_level = 'max_level=' + geeMaxLevel();
                 var source = 'source=' +
                        gees.tools.geEncodeURIComponent(geeSource());
                 var description = ('description=' +
                        gees.tools.geEncodeURIComponent(geeGlobeDescription()));
                 var globe_name = 'globe_name=' +
                        gees.tools.geEncodeURIComponent(new_globe_name);
                 var base_url = '/cgi-bin/globe_cutter_app.py", "cmd=';
                 // Always delete tmp directory after a successful build.
                 var save_tmp = 'save_tmp=f';
                 var front = 'GEE_postData("';
                 var back1;
                 var back2;
                 if (isServing == '2D') {
                   back1 = (FORCE_ARGUMENTS + '&uid=' + uid +
                                '&is_2d=t' + mercator_flag +
                                '", "status", "APPEND");');
                   back2 = (FORCE_ARGUMENTS + '&uid=' + uid +
                              '&is_2d=t' + mercator_flag +
                              '", "globe", "SET");');
                  } else {
                   back1 = (FORCE_ARGUMENTS + '&uid=' + uid +
                                '", "status", "APPEND");');
                   back2 = (FORCE_ARGUMENTS + '&uid=' + uid +
                                '", "globe", "SET");');
                 }
                 var wait_for_task = 'gee_checkTaskDone();';
                 var clean_up = 'gee_cleanUp();';
                 var sequence = [];
                 if (isServing == '2D') {
                   sequence[0] = front + base_url + 'ADD_GLOBE_DIRECTORY&' +
                       globe_name + '&' + description + back1;
                   sequence[1] = front + base_url + 'POLYGON_TO_QTNODES&' +
                       globe_name + '&' + polygon + '&' + polygon_level + back1;
                   sequence[2] = front + base_url + 'BUILD_GLOBE&' +
                       globe_name + '&' + source + '&' + default_level +
                       '&' + max_level + useAlternateMethodStr + back1;
                   sequence[3] = wait_for_task;
                   sequence[4] = front + base_url + 'EXTRACT_SEARCH_DB&' +
                       globe_name + '&' + source + '&' + polygon + back1;
                   sequence[5] = front + base_url + 'ADD_PLUGIN_FILES&' +
                       globe_name + '&' + source + back1;
                   sequence[6] = front + base_url + 'PACKAGE_GLOBE&' +
                       globe_name + '&' + save_tmp + back1;
                   sequence[7] = front + base_url + 'GLOBE_INFO&' +
                       globe_name + back2;
                   sequence[8] = front + base_url + 'CLEAN_UP&' +
                       globe_name + '&' + save_tmp + back1;
                   sequence[9] = clean_up;
                 } else {
                   sequence[0] = front + base_url + 'ADD_GLOBE_DIRECTORY&' +
                       globe_name + '&' + description + back1;
                   sequence[1] = front + base_url + 'POLYGON_TO_QTNODES&' +
                       globe_name + '&' + polygon + '&' + polygon_level + back1;
                   sequence[2] = front + base_url + 'REWRITE_DB_ROOT&' +
                       globe_name + '&' + source + includeHistoricalStr + back1;
                   sequence[3] = front + base_url + 'GRAB_KML&' +
                       globe_name + '&' + source + back1;
                   sequence[4] = front + base_url + 'BUILD_GLOBE&' +
                       globe_name + '&' + source + '&' + default_level +
                       '&' + max_level + back1;
                   sequence[5] = wait_for_task;
                   sequence[6] = front + base_url + 'EXTRACT_SEARCH_DB&' +
                       globe_name + '&' + source + '&' + polygon + back1;
                   sequence[7] = front + base_url + 'ADD_PLUGIN_FILES&' +
                       globe_name + '&' + source + back1;
                   sequence[8] = front + base_url + 'PACKAGE_GLOBE&' +
                       globe_name + '&' + save_tmp + back1;
                   sequence[9] = front + base_url + 'GLOBE_INFO&' +
                       globe_name + back2;
                   sequence[10] = front + base_url + 'CLEAN_UP&' +
                       globe_name + '&' + save_tmp + back1;
                   sequence[11] = clean_up;
                 }

                 button = gees.dom.get('CutButtonBlue');
                 gees.dom.addClass(button, 'Muted');
                 button = gees.dom.get('CancelButton');
                 gees.dom.setClass(button, 'button standard');
                 gees.dom.showInline(button);
                 button.disabled = false;

                 GEE_runAjaxSequence(sequence, GEE_SIZE_UPDATE_TIME,
                                     gee_showGlobeSize, gee_cleanUp);
               });
}

// Force a published portable to be unpublished, then unregistered.
// Called when user is overwriting a published globe.
function unpublishAndUnregisterPortable(name, publishPoint) {
  // It's published; unpublish it.
  if (publishPoint != '') {
    var unpublishRequest = gees.requests.doGet(GEE_BASE_URL +
        '/geserve/Publish?Cmd=UnPublishDb&TargetPath=' + publishPoint);
  }
  // Unregister (unpush) the globe.
  var unpushRequest =
      gees.requests.doGet('/geserve/StreamPush?Cmd=CleanupDb&DbName=' + name);
}
