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
  Shared resource for Server Viewer to provide search capabilities.
*/

// Globals used throughout for search.
var GEE_HOST = window.location.host;
var GEE_BASE_URL = window.location.protocol + '//' + GEE_HOST;
var SEARCH_DEFS_LIST = window.location.pathname + 'search_json';
var SEARCH_DEFS_REQUEST = GEE_BASE_URL + SEARCH_DEFS_LIST;
var DEFAULT_SEARCH_ZOOM_LEVEL = 5;
var MARKER_ICON = '/shared_assets/images/location_pin.png';

// Min distance (in degrees) for search bounds.
var MIN_DISTANCE_IN_DEGREES = 0.0028;  // Zoom to about level 17.
var MIN_HALF_DISTANCE_IN_DEGREES = MIN_DISTANCE_IN_DEGREES / 2.0;

// Globals to hold various states for search.
var searchDefsListing;
var isServing = '';
var markerArray = [];
var searchLabelHolder = [];
var searchPolygonHolder = [];
var searchResultsBounds;
var boundsArray = [];

// Confirm the item being viewed has related search information.
// If not, do nothing.  If so, initialize search tools.
function initSearch() {
  $.ajax({
        url: SEARCH_DEFS_REQUEST,
          success: function(e) {
            getSearchTabs();
          }
    });
}

// Get the list of search tabs associated with the target path.
function getSearchTabs() {
  searchLabelHolder = [];
  makeSearchLabel();
  searchDefsListing = JSON.parse(makeGETRequest(SEARCH_DEFS_REQUEST));
  // Get and define variables for the tab.
  for (var i = 0; i < searchDefsListing.length; i++) {
    searchLabelHolder.push(searchDefsListing[i].label);
    addSearchTab(searchDefsListing[i], i);
  }
}

// Returns a string to be parsed.
function makeGETRequest(url) {
  var retrieveDatabases = new XMLHttpRequest();
  retrieveDatabases.open('GET', url, false);
  retrieveDatabases.send();
  return retrieveDatabases.responseText;
}

// Create a heading label for the search tools div.
function makeSearchLabel() {
  var searchDiv = document.getElementById('SearchDiv');
  searchDiv.innerHTML = '<span>' +
  'Search tools<img onclick="toggleSearchDiv()"' +
  ' src="' + arrowCloseImg + '"/ id="SearchArrow"' +
  '></span>';
}

// Allow the user to toggle visibility of the search tools
// div.  Beginning state is hidden.
function toggleSearchDiv() {
  var display;
  var labelDiv = document.getElementById('SearchArrow');
  var element = document.getElementById(searchLabelHolder[0]);
  display = element.style.display == 'block' ? 'none' : 'block';
  for (var i = 0; i < searchLabelHolder.length; i++) {
    var searchElement = document.getElementById(searchLabelHolder[i]);
    searchElement.style.display = display;
  }
  if (display == 'block') {
    labelDiv.src = arrowOpenImg;
  } else {
    labelDiv.src = arrowCloseImg;
  }
}

// Allow user to toggle visibility of the div that holds the results
// of a recently performed search.
function toggleResultsDiv() {
  var resultsDiv = document.getElementById('ResultsDiv');
  var resultsArrow = document.getElementById('ResultsArrow');
  if (resultsDiv.style.display == 'block') {
    resultsDiv.style.display = 'none';
    resultsArrow.src = arrowCloseImg;
  } else {
    resultsDiv.style.display = 'block';
    resultsArrow.src = arrowOpenImg;
  }
}

// Add an element to the display that allows a search to be performed.
function addSearchTab(tab, number) {
  var searchDiv = document.getElementById('SearchDiv');
  var label = tab.label;
  searchDiv.innerHTML += '<div name="SearchLabel" id="' + label + '">' +
  label + createSearchInputs(tab.fields, number) + '</div>';
}

// Given a set of fields, create input fields for the search form.
function createSearchInputs(fields, number) {
  var element = '';
  for (var i = 0; i < fields.length; i++) {

    // Placeholder (hint) for input values that are not null.
    var placeholder = fields[i].label == 'null' || fields[i].label == null ?
        '' : fields[i].label;

    // Suggestion field that appears below the search input.
    var suggestion = fields[i].suggestion == 'null' ||
        fields[i].suggestion == null ? '' : fields[i].suggestion;

    // Add an input for each field.
    element += '<input name="SearchField" type="text"  id="' + number +
        '_' + fields[i].key + '" placeholder="' + placeholder + '"/>' +
        '<em>' + suggestion + '</em>';
  }
  // Add submit button that will perform the search.
  element += '<input class="StandardButton" type="submit" value="Search" ' +
      'onclick="submitSearchForm(\'' + number + '\')">';
  return element;
}

