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


if (GEE_SERVER_URL == undefined) {
  var GEE_SERVER_URL = '';
}

// The global variable geeServerDefs for loading the Earth database info of
// the current served database.
// This will have the fields:
// serverUrl : the url of the server
// isAuthenticated : true if the server is authenticated
// searchTabs : a JSON array of search tab definitions.
// layers : a JSON array of supplemental layer info
// If desired, you can point this to another GEE Server or use a different
// variable for multiple servers.
var geeServerDefs;

var ge = null; // The Google Earth plugin object.
               // Initialized by the init() method on load.
var gex = null;

var isMouseDown = false;
var outerBoundary = null;
var coords = null;
var pointCount = 0;
var doc = null;
var polygonPlacemark;

// The div ids for the left panel, map and header.
// These must match the html and css declarations.
var geeDivIds = {
    header: 'header',
    map: 'map',
    leftPanelParent: 'left_panel_cell',
    leftPanel: 'left_panel',
    searchTabs: 'search_tabs',
    searchTitle: 'search_results_title',
    searchResults: 'search_results_container',
    layersTitle: 'layers_title',
    layers: 'layers_container',
    collapsePanel: 'collapsePanel',
    collapseShim: 'collapseShim'
};
// Need the static URL for absolute addresses
var GEE_STATIC_URL = window.location.protocol + '//' + window.location.host;
// Static image paths in this example are relative to the earth server host.
var GEE_EARTH_IMAGE_PATH = GEE_STATIC_URL + '/earth/images/';
var GEE_MAPS_IMAGE_PATH = GEE_STATIC_URL + '/maps/mapfiles/';

// Search Timeout specifies how long to wait (in milliseconds) before
// determining the search failed.
// override with '?search_timeout=xxx' on the URL where xxx is in milliseconds.
var geeSearchTimeout = 5000;  // 5 seconds seems OK

// Initial view parameters are overrideable from the html or the url.
// Override initial lat lng with '?ll=32.898,-100.30' on the URL.
// Override initial zoom level with '?z=8' on URL
var geeInitialViewLat = 37.422;
var geeInitialViewLon = -122.08;
var geeInitialZoomLevel = 6;

// Default Altitude for panning, keep camera at a high enough level to see
// continents.
var DEFAULT_SINGLE_CLICK_ZOOM_LEVEL = 10;  // City Level.
var DEFAULT_DOUBLE_CLICK_ZOOM_LEVEL = 14;  // Neighborhood level.
var DEFAULT_PAN_TO_ALTITUDE_METERS = 400000;
var DEFAULT_BALLOON_MAX_WIDTH_PIXELS = 500;
// We cache a map of Maps Zoom Levels (0..32) to Earth camera altitudes.
var geeZoomLevelToCameraAltitudeMap = null;
var geeIconUrls = {};  // Cache of frequently used URLs.

// We need to cache some information to manage network link layers for the
// earth plugin.
var geIsNetworkLink = {};  // Keep track of whether each layer is a network link
                           // or not. layerId: boolean
var geNetworkLinks = {};  // Keep track of the network links KmlFeature objects.
var geLoadedNetworkLinks = {};  // Keep track of the loaded network links.
var geeFolderLayerChildren = {};  // Keep track of the layer id's of the
                                  // children of layer folders.
var geeSupplementalLayerInfo = {};  // Index for additional layer info that is
                                    // not accessible via the GE Plugin API.


/**
 * Inits the Google Earth plugin.
 * This will contact the GEE Server for the GEE Database info (geeServerDefs)
 * delegating an actual initialization job to the doInitEarth() function by
 * setting it as a callback function that will be triggered on finishing
 * the request.
 */
function geeInit() {
  // Issue an asynchronous HTTP request to get the ServerDefs.
  // Set the doInitEarth() as a callback-function which will be triggered on
  // finishing the request.
  getServerDefinitionAsync(GEE_SERVER_URL, false, doInitEarth);
}

/**
 * Does the actual initialization of the Google Earth plugin.
 * It initializes: 1) the earth plugin 2) the layer panel 3) the search tabs.
 */
