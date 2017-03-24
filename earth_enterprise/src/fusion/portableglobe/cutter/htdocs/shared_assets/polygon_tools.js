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
This file is called when a polygon is pasted; it parses the KML and
creates the polygon(s) on the map canvas.

It relies on the following javascript files:
- cutter/js/cutter_tools.js: initialization functions for the cutting tool.
- cutter/js/drawing_tools.js: drawing library for cutting tool.
*/

var ieVersion = getInternetExplorerVersion();
var poly_color = 'D13846';
var poly_color_2d = '#' + poly_color;
var poly_color_3d = hexToKmlColor(poly_color);
var addMarkerForPreview = false;
var mapPolygon = true;
var markerPosition = [];
var overlays = [];
var previewMode = false;
var kmlErrorMsg = '';
var REMOVE_POLYGON_INTERVAL = 1;
var existing3dPolygon = false;

// Loads a polygon to the Map/Globe or removes it.  If a polygon
// is being loaded, it is validated and handled in handleKml(),
// called within this function.  If timeout is passed, the polygon
// will disappear automatically.
function togglePolygonVisibility(timeout) {
  timeout = timeout == undefined;
  previewMode = true;
  if (mapPolygon) {

    // If a polygon is not visible/present, begin the process
    // of creating one by requesting the polygon KML and passing it
    // to a function that will handle the rendering of the polygon.
    if (!mapPolygon.visible && !existing3dPolygon) {
      var url = window.location.href + 'earth/polygon.kml';
      var getKml = new XMLHttpRequest();
      getKml.open('GET', url, false);
      getKml.send();
      var kmlString = getKml.responseText;
      handleKml(kmlString, timeout);
    } else {
      // Kill any polygons that may be present.
      removePolygon(REMOVE_POLYGON_INTERVAL);
      mapPolygon.visible = false;
    }
  }
}

// This function takes a pasted KML string, and draws
// one or more polygons on the maps canvas.
function handleKml(kmlString, timeout) {
 kmlString = kmlString.replace(/^\s+|\s+$/g, '');
 // If Internet Explorer detected, use ActiveXObject instead of DOMParser.
 var myKML;
 if (ieVersion > -1) {
  var xmlDoc = new ActiveXObject('Microsoft.XMLDOM');
  xmlDoc.async = false;
  xmlDoc.loadXML(kmlString);
  myKML = toGeoJSON.kml(xmlDoc, 'text/xml');
 } else {
  var parser = new DOMParser();
  myKML = toGeoJSON.kml(parser.parseFromString(kmlString, 'text/xml'));
 }
 if (myKML.features.length > 0) {
  loadMapPolygon(myKML);
  if (timeout) {
    removePolygon();
  }
  if (previewMode) {
    addPolygonOption();
  }
 } else {
  mapPolygon = false;
 }
}

// If a polygon has been detected and previewMode is true, an option
// is added to the menu to allow for reloading of the polygon.
function addPolygonOption() {
  var div = document.getElementById('PreviewOptions');
  var ems = div.getElementsByTagName('em').length;
  if (ems < 2) {
    div.innerHTML +=
        '<em onclick="togglePolygonVisibility(false);' +
        'togglePreviewOptions();">Toggle polygon (alt-shift-p)</em>';
  }
}

// Earth api takes colors in KML (aabbggrr) format.
// This converts from hex to kml color.
function hexToKmlColor(hexColor) {
  var alpha = '90';
  var r = hexColor.substr(0, 2);
  var g = hexColor.substr(2, 2);
  var b = hexColor.substr(4, 2);
  var kmlColor = alpha + b + g + r;
  return kmlColor;
}

// Check if we are serving Maps (2D) or Earth (3D), and load start loading
// the polygon using myKML, the object containing the polygon coordinates.
function loadMapPolygon(myKML) {
  boundsArray = [];
  var kmlFeatures = myKML.features[0];
  var kmlGeometry = kmlFeatures.geometry;
  var isValidKml = validatePolygonCoordinates(kmlGeometry);
  if (isValidKml) {
    if (isServing == '2D') {
     construct2dPolygon(kmlGeometry);
    } else if (isServing == '3D') {
     construct3dPolygon(kmlGeometry);
    }
  } else {
    kmlErrorMsg = 'There was a problem creating a polygon.  Error Message: ' +
        kmlErrorMsg;
    showErrorBar(kmlErrorMsg);
  }
}

