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
This file is the Main drawing library for the GEE
Server Cutting Tool Interface.

It relies on the following javascript files:
- cutter/js/cutter_tools.js: initialization functions for the cutting tool.
- shared_assets/polygon_tools.js: handles pasted KML and creates Polygon.
*/

// Globals for shapes.
var mapsMarker;
var earthMarker;
var mapsLine;
var temporaryLine;
var points;

// Global arrays for holding drawing progress.
var mapsPoly = [];
var overlays = [];
var lineHolder = [];
var pointArray = [];
var arrayCoords = [];
var boundsArray = [];
var temporaryLineHolder = [];
var lineArrayCoords = [];
var currentLat;
var currentLon;
var PolyColor = 'D13846';

var ieVersion = gees.tools.internetExplorerVersion();

function initDrawing() {
  if (currentCutterMode != 'complete') {
    if (isServing == '2D') {
      // Initiate a custom cursor while in drawing mode (2D only).
      geeMap.setOptions(
        {draggableCursor: 'url(./images/crosshair_draw.png) 20 20, crosshair'});
    }
    // Update listeners to drawing mode.
    drawListeners();
    // This function lives in cutter/js/cutter_tools.js.  It manipulates
    // the UI elements while we are drawing.
    cutterMode('draw');
  }
}

function startListeners() {
  // Start with a listener on zoom change.  Display current zoom level in UI.
  if (isServing == '2D') {
    google.maps.event.addListener(geeMap, 'mousemove', grabLatLonMaps);
    google.maps.event.addListener(geeMap, 'zoom_changed', getZoomLevel);
    google.maps.event.clearListeners(geeMap, 'mouseup');
  } else if (isServing == '3D') {
    google.earth.addEventListener(ge.getView(), 'viewchange', getZoomLevel);
    google.earth.addEventListener(
        ge.getWindow(), 'mousemove', grabLatLonEarth);
  }
}

function drawListeners() {
  // Once we're drawing, add a click listener.  It will grab current lat/lon
  // and use it to start, continue or finish a polygon.
  if (isServing == '3D') {
    google.earth.addEventListener(
        ge.getWindow(), 'click', drawListenerEvent3D);
  } else if (isServing == '2D') {
    google.maps.event.addListener(geeMap, 'click', drawListenerEvent2D);
  }
}

function polyListeners() {
  // Add some additional listeners once we have a polygon.
  if (isServing == '3D') {
    // Add a listener to complete the polygon when clicking original marker.
    google.earth.addEventListener(earthMarker, 'click', function(event) {
      finish();
    });
  } else if (isServing == '2D') {
    // Draw a line from previous point to current point.
    google.maps.event.addListener(geeMap, 'click', function() {
      drawLine();
    });
    // Constantly draw/redraw temporary line between clicks.
    google.maps.event.addListener(geeMap, 'mousemove', function() {
      intermittentLine();
    });
    // When clicking on original marker, close polygon and finish drawing.
    google.maps.event.addListener(mapsMarker, 'click', function() {
     finish();
    });
  }
}

function finishedListeners() {
  // When done drawing, kill drawing related listeners.
  if (isServing == '2D') {
    google.maps.event.clearListeners(geeMap, 'click');
    google.maps.event.clearListeners(geeMap, 'mousemove');
    google.maps.event.clearListeners(geeMap, 'mouseover');
    google.maps.event.clearListeners(geeMap, 'mousedown');
    google.maps.event.addListener(geeMap, 'mousemove', grabLatLonMaps);
  } else if (isServing == '3D') {
    google.earth.removeEventListener(ge.getWindow(),
      'click', drawListenerEvent3D);
  }
}

/** Some helper functions for the different listener events. **/
var grabLatLonEarth = function(event) {
  // Listener related function to constantly hold current Lat/Lon.
  currentLat = event.getLatitude();
  currentLon = event.getLongitude();
};

