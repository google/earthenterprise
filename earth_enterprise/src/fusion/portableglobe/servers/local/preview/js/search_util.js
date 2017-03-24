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
** This file handles all the functionality of portable search UI.
** Some of the heavy lifting is done by Python, but all of what
** happens in the UI is done here.
*/

RESULT_LIMIT = 5;
DEFAULT_SEARCH_ZOOM_LEVEL = 13;

var resultsArray = [];
var markerArray = [];
var numberOfIframes = [];

// The default list of search tabs for portable globes (3D).
// TODO: build list of search tabs based on information from globe.
var searchTabsFor3dDefault = [
  {
    args: [
      {
        screenLabel: '',
        urlTerm: 'searchTerm'
      }
    ],
    tabLabel: 'POI Search',
    url: '/Portable2dPoiSearch?service=PoiPlugin&displayKeys=searchTerm'
  },
  {
    args: [
      {
        screenLabel: 'Place',
        urlTerm: 'location'
      }
    ],
    tabLabel: 'GEPlaces',
    url: '/Portable2dPoiSearch?service=GEPlacesPlugin&displayKeys=location'
  }
];


/**
 * Unique ID.
 */
document.uidSequence = 1000;


/**
 * Make unique ID for each search result.
 * @return {int} the unique ID.
 */
function makeUniqueId() {
  document.uidSequence += 1;
  return document.uidSequence;
}


/**
 * Gets search tabs list for current portable.
 * Note: for 2D map, we get search tabs from geeServerDefs object,
 * for 3D globe (*.glb) - it is hardcoded.
 * @return {object} the search tabs list.
 */
function getSearchTabs() {
  if (isServing == '3D') {
    // it is 3D portable (*.glb).
    return searchTabsFor3dDefault;
  } else {
    // it is 2D portable.
    // Check that geeServerDefs are loaded.
    if (typeof(geeServerDefs) == 'undefined') {
      alert(
          'Error: The Google Earth Enterprise server does not recognize the ' +
          'requested database.');
      return;
    }

    return geeServerDefs.searchTabs;
  }
}


/**
 * Initializes Search pane:
 * - make sure there are search tabs to use in the first place, otherwise
 * don't let them search for nothing;
 * - make sure the POI search is first in the list if it exists.
 */
function initSearchPane() {
  var searchTabs = getSearchTabs();
  var numSearchTabs = searchTabs ? searchTabs.length : 0;

  // If there are search tabs, we can let them search through them,
  // so we'll display the search bar.
  document.getElementById('search_bar_holder').style.display =
      numSearchTabs > 0 ? 'block' : 'none';

  // Make sure "POI Search" is first if it exists.
  for (var z = 0; z < numSearchTabs; z++) {
    if (searchTabs[z].tabLabel == 'POI Search') {
      if (z > 0) {
        searchTabs.unshift(searchTabs.splice(z)[0]);
      }
      break;
    }
  }
}


/**
 * Executes search and displays results in search bar as user types in the
 * search field.
 */
function instantSearch() {
  handleSearchTabs(true);
}


/**
 * Executes search and displays results in SearchResults-window.
 */
function onClickSearch() {
  cleanSearchResults();
  document.getElementById('SearchResults').style.display = 'block';
  handleSearchTabs(false);
}


/**
 * Handles all search tabs - queries search services.
 * Get the entered search term, build search query, issue HTTP request
 * to search service, and display a list of returned results.
 * @param {boolean} isInstantSearch whether it is instant search query.
 */
function handleSearchTabs(isInstantSearch) {
  resultsArray = [];
  var searchTabs = getSearchTabs();
  var searchTerm = document.getElementById('SearchBar').value;
  document.getElementById('SearchResultsBar').style.display = 'block';
  document.getElementById('SearchResultsBar').innerHTML = '';
  if (searchTerm) {
    // Deferred object list to analyze search results after handling
    // all search tabs.
    var d = [];
    for (var z = 0; z < searchTabs.length; z++) {
      var name = searchTabs[z].tabLabel;
      var title = searchTabs[z].args[0].screenLabel;
      var tabUrl = searchTabs[z].url;
      if (name == 'GEPlaces') {
        var searchUrl = tabUrl + '&cb=a&location=' + searchTerm;
        var startingPoint = 3;
      } else if (name == 'POI Search') {
        var searchUrl = tabUrl + '&cb=a&searchTerm=' + searchTerm;
        var startingPoint = 2;
      }
      var opacity = title == 'Place' ? '.5' : '1';
      d.push(jQuery.Deferred());
      handleSearchTab(d[z],
                      searchUrl,
                      startingPoint,
                      opacity,
                      name,
                      isInstantSearch);

    }
    jQuery.when.apply(jQuery, d).done(function() {
      if (!isInstantSearch) {
        var div = document.getElementById('SearchResultsDiv');
        if (div && (!div.lastChild)) {
          // No search results.
          div.innerHTML = '<label>No results found.</label>';
        }
      }
    });
  }

  var optionsMenu = document.getElementById('OptionsMenu');
  if (optionsMenu.style.display == 'block') {
    optionsMenu.style.display = 'none';
    document.getElementById('DisplayText').innerHTML =
        '<div id="ListButton"></div>';
  }
}


