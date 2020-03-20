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
** This file controls the UI and (most) functionality of the /local/ directory
** primarily ../local/preview/setup.html. Some of these functions rely on other
** javascript dependencies. The ../local/maps/api/ folder contains javascript
** that helps Maps & Earth APIs load and function.
** ../local/preview/js/ contains a few important files aside from this one.
**   -earth_layer_tree.js controls the layer tree in Earth
**   -folders that have child/parent relationships.
**   -togeojson.js is a small library that assists in parsing KML to a GEO Json
**      usable object.
*/


/*
** This first section handles initialization and loading
** functions for either Maps or Earth APIs
*/

// We need to know what Operating System is viewing
var operatingSystemName = 'Unknown OS';
if (navigator.appVersion.indexOf('Win') != -1) operatingSystemName = 'Windows';
if (navigator.appVersion.indexOf('Mac') != -1) operatingSystemName = 'MacOS';
if (navigator.appVersion.indexOf('X11') != -1) operatingSystemName = 'UNIX';
if (navigator.appVersion.indexOf('Linux') != -1) operatingSystemName = 'Linux';

// vars needed for Earth/Maps APIs
var ge = null;
var geemap = null;

// Items needed to load the Google Earth Plugin directly to a
// Google Earth Enterprise Server
if (!('google' in window)) { window.google = {}; }
if (!('loader' in window.google)) { window.google.loader = {}; }
if (!('earth' in window.google)) { window.google.earth = {}; }
window.google.loader.ApiKey = 'ABCDEFGHIJKLMNOPgoogle.earth.ec.key';
window.google.loader.KeyVerified = true;
window.google.earth.allowUsageLogging = false;
window.google.earth.setErrorUrl('error.html');

// Catch for older browsers where window.location has no origin property.
if (!window.location.origin) {
  var GEE_PORT = window.location.port ? ':' + window.location.port : '';
  window.location.origin = window.location.protocol + '//' +
      window.location.hostname + GEE_PORT;
}

// Portable Globals pointing to Server.
var GEE_BASE_URL = window.location.origin;
var GEE_SERVER_URL = window.location.origin;
var GEE_SERVER_HOSTNAME = window.location.origin;
var GEE_UNUSED_CHANNEL = 0;

// Global links/Json accesible throughout Portable
var GLOBES_INFO_JSON = '/?cmd=globes_info_json';
var GLOBE_INFO_JSON = '/?cmd=globe_info_json';
var POLYGON_KML = '/earth/polygon.kml';
var SERVER_DEF_JSON = '/query?request=Json&vars=geeServerDefs';
var MAP_JSON = '/query?request=Json&vars=geeServerDefs&is2d=t';
var BROADCASTING_LINK = '/?cmd=is_broadcasting';
var BROADCAST_COMFIRM_ID = '/?cmd=confirmation_id';
var BROADCAST_WARNING = 'Once broadcast is turned on, your globe will be ' +
                        'visible to everyone in your network.\n\nAre you ' +
                        'sure you want to turn it on?\n';
var TURNON_BROADCAST = 'cmd=accept_all_requests';
var TURNOFF_BROADCAST = 'cmd=local_only';
var VERSION_TXT = '/local/version.txt';
var DESCRIPTION_LENGTH = 65;

// Some boolean values we'll check throughout this file
var isServing;
var isBroadcasting = 'disabled';
var isLocalHost = false;

// Various vars used in several functions
var layerTree;
var servingGlobeName;
var servingGlobeDescription;
var newServingGlobeDescription;
var descriptionLength;
var mapLayers = [];
var forceClearCache = Math.random() * 10000000000000000;

// namespace for portable server.
var gee_p = {
  // Names of events.
  events: {
    activeGlobeInfoReady: 'activeGlobeInfoReady',
    broadcastingInfoReady: 'broadcastingInfoReady',
  },
  // Keeps registered listeners.
  listeners: {
  },
};

// indexOf is used for some layer identification in Earth.  This is a helper
// function for IE8, where indexOf is not supported
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

function loadIframes() {
  document.getElementById('IframeSearchResults').style.display = 'block';
  document.getElementById('IframeInfoPane').style.display = 'block';
  document.getElementById('IframeMenu').style.display = 'block';
  document.getElementById('IframeGlobes').style.display = 'block';
  document.getElementById('IframeAbout').style.display = 'block';
}

/**
 * Issues an HTTP request to determine if the map is broadcasting,
 * or serving locally.
 **/
function getBroadcasting() {
  if (isLocalHost) {
    var queryUrl = GEE_SERVER_URL + BROADCASTING_LINK + '&' + forceClearCache;
    // HTTP request for get broadcasting option.
    jQuery.get(queryUrl, function(response, status) {
      isBroadcasting = false;
      if (status == 'success') {
        if (response === '\'disabled\'') {
          isBroadcasting = 'disabled';
        } else {
          isBroadcasting = evalBooleanStr(response, false);
        }
      }
      jQuery(gee_p.listeners).trigger(gee_p.events.broadcastingInfoReady);
    });
  } else {
    jQuery(gee_p.listeners).trigger(gee_p.events.broadcastingInfoReady);
  }
}

