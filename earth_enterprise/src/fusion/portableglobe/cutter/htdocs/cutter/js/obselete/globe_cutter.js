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
 * @fileoverview This file presents an example of a page that makes use of the
 *               Google Earth Plugin from a Google Earth Enterprise Server. It
 *               handles the following: examples of how to use the Google Earth
 *               Plugin API a simple layer panel for the layers provided by the
 *               GEE Server search tabs from the GEE Server Search Framework
 *               search results from the GEE Server.
 *
 * This file contains most of the work to customize the look and feel and
 * behaviors in the page as well as the calls to the plugin. Depends on :
 *   fusion_utils.js : some basic utilities for accessing DOM elements, and a
 *                     few low level utilities used by this example.
 *   search_tabs.js : sample classes for showing/managing search tabs and
 *                    results.
 *   earth_plugin_loader.js : do not modify...this code does the work to
 *                            load the plugin
 *
 * ASSUMES:
 *   GEE_SERVER_HOSTNAME is set
 */

/**
 * Load the specified javascript.
 * @param {string}
 *          path the path relative to the current web page of the javascript.
 */
function geeLoadScript(path) {
  document.write('<script src="' + path +
                 '" type="text/javascript"><\/script>');
}

// Load the Earth database info into the JS global variable 'geeServerDefs'.
// This will have the fields:
// serverUrl : the url of the server
// isAuthenticated : true if the server is authenticated
// searchTabs : a JSON array of search tab definitions.
// layers : a JSON array of supplemental layer info
// If desired, you can point this to another GEE Server or use a different
// variable for multiple servers.
if (GEE_SERVER_URL == '') {
  // We have to guess what the default is.
  geeLoadScript('/default_ge/query?request=Json&var=geeServerDefs');
} else {
  geeLoadScript(GEE_SERVER_URL + 'query?request=Json&var=geeServerDefs');
}

var g_ge;
var g_gex;
var g_geometryToolbox;
var g_dirty;
var g_editing;
var g_features;

var outerBoundary = null;
var coords = null;
var pointCount = 0;
var doc = null;
var polygonPlacemark;

// The div ids for the left panel, map and header.
// These must match the html and css declarations.
var geeDivIds = {
    header: 'header',
    map: 'map'
};

// Static image paths in this example are relative to the earth server host.
var GEE_EARTH_IMAGE_PATH = GEE_BASE_URL + '/earth/images/';

// Search Timeout specifies how long to wait (in milliseconds) before
// determining the search failed.
// override with '?search_timeout=xxx' on the URL where xxx is in milliseconds.
var geeSearchTimeout = 5000;  // 5 seconds seems OK

// Time between updates of size when cutting a globe.
var GEE_SIZE_UPDATE_TIME = 1500;  // 1.5 seconds seems OK

// Initial view parameters are overrideable from the html or the url.
// Override initial lat lng with '?ll=32.898,-100.30' on the URL.
// Override initial zoom level with '?z=8' on URL
var geeInitialViewLat = 37.422;
var geeInitialViewLon = -95.08;
var geeInitialZoomLevel = 3;

// Default Altitude for panning, keep camera at a high enough level to see
// continents.
var DEFAULT_SINGLE_CLICK_ZOOM_LEVEL = 10;  // City Level.
var DEFAULT_DOUBLE_CLICK_ZOOM_LEVEL = 14;  // Neighborhood level.
var DEFAULT_PAN_TO_ALTITUDE_METERS = 400000;
var DEFAULT_BALLOON_MAX_WIDTH_PIXELS = 500;

var DEFAULT_FEATURE_STYLE = {
  poly: {
    color: 'blue',
    opacity: 0.5
  }
};

// We cache a map of Maps Zoom Levels (0..32) to Earth camera altitudes.
var geeZoomLevelToCameraAltitudeMap = null;
var geeIconUrls = {};  // Cache of frequently used URLs.

// We need to cache some information to manage network link layers for the
// earth plugin.
var geIsNetworkLink = {};  // Keep track of whether each layer is a network link
                           // or not. layerId: boolean
var geNetworkLinks = {};  // Keep track of network links KmlFeature objects.
var geLoadedNetworkLinks = {};  // Keep track of the loaded network links.
var geeFolderLayerChildren = {};  // Keep track of the layer id's of the
                                  // children of layer folders.
