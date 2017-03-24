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



/**
 * @fileoverview This file presents an example of a page that makes use of the
 *               Google Maps API from a Google Earth Enterprise Server. It
 *               handles the following: examples of how to use the Google Maps
 *               API, a simple layer panel for the layers provided by the
 *               GEE Server search tabs from the GEE Server Search Framework
 *               search results from the GEE Server.
 *
 * This file contains most of the work to customize the look and feel and
 * behaviors in the page as well as the calls to the Maps API. Depends on :
  *   fusion_utils.js : some basic utilities for accessing DOM elements, and a
 *                     few low level utilities used by this example.
 *   search_tabs.js : sample classes for showing/managing search tabs and
 *                    results.
 *   earth_plugin_loader.js : do not modify...this code does the work to
 *                            load the plugin
 *   fusionmaps_local.js (which loads a specific version of the Google Maps API)
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
  document.write("<script src=\"" + path +
                 "\" type=\"text/javascript\"><\/script>");
}

// Load the Maps database info into the JS global variable "geeServerDefs".
// This will have the fields:
//  serverUrl : the URL of the server
//  isAuthenticated : true if the server is authenticated
//  searchTabs : a JSON array of search tab definitions.
//  layers : a JSON array of supplemental layer info
// If desired, you can point this to another GEE Server or use a different
// variable for multiple servers.
if (GEE_SERVER_URL == undefined) {
  var GEE_SERVER_URL = "";
}
geeLoadScript(GEE_SERVER_URL + "query?request=Json&var=geeServerDefs");

//The div ids for the left panel, map and header.
//These must match the html and css declarations.
var geeDivIds = {
 header: "header",
 map: "map",
 mapInner: "map_inner",
 leftPanelParent: "left_panel_cell",
 leftPanel: "left_panel",
 searchTabs: "search_tabs",
 searchTitle: "search_results_title",
 searchResults: "search_results_container",
 layersTitle: "layers_title",
 layers: "layers_container",
 collapsePanel: "collapsePanel",
 collapseShim: "collapseShim"
};
//Need the static URL for absolute addresses
var GEE_STATIC_URL = window.location.protocol + "//" + window.location.host;
//Static image paths in this example are relative to the earth server host.
var GEE_EARTH_IMAGE_PATH = GEE_STATIC_URL + "/earth/images/";
var GEE_MAPS_IMAGE_PATH = GEE_STATIC_URL + "/maps/api/icons/";

//Search Timeout specifies how long to wait (in milliseconds) before
//determining the search failed.
//override with "?search_timeout=xxx" on the URL where xxx is in milliseconds.
var geeSearchTimeout = 5000;  // 5 seconds seems OK

//Initial view parameters are overrideable from the html or the URL.
//Override initial lat lng with "?ll=32.898,-100.30" on the URL.
//Override initial zoom level with "?z=8" on URL
var geeInitialViewLat = 37.422;
var geeInitialViewLon = -122.08;
var geeInitialZoomLevel = 6;

//Default Altitude for panning, keep camera at a high enough level to see
//continents.
var DEFAULT_SINGLE_CLICK_ZOOM_LEVEL = 10;  // City Level.
var DEFAULT_DOUBLE_CLICK_ZOOM_LEVEL = 14;  // Neighborhood level.
var DEFAULT_PAN_TO_ALTITUDE_METERS = 400000;
var DEFAULT_BALLOON_MAX_WIDTH_PIXELS = 500;
//We cache a map of Maps Zoom Levels (0..32) to Earth camera altitudes.
var geeZoomLevelToCameraAltitudeMap = null;
var geeIconUrls = {};  // Cache of frequently used URLs.

// Google Maps specific variables.
var geeMap = null;
var geeBaseIcon = 0;  // To be initialized in LoadMap().

var _mFusionMapServer = "";  // This constant is used by other
                             // GEE Maps API code.

// Options passed to the GMap2 object.
// See http://code.google.com/apis/maps/documentation/reference.html#GMap2.GMap2
var geeMapOpts = {
 logoPassive: true // logoPassive is undocumented, but supported.
};

/**
 * Init the Google Maps Map widget.
 * This will be executed onLoad of the document and will
 * initialize: 1) the map 2) the layer panel 3) the search tabs
 */