var grabLatLonMaps = function(point) {
  // Get the current lat/lon for Earth.
  var mapCenter = geeMap.getCenter();
  currentLat = point.latLng.lat();
  currentLon = point.latLng.lng();
};

var drawListenerEvent3D = function(event) {
  // Listener related function to grab a Lat/Lon and use it to build polygon.
  setPointArray();
  drawingInProgress();
};

var drawListenerEvent2D = function(event) {
  // Set a point in Maps.
  point = new google.maps.LatLng(currentLat, currentLon);
  setPointArray();
  drawingInProgress(point);
};

function setPointArray() {
  var latLongStorage = [currentLat, currentLon];
  pointArray.push(latLongStorage);
  points = pointArray.length;
}
/** End listener helper functions **/

// Determine if we are drawing our starting point, or if we are in
// the process of drawing a polygon.
function drawingInProgress(pointData) {
  // Function for clearing temporary elements.
  clearTemporaryElements();
  if (points == 1) {
    // We've just started - draw our starting marker.
    drawMarker(pointData);
  } else if (points > 1) {
    // We have a marker, so we'll begin drawing a polygon.
    drawPoly(false, 0);
  }
}

// Clear temporary polygon elements as we draw, or when finished.
function clearTemporaryElements() {
  if (isServing == '3D') {
    removeChildNodes();
  } else if (isServing == '2D') {
    while (overlays[0]) {
      overlays.pop().setMap(null);
    }
  }
}

// Create the marker that serves as the starting point for our polygon.
function drawMarker(point) {
  if (isServing == '3D') {
    // create the elements and styles.
    earthMarker = ge.createPlacemark('');

    var earthPoint = ge.createPoint('');
    earthPoint.setLatitude(pointArray[0][0]);
    earthPoint.setLongitude(pointArray[0][1]);
    earthMarker.setGeometry(earthPoint);

    var icon = ge.createIcon('');
    icon.setHref(GEE_BASE_URL + '/cutter/images/icon_earth.png');

    var style = ge.createStyle('');
    style.getIconStyle().setIcon(icon);
    earthMarker.setStyleSelector(style);
    // Add the placemark to Earth.
    ge.getFeatures().appendChild(earthMarker);

  } else if (isServing == '2D') {
    var markerIcon = './images/icon_map.png';
      mapsMarker = new google.maps.Marker({
      map: geeMap,
      position: point,
      draggable: false,
      editable: false,
      icon: markerIcon
    });
  }
  // Now that we know we have at least 1 point, we initiate polygon listeners.
  polyListeners();
}

// Function that draws lines for each segment of the polygon.  These
// are needed, specifically, so that we can create the effect of an
// 'unclosed' polygon while it is in the process of being drawn.
function drawLine() {
 var i = points - 1;
 var holdLat = pointArray[i][0];
 var holdLon = pointArray[i][1];
 arrayCoords.push(new google.maps.LatLng(holdLat, holdLon));
 arrayCoords.push(new google.maps.LatLng(currentLat, currentLon));
  var lineOptions = {
    path: arrayCoords,
    strokeColor: '#' + PolyColor,
    strokeOpacity: 0.7,
    strokeWeight: 4,
    clickable: false
  };
  mapsLine = new google.maps.Polyline(lineOptions);
  lineHolder.push(mapsLine);
  mapsLine.setMap(geeMap);
}