// Completely remove the div containing the list of search results.
function hideResults() {
  resetSearchDisplayOptions();
  clearSearchInputs();
  gees.tools.removeElementChildren('ResultsDiv');
  document.getElementById('SearchResults').style.display = 'none';
}

// Submit a search form.  Num is an array location referring to a
// particular search service.
function submitSearchForm(num) {
  gees.tools.removeElementChildren('ResultsDiv');

  var tab = searchDefsListing[num];
  var fields = tab.fields;
  var url = tab.service_url + '?';
  var query = getQueryParams(fields, num);
  if (tab.additional_query_param && tab.additional_query_param != null) {
    query += '&' + tab.additional_query_param;
  }
  if (tab.additional_config_param && tab.additional_config_param != null) {
    query += '&' + tab.additional_config_param;
  }
  var isRelative = url.search('http') != 0;
  var requestUrl = '';
  if (isRelative) {
    // Search services formatted with a preceding slash will be
    // GEE_BASE_URL/search_service.  Otherwise, search service
    // should be preceded by Publish Point.  For relative paths only.
    var addPublishPoint = url.search('/') == 0 ?
        '' : window.location.pathname;
    requestUrl += GEE_BASE_URL + addPublishPoint;
  }
  requestUrl += url + query + '&callback=handleSearchResults';

  // Using the requestUrl constructed above, make a request to a search
  // service.  In some cases, valid JSON will be returned, which can be
  // immediately used to display results.  However, if the response is not
  // valid JSON, we assume it is JSONP (a string), and place it within a
  // script tag, directly into the DOM.
  gees.requests.asyncGetRequest(requestUrl, function(response) {

    // Try to parse response as JSON.  If the JSON is valid, it did not
    // accept the callback parameter of handleSearchResults.  So, the
    // JSON response must be sent to the function manually.
    try {
      response = jQuery.parseJSON(response);
      handleSearchResults(response);
    } catch(e) {

      // In this case, the response is JSONP.  This will be JSON wrapped
      // within the callback function (handleSearchResults), so all that
      // is needed is to place it within a script tag, and force it into
      // the DOM, which will cause the function to execute, and the search
      // results to be displayed.
      $('#SearchScriptHolder').html('<script type="text/javascript">' +
          response + '<\/script>');
    }
  },

  // If the request fails completely, it may be a cross domain request.  If
  // this is the case, we must force the requestUrl as a script reference into
  // the DOM to get around cross domain request restrictions.
  function(error) {
    $('#SearchScriptHolder').html('<script src="' + requestUrl +
        '" type="text/javascript"><\/script>');
  });
}

// Go over a form for a search service and return its contents.
function getQueryParams(fields, num) {
  var query = '';
  var delimiter = '';
  for (var i = 0; i < fields.length; i++) {
    var key = fields[i].key;
    var param = num + '_' + key;
    var value = document.getElementById(param).value;
    query += delimiter + key + '=' + encodeURIComponent(value);
    delimiter = '&';
  }
  return query;
}

// After retrieving search results, organize and insert into UI.
function handleSearchResults(searchResultJson) {

  // Reset all display options for the search results area of the UI.
  resetSearchDisplayOptions();

  // Quit if no search results exist.
  if (!searchResultJson) {
    return false;
  }

  // Initialize variable for the div that holds list of search results.
  var resultsDiv;
  document.getElementById('SearchResults').style.display = 'block';

  // Most results will contain Folder and Placemark objects.
  if (searchResultJson.Folder && searchResultJson.Folder.Placemark) {
    var searchResults = searchResultJson.Folder.Placemark;
    resultsDiv = setResultsDiv(searchResults);

    // If Point property is found on the base object, it is a simple
    // coordinate search with only one result.
    if (searchResults.Point) {
      var coordString = searchResults.Point.coordinates;
      var coordArray = coordString.split(',');

      // Note that coordArray is in lat/lng order.
      // It seems that this ordering is different from the rest.
      // TODO: unify coordinates order in search response?!
      var lat = parseFloat(coordArray[0]);
      var lng = parseFloat(coordArray[1]);

      // Create a label to represent the search result.
      createSearchResultLabel(lat, lng, coordString, resultsDiv);

      // Create a marker on the map for the search result.
      createSearchMarker(lat, lng, coordString);

      // Pan to the single search result.
      flyToSpot(lat, lng);

    // If Point property is not found on object, it is an array of
    // results or single result.
    } else {

      // Iterate over many search results and create labels & markers for each.
      if (searchResults.length) {
        for (var i = 0; i < searchResults.length; i++) {
          var item = searchResults[i];
          createSearchResultLabelAndMarker(item, resultsDiv);
        }
      } else {

        // Sometimes the response will be a single result object.  Create the
        // single search label and marker.
        createSearchResultLabelAndMarker(searchResults, resultsDiv, true);
      }
    }

  // If Folder and Placemark objects are not found on the object, this is a
  // Maps API Geocode search, and the results have a different format.
  } else {
    if (searchResultJson.results) {
      resultsDiv = setResultsDiv(searchResultJson.results);
      document.getElementById('SearchResults').style.display = 'block';
      createGeocodeSearchResultLinks(searchResultJson.results, resultsDiv);
    } else {
      setResultsDiv();
    }
  }
  setSearchResultBounds();
}