function geeInit() {
  // check that geeServerDefs are loaded.
  if (geeServerDefs == undefined) {
    alert("Error: The Google Earth Enterprise server does not recognize the " +
          "requested database.");
    return;
  }
  // Cache the folder icon URLs.
  geeIconUrls = {
      openedFolder: geeEarthImage("openfolder.png"),
      closedFolder: geeEarthImage("closedfolder.png"),
      cancelButton: geeEarthImage("cancel.png"),
      collapse: geeMapsImage("collapse.png"),
      expand: geeMapsImage("expand.png"),
      transparent: geeMapsImage("transparent.png"),
      defaultLayerIcon: geeEarthImage("default_layer_icon.png")
  };

  // If no search tabs are defined, we have a default search tab for jumping
  // to a lat lng (without hitting the server).
  // You can also hard code handy search tab functionality here by appending
  // to the existing geeServerDefs["searchTabs"].
  if (geeServerDefs.searchTabs == "") {
    // Create a default search tab if none are defined by the server.
    geeServerDefs["searchTabs"] = geeDefaultSearchTabs();
  }
  
  // Must set this variable to bootstrap the local version of the Maps API.
  _mFusionMapServer = window.location.protocol + "//" + window.location.host;

  geeInitLeftPanelDivs();  // The left panel needs some sub-divs for layers etc.

  // Need to resize before initial load of the map! and then again
  // to adjust for the search tabs if any.
  geeResizeDivs();

  // Create the Map...this will create the Map, initialize the layers
  // and search tabs and pan the view to the initial view.
  geeLoadMap(geeServerDefs, geeMapOpts);

  // Need to resize after search tabs are filled in.
  geeResizeDivs();  // Resize the map to fill the screen.
}

/**
 * Initialize the Map UI: the map, layers and search UI.
 * @param {Object}
 *          serverDefs is the object that contains the search and layer
 *          info for initializing the Map UI.
 * @param {Object}
 *          mapOpts is the object that contains all the options accepted by
 *          the GMap2 object used to configure the Map.
 */
function geeLoadMap(serverDefs, mapOpts) {
  // The map div must exist!
  var parentDiv = findElement(geeDivIds.map);
  if (!parentDiv) {
    alert("Cannot initialize map!  Missing Map Element.");
    return;
  }
  // Init the base icon info.
  geeInitBaseIcon();

  // Initialize the initial view (which can be overridden by URL parameters).
  var lat = geeInitialViewLat;
  var lon = geeInitialViewLon;
  var zoom = geeInitialZoomLevel;
  // Process the initial view overrides.
  if (getPageURLParameter("ll")) {
    var latlng = getPageURLParameter("ll").split(',');
    if (latlng.length == 2) {
      lat = latlng[0];
      lon = latlng[1];
    }
  }
  if (getPageURLParameter("z")) {
    zoom = parseInt(getPageURLParameter("z"));
  }

  if (!GBrowserIsCompatible()) {
    alert("This web browser is not compatible with the Google Maps API.");
    return;
  }
  // We need a new div element for GMap to own.
  var mapDiv = createElement(parentDiv, "div", geeDivIds.mapInner);
  // Create the Map, and the controls that we want to see.
  geeMap = new GFusionMap(mapDiv, serverDefs, mapOpts);
  var latlng = new GLatLng(parseFloat(lat), parseFloat(lon));
  geeMap.setCenter(latlng, zoom);
  geeMap.showInitialFusionLayers();

  geeMap.addControl(new GLargeMapControl());
  geeMap.addControl(new GMapTypeControl());
  geeMap.addControl(new GOverviewMapControl());
  geeMap.addControl(new GScaleControl());
  var keyboardHandler = new GKeyboardHandler(geeMap);
  geeMap.enableDoubleClickZoom();
  geeMap.enableScrollWheelZoom();
  // This is an example of what you could do to add a transparency control
  // for a layer. This example adds buttons for the 2nd layer.
  // geeInitTransparencyButtons(serverDefs.serverUrl, serverDefs.layers[1]);

  // Load the layers into the UI.
  geeInitLayerList(serverDefs.serverUrl, geeDivIds.layers, serverDefs.layers);

  // Initialize the search tabs.
  // Check for a search_timeout override.
  if (getPageURLParameter("search_timeout")) {
    geeSearchTimeout = getPageURLParameter("search_timeout");
  }
  initializeSearch(geeDivIds.searchTabs, geeServerDefs.searchTabs,
                   geeSearchTimeout);
}

/**
 * Example call to set up buttons to make a layer transparent/opaque.
 * Each press of the button increments/decrements the 
 * opacity by 0.1 (out of 1.0 scale).
 * @param {string}
 *          serverUrl the server URL for the layer                  
 * @param {Object} 
 *          layer the layer information from teh server (includes label and id).
 */
