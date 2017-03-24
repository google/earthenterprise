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
//  serverUrl : the URL of the server
//  isAuthenticated : true if the server is authenticated
//  searchTabs : a JSON array of search tab definitions.
//  layers : a JSON array of supplemental layer info
// If desired, you can point this to another GEE Server or use a different
// variable for multiple servers.
var geeServerDefs;

//The div ids for the left panel, map and header.
//These must match the html and css declarations.
var geeDivIds = {
 map: 'map'
};
//Need the static URL for absolute addresses
var GEE_STATIC_URL = window.location.protocol + '//' + window.location.host;
//Static image paths in this example are relative to the earth server host.
var GEE_EARTH_IMAGE_PATH = GEE_STATIC_URL + '/earth/images/';
var GEE_MAPS_IMAGE_PATH = GEE_STATIC_URL + '/maps/mapfiles/';

//Initial view parameters are overrideable from the html or the URL.
//Override initial lat lng with "?ll=32.898,-100.30" on the URL.
//Override initial zoom level with "?z=8" on URL
var geeInitialViewLat = 37.422;
var geeInitialViewLng = -122.08;
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

var _mFusionMapServer = '';  // This constant is used by other
                             // GEE Maps API code.


/**
 * Inits the Google Maps Map widget.
 * This will be executed onLoad of the document. It will query the ServerDefs
 * delegating an actual initialization job to the doInitMap() function by
 * setting it as a callback function that will be triggered on finishing
 * the request.
 */
function geeInit() {
  // Issue an asynchronous HTTP request to get the ServerDefs.
  // Set the doInitMap() as a callback-function which will be triggered on
  // finishing the request.
  getServerDefinitionAsync(GEE_SERVER_URL, true, doInitMap);
}

/**
 * Does the actual initialization of the Google Maps Map widget.
 * We set this function as a callback on getServerDefs request.
 * It will be executed on finishing of the getServerDefs-request and
 * will initialize: 1) the map 2) the layer panel 3) the search tabs.
 */
function doInitMap() {
  // check that geeServerDefs are loaded.
  if (geeServerDefs == undefined) {
    alert('Error: The Google Earth Enterprise server does not recognize the ' +
          'requested database.');
    return;
  }

  // Must set this variable to bootstrap the local version of the Maps API.
  _mFusionMapServer = GEE_STATIC_URL;

  // Create the Map...this will create the Map, initialize the layers
  // and search tabs and pan the view to the initial view.
  geeLoadMap(geeServerDefs);

}

/**
 * Initialize the Map UI: the map, layers and search UI.
 * @param {Object}
 *          serverDefs is the object that contains the search and layer
 *          info for initializing the Map UI.
 */
function geeLoadMap(serverDefs) {
  // Create the map.
  geeMap = new GFusionMap(geeDivIds.map, serverDefs);
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
  geeMap.panTo(lat, lng, zoomLevel);
}

/**
 * Close the open info window.
 */
function geeCloseInfoWindow() {
  geeMap.closeInfoWindow();
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
  geeMap.openInfoWindow(marker.getPosition(),
                        '<b>' + title + '</b><br/>' + innerText);
}

/*
 * Interface for search_tab.js: The following code implements the GEE Methods
 * required by search_tabs.js.
 */

/**
 * Deprecated.
 * @param {string} opt_name Balloon title.
 * @param {string} opt_description Balloon content.
 */
function geeCreateBalloon(opt_name, opt_description) {
}

/**
 * Create a placemark at the specified location.
 * @param {string}
 *          name the name of the placemark.
 * @param {string}
 *          description the description of the placemark.
 * @param {google.maps.LatLng}
 *          latlng the position of the placemark.
 * @param {string}
 *          iconUrl the URL of the placemark's icon.
 * @param {Object}
 *          balloon the balloon for the placemark (ignored for Maps API).
 * @return {google.maps.Marker} the placemark object.
 */
function geeCreateMarker(name, description, latlng, iconUrl, balloon) {
  // Create icon style for the placemark
  var point = new google.maps.LatLng(parseFloat(latlng.lat),
                                     parseFloat(latlng.lng));
  var marker = new google.maps.Marker({
      icon: iconUrl,
      map: map,
      position: point,
      draggable: false,
      title: name
    });

  google.maps.event.addListener(marker, 'click', function() {
      geeOpenBalloon(marker, null, name, description);
    });
  return marker;
}

 /**
  * Callback for Search Results Manager after results are cleared.
  * We simply clear any active info window.
  */
 function geeClearSearchResults() {
   geeMap.closeInfoWindow();
 }

 /**
  * Deprecated.
  * @param {google.maps.Marker}
  *          marker The marker to be added.
  */
 function geeAddOverlay(marker) {
 }

 /**
  * Removes the marker from the map.
  * @param {google.maps.Marker}
  *          marker The marker to be removed.
  */
 function geeRemoveOverlay(marker) {
   marker.setMap(null);
 }

/**
 * Return the URL for a layer icon (a specific request to the GEE Server).
 * @param {serverUrl}
 *          serverUrl the URL of the GEE Server.
 * @param {Array}
 *          layer the layer object.
 * @return {string} the kml feature object.
 */
function geeLayerIconUrl(serverUrl, layer) {
  return serverUrl + '/query?request=Icon&icon_path=' + layer.icon;
}

/**
 * Add polygon based on kml in form.
 */
function addPolygon() {
  var polygonKml = document.portable.polygon.value;
  showPolygon(polygonKml);
}

function showPolygon(polygonKml) {
  var coordinates = getCoordinates(polygonKml);
  drawPolygon(coordinates, '#FF0000', '#FFFF00');
  minLat = 90.0;
  maxLat = -90.0;
  minLng = 180.0;
  maxLng = -180.0;
  for (var i = 0; i < coordinates.length; i++) {
    lat = coordinates[i].lat();
    lng = coordinates[i].lng();
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

  latDiff = maxLat - minLat;
  lngDiff = maxLng - minLng;
  if (latDiff > lngDiff) {
    maxDist = latDiff;
  } else {
    maxDist = lngDiff;
  }

  zoomLevel = Math.floor(Math.log(360 / maxDist) / Math.log(2));
  geePanTo((minLat + maxLat) / 2, (minLng + maxLng) / 2, zoomLevel);
}

function getCoordinates(polygonKml) {
  var coordinates = [];
  var num_coordinates = 0;

  var start = polygonKml.indexOf('<coordinates>') + 13;
  var end = polygonKml.indexOf('</coordinates>');
  var coordinate_string = polygonKml.substr(start, end - start);
  var coordinate_list = coordinate_string.split(' ');
  for (var i = 0; i < coordinate_list.length; ++i) {
    if (coordinate_list[i].indexOf(',') > 0) {
      var latLng = coordinate_list[i].split(',');
      coordinates[num_coordinates++] =
        new google.maps.LatLng(parseFloat(latLng[1]), parseFloat(latLng[0]));
    }
  }

  return coordinates;
}

/**
 * Draw the polygon being drawn.
 * @param {array of google.maps.LatLng} coordinates Coordinates of the polygon.
 * @param {string} stroke_color Color of polygon border.
 * @param {string} fill_color Color of polygon interior.
 */
function drawPolygon(coordinates, stroke_color, fill_color) {
  var polygon = new google.maps.Polygon({
          paths: coordinates,
          strokeColor: stroke_color,
          strokeOpacity: 0.8,
          strokeWeight: 1,
          fillColor: fill_color,
          fillOpacity: 0.25
        });

  // Draw the new polygon.
  polygon.setMap(geeMap.map);
}