// Evaluate content to a boolean value, use defaultVal if failed.
function evalBooleanStr(content, defaultVal) {
  try {
    value = eval('(' + content + ')');
    return typeof value === 'boolean' ? value : defaultVal;
  } catch (Exception) {
    return defaultVal;
  }
}

// Retrieve the build version and date.
function getBuildVersion() {
  var buildDate = 'unknown';
  var buildVersion = 'unknown';
  jQuery.ajax({
    url: GEE_BASE_URL + VERSION_TXT,
    success: function(response) {
      // Expected response from /local/version.txt is
      // version-date, ex 5.0.0-20131231.
      var versionData = response.split('-', 2);
      buildVersion = versionData[0];
      var dt = versionData[1].split('', 8);
      var dtYear = dt[0] + dt[1] + dt[2] + dt[3];
      var dtMonth = dt[4] + dt[5];
      var dtDay = dt[6] + dt[7];
      buildDate = dtMonth + '-' + dtDay + '-' + dtYear;
      document.getElementById('BuildVersion').innerHTML = buildVersion;
      document.getElementById('BuildDate').innerHTML = buildDate;
    }
  });

}

// This is a helper function that validates a form the user
// completes before broadcasting a map
function checkForm() {
  if ((document.broadcast_key_form.key.value == '... enter key here' ||
      document.broadcast_key_form.key.value == '') &&
      !document.broadcast_key_form.accept_all_requests.checked) {
    var span = document.getElementById('ErrorMsg');
    span.innerHTML = 'Please enter a key or select ' +
        '<i>Broadcast without key</i> checkbox.';
    return false;
  }
  return true;
}

// Initializes globes info.
function initActiveGlobeInfo() {
  var queryUrl = GEE_SERVER_URL + GLOBE_INFO_JSON + '&' + forceClearCache;
  // Issue HTTP request for globes info.
  jQuery.get(queryUrl, function(response, status) {
    if (status == 'success' && response) {
      addGlobeInfo(response);
    }

    jQuery(gee_p.listeners).trigger(gee_p.events.activeGlobeInfoReady);
  });
}


/**
 * Retreives information about all available maps/globes.
 */
function openOfflineMap() {
  var queryUrl = GEE_SERVER_URL + GLOBES_INFO_JSON + '&' + forceClearCache;
  // Issue HTTP request for globes info.
  jQuery.get(queryUrl, function(response, status) {
    if (status == 'success' && response) {
      for (var i = 0; i < response.length; i++) {
        addGlobeInfo(response[i]);
      }
      showOpenOfflineMapDialog(true);
    }
  });
}

// Turn offline map dialog on/off.
function showOpenOfflineMapDialog(show) {
  if (show) {
    document.getElementById('OpenOfflineMap').style.display = 'block';
    document.getElementById('fade').style.display = 'block';
  } else {
    document.getElementById('OpenOfflineMap').style.display = 'none';
    document.getElementById('fade').style.display = 'none';
    document.getElementById('GlobeResults').innerHTML = '';
  }
}

// Sets up some global variables for the globe being served, and update
// the list of available globes in OpenOfflineMap dialog.
// Note: the serving globe is not added to the list of available globes.
function addGlobeInfo(jsonGlobeData) {
  var name = jsonGlobeData.name;
  var description = jsonGlobeData.description;
  var twoD = jsonGlobeData.is_2d;
  var threeD = jsonGlobeData.is_3d;
  var serving = jsonGlobeData.is_being_served;
  var path = jsonGlobeData.path;
  path = path.replace(' ', '%20');
  var fileSize = jsonGlobeData.size;
  var timestamp = jsonGlobeData.timestamp;
  var newTime = getLocalDateFromIso8601String(timestamp);
  // We want to turn any links w/in description to plain text.
  var oneDescription = description.replace(/<a href=/g, ' ');
  var listDescription = oneDescription.replace(/<\/a>/g, ' ');

  // For the Globe that is serving, we need to display some info, and
  // also use that globe's info to determine serving type, which can
  // be 2D, 3D or Both.
  if (serving == true) {
    // If no Polygon, remove button to show/hide polygon
    var hasPolygon = jsonGlobeData.has_polygon;
    servingGlobeName = name;
    servingGlobeDescription = listDescription;
    newServingGlobeDescription =
        servingGlobeDescription.substring(0, DESCRIPTION_LENGTH);
    descriptionLength = description.length;
    // The isServing variable is used throughout to make decisions.
    if (twoD == true && threeD == false) {
      isServing = '2D';
    } else if (threeD == true && twoD == false) {
      isServing = '3D';
    } else if (threeD == true && twoD == true) {
      isServing = '2D';
    }
    if (hasPolygon == false) {
      document.getElementById('MenuPolygonZoom').style.display = 'none';
    }
  } else {
    // Not serving means it is available to be served, so here we'll
    // generate a list of all available globes, their type, size,
    // timestamp & description.
    generatedHTML = '<a class="mapslink" title="Click to open ' + name +
        '" href="/?cmd=serve_globe&globe=' + path + '">';

    // List of available globes is populated
    generatedHTML += '<div id ="InfoPaneEntries">';
    if (twoD == true) {
      generatedHTML += '<img src ="/local/preview/images/2d.png"' +
          ' width="18px" height="18px" class="GlobeImg" border="0">';
    } else {
      generatedHTML += '<img src ="/local/preview/images/3d.png"' +
          ' width="18px" height="18px" class="GlobeImg" border="0">';
    }

    generatedHTML += '<div id="NonServingGlobe">' + name + '</div>';
    generatedHTML += '<br>';
    generatedHTML += '<div id="InfoPaneLabel">' + listDescription + '</div>';
    generatedHTML += '<div id="InfoPaneLabel2">';
    if (newTime && newTime != '') {
      generatedHTML += 'Created: ' + newTime + '&nbsp;';
    }
    generatedHTML += 'Size: ' + fileSize + '</div>';
    generatedHTML += '</div>';
    generatedHTML += '</a>';
    document.getElementById('GlobeResults').innerHTML += generatedHTML;
  }
}