// Set bounds for the viewport, showing a group of search results.
function setSearchResultBounds() {
  if (!searchResultsBounds) {
    return;
  }

  // Ensure that bounds are valid.
  if (searchResultsBounds.south >= searchResultsBounds.north) {
    return;
  }
  if (searchResultsBounds.west >= searchResultsBounds.east) {
    return;
  }

  // Call a normalize function that ensures valid bounds.
  searchResultsBounds.normalize();

  if (isServing == '2D') {
    set2dSearchBounds();
  } else {
    set3dSearchBounds();
  }
}

// Update the global search results bounds.
function updateSearchResultsBounds(lat, lng) {
  if (lat < searchResultsBounds.south) {
    searchResultsBounds.south = lat;
  }
  if (lat > searchResultsBounds.north) {
    searchResultsBounds.north = lat;
  }
  if (lng < searchResultsBounds.west) {
    searchResultsBounds.west = lng;
  }
  if (lng > searchResultsBounds.east) {
    searchResultsBounds.east = lng;
  }
}

// Set bounds on a Map canvas for a group of search results.
function set2dSearchBounds() {

  // Get the bounds for the map.
  var b = searchResultsBounds;
  var nw = new google.maps.LatLng(b.north, b.west);
  var se = new google.maps.LatLng(b.south, b.east);

  // Define map bounds.
  var mapBounds = new google.maps.LatLngBounds(nw, se);

  // Do not let the map zoom to an LOD greater than 18.
  if (se.lng() - nw.lng() <= MIN_DISTANCE_IN_DEGREES) {
    var ctr_lng = (se.lng() + nw.lng()) / 2;
    if (ctr_lng + MIN_HALF_DISTANCE_IN_DEGREES > 180) {
      ctr_lng = 180 - MIN_HALF_DISTANCE_IN_DEGREES;
    }  else if (ctr_lng - MIN_HALF_DISTANCE_IN_DEGREES <  -180) {
      ctr_lng = -180 + MIN_HALF_DISTANCE_IN_DEGREES;;
    }

    mapBounds.extend(new google.maps.LatLng(nw.lat(),
        ctr_lng - MIN_HALF_DISTANCE_IN_DEGREES));
    mapBounds.extend(new google.maps.LatLng(se.lat(),
        ctr_lng + MIN_HALF_DISTANCE_IN_DEGREES));
  }

  // Fit to bounds.
  geeMap.fitBounds(mapBounds);
}

// Set bounds on an Earth canvas for a group of search results.
function set3dSearchBounds() {
  var latDiff = searchResultsBounds.north - searchResultsBounds.south;
  var lngDiff = searchResultsBounds.east - searchResultsBounds.west;
  if (latDiff > lngDiff) {
   maxDist = latDiff;
  } else {
   maxDist = lngDiff;
  }
  if (maxDist < MIN_DISTANCE_IN_DEGREES) {
    maxDist = MIN_DISTANCE_IN_DEGREES;
  }
  var zoomLevel = Math.floor(Math.log(360 / maxDist) / Math.log(2));
  CenterLat = (searchResultsBounds.north + searchResultsBounds.south) / 2;
  CenterLng = (searchResultsBounds.east + searchResultsBounds.west) / 2;
  geeZoomLevelToCameraAltitudeMap = zoomLevelToAltitudeMap();
  var lookAt = ge.createLookAt('');
  lookAt.setLatitude(CenterLat);
  lookAt.setLongitude(CenterLng);
  lookAt.setRange(geeZoomLevelToCameraAltitudeMap[zoomLevel]);
  ge.getView().setAbstractView(lookAt);
}