// Confirm that polygon coordinates will work properly,
// return false if issues occur.
function validatePolygonCoordinates(coords) {
  kmlErrorMsg = '';
  var coordLen = coords.length;
  var lastCoord = coords[coordLen - 1];
  // Quit if there are less than 3 coords.
  if (coordLen < 3) {
    kmlErrorMsg += 'Polygon has too few points.';
    return false;
  }
  // For single polygons only (not kml with > 1 poygon).
  if (coords[0]) {
    // Quit if the first coord set is not equal to the last one. Only do
    for (var i = 0; i < coords[0].length; i++) {
      if (coords[0][i] != lastCoord[i]) {
        kmlErrorMsg += 'Final coordinate must match ' +
            'first coordinate.';
        return false;
      }
    }
  }
  // Quit if any coords are not numbers.
  for (var j = 0; j < coordLen; j++) {
    var point = coords[j];
    for (var k = 0; k < point.length; k++) {
      if (point[k] == NaN) {
        kmlErrorMsg += 'Polygon contains invalid characters.';
        return false;
      }
    }
  }
  // If all is well true is returned.
  return true;
}

// Build & define a polygon for the Maps plugin.
function construct2dPolygon(kmlGeometry, flyTo) {
  // flyTo is false if passed.  If not, default to true.
  flyTo = flyTo == undefined;
  var coordinates = [];
  if (kmlGeometry.geometries) {  // If multiple polygons.
   for (var a = 0; a < kmlGeometry.geometries.length; a++) {
     var geometry = kmlGeometry.geometries[a];
     for (var i = 0; i < geometry.coordinates[0].length; i++) {
       var coordsArray =
         new google.maps.LatLng(parseFloat(geometry.coordinates[0][i][1]),
         parseFloat(geometry.coordinates[0][i][0]));
       // Push each individual polygon into an array.
       coordinates.push(coordsArray);
       // Push all coordinates to one massive polygon that we'll use to
       // define our bounds.
       boundsArray.push(coordsArray);
      }
    // Define the polygon for Maps.
    var mapCutPolygons =
      drawMapPolygon(coordinates, flyTo);
    // Clear array so a new polygon can be drawn.
    coordinates = [];
   }
  } else {  // There is just one polygon.
   for (var z = 0; z < kmlGeometry.coordinates[0].length; z++) {
     var myCoords = new google.maps.LatLng(
         parseFloat(kmlGeometry.coordinates[0][z][1]),
         parseFloat(kmlGeometry.coordinates[0][z][0]));
     // Push the coordinates into an array for the Polygon.
     coordinates.push(myCoords);
   }
   var mapCutPolygon =
    drawMapPolygon(coordinates, flyTo);
  }
  return coordinates;
}

// Build arrays for drawing polygon & setting bounds.
function construct3dPolygon(kmlGeometry, flyTo) {
  // flyTo is false if passed.  If not, default to true.
  existing3dPolygon = true;
  flyTo = flyTo == undefined;
  if (kmlGeometry.geometries) {
    for (var i = 0; i < kmlGeometry.geometries.length; i++) {
      drawEarthPolygon(kmlGeometry.geometries[i]);
      set3dBounds(kmlGeometry.geometries[i], true, flyTo);
    }
  } else {
    drawEarthPolygon(kmlGeometry);
    set3dBounds(kmlGeometry, false, flyTo);
  }
}

// Assembles coordinate list, then styles and draws polygon for Earth plugin.
function drawEarthPolygon(kmlGeometry, flyTo) {
  // If plugin is not active, do not proceed.
  if (!ge) {
    return;
  }
  // flyTo is false if passed.  If not, default to true.
  flyTo = flyTo == undefined;
  var coordinates = [];
  var polygonPlacemark = ge.createPlacemark('');
  var polygon = ge.createPolygon('');
  polygonPlacemark.setGeometry(polygon);
  var outer = ge.createLinearRing('');
  polygon.setOuterBoundary(outer);

  // Square outer boundary.
  var center = ge.getView().copyAsLookAt(ge.ALTITUDE_RELATIVE_TO_GROUND);
  var coords = outer.getCoordinates();
  var lat = center.getLatitude();
  var lon = center.getLongitude();

  for (var i = 0; i < kmlGeometry.coordinates[0].length; i++) {
   var latitude = parseFloat(kmlGeometry.coordinates[0][i][1]);
   var longitude = parseFloat(kmlGeometry.coordinates[0][i][0]);
   var entry = [latitude, longitude];
   coordinates.push(entry);
   coords.pushLatLngAlt(latitude, longitude, 0);
  }

  polygonPlacemark.setStyleSelector(ge.createStyle(''));
  var lineStyle = polygonPlacemark.getStyleSelector().getLineStyle();
  lineStyle.setWidth(lineStyle.getWidth() + 2);
  lineStyle.getColor().set(poly_color_3d);
  var polyColor = polygonPlacemark.getStyleSelector().getPolyStyle();
  polyColor.getColor().set(poly_color_3d);
  ge.getFeatures().appendChild(polygonPlacemark);
}