// This function creates what we call an 'intermittent' line;
// as the polygon is being drawn, it creates a dotted line that indicates
// progress, which then dissapears when the line is drawn.
function intermittentLine(arg) {
 // This function runs in Maps/2D Only.
 var lineSymbol = {
   // Here, path is a small set of coordinates for custom symbols to create
   // the effect of a dotted line.  In the Maps API these are referred to as
   // 'Custom Symbols' or 'Overlay Symbols'.
   path: 'M -2,0 0,-2 2,0 0,2 z',
   strokeColor: '#' + PolyColor,
   fillColor: '#' + PolyColor,
   fillOpacity: 9,
   rotation: 45,
   scale: 0.8
 };
 // This function is constantly running on a mousemove listener.  Here we
 // clear out the array that this function is filling.
 if (lineArrayCoords.length > 0) {
  temporaryLine.setMap(null);
 }
 lineArrayCoords = [];
 // Grab and hold on to the last point we clicked.
 var i = points - 1;
 var holdLat = pointArray[i][0];
 var holdLon = pointArray[i][1];
 lineArrayCoords.push(new google.maps.LatLng(holdLat, holdLon));
 // Also grab the current lat/lon of the mouse.
 lineArrayCoords.push(new google.maps.LatLng(currentLat, currentLon));
  // Create line with the two sets of coordinates we just pushed.
  var lineOptions = {
    path: lineArrayCoords,
    strokeColor: '#000000',
    strokeOpacity: 0.7,
    strokeWeight: 0,
    icons: [{
      icon: lineSymbol,
      offset: '0',
      repeat: '8px'
    }],
    clickable: false
  };
  // Set the line on the map.
  temporaryLine = new google.maps.Polyline(lineOptions);
  temporaryLineHolder.push(temporaryLine);
  temporaryLine.setMap(geeMap);
}

// Draws polygon and places it on the canvas.
function drawPoly(canBeEdited, strokeWeight) {
 // We remove and redraw the polygon each time this runs.
 clearAllOverlays();
    if (isServing == '3D' && ge) {
      // Colors for the Earth plugin are taken as hex.
      var EarthPolyColor = hexToKmlColor(PolyColor);
      // Create the placemark.
      var polygonPlacemark = ge.createPlacemark('');
      // Create the polygon.
      var polygon = ge.createPolygon('');
      polygonPlacemark.setGeometry(polygon);
      var outer = ge.createLinearRing('');
      for (var i = 0; i < pointArray.length; i++) {
        outer.getCoordinates().pushLatLngAlt(pointArray[i][0],
         pointArray[i][1], 700);
      }
      polygon.setOuterBoundary(outer);
      // Create a style and set width and color of line.
      polygonPlacemark.setStyleSelector(ge.createStyle(''));
      var lineStyle = polygonPlacemark.getStyleSelector().getLineStyle();
      lineStyle.setWidth(5);
      lineStyle.getColor().set(EarthPolyColor);
      var polygonColor = polygonPlacemark.getStyleSelector().getPolyStyle();
      polygonColor.getColor().set(EarthPolyColor);
      // Add the placemark to Earth.
      ge.getFeatures().appendChild(polygonPlacemark);
      // Add our initial marker each time we redraw the polygon.
      drawMarker();
    } else if (isServing == '2D') {
      arrayCoords = [];
      for (var i = 0; i < pointArray.length; i++) {
        arrayCoords.push(new google.maps.LatLng(pointArray[i][0],
          pointArray[i][1]));
      }
      var polyOptions = {
        paths: arrayCoords,
        strokeColor: PolyColor,
        strokeOpacity: 0.7,
        strokeWeight: strokeWeight,
        fillColor: PolyColor,
        fillOpacity: 0.3,
        name: 'points',
        editable: canBeEdited,
        clickable: false
      };
      mapsPoly = new google.maps.Polygon(polyOptions);
      overlays.push(mapsPoly);
      // Listener to check if user has edited the polygon.
      google.maps.event.addListener(mapsPoly, 'mouseup', function(point) {
        resetPolygonCoordinates();
      });
      // Set Polygon.
      mapsPoly.setMap(geeMap);
    }
}

// Once finished, the polygon is editable.  We have a mouseup listener to
// let us know when that's happened.  It waits a moment, then grabs the
// coordinates of the 'new' polygon and replaces our old coordinates.
function resetPolygonCoordinates() {
  setTimeout(function() {
    if (mapsPoly) {

      // Clear our current polygon array.
      pointArray = [];

      // Get the coordinates of the edited polygon.
      var mapsPolyCoords = mapsPoly.getPath().getArray();
      if (mapsPolyCoords.length) {
        for (var i = 0; i < mapsPolyCoords.length; i++) {
          var newLat = mapsPolyCoords[i].lat();
          var newLon = mapsPolyCoords[i].lng();
          var latLongStorage = [newLat, newLon];
          pointArray.push(latLongStorage);
        }
      }

      // Refresh the kml string that is being stored in preparation
      // for finishing the cut.
      insertKML();
    }
  }, 200);
}