function getLocalDateFromIso8601String(timestamp) {
  // Return a string to be displayed in place of timestamp
  // if it is empty/null.
  if (!timestamp || timestamp == '') {
    return false;
  }

  // Create a Javascript Date object.
  var newTime = new Date(timestamp);
  if (isNaN(newTime)) {

    // If newTime is not a number, user is likely using an older browser.
    // In this case, display the string date that was received.
    timestamp = timestamp.split('T')[0];
    return timestamp.split(' ')[0];
  }

  // If newTime is valid, user is in a modern browser.  Return value can be
  // constructed from a valid ISO-8601 string with time zone info.
  var day = ('0' + newTime.getDate()).slice(-2);
  var month = ('0' + (newTime.getMonth() + 1)).slice(-2);
  var year = newTime.getFullYear();

  return [year, month, day].join('-');
}

// Returns the version of Windows Internet Explorer or a -1
// (indicating the use of another browser).
function getInternetExplorerVersion() {
  var rv = -1; // Return value assumes failure.
  if (navigator.appName == 'Microsoft Internet Explorer') {
    var ua = navigator.userAgent;
    var re = new RegExp('MSIE ([0-9]{1,}[\.0-9]{0,})');
    if (re.exec(ua) !== null) {
      rv = parseFloat(RegExp.$1);
    }
  }
  return rv;
}

// Init function - executed on document loading.
function init() {
  // Get build version info from server.
  getBuildVersion();

  // Bind the listeners to the events to manage dependencies in further
  // initialization steps.
  jQuery(gee_p.listeners).bind(
      gee_p.events.activeGlobeInfoReady, initServing);
  jQuery(gee_p.listeners).bind(
      gee_p.events.broadcastingInfoReady, updateBroadcastIcon);

  // Init the globes info and trigger further initialization steps:
  // initialize serving (Maps or Earth plugin depending on OS and serving
  // type), initialize broadcasting widgets.
  initActiveGlobeInfo();

  // Check whether the client is localhost.
  isLocalHost = document.location.hostname == 'localhost' ||
                document.location.hostname == '127.0.0.1';
  updateLocalHostIcons();

  getBroadcasting();
}

// Decide if we are trying to serve a 2D or 3D globe.  If it is
// 3D & Linux (no Linux plugin), or Undefined, we serve an error message.
function initServing() {
  populateInfoPane();
  if (isServing == '3D') {
    init3DServing();
  } else if (isServing == '2D') {
    // Issue asynchronous HTTP request to get serverDefs setting init2DServing
    // as callback-function which will be triggered when request finished.
    getServerDefinitionAsync(GEE_SERVER_URL, true, init2DServing);
  } else if (isServing == undefined) {
    document.getElementById('NoMap').style.display = 'block';
  }
}

// Initializes Earth plugin serving.
function init3DServing() {
  document.getElementById('NoMap').style.display = 'block';
}

// Initializes Map serving.
function init2DServing() {
  // Check that geeServerDefs are loaded.
  if (typeof(geeServerDefs) == 'undefined') {
    alert('Error: The Google Earth Enterprise server does not recognize the ' +
          'requested database.');
    return;
  }

  initMaps();  // requires the geeServerDefs.
  document.getElementById('ZmControls').style.display = 'block';
  document.getElementById('LayerList').style.display = 'block';
  initSearchPane();  // requires the geeServerDefs.
}

// Update the broadcast button.
function updateBroadcastIcon() {
  if (isBroadcasting == 'disabled') {
    document.getElementById('BroadcastButton').style.display = 'none';
  } else {
    document.getElementById('BroadcastButton').innerHTML =
        '<img src="/local/preview/images/' +
        (isBroadcasting ? 'broadcast_true.gif' : 'broadcast_false.gif') +
        '" width="14px" height="11px">';
    document.getElementById('BroadcastButton').style.display = 'block';
  }
}

function updateLocalHostIcons() {
  var display = isLocalHost ? 'block' : 'none';
  document.getElementById('BroadcastButton').style.display = display;
  document.getElementById('OpenButton').style.display = display;
  document.getElementById('ListButton').style.display = display;
}