// Use Coords found in loadMapPolygon() to form the polygon (2D Only).
function drawMapPolygon(coordinates, flyTo) {
  // Create a polygon to zoom to.  Happens for every polygon found in the KML.
   mapPolygon = new google.maps.Polygon({
     paths: coordinates,
     strokeColor: poly_color_2d,
     strokeOpacity: 1.0,
     strokeWeight: 1,
     fillColor: poly_color_2d,
     fillOpacity: 0.4
   });
  mapPolygon.setMap(geeMap);
  overlays.push(mapPolygon);
  // If there are multiple polygons, we create a dummy polygon
  // of all coords, and use that as the bounds to zoom to.
  var polygonForBounds = new google.maps.Polygon({
    paths: boundsArray
    });
    polygonForBounds.setMap(geeMap);
    polygonForBounds.setMap(null);
  var bounds = new google.maps.LatLngBounds;
  if (boundsArray.length == 0) {
    // There's only 1 polygon, so use the path of var polygon for bounds.
    mapPolygon.getPath().forEach(function(latLng) {
    bounds.extend(latLng);
    });
  } else {
    // Because there are mutliple polygons, we've created one additional,
    // dummy polygon made up of all the points, so that we can reference that
    // when setting the bounds.
    polygonForBounds.getPath().forEach(function(latLng) {
    bounds.extend(latLng);
    });
  }
  if (flyTo) {
    geeMap.fitBounds(bounds);
  }
}

function set3dBounds(kmlGeometry, hasMultiple, flyTo) {
  // If plugin is not active, do not proceed.
  if (!ge) {
    return;
  }
  var coordinates = [];
  for (var i = 0; i < kmlGeometry.coordinates[0].length; i++) {
    var lat = parseFloat(kmlGeometry.coordinates[0][i][1]);
    var lon = parseFloat(kmlGeometry.coordinates[0][i][0]);
    var entry = [lat, lon];
    boundsArray.push(entry);
    coordinates.push(entry);
  }
  var bounds;
  if (hasMultiple == true) {
    bounds = geePolygonBounds(boundsArray);
  } else {
    bounds = geePolygonBounds(coordinates);
  }
  var latDiff = bounds.north - bounds.south;
  var lngDiff = bounds.east - bounds.west;
  if (latDiff > lngDiff) {
   maxDist = latDiff;
  } else {
   maxDist = lngDiff;
  }
  var zoomLevel = Math.floor(Math.log(360 / maxDist) / Math.log(2));
  CenterLat = (bounds.north + bounds.south) / 2;
  CenterLon = (bounds.east + bounds.west) / 2;
  geeZoomLevelToCameraAltitudeMap = zoomLevelToAltitudeMap();
  var lookAt = ge.createLookAt('');
  lookAt.setLatitude(CenterLat);
  lookAt.setLongitude(CenterLon);
  lookAt.setRange(geeZoomLevelToCameraAltitudeMap[zoomLevel]);
  if (flyTo) {
    ge.getView().setAbstractView(lookAt);
  }
}

// Returns coords for bounds of the polygon.  For polygons that cross the
// 180th/anti-meridian, bounds will be the entire world.
function geePolygonBounds(coordinates) {
  var minLat = 90.0;
  var maxLat = -90.0;
  var minLng = 180.0;
  var maxLng = -180.0;
  for (var i = 0; i < coordinates.length; i++) {
    lat = coordinates[i][0];
    lng = coordinates[i][1];
    minLat = Math.min(lat, minLat);
    minLng = Math.min(lng, minLng);
    maxLat = Math.max(lat, maxLat);
    maxLng = Math.max(lng, maxLng);
  }
  return {
    south: minLat,
    north: maxLat,
    west: minLng,
    east: maxLng
  };
}

// Create an index of Earth altitudes that we can compare to Maps zoom levels.
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

// Remove the polygon after 4 seconds.
function removePolygon(interval) {
  interval = interval || 4000;
  var polyTimer = setTimeout(function() {
      if (isServing == '2D') {
        if (mapPolygon && mapPolygon.setMap) {
          mapPolygon.setMap(null);
        }
      } else {
        if (ge.getFeatures().getChildNodes().getLength() > 0) {
          ge.getFeatures().removeChild(ge.getFeatures().getFirstChild());
        }
      }
      existing3dPolygon = false;
    }, interval);
}

function showButterBar(message, timeout) {
  notificationBar('ButterBar', message, timeout);
}

function showErrorBar(message, timeout) {
  notificationBar('ErrorBar', message, timeout);
}

// Global notification function.  If timeout arg is false, the bar will
// remain visible until the user closes it.
function notificationBar(element, message, timeout) {
  timeout = timeout == undefined ? true : timeout;
  if (notificationTimeout) {
    window.clearInterval(notificationTimeout);
  }
  getElement(element).style.display = 'block';
  getElement(element + 'Message').innerHTML = message;
  if (timeout) {
    var notificationTimeout = setTimeout(function() {
        getElement(element).style.display = 'none';
      }, 9000);
  }
}