// Cancel out of whatever mode we are in, and start over.
// Restore UI elements to original state.
function cancel() {
  if (isServing == '2D') {
    geeMap.setOptions({draggableCursor: null});
  }
  if (mapsMarker != undefined) {
    mapsMarker.setMap(null);
  }
  finishedListeners();
  clearAllOverlays('cancel');
  pointArray = [];
  lineArrayCoords = [];
  boundsArray = [];
  cutterMode('start');
  gees.dom.setClass('GlobeIdHolder', 'CutterSelect BigSelect');
  gees.dom.setHtml('RegionNotes', 'Draw polygon to define region');
  gees.dom.show('button_holder');
  gees.dom.get('kml_field').value = '';
  populateZoomDropdowns(0, 'inner_zoom', '--');
  populateZoomDropdowns(0, 'outer_zoom', '--');
  populateZoomDropdowns(0, 'polygon_zoom', '--');
  resetRegionOptions();
}

// Finish drawing a polygon.
function finish() {
  // Cancel operation if there are no polygon points found.
  if (pointArray.length == 0) {
    cancel();
  } else if (pointArray.length > 2) {
    // If there is a polygon, do the following.
    cutterMode('complete');
    finishedListeners();
    clearAllOverlays();
    drawPoly(true, 4);
    if (isServing == '2D') {
      geeMap.setOptions({draggableCursor: null});
      resetPolygonCoordinates();
      if (mapsMarker != undefined) {
        mapsMarker.setMap(null);
      }
    }
    // Save our current polygon coordinates as KML.
    insertKML();
    gees.tools.setElementDisplay(DISPLAY_ELEMENTS_KML, 'none');
  } else {
    // If they haven't drawn a poly, encourage them to draw or cancel.
    var msg =
        'Not enough points. Please draw at least 3 points, or click Cancel.';
    gees.tools.errorBar(msg);
  }
}

// This function communicates with shared_assets/polygon_tools.js.  It takes
// a pasted KML string, and draws one or more polygons on the maps canvas.
function finishKML() {
 // Do not attempt to clear overlays if plugin is not detected.
 clearAllOverlays();
 var kmlString = gees.dom.get('kml_field').value;
 var kmlEdit = kmlString.replace(/^\s+|\s+$/g, '');
 // If Internet Explorer detected, use ActiveXObject instead of DOMParser.
 if (ieVersion > -1) {
  var xmlDoc = new ActiveXObject('Microsoft.XMLDOM');
  xmlDoc.async = false;
  xmlDoc.loadXML(kmlEdit);
  myKML = toGeoJSON.kml(xmlDoc, 'text/xml');
 } else {
  var parser = new DOMParser();
  myKML = toGeoJSON.kml(parser.parseFromString(kmlEdit, 'text/xml'));
 }
 if (myKML.features.length == 0) {
  popupTitle = 'Invalid KML';
  popupContents = 'Please confirm that you have entered valid KML. ';
  popupButtons = '<a href=\'#\' class=\'button blue finishDraw\'' +
      ' onclick=\'closeAllPopups();cutterMode("paste")\'>Ok</a>';
  gees.dom.get('WhiteOverlay').onclick = 'closeAllPopups()';
  gees.tools.toggleNotification(popupTitle, popupContents, popupButtons);
 } else {
  loadMapPolygon(myKML);
  cutterMode('complete');
  gees.dom.setClass('CutButtonBlue', 'button blue');
  gees.dom.get('kml_field').value = kmlEdit;
  gees.tools.setElementDisplay(DISPLAY_ELEMENTS_KML, 'none');
 }
}