function geeInitTransparencyButtons(serverUrl, layer) {
  // Add a button to make the imagery Opaque.
  var callbackOpaque = function (theLayer, theMap) {
    return function (map) {
      // Ignore map, we have a global map.
      theMap.adjustFusionLayerOpacity(theLayer.id, .1);
   }
  }(layer, geeMap);
  var leftPad = 7;
  var topPad = 30;
  geeMap.addControl(new geeMapsButton(layer.label + " Opaque", callbackOpaque,
                                      G_ANCHOR_TOP_RIGHT, leftPad, topPad));
  // Add a button to make the imagery Transparent.
  var callbackTransparent = function (theLayer, theMap) {
    return function (map) {
      // Ignore map, we have a global map.
      theMap.adjustFusionLayerOpacity(theLayer.id, -.1);
  }
  }(layer, geeMap);
  topPad += 25;
  geeMap.addControl(new geeMapsButton(layer.label + "Transparent", 
                                      callbackTransparent,
                                      G_ANCHOR_TOP_RIGHT, leftPad, topPad));
}
  
/**
 * Fill in the LayerDiv with a list of checkboxes for the layers from the
 * current Google Earth Database. Each checkbox refers to a geeToggleLayer
 * callback.
 *
 * @param {serverUrl}
 *          serverUrl the URL of the GEE Server.
 * @param {String}
 *          layerDivId the div id of the layer panel.
 * @param {Array}
 *          layers the array of layers.
 */
function geeInitLayerList(serverUrl, layerDivId, layers) {
  var layerDiv = document.getElementById(layerDivId);
  var layerList = createElement(layerDiv, "ul");
  layerList.className = "layer_list";
  geeCreateLayerElements(serverUrl, layerList, layers);
}

/**
 * Fill in the given list with list items for each child layer.
 * @param {serverUrl}
 *          serverUrl the URL of the GEE Server.
 * @param {Element}
 *          parentList the DOM list element to add these layers to.
 * @param {Array}
 *          childLayers the child layers to be added to this list element.
 */
function geeCreateLayerElements(serverUrl, parentList,
                                childLayers) {
  for (var i = 0; i < childLayers.length; ++i) {
    var layer = childLayers[i];
    if (i == 0 && layer.requestType == "ImageryMaps") {
      continue;  // Skip the first imagery layer...it will have a button.
    }
    var item = geeCreateLayerItem(serverUrl, parentList, layer);
  }
}

/**
 * Create a single layer item entry with a checkbox and appropriate hover and
 * click behaviors.
 * @param {serverUrl}
 *          serverUrl the URL of the GEE Server.
 * @param {Element}
 *          layerList the list element container for the item being created.
 * @param {KmlLayer}
 *          layer the layer object.
 * @return {Element} the list item for the layer.
 */
function geeCreateLayerItem(serverUrl, layerList, layer) {
  var isChecked = layer.initialState;
  var layerId = layer.id;
  var layerName = layer.label;
  var layerIconUrl = geeLayerIconUrl(serverUrl, layer);

  // Create the list item for the layer.
  var item = geeCreateLayerItemBasic(layerList, layerId, layerName,
                                     layerIconUrl, isChecked, "geeToggleLayer");

  // Set to zoom in on layer on click
  item.onclick = function(layerObject) {
    return function(e) {
      var lookAt = layerObject.lookAt;
      if (lookAt == "none") return;
      geePanTo(lookAt.lat, lookAt.lng, lookAt.zoom);
      cancelEvent(e);
    };
  }(layer);

  return item;
}

/**
 * Toggle the visibility of the specified layer.
 * @param {Object}
 *          e  the event argument.
 * @param {string}
 *          checkBoxId the name of the checkbox which maintains the layer's
 *          visibility state.
 * @param {string}
 *          layerId the id of the layer to toggle.
 * @param {string}
 *          layerName the name of the layer to toggle only used for printing
 *          error message.
 */
function geeToggleLayer(e, checkBoxId, layerId, layerName) {
  try {
    var cb = document.getElementById(checkBoxId);
    try {
      if (cb.checked) {
        geeMap.showFusionLayer(layerId);
      } else {
        geeMap.hideFusionLayer(layerId);
      }
    } catch (err2) {
      alert("Failed attempt to enable/disable layer: " + layerName + "\n" + layerId + "\n" + err2);
    }
  } catch (err) {
    alert("Failed attempt to get checkbox for layer: " + layerName + "\n" + err);
  }
  cancelEvent(e);
}

/*
 * Interface for search_tab.js: The following code implements the GEE Methods
 * required by search_tabs.js.
 */