// Populates the information for the Serving Globe in the
// info pane.
function populateInfoPane() {
  var generatedHTML = '<div id ="InfoPaneEntries">';
  generatedHTML += '</div>';
  generatedHTML += servingGlobeName;
  document.getElementById('InfoPaneServingGlobe').innerHTML =
      servingGlobeName;

  if (descriptionLength > DESCRIPTION_LENGTH) {
    document.getElementById('InfoPaneDescription').innerHTML +=
        newServingGlobeDescription += '... <a href="#"' +
        'onclick="document.getElementById(\'InfoPaneExpandedDescription\')' +
        '.style.display=\'block\';document.getElementById(' +
        '\'InfoPaneDescription\').style.display=\'none\';">more</a>';
    document.getElementById('InfoPaneExpandedDescription').innerHTML +=
        servingGlobeDescription += '&nbsp;&nbsp;<a href="#"' +
        'onclick="document.getElementById(\'InfoPaneExpandedDescription\')' +
        '.style.display=\'none\';document.getElementById(' +
        '\'InfoPaneDescription\').style.display=\'block\';">hide</a>';
  } else {
    document.getElementById('InfoPaneDescription').innerHTML +=
        servingGlobeDescription;
  }
}

// Initializing functions for Earth.
function initEarth() {
  google.earth.createInstance(
      'earth', initCB, failureCB, {database: 'http://localhost:9335'});
}

// Earth callback function. Sets controls for Earth.
function initCB(earth) {
  ge = earth;
  ge.getWindow().setVisibility(true);
  ge.getNavigationControl().setVisibility(ge.VISIBILITY_AUTO);

  // Load the polygon for Earth, and remove it after 4 seconds.
  if (ge.getFeatures().getChildNodes().getLength() < 1) {
    var kmlUrl = GEE_SERVER_URL + POLYGON_KML;
    google.earth.fetchKml(ge, kmlUrl);
  }
  // Load polygon and set camera view.
  loadMapPolygon();
  // Create layer tree.
  showTree();
}

function failureCB(earth) {
  alert('ALERT! The Google Earth Plugin failed to load!');
  document.getElementById('earth').style.display = 'none';
  document.getElementById('NoMap').style.display = 'block';
}

// Initializes Maps if we are serving a 2D Globe.
// Collects layer information for Map to be displayed
// within the InfoPane.
function initMaps() {
  for (var i = 0; i < geeServerDefs.layers.length; i++) {
    var myLayer = new Object();
    var glmId;
    if (geeServerDefs.layers[i].glm_id) {
      glmId = geeServerDefs.layers[i].glm_id;
    } else {
      glmId = 0;
    }
    myLayer.id = glmId + '-' + geeServerDefs.layers[i].id;
    myLayer.label = geeServerDefs.layers[i].label;
    myLayer.state = geeServerDefs.layers[i].initialState;
    mapLayers.push(myLayer);

    //Created a div and an input.  Class name, etc
    var SingleLayerDiv = document.createElement('div');
    var checkbox = document.createElement('input');
    checkbox.type = 'checkbox';

    // We are creating a list of layers for 2D globes.  We are doing some
    // DOM manipulation, and since things are generally different in Win,
    // we are going to handle it a little differently so it works across
    // all browsers as gracefully as possible.
    if (operatingSystemName == 'Windows') {
      if ((geeServerDefs.layers[i].requestType == 'ImageryMaps') && (i == 0)) {
        checkbox.disabled = true;
      }
      var LayerLabel = document.createElement('span');
      if (geeServerDefs.layers[i].initialState == true) {
        if ((geeServerDefs.layers[i].requestType == 'ImageryMaps') &&
            (i == 0)) {
          LayerLabel.innerHTML =
              '<input onclick="Clicked(\'' + myLayer.id + '\');"' +
              'type="checkbox" disabled checked/>' + myLayer.label + "<br/>";
        } else {
          LayerLabel.innerHTML =
              '<input onclick="Clicked(\'' + myLayer.id + '\');"' +
              'type="checkbox" checked/>' + myLayer.label + "<br/>";
        }
      } else {  // initialState is false.
        if ((geeServerDefs.layers[i].requestType == 'ImageryMaps') &&
            (i == 0)) {
          LayerLabel.innerHTML =
              '<input onclick="Clicked(\'' + myLayer.id + '\');"' +
              'type="checkbox" disabled/>' + myLayer.label + "<br/>";
        } else {
          LayerLabel.innerHTML =
              '<input onclick="Clicked(\'' + myLayer.id + '\');"' +
              'type="checkbox"/>' + myLayer.label + "<br/>";
        }
      }
      LayerLabel.innerHTML +=
          '<input type=range id=opacity_' + i +
          ' min=0 value=100 max=100 step=10 ' +
          'oninput="geemap.setOpacity(\'' + myLayer.id +
          '\', Number(value/100))" ' +
          'onchange="outputOpacityValue(\'' + i + '\', Number(value))">';
      LayerLabel.innerHTML += '<output id=opacity_out_' + i + '>100%</output>';
    } else {  // non-Windows OS.
      if ((geeServerDefs.layers[i].requestType == 'ImageryMaps') &&
          (i == 0)) {
        checkbox.disabled = true;
      }
      ClickFnctn = 'Clicked("' + myLayer.id + '");';
      checkbox.className += 'gCheckBox';
      checkbox.setAttribute('onclick', ClickFnctn);
      if (geeServerDefs.layers[i].initialState == true) {
        checkbox.checked = true;
      } else {
        checkbox.checked = false;
      }

      SingleLayerDiv.appendChild(checkbox);
      var LayerLabel = document.createElement('span');
      LayerLabel.innerHTML = '  ' + myLayer.label + "<br/>";
      LayerLabel.innerHTML +=
          '<input type=range id=opacity_' + i +
          ' min=0 value=100 max=100 step=10 ' +
          'oninput="geemap.setOpacity(\'' + myLayer.id +
          '\', Number(value/100))" ' +
          'onchange="outputOpacityValue(\'' + i + '\', Number(value))">';
      LayerLabel.innerHTML += '<output id=opacity_out_' + i + '>100%</output>';
    }


    SingleLayerDiv.appendChild(LayerLabel);

    document.getElementById('LayerList').appendChild(SingleLayerDiv);
  }
  document.getElementById('map').style.display = 'block';
  document.getElementById('earth').style.display = 'none';
  loadMap();
  loadMapPolygon();
}