// Take our polygon coordinates or pasted KML and insert it into our
// KML form. Our utilities will look for KML there when preparing a cut.
function insertKML() {
  var polyField = gees.dom.get('kml_field');
  var kmlFromForm = '';
  // Run through our coordinates, then insert into our kml tempalte below.
  for (var i = 0; i < pointArray.length; i++) {
    var point = pointArray[i][1] + ',' + pointArray[i][0] + ',' + '0 ';
    kmlFromForm += point;
  }

  kmlFromForm += pointArray[0][1] + ',' + pointArray[0][0] + ',' + '0 ';

  var kmlString = '<?xml version="1.0" encoding="UTF-8"?> ' +
  '<kml xmlns="http://www.opengis.net/kml/2.2"' +
  ' xmlns:gx="http://www.google.com/kml/ext/2.2"' +
  ' xmlns:kml="http://www.opengis.net/kml/2.2"' +
  ' xmlns:atom="http://www.w3.org/2005/Atom"> <Placemark> ' +
  '<Style> <PolyStyle> <color>c0800080</color> </PolyStyle> ' +
  '</Style> <Polygon> <outerBoundaryIs> ' +
  '<LinearRing> <coordinates> ' + kmlFromForm + ' </coordinates> ' +
  '</LinearRing> </outerBoundaryIs> </Polygon> </Placemark> </kml>';

  polyField.value = kmlFromForm == '' ? kmlFromForm : kmlString;
}

// Constantly update the current Zoom Level in the UI.  We do this so it is
// easier to know which zoom level you're on while making a cut.  Some of the
// functionality for this task resides in shared_assets/polygon_tools.js.
function getZoomLevel() {
  if (isServing == '2D') {
    gees.dom.setHtml(
      'ZoomDisplay', 'Your current zoom level is: ' + geeMap.zoom);
  } else {
    var camera = ge.getView().copyAsCamera(ge.ALTITUDE_RELATIVE_TO_GROUND);
    var alt = camera.getAltitude().toFixed(2);
    geeZoomLevelToCameraAltitudeMap = zoomLevelToAltitudeMap();
    var zoomArray = geeZoomLevelToCameraAltitudeMap.length;
    for (var i = 0; i < 26; i++) {
      var next = i + 1;
      var zoomLevel = geeZoomLevelToCameraAltitudeMap[i];
      var nextZoom = geeZoomLevelToCameraAltitudeMap[next];
      var currentZoomLevel;
      if (currentZoomLevel == undefined) {
       currentZoomLevel = 24;
      }
      if (alt < zoomLevel && alt > next) {
        if (alt < 0) {
         currentZoomLevel = 24;
        } else {
         currentZoomLevel = i;
        }
      }
      gees.dom.setHtml('ZoomDisplay',
          'Your current zoom level is: ' + currentZoomLevel + '<br>');
    }
  }
}

// We make temporary elements as we draw the polygon, and this removes them.
// We can simply remove the last element in Earth, but we need to push & pop
// from an array to keep track of elements in the Maps api.
function clearAllOverlays(arg) {
  if (isServing == '3D') {
    removeChildNodes();
  } else if (isServing == '2D') {
   while (overlays[0]) {
    overlays.pop().setMap(null);
   }
   while (lineHolder[0]) {
    lineHolder.pop().setMap(null);
   }
   while (temporaryLineHolder[0]) {
    temporaryLineHolder.pop().setMap(null);
   }
    if (arg == 'cancel') {
      arrayCoords = [];
      lineHolder = [];
    }
  }
}

// Remove all overlays from the earth plugin.
function removeChildNodes() {
  // Only perform if plugin is detected.
  if (ge) {
    var feature = ge.getFeatures();
    var nodes = feature.getChildNodes();
    var numOfNodes = nodes.getLength();
    for (var i = 0; i < numOfNodes; i++) {
      feature.removeChild(feature.getLastChild());
    }
  }
}