var geeSupplementalLayerInfo = {};  // Index for additional layer info that is
                                    // not accessible via the GE Plugin API.

// Example:
// var FORCE_ARGUMENTS = '&FORCE_PORTABLE_PORT=8778' +
//                       '&FORCE_PORTABLE_TMP=' +
//                           '/usr/local/google/portable_server/tmp';
var FORCE_ARGUMENTS = '';

/**
 * Check that we have all of the need values for the globe.
 * @return {string} Error messages or empty string if parameters are ok.
 */
function geeCheckGlobeParameters() {
  var msg = '';
  var globe_name = document.globe_cutter_form.globe_name.value;
  if (globe_name.substring(0, 1) == ' ') {
    msg += 'Please enter a globe name (no spaces).<br/>';
  } else {
    // If we are accepting the globe name,
    // remove unwanted character patterns from it.
    globe_name = globe_name.replace(/\s+/g, '_');
    globe_name = globe_name.replace(/\.\./g, '_');
    globe_name = globe_name.replace(/\//g, '_');
  }

  document.globe_cutter_form.globe_name.value = globe_name

  if (!geeCheckInt(document.globe_cutter_form.default_level, 1, 24)) {
    msg += 'Minimum resolution should be in range 1 to 24.<br/>';
  }

  if (!geeCheckInt(document.globe_cutter_form.max_level, 1, 24)) {
    msg += 'Maximum resolution should be in range 1 to 24.<br/>';
  }

  if (!geeCheckInt(document.globe_cutter_form.polygon_level, 1, 24)) {
    msg += 'Polygon resolution should be in range 1 to 24.<br/>';
  }

  return msg;
}

/**
 * Use a series of AJAX calls to cut a portable globe and make it available
 * for downloading. The AJAX calls provide some feedback along the way.
 */
function geeBuildGlobe() {
  geeSetResponse('Building portable globe ...');
  var msg = geeCheckGlobeParameters();
  if (msg != '') {
    geeSetResponse(msg + 'Please fill in all required values.');
    return;
  }

  var url = '/cgi-bin/globe_cutter_app.py?cmd=UID&globe_name=' + geeGlobeName();
  if (geeAllowOverwrite()) {
    url += '&allow_overwrite=t'
  }
  jQuery.get(url,
             function(uid_msg) {
                 var new_globe_info = uid_msg.split(' ');
                 var uid = new_globe_info[0];
                 GEE_setUid(uid);
                 GEE_setForceArguments(FORCE_ARGUMENTS);
                 var new_globe_name = new_globe_info[1];
                 GEE_setGlobeName(new_globe_name);
                 if (new_globe_info[2] == 'True') {
                   geeSetResponse('Overwriting <i>' +
                                  new_globe_name + '</i> ...');
                 } else {
                   geeSetResponse('Building <i>' + new_globe_name +
                                  '</i> ...');
                 }

                 var polygon_level = 'polygon_level=' + geePolygonLevel();
                 var polygon = 'polygon=' + escape(geePolygon());
                 var default_level = 'default_level=' + geeDefaultLevel();
                 var max_level = 'max_level=' + geeMaxLevel();
                 var source = 'source=' + escape(geeSource());
                 var description = ('description=' +
                                    escape(geeGlobeDescription()));
                 var globe_name = 'globe_name=' + escape(new_globe_name);
                 var base_url = '/cgi-bin/globe_cutter_app.py", "cmd=';
                 // Always delete tmp directory after a successful build.
                 var save_tmp = 'save_tmp=f';
                 var front = 'GEE_postData("';
                 var back1 = (FORCE_ARGUMENTS + '&uid=' + uid +
                              '", "status", "APPEND");');
                 var back2 = (FORCE_ARGUMENTS + '&uid=' + uid +
                              '", "globe", "SET");');
                 var wait_for_task = 'GEE_checkTaskDone();';
                 var sequence = [];
                 sequence[0] = front + base_url + 'ADD_GLOBE_DIRECTORY&' +
                     globe_name + '&' + description + back1;
                 sequence[1] = front + base_url + 'POLYGON_TO_QTNODES&' +
                     globe_name + '&' + polygon + '&' + polygon_level + back1;
                 sequence[2] = front + base_url + 'REWRITE_DB_ROOT&' +
                     globe_name + '&' + source + back1;
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

                 geeClearGlobeInfo();
                 GEE_runAjaxSequence(sequence, GEE_SIZE_UPDATE_TIME);
               });
}

/**
 * The Earth Plugin init callback.
 * @param {GEPlugin}
 *          object the just created Google Earth plugin object.
 */
function geeEarthPluginInitCb(object) {
  g_ge = object;

  // create the polygon and set its boundaries
  outerBoundary = g_ge.createLinearRing('');
  coords = outerBoundary.getCoordinates();

  // show lat,lon,height,eye_alt
  g_ge.getOptions().setStatusBarVisibility(true);
  // show scale legend
  g_ge.getOptions().setScaleLegendVisibility(true);
  g_ge.getWindow().setVisibility(true);
  g_ge.getNavigationControl().setVisibility(g_ge.VISIBILITY_SHOW);

  // Initialize the initial view (which can be overridden by URL parameters).
  var lat = geeInitialViewLat;
  var lon = geeInitialViewLon;
  var zoom = geeInitialZoomLevel;
  // Process the initial view overrides.
  if (getPageURLParameter('ll')) {
    var latlng = getPageURLParameter('ll').split(',');
    if (latlng.length == 2) {
      lat = latlng[0];
      lon = latlng[1];
    }
  }

  if (getPageURLParameter('z')) {
    zoom = parseInt(getPageURLParameter('z'));
  }

  geePanTo(lat, lon, zoom);
  g_gex = new GEarthExtensions(g_ge);

  geeCreateGeometryToolbox();
}

/**
 * The Earth Plugin failure callback.
 * @param {string}
 *          message the error message.
 */
function geeEarthPluginFailureCb(message) {
  // The default behavior of the plugin will display a friendly error message.
}

/**
 * Clear polygon from the earth plugin.
 */
function geeClearPolygon() {
  g_gex = new GEarthExtensions(g_ge);
  g_gex.dom.clearFeatures();
}

/**
 * Add polygon based on kml in form.
 */
function geeAddPolygon() {
  var polygonKml = document.globe_cutter_form.polygon.value;
  polygonKml = polygonKml.replace('<visibility>0</visibility>',
                                  '<visibility>1</visibility>');
  var kmlObject = g_ge.parseKml(polygonKml);
  if (kmlObject.getType() == 'KmlDocument') {
    polygonKml = polygonKml.replace(/\r/g, ' ');
    polygonKml = polygonKml.replace(/\n/g, ' ');
    polygonKml = polygonKml.replace(/<Document>.*<Placemark>/,
                                    '<Placemark>');
    polygonKml = polygonKml.replace(/<\/Placemark>.*<\/Document>/,
                                    '</Placemark>');
    kmlObject = g_ge.parseKml(polygonKml);
  }

  if (kmlObject.getType() == 'KmlPlacemark') {
    geeClearPolygon();
    g_ge.getFeatures().appendChild(kmlObject);

    if (kmlObject.getAbstractView() !== null) {
      g_ge.getView().setAbstractView(kmlObject.getAbstractView());
    }

    var coordArray =
        kmlObject.getGeometry().getOuterBoundary().getCoordinates();
    minLat = 90.0;
    maxLat = -90.0
    minLon = 180.0;
    maxLon = -180.0;
    for (var i = 0; i < coordArray.getLength(); i++) {
      lat = coordArray.get(i).getLatitude();
      lon = coordArray.get(i).getLongitude();
      if (lat < minLat) {
        minLat = lat;
      }
      if (lat > maxLat) {
        maxLat = lat;
      }
      if (lon < minLon) {
        minLon = lon;
      }
      if (lon > maxLon) {
        maxLon = lon;
      }
    }

    latDiff = maxLat - minLat;
    lonDiff = maxLon - minLon;
    if (latDiff > lonDiff) {
      maxDist = latDiff;
    } else {
      maxDist = lonDiff;
    }
    zoomLevel = Math.floor(Math.log(360 / maxDist) / Math.log(2))
    geePanTo((minLat + maxLat) / 2, (minLon + maxLon) / 2, zoomLevel);
  }
}

/**
 * Pan and Zoom the Earth viewer to the specified lat, lng and zoom level.
 * @param {string}
 *          lat the latitude of the position to pan to.
 * @param {string}
 *          lng the longitude of the position to pan to.
 * @param {number}
 *          zoomLevel [optional] the zoom level (an integer between 1 : zoomed
 *          out all the way, and 32: zoomed in all the way) indicating the zoom
 *          level for the view.
 */
function geePanTo(lat, lng, zoomLevel) {
  lat = parseFloat(lat);
  lng = parseFloat(lng);
  var la = g_ge.createLookAt('');
  if (zoomLevel == null) {
    zoomLevel = DEFAULT_SINGLE_CLICK_ZOOM_LEVEL;
  }

  la.set(lat, lng, 100, g_ge.ALTITUDE_RELATIVE_TO_GROUND, 0, 0,
         geeZoomLevelToCameraAltitudeMap[zoomLevel]);
  g_ge.getView().setAbstractView(la);
}

////////////////////////////////////////////////////////////////////////////////
// GEOMETRY TOOLBOX-SPECIFIC FUNCTIONS
/**
* Creates a geometry toolbox button with the given properties. The sole
* parameter, button, should have the following properties:
*   - id: String
*   - pos: Number[2], indicating the x, y in pixels
*   - size: Number[2], indicating the width, height in pixels
*   - image: String, URL of image
*   - imageOn: String, URL of image to use when the button is selected
*   - onClick: Function, a function to be called when this button is clicked
* @param {object} button Button info for creating an embedded button.
*/
function geeCreateToolboxButton(button) {
  button.earthObject = g_gex.dom.buildScreenOverlay({
    drawOrder: 1000,
    icon: button.image,
    screenXY: { left: button.pos[0], top: button.pos[1] },
    size: { width: button.size[0], height: button.size[1] }
  });

  button.earthObjectOn = g_gex.dom.buildScreenOverlay({
    drawOrder: 1001,
    icon: button.imageOn,
    screenXY: { left: button.pos[0], top: button.pos[1] },
    size: { width: button.size[0], height: button.size[1] }
  });

  g_geometryToolbox.buttonContainerEarthObject
      .getFeatures().appendChild(button.earthObject);
  g_geometryToolbox.buttonContainerEarthObject
      .getFeatures().appendChild(button.earthObjectOn);

  g_geometryToolbox.buttons[button.id] = button;
}

function geeCreateGeometryToolbox() {
  if (g_geometryToolbox) {
    return;
  }

  g_geometryToolbox = {
    buttons: {},
    buttonContainerEarthObject: g_ge.createDocument('')
  };

  var args_start = window.location.href.lastIndexOf('?');
  var pagePath;
  if (args_start >= 0) {
    pagePath = window.location.href.substring(0, args_start);
  } else {
    pagePath = window.location.href;
  }
  pagePath = pagePath.substring(0, pagePath.lastIndexOf('/')) + '/';
  var button_size = 31
  var x = 20 - button_size;

  // create buttons
  geeCreateToolboxButton({
      id: 'hand',
      pos: [x += button_size, 20],
      size: [button_size, button_size],
      image: pagePath + 'images/etb-hand.png',
      imageOn: pagePath + 'images/etb-hand-on.png',
      onClick: function() { geeAbortCreateFeature(); }
      });

  geeCreateToolboxButton({
      id: 'poly',
      pos: [x += button_size, 20],
      size: [button_size, button_size],
      image: pagePath + 'images/etb-poly.png',
      imageOn: pagePath + 'images/etb-poly-on.png',
      onClick: function(){ geeBeginCreateFeature('poly'); }
      });

  geeSelectGeometryToolboxButton('hand');
  g_ge.getFeatures().appendChild(g_geometryToolbox.buttonContainerEarthObject);

  // set up event listener to trap button clicks
  google.earth.addEventListener(
      g_ge.getWindow(), 'mousedown', geeGeometryToolboxWindowMousedownHandler);
}

/**
* Window mousedown handler that checks for clicks in geometry toolbox
* buttons and fires off the appropriate onClick handlers.
* @param {object} event Next mouse down event.
* @return {bool} whether event should still be handled.
*/
function geeGeometryToolboxWindowMousedownHandler(event) {
  if (event.getButton() != 0) { // left click
    return false;
  }

  var mousePos = [event.getClientX(), event.getClientY()];

  for (var id in g_geometryToolbox.buttons) {
    var button = g_geometryToolbox.buttons[id];

    // see if this button was clicked
    if (button.pos[0] <= mousePos[0] && mousePos[0] <= button.pos[0] +
      button.size[0] &&
      button.pos[1] <= mousePos[1] && mousePos[1] <= button.pos[1] +
      button.size[1]) {
        if (g_geometryToolbox.featureInCreation != null) {
          return false;
        }
        geeSelectGeometryToolboxButton(id);
        if (button.onClick) {
          button.onClick.call(button);
        }

        event.preventDefault();
        event.stopPropagation();
        return false;
      }
    }

    return true;
  }

/**
 * Selects the geometry toolbox button with the given ID.
 * @param {string} selId Id of the selected item.
 */
function geeSelectGeometryToolboxButton(selId) {
  g_gex.util.batchExecute(function() {
    for (var id in g_geometryToolbox.buttons) {
      var button = g_geometryToolbox.buttons[id];
      button.earthObjectOn.setVisibility(id === selId);
    }
  });
}

function geeBeginCreateFeature(featureType) {
  geeSetResponse('Double click to complete polygon.');
  g_gex.dom.removeObject(polygonPlacemark);
  g_geometryToolbox.featureInCreation = {
    id: 'poly2010',
    title: featureType.charAt(0).toUpperCase() +
    featureType.substring(1),
    kml: null,
    earthObject: null,
    type: featureType,
    dirty: true
  };

  switch (featureType) {
    case 'poly':
      polygonPlacemark = g_gex.dom.addPlacemark({
          polygon: [],
          style: DEFAULT_FEATURE_STYLE
          });
      g_gex.edit.drawLineString(
          polygonPlacemark.getGeometry().getOuterBoundary(), {
            finishCallback: function() {
              var coordArray = polygonPlacemark.getGeometry()
                               .getOuterBoundary().getCoordinates();

            polygonPlacemark.getStyleSelector()
                .getPolyStyle().getColor().set('c0800080');
            geeFinishCreateFeature();
          } });
    break;
  }
}

function geeAbortCreateFeature() {
  // Set the polygon form field so it can be used for cutting the globe.
  geeSetResponse('Aborted polygon.');
  geeSetPolygon(polygonPlacemark.getKml());
}

function geeFinishCreateFeature() {
  // Set the polygon form field so it can be used for cutting the globe.
  geeSetPolygon(polygonPlacemark.getKml());
  geeSelectGeometryToolboxButton('hand');

  g_geometryToolbox.featureInCreation = null;
  geeSetResponse('ROI selected.');
}

function geeInsertIntoDiv() {
  // Init some globals that require other JS modules to be loaded.
  geeZoomLevelToCameraAltitudeMap = zoomLevelToAltitudeMap();

  // --------------------------------Begin GEE specific settings
  // Required for Behind the firewall usage.
  // Enterprise specific overrides for running the Earth plugin behind
  // the firewall.
  if (!('google' in window)) { window.google = {}; }
  if (!('loader' in window.google)) { window.google.loader = {}; }
  if (!('earth' in window.google)) { window.google.earth = {}; }
  // Enterprise Earth Plugin Key
  window.google.loader.ApiKey = 'ABCDEFGHIJKLMNOPgoogle.earth.ec.key';
  window.google.loader.KeyVerified = true;
  // Turn off logging.
  window.google.earth.allowUsageLogging = false;
  // Override the default google.com error page.
  window.google.earth.setErrorUrl('/earth/error.html');
  // Override the default loading icon.
  window.google.earth.setLoadingImageUrl(geeEarthImage('loading.gif'));
  // --------------------------------End GEE specific settings

  // **************************************************************************
  // You will need to replace yourserver.com with the appropriate server name.
  // For authentication to your database,
  // simply add arguments 'username' and 'password' to earthArgs.
  // IE6 compatibility note: no trailing commas in dictionaries or arrays.
  var earthArgs = {
    'database' : geeServerDefs['serverUrl']
  };
  if (geeServerDefs['isAuthenticated']) {
    // Pop up auth dialog if desired.
    var username = '';
    var password = '';
    earthArgs['username'] = username;
    earthArgs['password'] = password;
  }
  // We construct the internal layer container and title divs.
  // The inner container is needed globally.
  google.earth.createInstance(geeDivIds.map, geeEarthPluginInitCb,
                              geeEarthPluginFailureCb, earthArgs);
}