// Update the current opacity value (as a percentage) shown on the slider.
function outputOpacityValue(id, val) {
  document.querySelector('#opacity_out_' + id).value = val + '%';
}

// Primary loading function for Maps API.
function loadMap() {
  var mapOpts = {
    zoom: 3,
    center: new google.maps.LatLng(0, 0),
    navigationControl: false,
    mapTypeControl: false,
    streetViewControl: false,
    scaleControl: false,
    panControl: false,
    zoomControl: false
  };
  geemap = geeCreateFusionMap('map', geeServerDefs, mapOpts);
}

function createTree() {
  layerTree = new dTree('layerTree');
  var childNodes = ge.getLayerRoot().getFeatures().getChildNodes();
  var firstLayerID = childNodes.item(0).getParentNode().getId();
  firstLayerID = firstLayerID.replace(/\D/g, '');
  layerTree.add(firstLayerID, -1, '');
}

function showTree() {
  createTree();
  queryEarthLayers();
  document.getElementById('TreeStyle').innerHTML = layerTree;
  document.getElementById('MacDiv').style.display = 'block';
  layerTree.openAll();
}

// This is a very important function that gathers layer information
// for a 3D globe.  This is very different from the 2D layers in that
// 3D globe layers can have Child/Parent relationships.  We use
// earth_layer_tree.js, as well as showTree() and createTree()
// functions to help complete this task.
function LayerDeepDive(childLayers, action) {
  for (var z = 0; z < childLayers.getLength(); z++) {
    var layer = childLayers.item(z);
    var id = layer.getId();
    var newId = id.replace(/\D/g, '');
    var name = layer.getName();
    var type = layer.getType();
    var url = layer.getUrl();
    var visibility = layer.getVisibility();
    var parentId = layer.getParentNode().getId();
    var newParentId = parentId.replace(/\D/g, '');
    var layerStyle = layer.getComputedStyle().getListStyle();
    var expandable = layerStyle.getListItemType();
    if (action !== null) {
      layer.setVisibility(false);
    }
    if (visibility == true) {
      var checked = ' checked';
    } else {
      checked = '';
    }
    if (operatingSystemName != 'Windows') {
      var checkClass = 'gCheckBox';
    } else {
      checkClass = '';
    }
    // Layers with type KmlLayerRoot cannot be listed like
    // KmlFolders or KmlLayers.
    if (type == 'KmlLayerRoot') {
      document.getElementById('DbRootLayers').innerHTML +=
          '<span><input type="checkbox" class="' + checkClass + '"' +
          'onclick="toggleDbLayer(\'' + url + '\')"' +
          ' checked>' + name + '</span>';
    } else {
      var children;
      if (type == 'KmlFolder') {
        children = layer.getFeatures().getChildNodes().getLength();
        if (layer.getVisibility() == false) {
          for (var w = 0; w < children; w++) {
            layer.getFeatures().getChildNodes().item(w).setVisibility(false);
          }
        }
      } else {
        children = 0;
      }
      if (!newId) {
        newId = 1;
      }
      if (!newParentId) {
        newParentId = 2;
      }
      document.getElementById('TreeStyle').innerHTML = name;

      if (name == 'Imagery') {
        layerTree.add(
            newId,
            newParentId,
            '<input type="checkbox" disabled="disabled" class="' +
            checkClass + '" onclick="toggleLayer(\'' + id + '\')"' +
            checked + '>' + name);
      } else if (name == 'Terrain' && newId == '1') {
        layerTree.add(
            'terrain' + newId,
            newParentId,
            '<input type="checkbox" class="' +
            checkClass + '" onclick="toggleDbLayer(\'' + url + '\')"' +
            checked + '>' + name);
      } else {
        layerTree.add(
            newId,
            newParentId,
            '<input type="checkbox" class="' +
            checkClass + '" onclick="toggleLayer(\'' + id + '\')"' +
            checked + '>' + name);
      }
      if (expandable == 1 && children > 0) {
        LayerDeepDive(layer.getFeatures().getChildNodes(), action);
      }
    }
  }
}

