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
This is the main initializing file for the cutting tool.  It loads the
necessary api information and handles all UI changes not directly
related to drawing polygons & pasting KML.

It relies on the following javascript files:
- cutter/js/drawing_tools.js: drawing library for cutting tool.
- shared_assets/polygon_tools.js: handles pasted KML and creates Polygon.
*/

// Items needed to load the Google Earth Plugin directly to a
// Google Earth Enterprise Server.
if (!('google' in window)) { window.google = {}; }
if (!('loader' in window.google)) { window.google.loader = {}; }
if (!('earth' in window.google)) { window.google.earth = {}; }
window.google.loader.ApiKey = 'ABCDEFGHIJKLMNOPgoogle.earth.ec.key';
window.google.loader.KeyVerified = true;
window.google.earth.allowUsageLogging = false;
window.google.earth.setErrorUrl('error.html');

// Globals needed for Cutter UI.
var ge = null;
var map;
var isServing;
var openDropDown;
// Search url for a specific server/database request.
var geeTargetPath = gees.tools.getTargetPathForCutter();
var temporaryTargetPath;
var BASE_ZOOM_LEVEL = 0;
var MIN_POLY_ZOOM = 4;
var DEFAULT_POLY_ZOOM = 18;
var currentCutterMode;

// Global sets of element IDs, and the mode in which they are visible.
// These arrays are meant to be passed to
// gees.tools.setElementDisplay(array, display).
var DISPLAY_ELEMENTS_IFRAME = ['IframeHeader',
                               'IframeInfoPane',
                               'IframeButtons',
                               'IframeResponse',
                               'IframeKML',
                               'IframeError'];

var DISPLAY_ELEMENTS_KML = ['NotificationModal',
                            'WhiteOverlay',
                            'PasteKMLDiv'];

// The gees.assets.cutterItems var below is created in
// /htdocs/js/gees_initialize.js. It holds the list of avaiable
// assets avaiable to cutter.

// Sort the available (published) items by name before creating
// the cutter's dropdown menu.
sortCutterItems();

// TODO: build cutterItems list here,
//                rather than in gees_initialize.js.
// If there are published Databases & Portables, find our serverDefs.
if (gees.assets.cutterItems.length > 0) {
  if (!geeTargetPath) {
    // If no specified server, choose the first from our list of databases.
    switchDatabase(gees.assets.cutterItems[0].target_path);
  } else {
    // Otherwise, pull the specified Server/Target Path from url; need to
    // update the hostname to be consistent with browser's current hostname
    // to prevent an "Access-Control_allow-Origin" error.
    var serverDefsRequest =
      gees.tools.replaceHost(gees.tools.getServerUrl(geeTargetPath));
    var geeServerDefs = gees.requests.getServerDefs(serverDefsRequest);
  }
} else {
  // If list of Databases is empty, let the user know.
  popupTitle = 'No databases found';
  popupContents = 'You do not appear to have any published databases or ' +
      'portables.  Please confirm that you have published databases and ' +
      'portables to cut from, and refresh this page.';
  popupButtons = '';
  gees.dom.get('WhiteOverlay').onclick = ' ';
  gees.tools.toggleNotification(popupTitle, popupContents, popupButtons);
}

// Start filling out the UI and loading the database.
function initializeCutter() {
  // Make sure we've got a working database.
  validateVarDefs();
  // Decide if we are using Maps (2D) or Earth (3D).
  determineServingType();
  checkIeVersion();
  if (isServing == '3D') {
    // Load the 3D elements, and make the first call to the Earth plugin.
    gees.dom.show('map3d');
    google.earth.createInstance('map3d', initCB, failureCB,
      {database: gees.tools.getServerUrl(geeTargetPath)});
  } else if (isServing == '2D') {
    // Initialize the Maps plugin.
    loadMapDiv();
    // Get and display current zoom level.
    getZoomLevel();
    // Initialize event listeners.
    startListeners();
  } else {
    popupTitle = 'An error occurred';
    popupContents = 'Cutter is unable to initialize.  Please confirm ' +
        'that you have published Databases or Portables to cut from, and ' +
        'refresh this page.';
    popupButtons = '';
    gees.dom.get('WhiteOverlay').onclick = ' ';
    gees.tools.toggleNotification(popupTitle, popupContents, popupButtons);
  }

  // Fill zoom leve drop downs with values 1-24.
  populateZoomDropdowns(BASE_ZOOM_LEVEL, 'inner_zoom', '--');
  populateZoomDropdowns(BASE_ZOOM_LEVEL, 'outer_zoom', '--');
  populatePolygonZoomDropdown('polygon_zoom');
  resetRegionOptions();
  // Prepare cutter for input of drawn/pasted KML.
  cutterMode('start');
  // For layering on Earth in Windows OS.
  gees.tools.setElementDisplay(DISPLAY_ELEMENTS_IFRAME, 'block');
  // Populate the dropdown list of available databases.
  getDatabaseList();
}