/**
 * Handles query to search service.
 * @param {object} d the deferred object for synchronization.
 * @param {string} searchUrl the query URL.
 * @param {int} searchDataStart the index where search data begins in response.
 * @param {float} opacity the opacity value for displaying result in pane.
 * @param {string} label the label (name of search service) for displaying
 *        with search result record.
 * @param {boolean} isInstantSearch whether the instant search is triggered.
 */
function handleSearchTab(d,
                         searchUrl,
                         searchDataStart,
                         opacity,
                         label,
                         isInstantSearch) {
  querySearchService(
      searchUrl,
      searchDataStart,
      function(searchResults) {
        displaySearchResults(searchResults, opacity, label, isInstantSearch);
        // The job is done - trigger resolve.
        d.resolve();
      });
}


/**
 * Queries search service by issuing an asynchronous HTTP GET request.
 * @param {string} searchUrl the query URL.
 * @param {int} searchDataStart the index where search data begins in response.
 * @param {function} callbackFunc the callback-function to trigger on finishing
 *                   the request.
 */
function querySearchService(searchUrl, searchDataStart, callbackFunc) {
  jQuery.get(searchUrl, function(response, status) {
    var searchResults;
    if (status == 'success' &&
        response && response.length > searchDataStart + 2) {
      var searchDataEnd = response.length - 2;
      var searchData = response.substring(searchDataStart, searchDataEnd);
      try {
        searchData = JSON.stringify(eval('(' + searchData + ')'));
        searchResults = jQuery.parseJSON(searchData);
      } catch (err) {
        console.error('Failed to parse search data',
                      err.message);
      }
    }
    callbackFunc(searchResults);
  });
}


/**
 * Displays the list of search results (make them clickable) in either the
 * search bar pane (for instant search) or the SearchResults-pane (for onClick
 * search), and creates search markers.
 * @param {object} searchResults the search results.
 * @param {float} opacity the opacity value for displaying result in the search
 *        bar pane.
 * @param {string} label the label (name of search service) for displaying
 *        with search result record.
 * @param {boolean} isInstantSearch whether the instant search is triggered.
 */
function displaySearchResults(searchResults, opacity, label,
                              isInstantSearch) {
  if (typeof(searchResults) == 'undefined' ||
      typeof(searchResults.responses) == 'undefined' ||
      searchResults.responses.length == 0) {
    // Do nothing.
    return;
  }

  var data = searchResults.responses[0].data;
  var numResultsToShow = data.length;
  var div = document.getElementById('SearchResultsDiv');
  if (isInstantSearch) {
    numResultsToShow = Math.min(RESULT_LIMIT, numResultsToShow);
    div = document.getElementById('SearchResultsBar');
  }

  for (var i = 0; i < numResultsToShow; i++) {
    var searchData = data[i];
    var resultLat = parseFloat(searchData.lat);
    var resultLng = parseFloat(searchData.lon);
    var resultName = String(searchData.name);
    // Random unique ID for result info.  ID is used later to find the point
    // and remove from the map.
    var recordId = makeUniqueId();
    var markerData = [resultLat, resultLng, resultName, recordId];
    resultsArray.push(markerData);
    if (isInstantSearch) {
      // Show a search result in the SearchResultsBar-pane.
      div.innerHTML += '<a class="InstantSearchResults"' +
          'href="javascript:onClickResultInSearchPane(' + recordId + ');"' +
          '><div id="SearchResultsSpan">' +
          '<img src="/local/preview/images/red_pin.png" width="7px" ' +
          'height="12px" style="opacity:' + opacity + ';"  border="0">&nbsp;' +
          resultName + '<label>' + label + '</label></div></a>';
    } else {
      // Draw a marker.
      dropMarker(resultLat, resultLng, resultName);
      // Show a search result in the SearchResults-pane.
      displaySearchResult(div, resultLat, resultLng, resultName);
    }
  }
}


/**
 * Handles click on a search result in SearchResultBar-pane:
 * - clean existing markers and SearchResults-pane;
 * - for a search result addressed by recordId, create marker and display
 *   in SearchResults-pane;
 * - pan and zoom to the search result location.
 * @param {int} resultId the ID of clicked item.
 */
function onClickResultInSearchPane(resultId) {
  // Clean existing search results.
  cleanSearchResults();

  // Create/display a clicked search result.
  var numOfResults = resultsArray.length;
  document.getElementById('SearchResults').style.display = 'block';
  for (var i = 0; i < numOfResults; i++) {
    var result = resultsArray[i];
    if (result[3] == resultId) {
      flyToSpot(result[0], result[1]);
      dropMarker(result[0], result[1], result[2]);
      var div = document.getElementById('SearchResultsDiv');
      displaySearchResult(div, result[0], result[1], result[2]);
      break;
    }
  }
  resultsArray = [];
  removeSearchPane();
}