// Calculates the altitude for the Earth Plugin's camera in order to roughly
// match the Maps API zoom levels (0..31).
// This assumes a fixed GE Field of View (FOV).
function zoomLevelToAltitudeMap() {
  // The following constants
  // Constants needed to calculate zoom level info.
  var RADIANS_PER_DEGREE = 0.017453293;
  var GE_FOV_RADIANS = 30 * RADIANS_PER_DEGREE; // 30 degrees
  var EARTH_RADIUS_METERS = 6367.0 * 1000;

  // We compute it at a close to linear level...say level 10.
  var level = 10;
  var earthCircumference = EARTH_RADIUS_METERS * 2 * Math.PI;
  var twoPowerLevel = 1 << level;  // Each level cuts the horizontal FOV by 2.
  var lengthOfTile = earthCircumference / twoPowerLevel;
  var altitudeAtLevel = (lengthOfTile / 2.0) / Math.tan(GE_FOV_RADIANS / 2.0);
  var map_c = {};

  // We didn't really need to do it that way, but we're extrapolating back
  // to level 0, and will run down the levels by adding a factor of 2.
  // This may not be very "realistic" at levels 0-3, but that's ok for our
  // purposes.
  altitudeAtLevel *= twoPowerLevel;
  for (level = 0; level < 32; ++level) {
    map_c[level] = altitudeAtLevel;
    altitudeAtLevel /= 2.0;
  }
  return map_c;
}

/*
** The section below deals with interactivity functions, such as
** click actions, overlays, and hiding/showing elements.
*/

// Various functions for manipulating Earth Layers
function turnLayerOn(layerId) {
  ge.getLayerRoot().enableLayerById(layerId, true);
}

function turnLayerOff(layerId) {
  ge.getLayerRoot().enableLayerById(layerId, false);
}

function queryEarthLayers() {
  var childLayers = ge.getLayerRoot().getFeatures().getChildNodes();
  LayerDeepDive(childLayers, null);
}

function toggleLayer(layerId) {
  if (ge.getLayerRoot().getLayerById(layerId).getVisibility() == false) {
    ge.getLayerRoot().enableLayerById(layerId, true);
  } else {
    ge.getLayerRoot().enableLayerById(layerId, false);
    if (ge.getLayerRoot().getLayerById(layerId).getType() == 'KmlFolder') {
      var childLayers =
          ge.getLayerRoot().getLayerById(layerId).getFeatures().getChildNodes();
      LayerDeepDive(childLayers, false);
    }
  }
  showTree();
}

function toggleDbLayer(dbName) {
  var children = ge.getLayerRoot().getFeatures().getChildNodes();
  for (var z = 0; z < children.getLength(); z++) {
    var layer = children.item(z);
    var layerUrl = layer.getUrl();
    var layerVisibility = layer.getVisibility();
    if (layerUrl == dbName) {
      if (layerVisibility == true) {
        layer.setVisibility(false);
      } else {
        layer.setVisibility(true);
      }
    }
  }
}


/**
 * Loads map/globe polygon.
 */
function loadMapPolygon() {
  getPolygon(doLoadPolygon);
}


/**
 * Gets polygon info.
 * Issues asynchronous HTTP request to server to get polygon info.
 * @param {function} callbackFunc the callback function to trigger on finishing
 *       the request. The callback function should wait an object of GeoJson
 *       data as an argument.
 */
function getPolygon(callbackFunc) {
  var queryUrl = GEE_SERVER_URL + POLYGON_KML + '?' + forceClearCache;
  // Issue HTTP request for getting polygon info.
  jQuery.get(queryUrl, function(response, status) {
    var polygonJson = undefined;
    if (status == 'success' && response) {
      // Get geojson from polygon XML/KML.
      var ver = getInternetExplorerVersion();
      if (ver > -1) {
        var xmlDoc = new ActiveXObject('Microsoft.XMLDOM');
        xmlDoc.async = false;
        xmlDoc.loadXML(response);
        polygonJson = toGeoJSON.kml(xmlDoc, 'text/xml');
      } else {
        // Clean the KML of any leading whitespace. In some cases whitespace,
        // or an unneccesary line break will stop the KML from being parsed.
        var cleanedXml = response.replace(/^\s+|\s+$/g, '');
        var parser = new DOMParser();
        polygonJson = toGeoJSON.kml(
            parser.parseFromString(cleanedXml, 'text/xml'));
      }
    }
    callbackFunc(polygonJson);
  });
}


/**
 * Reads polygon coordinates from object of GeoJson data containing a KML
 * polygon and draws polygon.
 *
 * @param {object} polygonJson the object of GeoJSON data.
 */