// Some IE versions will not obey the 'placeholder' (hint) property of
// input elements, so force text into these elements in that event.
function checkIeVersion() {
  var ie = gees.tools.internetExplorerVersion();
  if (ie == 7 || ie == 8 || ie == 9) {
    gees.dom.get('name_field').value = 'name';
    gees.dom.get('text_field').value = 'description';
  }
}

// Create list of published globes.
function getDatabaseList() {
  var targetDiv = gees.dom.get('GlobeIdHolder');
  // Reset the list to blank.
  clearList(targetDiv);
  // Insert the name of the globe/database we are currently viewing.
  insertListItem(targetDiv, geeTargetPath, false);
  // Loop a list of published items and insert them all in the list.
  for (var i = 0; i < gees.assets.cutterItems.length; i++) {
    var item = gees.assets.cutterItems[i];
    // Change '/TargetPath' to 'TargetPath'.
    var databaseTargetPathName = item.target_path.substr(1);
    // Omit the item if it is the one we are serving.
    var isNotGlc = item.name.search('.glc') == -1;
    if (databaseTargetPathName != geeTargetPath && isNotGlc) {
      insertListItem(targetDiv, databaseTargetPathName, false);
    }
  }
}

// Fill in our zoom/region level dropdown menus.
function populateZoomDropdowns(startingNumber, id, defaultValue) {
  var targetDiv = gees.dom.get(id);
  clearList(targetDiv);
  insertListItem(targetDiv, defaultValue, true);
  for (var i = parseInt(startingNumber); i <= 24; i++) {
    insertListItem(targetDiv, i, false);
  }
  // Fill label and dropdown.  Example: 4 to [15], where 4 is
  // startingNumber in the label, and 15 is defaultValue in the dropdown.
  gees.dom.setHtml(id + '_em', startingNumber + ' to');
}

// Fill in the polygon zoom level dropdown menu.
function populatePolygonZoomDropdown(id) {
  var targetDiv = gees.dom.get(id);
  clearList(targetDiv);
  for (var i = MIN_POLY_ZOOM; i <= 24; i++) {
    insertListItem(targetDiv, i, i == DEFAULT_POLY_ZOOM);
  }
}

// Set Draw/Paste KML dropdown to initial state.
function resetRegionOptions() {
  var targetDiv = gees.dom.get('region_options');
  clearList(targetDiv);
  insertListItem(targetDiv, 'Manual', false);
  insertListItem(targetDiv, 'Paste KML', false);
}

// Update the region zoom dropdown to illustrate its connection with the
// world zoom dropdown menu.
function updateRegionZoom() {
  var innerSelect = gees.dom.get('inner_zoom');
  var worldZoom = innerSelect.options[innerSelect.selectedIndex].value;
  var outerSelect = gees.dom.get('outer_zoom');
  var oldRegionZoom = outerSelect.options[outerSelect.selectedIndex].value;
  // If current region zoom is larger than selected world zoom, leave it
  // alone and just update the label.  If world zoom is larger, reset
  // the region zoom, forcing user to select a new value.
  if (parseInt(oldRegionZoom) > parseInt(worldZoom)) {
    populateZoomDropdowns(regionZoom, 'outer_zoom', oldRegionZoom);
  } else {
    populateZoomDropdowns(regionZoom, 'outer_zoom', '--');
  }
}

// Insert an <option> element into a <select> element.
function insertListItem(targetList, item, selected) {
  if (targetList) {
    var option = gees.dom.create('option');
    option.text = item;
    if (selected) {
      option.selected = true;
    }
    targetList.add(option, null);
  }
}