/**
 * Create the content for an info window balloon for a search result item.
 * @param {string} opt_name  the optional title for the balloon.
 * @param {string} opt_description  the optionalcontents of the balloon.
 * @return {string|Element} If opt_name is empty, we
 *         return an empty string (no "Balloon" object in maps), otherwise,
 *         we return the info div for the item's balloon content.
 */
function geeCreateBalloon(opt_name, opt_description) {
  if (opt_name == undefined) {
    return "";  // No "Balloon" object in Maps API.
  } else {
    // Create the info balloon html here.
    var infoDiv = document.createElement("div");
    var nameDiv = createElement(infoDiv, "div", "popup_name");
    nameDiv.innerHTML = opt_name;
    var bodyDiv = createElement(infoDiv, "div", "popup_body");
    bodyDiv.innerHTML = opt_description;
    return infoDiv;
  }
}

/**
 * Create a placemark at the specified location.
 * @param {string}
 *          name the name of the placemark.
 * @param {string}
 *          description the description of the placemark.
 * @param {GLatLng}
 *          latlng the position of the placemark.
 * @param {string}
 *          iconUrl the URL of the placemark's icon.
 * @param {Object}
 *          balloon the balloon for the placemark (ignored for Maps API).
 * @return {GMarker} the placemark object.
 */
function geeCreateMarker(name, description, latlng, iconUrl, balloon) {
  // Create icon style for the placemark
  var icon = new GIcon(G_DEFAULT_ICON);
  icon.image = iconUrl;
  var markerOptions = { "icon" : icon };
  var point = new GLatLng(parseFloat(latlng.lat), parseFloat(latlng.lon));
  var pointPlacemark = new GMarker(point, markerOptions);

  // Create the info balloon html here.
  var infoDiv = geeCreateBalloon(name, description);

  // Add a listener for the popup balloon.
  GEvent.addListener(pointPlacemark, "click", function() {
    pointPlacemark.openInfoWindow(infoDiv);
   });

  return pointPlacemark;
}

 /**
  * Callback for Search Results Manager after results are cleared.
  * We simply clear any active info window.
  */
 function geeClearSearchResults() {
   geeCloseInfoWindow();
 }
/**
 * Initialize the geeBaseIcon in LoadMap() after all javascript has been loaded.
 */
function geeInitBaseIcon() {
  geeBaseIcon = new GIcon();
  geeBaseIcon.shadow = geeMapsImage("shadow50.png");
  geeBaseIcon.iconSize = new GSize(20, 34);
  geeBaseIcon.shadowSize = new GSize(37, 34);
  geeBaseIcon.iconAnchor = new GPoint(9, 34);
  geeBaseIcon.infoWindowAnchor = new GPoint(9, 2);
  geeBaseIcon.infoShadowAnchor = new GPoint(18, 25);
}

/**
 * Add the specified placemark to the map.
 * @param {GMarker}
 *          placemark the placemark to add.
 */
function geeAddOverlay(placemark) {
  geeMap.addOverlay(placemark);
}

/**
 * Remove the specified placemark from the map.
 * @param {GMarker}
 *          placemark the placemark to remove.
 */
function geeRemoveOverlay(placemark) {
  geeMap.removeOverlay(placemark);
}

/**
 * Close the open info window.
 */
function geeCloseInfoWindow() {
  geeMap.closeInfoWindow();
}

/**
 * Pan and Zoom the Earth viewer to the specified lat, lng and zoom level.
 * @param {string}
 *          lat the latitude of the position to pan to.
 * @param {string}
 *          lng the longitude of the position to pan to.
 * @param {Number}
 *          zoomLevel [optional] the zoom level (an integer between 1 : zoomed
 *          out all the way, and 32: zoomed in all the way) indicating the zoom
 *          level for the view.
 */
function geePanTo(lat, lng, zoomLevel) {
  lat = parseFloat(lat);
  lng = parseFloat(lng);
  if (zoomLevel == null) {
    zoomLevel = DEFAULT_SINGLE_CLICK_ZOOM_LEVEL;
  }
  geeMap.setCenter(new GLatLng(lat, lng), zoomLevel);
}

/**
 * Open a balloon in the Map.
 * @param {Object}
 *          marker the marker that is attached to the balloon.
 * @param {Object}
 *          balloon the balloon object (no such thing in Maps API).
 * @param {string}
 *          title the title of the balloon contents.
 * @param {string}
 *          innerText the inner text of the balloon text.
 */
function geeOpenBalloon(marker, balloon, title, innerText) {
  // Create the info balloon html here.
  var infoDiv = geeCreateBalloon(title, innerText);
  marker.openInfoWindowHtml(infoDiv);
}