function doLoadPolygon(polygonJson) {
  if (typeof(polygonJson) == 'undefined' ||
      typeof(polygonJson.features) == 'undefined' ||
      polygonJson.features.length == 0 ||
      typeof(polygonJson.features[0].geometry.coordinates) == 'undefined' ||
      polygonJson.features[0].geometry.coordinates.length == 0) {
    // Do nothing.
    return;
  }

  var coordinates = [];

  if (isServing == '2D') {
    var polygonCoords = polygonJson.features[0].geometry.coordinates[0];
    for (var z = 0; z < polygonCoords.length; z++) {
      var lat = parseFloat(polygonCoords[z][1]);
      var lng = parseFloat(polygonCoords[z][0]);
      // If lat or lng value appears bogus, abandon the polygon.
      if (isNaN(lat) || (lat < -90.0) || (lat > 90.0)) {
        return;
      }
      if (isNaN(lat) || (lng < -180.0) || (lng > 180.0)) {
        return;
      }
      var latLng = new google.maps.LatLng(lat, lng);
      coordinates.push(latLng);
    }
    drawMapPolygon(coordinates, '#FF0000', '#FF00FF');
  } else if (isServing == '3D') {
    var polygonPlacemark = ge.createPlacemark('');
    var polygonKml = ge.createPolygon('');
    polygonPlacemark.setGeometry(polygonKml);
    var outer = ge.createLinearRing('');
    polygonKml.setOuterBoundary(outer);

    // Outer boundary coordinates.
    var outer_coords = outer.getCoordinates();

    var polygonCoords = polygonJson.features[0].geometry.coordinates[0];
    for (var z = 0; z < polygonCoords.length; z++) {
      var lat = parseFloat(polygonCoords[z][1]);
      var lng = parseFloat(polygonCoords[z][0]);
      var entry = [lat, lng];
      // TODO: calculate bounding box on the fly.
      coordinates.push(entry);
      outer_coords.pushLatLngAlt(lat, lng, 0);
    }
    var bounds = geePolygonBounds(coordinates);
    var height = bounds.north - bounds.south;
    var width = bounds.east - bounds.west;
    var zoomLevel = Math.floor(
        Math.log(360 / Math.max(height, width)) / Math.log(2));
    var centerLat = (bounds.north + bounds.south) / 2;
    var centerLng = (bounds.east + bounds.west) / 2;
    geeZoomLevelToCameraAltitudeMap = zoomLevelToAltitudeMap();
    var lookAt = ge.createLookAt('');
    lookAt.setLatitude(centerLat);
    lookAt.setLongitude(centerLng);
    lookAt.setRange(geeZoomLevelToCameraAltitudeMap[zoomLevel]);
    ge.getView().setAbstractView(lookAt);
    polygonPlacemark.setStyleSelector(ge.createStyle(''));
    var lineStyle = polygonPlacemark.getStyleSelector().getLineStyle();
    lineStyle.setWidth(lineStyle.getWidth() + 2);
    // Color is specified in 'aabbggrr' format.
    lineStyle.getColor().set('c0800080');
    // Color can also be specified by individual color components.
    var polyColor = polygonPlacemark.getStyleSelector().getPolyStyle();
    polyColor.getColor().set('c0800080');
    ge.getFeatures().appendChild(polygonPlacemark);
    polygonMagic();
  }
}

// Draws polygon on map (2D).
function drawMapPolygon(coordinates, stroke_color, fill_color) {
  var polygon = new google.maps.Polygon({
    paths: coordinates,
    strokeColor: stroke_color,
    strokeOpacity: 1.0,
    strokeWeight: 1,
    fillColor: fill_color,
    fillOpacity: 0.4
  });
  polygon.setMap(geemap);
  //Figure out the polygon bounds and pan/zoom there
  var bounds = new google.maps.LatLngBounds;
  polygon.getPath().forEach(function(latLng) {
    bounds.extend(latLng);
  });
  geemap.fitBounds(bounds);
  var myVar = setTimeout(function() {
    polygon.setMap(null);
  }, 3000);
}

// Next three functions get, display, and remove the Polygon in 3D.
function GetKmlCoordinates(kmlObject) {
  return kmlObject.getGeometry().getOuterBoundary().getCoordinates();
}

// Remove polygon function (3D)
function RemoveKMLPolygon() {
  if (ge.getFeatures().getChildNodes().getLength() > 0) {
    ge.getFeatures().removeChild(ge.getFeatures().getFirstChild());
  }
}

// Helper function to auto-remove polygon after 3 seconds
function polygonMagic() {
  var myVar = setTimeout(function() {polygonTimer()}, 3000);
  function polygonTimer() {
    RemoveKMLPolygon();
  }
  return polygonTimer;
}

// Returns bounding box of the polygon.
function geePolygonBounds(coordinates) {
  var minLat = 90.0;
  var maxLat = -90.0;
  var minLng = 180.0;
  var maxLng = -180.0;
  for (var i = 0; i < coordinates.length; i++) {
    lat = coordinates[i][0];
    lng = coordinates[i][1];
    if (lat < minLat) {
      minLat = lat;
    }
    if (lat > maxLat) {
      maxLat = lat;
    }
    if (lng < minLng) {
      minLng = lng;
    }
    if (lng > maxLng) {
      maxLng = lng;
    }
  }
  if (minLat < -90.0) {
    minLat = -90.0;
  }
  if (maxLat > 90.0) {
    maxLat = 90.0;
  }
  if (minLng < -180.0) {
    minLng = -180.0;
  }
  if (maxLng > 180.0) {
    maxLng = 180.0;
  }
  return {
    south: minLat,
    north: maxLat,
    west: minLng,
    east: maxLng
  };
}