/**
 * Tells the camera where to look when a marker is created.
 * @param {double} lat the latitude.
 * @param {double} lng the longitude.
 */
function flyToSpot(lat, lng) {
  if (isServing == '3D') {
    var lookAt = ge.createLookAt(name);
    lookAt.setLatitude(lat);
    lookAt.setLongitude(lng);
    lookAt.setRange(geeZoomLevelToCameraAltitudeMap[DEFAULT_SEARCH_ZOOM_LEVEL]);
    ge.getView().setAbstractView(lookAt);
  } else if (isServing == '2D') {
    geemap.panAndZoom(lat, lng, DEFAULT_SEARCH_ZOOM_LEVEL);
  }
}


/**
 * Tells Maps/Earth where to place a marker/placemark on the screen.
 * Also creates an "entry" listing for the infoPane, which allows
 * for panning back to and removing the marker.
 * @param {double} lat the latitude.
 * @param {double} lng the longitude.
 * @param {string} name the name of placemark.
 */
function dropMarker(lat, lng, name) {
  if (isServing == '3D') {
    markerArray.push(name);
    // Create the placemark
    var searchMarker = ge.createPlacemark('');
    searchMarker.setName(name);
    var point = ge.createPoint('');
    point.setLatitude(lat);
    point.setLongitude(lng);
    searchMarker.setGeometry(point);
    ge.getFeatures().appendChild(searchMarker);
    // Create style for placemark
    var icon = ge.createIcon('');
    icon.setHref(GEE_SERVER_URL + '/local/preview/images/red-circle.png');
    var style = ge.createStyle('');
    style.getIconStyle().setIcon(icon);
    searchMarker.setStyleSelector(style);
  } else if (isServing == '2D') {
    var point = new google.maps.LatLng(lat, lng);
    var infowindow = new google.maps.InfoWindow({
      content: '<div id="InfoWindow">' + String(name) + '</div>'
    });
    var markerIcon = '/local/preview/images/location_pin.png';
    var markerName = new google.maps.Marker({
      map: geemap,
      position: point,
      draggable: false,
      icon: markerIcon,
      title: name
    });
    markerArray.push(markerName);
    google.maps.event.addListener(markerName, 'click', function() {
      infowindow.open(geemap, markerName);
    });
  }
}


/**
 * Displays a search result in the SearchResults-pane.
 * @param {Element} div the SearchResults pane element.
 * @param {double} lat the latitude.
 * @param {double} lng the langitude.
 * @param {string} name the name of search result.
 */
function displaySearchResult(div, lat, lng, name) {
  var search_label = document.createElement('label');
  div.appendChild(search_label);
  search_label.innerHTML =
      '<img src="/local/preview/images/red_pin.png" width="7px" ' +
      'height="12px" border="0"/>' + name;
  search_label.onclick = (function(lat, lng) {
    return function() { flyToSpot(lat, lng); }
  }) (lat, lng);
  div.appendChild(document.createElement('br'));
}


/**
 * Cleans up display elements of search results, and hides
 * the SearchResults-pane.
 */
function closeSearchResults() {
  cleanSearchResults();
  document.getElementById('SearchResults').style.display = 'none';
}


/**
 * Cleans up display elements (markers, SearchResults-pane) of search results
 * and marker list.
 */
function cleanSearchResults() {
  cleanMarkers();
  cleanSearchResultsPane();
}


/**
 * Removes marker or placemark.  Maps must have a specifically id'ed marker.
 */
function cleanMarkers() {
  if (isServing == '2D') {
    for (var i = 0; i < markerArray.length; i++) {
      markerArray[i].setMap(null);
    }
  } else {
    var features = ge.getFeatures();
    while (features.getFirstChild()) {
      features.removeChild(features.getFirstChild());
    }
  }
  markerArray = [];
}


/**
 * Removes search results from the SearchResults-pane.
 */
function cleanSearchResultsPane() {
  var div = document.getElementById('SearchResultsDiv');
  if (div) {
    while (div.lastChild) {
      div.removeChild(div.lastChild);
    }
  }
}


/**
 * Set keyup handler.
 * Typing anything in the search bar performs instant search and displays top
 * results; pressing enter or return calls onClick search and displays
 * full results.
 */
jQuery(document).keyup(function(d) {
  if (document.activeElement.id == 'SearchBar') {
    if (d.which == 13) { // Return or enter key.
      onClickSearch();
    } else {
      instantSearch();
    }
  }
});


/**
 * Cleans up SearchResultsBar pane.
 */
function removeSearchPane() {
  document.getElementById('SearchResultsBar').style.display = 'none';
  document.getElementById('searchfade').style.display = 'none';
  document.getElementById('SearchBar').value = '';
}