// Fly to a polygon that is being held in a list.
function searchFlyToPolygon(id) {
  // Pull the array of coordinates for the polygon, using its id.
  var array = searchPolygonHolder[id].coordinates[0];
  if (isServing == '2D') {
    set2dSearchBounds(array);
  } else {
    set3dSearchBounds(array);
  }
}

// Reset some variables, used before perfoming a search.
function resetSearchDisplayOptions() {
  clearSearchPolygons();
  clearSearchMapMarkers();
  searchPolygonHolder = [];
  overlays = [];
  markerArray = [];
  searchResultsBounds = {
    south: 90.0,
    north: -90.0,
    west: 180.0,
    east: -180.0,
    normalize: function() {
      if (this.north > 90) {
        this.north = 90;
      }
      if (this.south < -90) {
        this.south = -90;
      }
      if (this.west < -180) {
        this.west = -180;
      }
      if (this.east > 180) {
        this.east = 180;
      }
    }
  };
}

// Clear past searches from all inputs.
function clearSearchInputs() {
  var inputs = document.getElementsByName('SearchField');
  for (var i = 0; i < inputs.length; i++) {
    inputs[i].value = '';
  }
}

// Create a label element that will be a clickable label to view a
// search result.  Also create a marker (polygon in some cases) for
// each search result found.
function createSearchResultLabelAndMarker(item, div, singleItem) {
  var name = item.name;
  var type = item.type;
  var coords = item.coordinates;
  var desc = item.description;
  if (type == 'Point') {
    // Note that Point coordinates are in lng/lat order.
    var lat = parseFloat(coords[1]);
    var lng = parseFloat(coords[0]);
    updateSearchResultsBounds(lat, lng)
    createSearchResultLabel(lat, lng, name, div);
    createSearchMarker(lat, lng, name, desc);
    if (singleItem) {
      flyToSpot(lat, lng);
    }
  } else if (type == 'MultiPolygon' || type == 'Polygon') {
    createSearchResultPolygon(item.coordinates, div, name);
  }
}

// Create a search result label and map pin for each result from
// a Geocode search.
function createGeocodeSearchResultLinks(results, div) {
  for (var i = 0; i < results.length; i++) {
    var result = results[i];
    var address = result.formatted_address;
    var lat = result.geometry.location.lat;
    var lng = result.geometry.location.lng;
    updateSearchResultsBounds(lat, lng);
    createSearchMarker(lat, lng, address);
    createSearchResultLabel(lat, lng, address, div);
  }
}

// Place a marker on the Map or Earth canvas.
function createSearchMarker(lat, lng, name, desc) {
  desc = desc || name;
  if (isServing == '2D') {
    makeMapMarker(lat, lng, name, desc);
  } else {
    makeEarthMarker(lat, lng, name, desc);
  }
}

function flyClosure(lat, lng) {
  function fly() {
    flyToSpot(lat, lng);
  }
  return fly;
}

// Create a label in the list of search results to represent an
// individual result.
function createSearchResultLabel(lat, lng, name, div) {
  search_label = document.createElement('label');
  div.appendChild(search_label);
  search_label.innerHTML =
      '<img src="/shared_assets/images/search_result_pin.png"/>' + name;
  search_label.onclick = flyClosure(lat, lng);
}

// For a polygon result, create a label element that will serve as a link
// to view the polygon.  Also push the polygon coordinates into an array
// that will be referenced to load the polygon.
function createSearchResultPolygon(coords, div, name) {
  var div = document.getElementById('ResultsDiv');
  // Some results may refer to multiple polygons.  Iterate over them to place
  // all of them on the canvas, but only create one label for the
  // entire result.
  for (var i = 0; i < coords.length; i++) {
    var polygon = {};
    polygon.type = 'polygon';
    polygon.coordinates = [];
    // Our polygon parsing function expects the coords to be on the 0th
    // node of the coordinate property.
    polygon.coordinates[0] = [];
    getPolygonAndUpdateBounds(coords[i], polygon);
    var polyId = searchPolygonHolder.length;
    searchPolygonHolder.push(polygon);
    // Show the polygon on the canvas.
    showSearchResultPolygon(polyId);
  }
  div.innerHTML +=
      '<label onclick="searchFlyToPolygon(\'' + polyId + '\')">' +
      '<img src="/shared_assets/images/search_result_poly.png"/>' + name +
      '</label>';
}