// Removes all <option> items from a <select> element.
function clearList(targetList) {
  while (targetList.length > 0) {
    targetList.remove(targetList.length - 1);
  }
}

// Prepare to switch databases and move on to a confirmation function.
function selectGlobeSwitcher() {
  var select = gees.dom.get('GlobeIdHolder');
  var selectedGlobe = select.options[select.selectedIndex].value;
  selectedGlobe = selectedGlobe.replace(/\//g, '%2F');
  confirmSwitchDatabase('/' + selectedGlobe);
}

// Make sure user actually wants to switch Databases.
function confirmSwitchDatabase(path) {
  temporaryTargetPath = path;
  if (arrayCoords.length > 0) {
    popupTitle = 'Change source database?';
    popupContents = 'You have started to define your region ' +
        'of interest.  Are you sure you want to switch databases?';
    popupButtons = '<a href=\'#\' class=\'button blue\'' +
        'onclick="switchDatabase(\'' + path + '\')">Switch</a>' +
        '<a href=\'#\' class=\'button standard\'' +
        'onclick="closeAllPopups();">Cancel</a>';
    gees.tools.toggleNotification(popupTitle, popupContents, popupButtons);
  } else {
    switchDatabase(path);
  }
}

// Function for switching Globes using the dropdown menu.
function switchDatabase(targetPathOfDatabase) {
  // Remove '/' from target path, and reload cutter with
  // this database selected.
  var strippedTargetPath;
  if (targetPathOfDatabase.substr(0, 1) == '/') {
    strippedTargetPath = targetPathOfDatabase.substr(1);
  } else {
    console.log('Expected leading slash on target path.');
    strippedTargetPath = targetPathOfDatabase;
  }
  // Need to keep the original protocol from target_base_url.
  var baseUrl = gees.assets.getBaseUrl(targetPathOfDatabase);
  // Keep browser's current hostname to maintain a consistent list of available
  // databases (targets), which are subject to change depending on whether
  // the client is from remote or local host.
  baseUrl = gees.tools.replaceHost(baseUrl);
  window.location =
      baseUrl + '/cutter/index.html?server=' + strippedTargetPath;
}

// Determine if the Database is 2D/3D and set isServing var.
function determineServingType() {
  for (var i = 0; i < gees.assets.databases.length; i++) {
    var grabTargetPath = '/' + geeTargetPath;
    var databaseTargetPath = gees.assets.databases[i].target_path;
    var databaseType = gees.assets.databases[i].projection;
    if (grabTargetPath == databaseTargetPath) {
      isServing = databaseType;
    }
  }
}

// Create the instance of the Google Maps Api.
function loadMapDiv() {
  // Load the map.
  var map_opts = {
      zoom: 3,
      center: new google.maps.LatLng(0, 0),
      navigationControl: false,
      mapTypeControl: false,
      streetViewControl: false,
      panControl: false,
      zoomControl: false,
      scaleControl: false
  };
  // Show map zoom controls, which are hidden if isServing == '3D'.
  gees.dom.show('zoom_controls');
  var targetPath = window.location.href.split('=')[1].replace(/%2F/g, '/');
  geeServerDefs.serverUrl = gees.tools.getServerUrl(targetPath);
  geeMap = geeCreateFusionMap('map', geeServerDefs, map_opts);
}

// Earth's standard initilization callback.  Creates instance of the GE plugin
// and sets the default camera position and altitude.
function initCB(instance) {
  ge = instance;
  ge.getWindow().setVisibility(true);
  // Set altitude
  var lookAt = ge.getView().copyAsLookAt(ge.ALTITUDE_RELATIVE_TO_GROUND);
  lookAt.setLatitude(0);
  lookAt.setLongitude(0);
  lookAt.setRange(20000000);
  ge.getView().setAbstractView(lookAt);
  ge.getNavigationControl().setVisibility(ge.VISIBILITY_SHOW);
  // Set buttons and divs to initial state.
  startListeners();
}

function failureCB(errorCode) {
  gees.dom.hide('button_holder');
}

// Zooming function, for 2D Maps only.
function ZoomTheMap(action) {
  var zoom = geeMap.zoom;
  var newZoom;
  if ((action == 'out') && (zoom > 1)) {
    if (zoom == null || zoom == undefined) {
      zoom = 1;
    }
    newZoom = zoom - 1;
    geeMap.setZoom(newZoom);
  }
  if ((action == 'in') && (zoom < 24)) {
    if (zoom == null || zoom == undefined) {
      zoom = 24;
    }
    newZoom = zoom + 1;
    geeMap.setZoom(newZoom);
  }
}

// Toggles visibility of an element.
function toggleElementDisplay(el) {
  if (gees.dom.getDisplay(el) == 'block' || gees.dom.getDisplay(el) == '') {
    gees.dom.hide(el);
  } else {
    gees.dom.show(el);
  }
}

// Toggles visibility of div displaying build progress.
function hideBuildResponse() {
  toggleElementDisplay(gees.dom.get('ResponseHolder'));
}

// A function for switching drawing modes. For example, switching from
// drawing to pasting, or completing drawing a polygon.
function cutterMode(mode) {
  currentCutterMode = mode;
  // Reset all elements to original state.
  resetInfoPaneDisplay();
  if (mode == 'start') {
    // Simulate click on the hand button.  This is the mode Cutter starts in.
    gees.dom.setClass('poly_button', '');
    gees.dom.get('poly_button').title =
        'Draw polygon to define region of interest';
    gees.dom.setClass('hand_button', 'clicked_button');
    gees.dom.get('hand_button').title = 'Drag to pan map.';
    gees.dom.hide('OuterZoomHolder');
    gees.dom.hide('AdvancedZoomHolder');
    gees.dom.show('RegionHolder');
    gees.dom.hide('ClearHolder');
  } else if (mode == 'draw') {
    // Prepare Cutter for drawing.  Disable other UI interactions until
    // the polygon is complete.
    gees.dom.setClass('poly_button', 'clicked_button');
    gees.dom.setClass('hand_button', '');
    gees.dom.get('hand_button').title = 'Click to complete polygon.';
    gees.dom.addClass('CutButtonBlue', 'Muted');
    gees.dom.addClass('GlobeIdHolder', 'Muted');
    gees.dom.addClass('InnerZoomHolder', 'Muted');
    gees.dom.addClass('OuterZoomHolder', 'Muted');
    gees.dom.addClass('AdvancedZoomHolder', 'Muted');
  } else if (mode == 'paste') {
    // Allow user to paste and edit KML.
    gees.dom.show('WhiteOverlay');
    gees.dom.show('PasteKMLDiv');
  } else if (mode == 'complete') {
    // Hide div for pasting KML.
    gees.tools.setElementDisplay(DISPLAY_ELEMENTS_KML, 'block');
    // Update a status concerning the region of interest.
    var regionContent =
        'Clear region before defining new one.';
    updateRegionNotes('block', regionContent);
    // Set drawing controls to original state.
    gees.dom.setClass('GlobeIdHolder', 'CutterSelect BigSelect');
    gees.dom.setClass('InnerZoomHolder', 'SmallDropHolder');
    gees.dom.setClass('OuterZoomHolder', 'SmallDropHolder');
    gees.dom.setClass('AdvancedZoomHolder', 'SmallDropHolder');
    gees.dom.setClass('poly_button', 'MutedWithEvents');
    gees.dom.get('poly_button').title =
        'Clear region of interest before defining new one.';
    gees.dom.setClass('hand_button', 'clicked_button');
    gees.dom.get('hand_button').title = 'Drag to pan map.';
    // We now know there is a polygon, so we will make the region-specific
    // level selector visible.
    gees.dom.show('OuterZoomHolder');
    gees.dom.show('AdvancedZoomHolder');
    // Hide the "Select Region" control and instead display the Clear button.
    gees.dom.hide('RegionHolder');
    gees.dom.show('ClearHolder');
  }
}

// Reset divs that display build progress.  Important if a user kicks off
// a second cut from the same globe; erases the previous build's information.
function resetMessageDivs() {
  gees.dom.clearHtml('ResponseMessage');
  gees.dom.clearHtml('status');
  gees.dom.clearHtml('globe');
}

// Change the appearance of our Build Progress div when the build is complete.
function hideBuildComplete() {
  gees.dom.hide('BuildResponseDiv');
  resetMessageDivs();
  buildBar = gees.dom.get('ProgressBar');
  gees.dom.setClass(buildBar, 'buildProgress');
  var statusMsg = 'Build Status' +
      '<a href="javascript:void(0)" onclick="hideBuildResponse()">-</a>';
  gees.dom.setHtml(buildBar, statusMsg);
}

function errorProgressHeader() {
  buildBar = gees.dom.get('ProgressBar');
  gees.dom.addClass(buildBar, 'buildCompleted');
  var errorMsg = 'Form error' +
      '<a href="javascript:void(0)" onclick="hideBuildResponse()">-</a>';
  gees.dom.setHtml(buildBar, errorMsg);
}

// Restore UI elements to their original state.
function resetInfoPaneDisplay() {
  gees.dom.setClass('poly_button', '');
  gees.dom.setClass('hand_button', 'clicked_button');
  gees.dom.setClass('CutButtonBlue', 'button blue');
}

// Selects item from dropdown menu and places it in the appropriate div.
function selectRegion() {
  var regionContent;
  var select = gees.dom.get('region_options');
  var method = select.options[select.selectedIndex].value;
  if (method == 'Manual') {
    regionContent = 'Draw polygon to define region';
    updateRegionNotes('block', regionContent);
  }
  if (method == 'Paste KML') {
    regionContent = 'Paste KML to define region <a href="javascript:void(0)"' +
        'onclick="cutterMode(\'paste\')">Paste</a>';
    updateRegionNotes('none', regionContent);
    cutterMode('paste');
  }
}

// Updates an element that explains the status of the region of interest.
function updateRegionNotes(displayValue, content) {
  var buttonHolder = gees.dom.get('button_holder');
  var regionNotes = gees.dom.get('RegionNotes');
  gees.dom.setDisplay(buttonHolder, displayValue);
  gees.dom.setHtml(regionNotes, content);
}

// When inner_zoom is updated, we need to update outer_zoom label.
function updateOuterZoom(inner_zoom) {
  var outerZoomValue = gees.dom.get('outer_zoom_level').innerHTML;
  // If outer_zoom is not '--', it is a number, and we need to look at it.
  if (outerZoomValue != '--') {
    // If it is larger than our inner_zoom, it can stay as it is.
    if (parseInt(outerZoomValue) > parseInt(inner_zoom)) {
      outerZoomValue = parseInt(outerZoomValue);
    } else {
      // Set outer_zoom back to its default.
      outerZoomValue = '--';
    }
  }
  populateZoomDropdowns(inner_zoom, 'outer_zoom', outerZoomValue);
}

function toggleAdvancedZoom() {
  toggleElementDisplay(gees.dom.get('polyZoomHolder'));
  if (isServing == '3D') {
    toggleElementDisplay(gees.dom.get('IncludeHistoricalHolder'));
  }
  if (isServing == '2D') {
    toggleElementDisplay(gees.dom.get('UseAlternateMethodHolder'));
  }
}

// First step to loading cutter is loading vardefs. If those fail, throw an
// error.
function validateVarDefs() {
    $.ajax({
    url: serverDefsRequest,
    error: function() {
      popupTitle = 'Failed to load database';
      popupContents = 'Your database did not load correctly. There may be a ' +
          'typo in your url, or your database file may be corrupt.  Please ' +
          'check your database and refresh this page';
      popupButtons =
          '<a href="javascript:void(0)" class="button standard finishDraw"' +
          'onclick="closeAllPopups()">Cancel</a>';
      gees.tools.toggleNotification(popupTitle, popupContents, popupButtons);
    }
  });
}

// Function that looks for all popups and closes them.
function closeAllPopups() {
  gees.tools.setElementDisplay(DISPLAY_ELEMENTS_KML, 'none');
  gees.dom.setDisplay('BuildResponseDiv', 'none');
}

// Function to sort the cutter globes/maps by name.
function sortCutterItems() {
  for (var i = 0; i < gees.assets.cutterItems.length; i++) {
    // Grab name as is (eg BlueMarble).
    var item = gees.assets.cutterItems[i];
    item.sort_name = item.target_path.toLowerCase();
  }
  // Sort the list.
  gees.assets.cutterItems.sort(gees.tools.sort.asc('sort_name'));
}