function doInitEarth() {
  // check that geeServerDefs are loaded.
  if (geeServerDefs == undefined) {
    alert('Error: The Google Earth Enterprise server does not recognize the ' +
          'requested database.');
    return;
  }
  // Init some globals that require other JS modules to be loaded.
  geeZoomLevelToCameraAltitudeMap = zoomLevelToAltitudeMap();
  // Cache the folder icon URLs.
  geeIconUrls = {
      openedFolder: geeEarthImage('openfolder.png'),
      closedFolder: geeEarthImage('closedfolder.png'),
      cancelButton: geeEarthImage('cancel.png'),
      collapse: geeMapsImage('collapse.png'),
      expand: geeMapsImage('expand.png'),
      transparent: geeMapsImage('transparent.png'),
      defaultLayerIcon: geeEarthImage('default_layer_icon.png')
  };

  // If no search tabs are defined, we have a default search tab for jumping
  // to a lat lng (without hitting the server).
  // You can also hard code handy search tab functionality here by appending
  // to the existing geeServerDefs['searchTabs'].
  if (geeServerDefs.searchTabs == '') {
    // Create a default search tab if none are defined by the server.
    geeServerDefs['searchTabs'] = geeDefaultSearchTabs();
  }

  geeInitSupplementalLayerInfo(geeServerDefs.layers);

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

/**
 * Initialize an indexed object geeSupplementalLayerInfo of supplemental layer
 * information provided by
 * the GEE Server.
 * @param  {Array.<Object>}
 *            layers the array of layer info.
 */
function geeInitSupplementalLayerInfo(layers) {
  for (var i = 0; i < layers.length; ++i) {
    var id = layers[i].id;
    geeSupplementalLayerInfo[id] = layers[i];
  }
}

/**
 * Clear polygon from the earth plugin.
 */
function clearPolygon() {
  gex = new GEarthExtensions(ge);
  gex.dom.clearFeatures();
}

function getCurrentZoomLevel() {
  var camera = ge.getView().copyAsCamera(ge.ALTITUDE_RELATIVE_TO_GROUND);
  var altitude = camera.getAltitude();
  return altitudeToZoomLevel(altitude) + 1;
}

function getKml() {
  doc = ge.createDocument('');
  ge.getFeatures().appendChild(doc);
  var polygon = ge.createPolygon('');
  polygon.setOuterBoundary(outerBoundary);
  polygonPlacemark.setGeometry(polygon);
}


/**
 * Add polygon based on kml in form.
 */
function addPolygon() {
  var polygonKml = document.portable.polygon.value;
  showPolygon(polygonKml);
}

function showPolygon(polygonKml) {
  var polygonKml = polygonKml.replace('<visibility>0</visibility>',
                                      '<visibility>1</visibility>');
  var kml_index = polygonKml.indexOf('<kml');
  polygonKml = polygonKml.substring(kml_index);
  var kmlObject = ge.parseKml(polygonKml);
  if (kmlObject.getType() == 'KmlDocument') {
    polygonKml = polygonKml.replace(/\r/g, ' ');
    polygonKml = polygonKml.replace(/\n/g, ' ');
    polygonKml = polygonKml.replace(/<Document>.*<Placemark>/,
                                    '<Placemark>');
    polygonKml = polygonKml.replace(/<\/Placemark>.*<\/Document>/,
                                    '</Placemark>');
    kmlObject = ge.parseKml(polygonKml);
  }

  if (kmlObject.getType() == 'KmlPlacemark') {
    clearPolygon();
    ge.getFeatures().appendChild(kmlObject);

    if (kmlObject.getAbstractView() !== null) {
      ge.getView().setAbstractView(kmlObject.getAbstractView());
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
 * The Earth Plugin init callback.
 * @param {GEPlugin}
 *          object the just created Google Earth plugin object.
 */
function geeEarthPluginInitCb(object) {
  ge = object;

  // create the polygon and set its boundaries
  outerBoundary = ge.createLinearRing('');
  coords = outerBoundary.getCoordinates();

  ge.getOptions().setStatusBarVisibility(true);  // show lat,lon,height,eye_alt
  ge.getOptions().setScaleLegendVisibility(true);  // show scale legend
  ge.getWindow().setVisibility(true);
  ge.getNavigationControl().setVisibility(ge.VISIBILITY_SHOW);

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

  // On successful init, we can now load the layers and initialize the search
  // tabs.
  geeInitLayerList(ge, geeDivIds.layers);

  // Check for a search_timeout override.
  if (getPageURLParameter('search_timeout')) {
    geeSearchTimeout = getPageURLParameter('search_timeout');
  }
  initializeSearch(geeDivIds.searchTabs, geeServerDefs.searchTabs,
                   geeSearchTimeout);
  geeCreateShims();
  geeResizeDivs();

  gex = new GEarthExtensions(ge);
  gex.util.lookAt([0, 0], { range: 800000 });

  // google.earth.addEventListener(ge.getGlobe(), 'mousedown', onmousedown);
  // google.earth.addEventListener(ge.getGlobe(), 'mousedown', onmouseup);
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
 * Fill in the LayerDiv with a list of checkboxes for the layers from the
 * current Google Earth Database. Each checkbox refers to a geeToggleLayer
 * callback.
 * @param {GEPlugin}
 *          earth the earth plugin object.
 * @param {string}
 *          layerDivId the div id of the layer panel.
 */
function geeInitLayerList(earth, layerDivId) {
  var rootLayer = earth.getLayerRoot();
  var layerDiv = document.getElementById(layerDivId);

  var features = rootLayer.getFeatures();
  var childLayers = features.getChildNodes();
  var layerList = createElement(layerDiv, 'ul');
  layerList.className = 'layer_list';
  var rootLayerId = 'root';
  geeFolderLayerChildren[rootLayerId] = [];  // Init the array for each folder.
  geeCreateLayerElements(rootLayerId, layerList, childLayers);
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
  var la = ge.createLookAt('');
  if (zoomLevel == null) {
    zoomLevel = DEFAULT_SINGLE_CLICK_ZOOM_LEVEL;
  }

  la.set(lat, lng, 100, ge.ALTITUDE_RELATIVE_TO_GROUND, 0, 0,
         geeZoomLevelToCameraAltitudeMap[zoomLevel]);
  ge.getView().setAbstractView(la);
}

/**
 * Convert altitude to zoom level.
 * @param {number} altitude
 * @return {number} zoom level
 */
function altitudeToZoomLevel(altitude) {
  for (zoomLevel = 1; zoomLevel < 31; ++zoomLevel) {
    if (altitude > geeZoomLevelToCameraAltitudeMap[zoomLevel]) {
      return zoomLevel;
    }
  }

  return zoomLevel;
}

/**
 * Set level at which polygon qtnodes are selected.
 * @param {number} zoomLevel Zoom level of viewer (Tumbler).
 */
function setPolygonLevel(zoomLevel) {
  document.portable.polygon_level.value = zoomLevel;
}

/**
 * Get max level at which data is kept.
 */
function getPolygonLevel() {
  return document.portable.polygon_level.value;
}