// Given a polygon, add its coords to the global bounds object.
function getPolygonAndUpdateBounds(coords, polygon) {
  for (var i = 0; i < coords.length; i++) {
    if (typeof coords[i][0] == 'number') {
      polygon.coordinates[0].push(coords[i]);
      // Note that Point coordinates are in lng/lat order.
      var lat = parseFloat(coords[i][1]);
      var lng = parseFloat(coords[i][0]);
      updateSearchResultsBounds(lat, lng);
    } else {
      getPolygonAndUpdateBounds(coords[i], polygon);
    }
  }
}

// Helper function for setting state of Search Results element.
function setResultsDiv(results) {
  var div = document.getElementById('ResultsDiv');
  if (div.style.display == 'none') {
    toggleResultsDiv();
  }

  // Display the total number of results.
  if (results) {

    // If results has a length property, it is a list of results.
    if (results.length) {
      div.innerHTML += '<em>' + results.length + ' results</em>';

    // If results exist but have no length property, results is an object
    // with a single search result.
    } else {
      div.innerHTML += '<em>1 result</em>';
    }
  } else {
    div.innerHTML = '<label>No results found.</label>';
  }
  return div;
}

// Create a polygon for a search result.
function showSearchResultPolygon(index) {
  if (isServing == '2D') {
    construct2dPolygon(searchPolygonHolder[index], false);
  } else if (isServing == '3D') {
    construct3dPolygon(searchPolygonHolder[index], false);
  }
}

// Clear all polygons from the current canvas.
function clearSearchPolygons() {
  if (isServing == '2D') {
    for (var i = 0; i < overlays.length; i++) {
      overlays[i].setMap(null);
    }
  } else if (isServing == '3D') {
    clearSearchEarthOverlays();
  }
  overlays = [];
}

// Clear all markers from the current canvas.
function clearSearchMapMarkers() {
  if (isServing == '2D') {
    for (var i = 0; i < markerArray.length; i++) {
      markerArray[i].setMap(null);
    }
  } else {
    clearSearchEarthOverlays();
  }
  markerArray = [];
}

// Clear any overlays that might exist on the earth plugin.
function clearSearchEarthOverlays() {
  while (ge.getFeatures().getFirstChild()) {
    ge.getFeatures().removeChild(ge.getFeatures().getFirstChild());
  }
}

// Make a marker to add to a Google Maps canvas.
function makeMapMarker(lat, lng, name, desc) {
  var markerLoc = new google.maps.LatLng(lat, lng);
  var marker = new google.maps.Marker({
      position: markerLoc,
      icon: MARKER_ICON,
      map: geeMap,
      title: name
    });
  var markerInfo = new google.maps.InfoWindow({
      content: name + '<br>' + desc
    });
  google.maps.event.addListener(marker, 'click', function() {
      markerInfo.open(geeMap, marker);
    });
  markerArray.push(marker);
}

// Marke a marker to add to the Google Earth plugin.
function makeEarthMarker(lat, lng, name, desc) {
  // Create the placemark.
  var placemark = ge.createPlacemark('');
  placemark.setName(name);

  // Set the placemark's location.
  var point = ge.createPoint('');
  point.setLatitude(lat);
  point.setLongitude(lng);
  placemark.setGeometry(point);

  // Add the placemark to Earth.
  ge.getFeatures().appendChild(placemark);

  // Create style for placemark
  var icon = ge.createIcon('');
  icon.setHref(GEE_BASE_URL + MARKER_ICON);
  var style = ge.createStyle('');
  style.getIconStyle().setIcon(icon);
  placemark.setStyleSelector(style);
}

// Tells the camera where to look when a marker is created.
function flyToSpot(lat, lng) {
  if (isServing == '3D') {
    geeZoomLevelToCameraAltitudeMap = zoomLevelToAltitudeMap();
    var lookAt = ge.createLookAt(name);
    lookAt.setLatitude(lat);
    lookAt.setLongitude(lng);
    lookAt.setRange(geeZoomLevelToCameraAltitudeMap[DEFAULT_SEARCH_ZOOM_LEVEL]);
    ge.getView().setAbstractView(lookAt);
  } else if (isServing == '2D') {
    geeMap.panAndZoom(lat, lng, DEFAULT_SEARCH_ZOOM_LEVEL);
  }
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