// Lightbox effect for popups.
function lightBox() {
  var ele = document.getElementById('OptionsMenu');
  var text = document.getElementById('DisplayText');
  if (ele.style.display == 'block') {
    ele.style.display = 'none';
    text.innerHTML = '<div id="ListButton"></div>';
  } else {
    ele.style.display = 'block';
    text.innerHTML = '<div id="ListButton"></div>';
  }
}


// Set a listener for the About item on the menu in Portable.
jQuery(document).ready(function() {

  // Menu item div that contains text 'About'.
  var aboutDiv = document.getElementById('aboutDiv');

  // Hidden element that contains details about Portable.
  var aboutBox = document.getElementById('aboutBox');

  // Show Portable information on hover.
  aboutDiv.addEventListener('mouseenter', function() {
    aboutBox.style.display = 'block';
  });

  // Hide Portable information when About menu item loses focus.
  aboutDiv.addEventListener('mouseleave', function() {
    aboutBox.style.display = 'none';
  });

});

// Shows/ hides list of layers in InfoPane
function toggleLayerList() {
  var ele = document.getElementById('TreeStyle');
  var lineBreak = document.getElementById('LineBreakMenu');
  var mapList = document.getElementById('LayerList');
  var dbLayers = document.getElementById('DbRootLayers');
  var listOfLayers = document.getElementById('LayerListToggle');
  if (ele.style.display == 'block' || mapList.style.display == 'block') {
    ele.style.display = 'none';
    mapList.style.display = 'none';
    dbLayers.style.display = 'none';
    lineBreak.style.display = 'none';
    listOfLayers.innerHTML = '<label><a href="javascript:lightBox();" ' +
        'onclick = "toggleLayerList();" title="Show Cut Area">' +
        'Show layer list</a></label>';
  } else {
    ele.style.display = 'block';
    lineBreak.style.display = 'block';
    mapList.style.display = 'block';
    dbLayers.style.display = 'block';
    listOfLayers.innerHTML = '<label><a href="javascript:lightBox();" ' +
        'onclick = "toggleLayerList();" title="Show Cut Area">' +
        'Hide layer list</a></label>';
  }
}

// Toggle broadcast by the broadcast button.
function toggleBroadcast() {
  // Should always be local and broadcast enabled, do extra check here in case
  // it is called by other function.
  if (!isLocalHost || isBroadcasting == 'disabled') {
    return;
  }

  var queryUrl = GEE_SERVER_URL + BROADCAST_COMFIRM_ID + '&' +
                 forceClearCache;

  // HTTP request for get broadcasting option.
  jQuery.get(queryUrl, function(response, status) {
    if (status == 'success' && response) {
      confirmId = response;
      if (isBroadcasting || !isBroadcasting && confirm(BROADCAST_WARNING)) {
        queryParms = (isBroadcasting ? TURNOFF_BROADCAST : TURNON_BROADCAST) +
          '&confirmation_id=' + confirmId + '&' + forceClearCache;
        jQuery.post(GEE_SERVER_URL, queryParms);
        isBroadcasting = !isBroadcasting;
        jQuery(gee_p.listeners).trigger(gee_p.events.broadcastingInfoReady);
      }
    }
  });
}

// Helper function for clicking Map layers
function LookupLayer(keyId) {
  var index = undefined;
  for (var i = 0; i < mapLayers.length; i++) {
    if (mapLayers[i].id == keyId) {
      index = i;
      break;
    }
  }
  return index;
}

// Turn layer on/off fn for Maps
function Clicked(layerId) {
  var myLayerLoc = LookupLayer(layerId);
  if (myLayerLoc == undefined) {
    return;
  }
  if (mapLayers[myLayerLoc].state == true) {
    geemap.hideFusionLayer(layerId);
    mapLayers[myLayerLoc].state = false;
  } else {
    geemap.showFusionLayer(layerId);
    mapLayers[myLayerLoc].state = true;
  }
}

//Zooming effects (2D Only)
function ZoomTheMap(action) {
  var CurZm = geemap.zoom;
  if ((action == 'out') && (CurZm > 1)) {
    if (CurZm == null || CurZm == undefined) {
      CurZm = 1;
    }
    var NewZm = CurZm - 1;
    geemap.setZoom(NewZm);
  }
  if ((action == 'in') && (CurZm < 19)) {
    if (CurZm == null || CurZm == undefined) {
      CurZm = 19;
    }
    NewZm = CurZm + 1;
    geemap.setZoom(NewZm);
  }
}

// Close popups on ESC key if applicable
jQuery(document).keyup(function(e) {
  if (e.keyCode == 27) {
    closeAllPopups();
  }
});

// Check which popups are open so we can close them
function closeAllPopups() {
  if (document.getElementById('OpenOfflineMap').style.display == 'block') {
    showOpenOfflineMapDialog(false);
  }
  if (document.getElementById('fade').style.display == 'block') {
    document.getElementById('fade').style.display = 'none';
  }
}
