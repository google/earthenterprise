/*
 * Copyright 2017 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Description: Javascript utilities for Google Earth Enterprise example
 * javascript applications.
 * Includes:
 *  UI utilities for setting up and resizing the HTML elements for
 *    the basic GEE Example UI.
 *  Default search tab definitions.
 *  Element access utilities
 *  Event utilities
 *  URL parsing utilities
 *  JSON loading utilities
 * Version: 1.01
 *
 */

/*
 * Keep track of left panel state.
 */
var geeIsLeftPanelOpen = false;  // Keep track of the state of the left panel.
var geeLeftPanelWidth = 0;  // Keep track of this initial setting.
var geeShimZIndex = 10;  // For Earth plugin, we need to shim controls on top
                         // of the earth panel.
var geeGlobesTitle = 'Globes and Maps';
var geePolygonDisplayTime = 1500;
if (typeof GEE_POLYGON_DISPLAY_TIME !== 'undefined') {
  geePolygonDisplayTime = GEE_POLYGON_DISPLAY_TIME;
}

/**
 * Resize the leftPanel and map divs to match contents and window size.
 * Assumes the global variable geeDivIds is defined with the appropriate
 * members.
 */
function geeResizeDivs() {
  resizeMap(geeDivIds);
}

/**
 * Construct the required divs in the left panel div.
 * Assumes the global variable geeDivIds is defined with the appropriate
 * members.
 * @param {boolean}
 *         showGlobesList Include globe selector in LHP.
 */
function geeInitLeftPanelDivs(showGlobesList) {
  var leftPanelDiv = document.getElementById(geeDivIds.leftPanel);
  geeLeftPanelWidth = getElementWidth(leftPanelDiv);

  // Search Results Divs
  var searchTitle = createElement(leftPanelDiv, 'div', geeDivIds.searchTitle);
  var cancelButton =
    '<img id="search_results_cancel" src="' +
    geeIconUrls.cancelButton + '" title = "Close" />';
  searchTitle.innerHTML = cancelButton + '<span>Search Results</span>';
  var searchContainer = createElement(leftPanelDiv, 'div',
                                      geeDivIds.searchResults);
  // Globes Divs
  if (showGlobesList) {
    var globesTitle = createElement(leftPanelDiv, 'div', geeDivIds.globesTitle);
    globesTitle.innerHTML = geeGlobesTitle;
    var globesContainer = createElement(leftPanelDiv, 'div',
                                        geeDivIds.globes);
    globesContainer.innerHTML = '';
  }

  // Layers Divs
  var layersTitle = createElement(leftPanelDiv, 'div', geeDivIds.layersTitle);
  layersTitle.innerHTML = 'Layers';
  var layerContainer = createElement(leftPanelDiv, 'div', geeDivIds.layers);

  var headerDiv = findElement(geeDivIds.header);
  var collapsePanelDiv = createElement(headerDiv, 'div',
                                       geeDivIds.collapsePanel);

  // Toggle the left panel open to initialize it...hold off on resize, will
  // happen soon after this call.
  geeToggleLeftPanelOpen(true, false);
}

/**
 * Hack Alert: we need an iframe shim to get the collapse button to
 * appear above the GE Plug-in. This shim must be created after the plugin is
 * initialized.
 */
function geeCreateShims() {
  var collapsePanelDiv = findElement(geeDivIds.collapsePanel);
}

/**
 * Concatenate layerId and channel to create uid.
 * @param {string}
 *          layerId Id of layer which may contain one or more sublayers.
 * @param {string}
 *          channel Id of sublayer.
 * @return {string} uid derived from layer and channel ids.
 */
function geeUid(layerId, channel) {
  return layerId + '-' + channel;
}

/**
 * Create a basic layer item with an icon and name along with hover
 * behaviors and toggle checkbox that calls a global callback.
 * @param {Element}
 *          layerList the list element container for the item being created.
 * @param {string}
 *          checkboxId some id to make checkbox name unique.
 * @param {string}
 *          channel is the sub-layer id within the layer.
 * @param {string}
 *          layerId is the id of the layer item.
 * @param {string}
 *          layerName is the name/label for the item.
 * @param {string}
 *          layerIconUrl is the URL for the icon associated with the icon.
 * @param {boolean}
 *          isChecked is true if the layer should be toggle on initially.
 * @param {string}
 *          toggleCallbackName is the name of the layer toggle callback
 *          function. The callback should take the following arguments
 *          function(Event, checkboxId, layerId, layerName).
 * @return {Element}
 *           the list item with checkbox toggle, icon and label.
 */
function geeCreateLayerItemBasic(layerList,
                                 checkboxId,
                                 channel,
                                 layerId,
                                 layerName,
                                 layerIconUrl,
                                 isChecked,
                                 toggleCallbackName) {
  // Construct the innerHTML fully before assignment to item.
  // The checkbox behavior includes an onclick event to toggle the layer
  // visibility.
  var itemHtml = [];  // Build up the item string using Array + join.
  var i = -1;
  itemHtml[++i] = '<input id="';
  itemHtml[++i] = checkboxId;
  itemHtml[++i] = '" type="checkbox" onclick=\'';
  itemHtml[++i] = toggleCallbackName;
  itemHtml[++i] = '(arguments[0], "';
  itemHtml[++i] = checkboxId;
  itemHtml[++i] = '", "';
  itemHtml[++i] = channel;
  itemHtml[++i] = '", "';
  itemHtml[++i] = layerId;
  itemHtml[++i] = '", "';
  itemHtml[++i] = layerName;
  itemHtml[++i] = '");\' ';
  if (isChecked) {
    itemHtml[++i] = 'checked="true"';
  }
  itemHtml[++i] = ' \/> ';
  if (layerIconUrl) {
    itemHtml[++i] = '<span id="' + layerName + '_icon"><img src="';
    itemHtml[++i] = layerIconUrl;
    itemHtml[++i] = '" \/></span> ';
  }
  itemHtml[++i] = '<span id="';
  itemHtml[++i] = geeUid(layerId, channel);
  itemHtml[++i] = '_name">';
  itemHtml[++i] = layerName;
  itemHtml[++i] = '<\/span>';
  // Create the item.
  var item = createElement(layerList, 'li');
  item.className = 'layer_item';
  item.innerHTML = itemHtml.join('');

  // Highlight the layer on hover.
  item.onmouseover = function(layerItem) {
    return function(e) {
      layerItem.className = 'layer_item_selected';
      cancelEvent(e);
    };
  }(item);
  item.onmouseout = function(layerItem) {
    return function(e) {
      layerItem.className = 'layer_item';
      cancelEvent(e);
    };
  }(item);
  return item;
}

/**
 * Create an array of 1 default search tab. This search tab will allow the user
 * to jump to a LatLng without any server request.
 * @return {Array.<Object>} the list of search tab dictionary entries.
 */
function geeDefaultSearchTabs() {
  var searchTab = {
      'tabLabel': 'Fly To',
      'url': '',  // This is a callback tab, not a url tab.
      'callback': function(latlng) { geeSubmitLatLng(latlng); },
      'args':
        [{ 'screenLabel':
              'Latitude Longitude (e.g., "40.1 -90.7" or "40.1, -90.7")',
           'urlTerm': 'latlng' }]
    };
  var searchTabs = [searchTab];
  return searchTabs;
}

/**
 * The default callback for a LatLng geocode query.
 *
 * @param {string}
 *          latlng a string containing the latitude and longitude Expects the
 *          format to be:
 *          1) comma separated or
 *          2) space separated "lat lng", with the two numbers separated by one
 *          or more spaces. This will handle multiple spaces in or at the
 *          beginning/end of the string.
 */
function geeSubmitLatLng(latlng) {
  // First try comma
  var latlng_array = latlng.split(',');
  if (latlng_array.length >= 2) {
    geePanTo(latlng_array[0], latlng_array[1]);
    return;
  }

  // 2nd, try space separated.
  latlng = latlng.replace(/^[ ]+/g, '');  // Remove leading spaces.
  latlng = latlng.replace(/[ ]+/g, ' ');  // Replace duplicate spaces.
  latlng_array = latlng.split(' ');
  if (latlng_array.length < 2) {
    return;
  }
  geePanTo(latlng_array[0], latlng_array[1]);
}

/**
 * Get the Search Result Icon URL for the index'th result. This get's a marker
 * with letters A-Z for index 0-25 respectively.
 * @param {number}
 *          index the rank in the list of a search result.
 * @return {string} the URL of the placemark's icon.
 */
function geeSearchMarkerIconUrl(index) {
  var letter = '';
  if (index < 26) {
    letter = String.fromCharCode('A'.charCodeAt(0) + index);
  }
  var iconUrl = geeMapsImage('marker' + letter + '.png');
  return iconUrl;
}

/**
 * Create a list item for the search results. Set the event behaviors and css
 * class for a search result item.
 * @param {Element}
 *          listElement the parent list element.
 * @param {Object}
 *          result the dictionary of the search result item (expects fields :
 *          name, snippet, lat, lon).
 * @param {Object}
 *          marker the marker for the result.
 * @param {Object}
 *          balloon the balloon object for the feature.
 * @param {string}
 *          iconUrl the URL of the placemark's icon.
 * @return {Element} the
 *         <li> list item.
 * Assumes: geeOpenBalloon is defined.
 */
 function geeCreateSearchListItem(listElement, result,
                                  marker, balloon, iconUrl) {
  var snippet = '';
  if (result.snippet) {
    snippet = result.snippet;
  }
  var item = createElement(listElement, 'li');
  var itemHtml = [];  // Build the string using array and join.
  var i = -1;
  itemHtml[++i] = '<img class="padded_floatleft" src = "';
  itemHtml[++i] = iconUrl;
  itemHtml[++i] = '" /> ';
  itemHtml[++i] = result.name;
  itemHtml[++i] = ' <div class = "search_result_snippet"> ';
  itemHtml[++i] = snippet;
  itemHtml[++i] = '</div>';
  item.innerHTML = itemHtml.join('');

  item.className = 'search_result_item';

  // Set to zoom in and open balloon on click.
  item.onclick = function(theResult, theMarker, theBalloon) {
    return function() {
      geePanTo(theResult.lat, theResult.lon, DEFAULT_SINGLE_CLICK_ZOOM_LEVEL);
      geeOpenBalloon(theMarker, theBalloon,
                     theResult.name, theResult.description);
    };
  }(result, marker, balloon);

  // Set to zoom in (closer) and open balloon on double click.
  item.ondblclick = function(theResult, theMarker, theBalloon) {
    return function() {
      geePanTo(theResult.lat, theResult.lon, DEFAULT_DOUBLE_CLICK_ZOOM_LEVEL);
      geeOpenBalloon(theMarker, theBalloon,
                     theResult.name, theResult.description);
    };
  }(result, marker, balloon);
  item.onmouseover = function() {
    item.className = 'search_result_item_selected';
  };
  item.onmouseout = function() {
    item.className = 'search_result_item';
  };
  return item;
}

/**
 * Callback for Search Results Manager after results arrive.
 * We simply resize to adjust the left panel divs for search results.
 */
function geeUpdateSearchResults() {
  // Resizing the map adjusts the left panel sizes.
  geeResizeDivs();
}

/**
 * Return the absolute url to the GEE earth image/icon.
 * @param {string}
 *           iconName  is the name of the icon relative to the earth image
 *                     directory.
 * @return {string}
 *           the URL of the icon.
 */
function geeEarthImage(iconName) {
  return GEE_EARTH_IMAGE_PATH + iconName;
}

/**
 * Return the absolute url to the GEE maps image/icon.
 * @param {string}
 *           iconName  is the name of the icon relative to the maps image
 *                     directory.
 * @return {string}
 *           the URL of the icon.
 */
function geeMapsImage(iconName) {
  return GEE_MAPS_IMAGE_PATH + iconName;
}

/**
 * Stop the event from propagating up to parent nodes.
 * Handles differences between IE and FF.
 * @param {Event}
 *             e  the event.
 */
function cancelEvent(e) {
  if (!e) var e = window.event;
  e.cancelBubble = true;
  if (e.stopPropagation) {
    e.stopPropagation();
  }
}

/**
 * Return true if the current browser type matches the specified type.
 * Typical types are: "Firefox" and "MSIE"
 * @param {string}
 *           type  is the requested browser type.
 * @return {boolean}
 *           true if the browser matches the type.
 */
function isBrowser(type) {
  return navigator.userAgent.match(type);
}

/**
 * Return the requested style of the specified HTML element.
 * @param {Element}
 *           elem  is the HTML element.
 * @param {string}
 *           style  is the requested style name.
 * @return {string}
 *           the value of the style.
 */
function getElementStyle(elem, style) {
  // Try currentStyle first, if it fails, fall through to getComputedStyle.
  if (elem.currentStyle) {
    var result = elem.currentStyle[style];
    if (result) {
      return result;
    }
  }
  var computedStyle = document.defaultView.getComputedStyle(elem, null);
  return computedStyle.getPropertyValue(style);
}

/**
 * Return the integer value of the requested style of the specified HTML
 * element.
 * @param {Element}
 *           elem  is the HTML element.
 * @param {string}
 *           style  is the requested style name.
 * @return {number}
 *           the integer value of the style.
 *           returns "undefined" if the style is not found.
 */
function getElementStyleInt(elem, style) {
  var styleResult = getElementStyle(elem, style);
  if (styleResult) {
    return parseInt(styleResult);
  }
  return undefined;
}

/**
 * Return the integer value of the width of the specified HTML element.
 * @param {Element}
 *           elem  is the HTML element.
 * @return {number}
 *           the integer value of the element's width.
 */
function getElementWidth(elem) {
  return parseInt(elem.offsetWidth);
}

/**
 * Return the integer value of the left of the specified HTML element.
 * @param {Element}
 *           elem  is the HTML element.
 * @return {number}
 *           the integer value of the element's left-most position.
 */
function getElementLeft(elem) {
  return parseInt(elem.offsetLeft);
}

/**
 * Return the integer value of the top of the specified HTML element.
 * @param {Element}
 *           elem  is the HTML element.
 * @return {number}
 *           the integer value of the element's top-most position.
 */
function getElementTop(elem) {
  return parseInt(elem.offsetTop);
}

/**
 * Return the integer value of the height of the specified HTML element.
 * @param {Element}
 *           elem  is the HTML element.
 * @return {number}
 *           the integer value of the element's height.
 */
function getElementHeight(elem) {
  return parseInt(elem.offsetHeight);
}

/**
 * Return the browser inner dimensions (width and height).
 * @return {Object} object with "height" and "width" fields with an integer
 *                  value for the number of pixels for each.
 */
function browserInnerDimensions() {
  var result = {};
  if (window.innerWidth) {
    result['width'] = window.innerWidth;
    result['height'] = window.innerHeight;
  } else {
    result['height'] = document.body.clientHeight;
    result['width'] = document.body.clientWidth;
  }
  return result;
}

/**
 * Return the sum of the vertical border widths for an HTML element.
 * @param {Element}
 *           elem  is the HTML element.
 * @return {number} object with height and width fields with an integer
 *                  value for the number of pixels for each.
 */
function getElementVerticalBorderWidths(elem) {
  if (isBrowser('MSIE')) {
    return getElementStyleInt(elem, 'borderTopWidth') +
           getElementStyleInt(elem, 'borderBottomWidth');
  } else {
    return getElementStyleInt(elem, 'border-top-width') +
           getElementStyleInt(elem, 'border-bottom-width');
  }
}

/**
 * Create an HTML element within an existing HTML element.
 * @param {Element}
 *           parent  is the parent HTML element.
 * @param {string}
 *           type  is the type of HTML element to create.
 * @param {string}
 *           opt_id  is the optional id of the created HTML element.
 * @return {Element}
 *           the HTML element.
 */
function createElement(parent, type, opt_id) {
  var elem = document.createElement(type);
  if (opt_id)
    elem.setAttribute('id', opt_id);
  parent.appendChild(elem);
  return elem;
}

/**
 * Create an HTML input element within an existing HTML element.
 * @param {Element}
 *           parent  is the parent HTML element.
 * @param {string}
 *           type  is the type of HTML element to create.
 * @param {string}
 *           opt_id  is the optional id of the created HTML element.
 * @return {HTMLInputElement}
 *           the HTML input element.
 */
function createInputElement(parent, type, opt_id) {
  var elem = document.createElement('input');
  elem.setAttribute('type', type);
  if (opt_id)
    elem.setAttribute('id', opt_id);
  parent.appendChild(elem);
  return elem;
}

/**
 * Create an HTML radio input element within an existing HTML element.
 * @param {Element}
 *           parent  is the parent HTML element.
 * @param {string}
 *           name  is the name of the radio input element.
 * @param {string}
 *           opt_id  is the optional id of the created HTML element.
 * @return {HTMLInputElement}
 *           the HTML radio input element.
 */
function createRadioInputElement(parent, name, opt_id) {
  var elem;
  try {
    elem = document.createElement('<input type="radio" name="' + name + '" />');
  } catch (err) {
    elem = document.createElement('input');
    elem.setAttribute('type', 'radio');
    elem.setAttribute('name', name);
  }
  if (opt_id)
    elem.setAttribute('id', opt_id);
  parent.appendChild(elem);
  return elem;
}

/**
 * Create and append a text node within an existing HTML element.
 * @param {Element}
 *           parent  is the parent HTML element.
 * @param {string}
 *           text  is text of the node to create.
 */
function createTextNode(parent, text) {
  parent.appendChild(document.createTextNode(text));
}

/**
 * Find the child HTML element of the specified element with the specified node
 * id.
 * @param {Element}
 *           parent  is the parent HTML element.
 * @param {string}
 *           id  is the id of the HTML element we are searching for.
 * @return {Element}
 *           the HTML element if found, otherwise null.
 */
function findChildById(parent, id) {
  for (var child = parent.firstChild; child != null;
       child = child.nextSibling) {
    if (child.id == id)
      return child;
  }
  return null;
}

/**
 * Find the HTML element with the specified node id.
 * @param {string}
 *           id  is the id of the HTML element we're searching for.
 * @return {Element}
 *           the HTML element if found, otherwise null.
 */
function findElement(id) {
  return (document.getElementById) ? document.getElementById(id) :
                                     document.all[id];
}

/**
 * Remove all child nodes of the specified node.
 * @param {Node}
 *           node  is the node from which we are removing child nodes.
 */
function removeAllChildren(node) {
  while (node.hasChildNodes()) {
    node.removeChild(node.firstChild);
  }
}

/**
 * Gets lat, lng and zoom from the current page's URL, if there
 * is a paramter of the form llz=<lat>,<lng>,<zoom>. If no zoom
 * is given, the zoom value is arbitrarily set to 5.
 *
 * @return {object} object with lat, lng, and zoom.
 */
function getLatLngZoomParameter() {
  var llz = getURLParameter(window.location.href, 'llz');
  if (!llz) {
    return undefined;
  }

  var latLngZoom = llz.split(',');
  latLngZoomObj = { };
  if (latLngZoom.length >= 2) {
    latLngZoomObj.lat = latLngZoom[0];
    latLngZoomObj.lng = latLngZoom[1];
    latLngZoomObj.zoom = 5;
  } else {
    return undefined;
  }
  if (latLngZoom.length >= 3) {
    latLngZoomObj.zoom = latLngZoom[2];
  }

  return latLngZoomObj;
}

/**
 * Gets the value of a specified parameter in the current page's URL.
 * @param {string}
 *           param  URL parameter to extract.
 * @return {string | boolean}
 *            Extracted parameter value if set, or true if the parameter is
 *            present but unset, otherwise false.
 *
 */
function getPageURLParameter(param) {
  return getURLParameter(window.location.href, param);
}

/**
 * Gets the value of a parameter from a URL.
 * @param {string}
 *           url  URL to extract from.
 * @param {string}
 *           param  URL parameter to extract.
 * @return {string | boolean}
 *            Extracted parameter value if set, or true if the parameter is
 *            present but unset, otherwise false.
 */
function getURLParameter(url, param) {
  // Look for a portion after the question mark:
  var halves = url.split('?');
  if (halves.length < 2) {
    return false;
  }
  // The second half contains the search parameters:
  var pairs = halves[1].split('&');
  for (var i = 0; i < pairs.length; i++) {
    var names = pairs[i].split('=');
    if (names[0] == param) {
      if (names.length > 1) {
        return names[1];
      } else {
        return true;  // A parameter with no = returns true
      }
    }
  }
  return false;
}

/**
 * Extract the first occurence of the given element from the xml text.
 * This assumes the long form of the element is used: i.e.,
 * <elementName>text to return</elementName>.
 * @param {string}
 *           xmlText  the XML string to extract from.
 * @param {string}
 *           elementName  the name of the element in the XML.
 * @return {string}
 *            Extracted text of the element.
 *            Returns '' if not found.
 */
function extractElementFromXml(xmlText, elementName) {
  var xmlString = new String(xmlText);
  var start = xmlString.indexOf('<' + elementName + '>');
  if (start < 0) {
    return '';
  }
  var end = xmlString.indexOf('</' + elementName + '>');
  if (end < 0) {
    return '';
  }
  start += elementName.length + 2;  // Skip past the element tag.
  var elementText = xmlString.substring(start, end);
  return elementText;
}

/**
 * Calculates the altitude for the Earth Plugin's camera in order to roughly
 * match the Maps API zoom levels (0..32).
 * This assumes a fixed GE Field of View (FOV).
 * Note: this mapping is not perfect because of perceptual distortion
 * of the sphere at low zoom levels (e.g., 0..3), but it is reasonably close
 * for higher zoom levels where the mapping becomes nearly linear.
 * @return {Object}
 *            A map of zoom levels (0..32) to Earth camera altitudes.
 */
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
  var altitudeAtLevel = (lengthOfTile / 2.0) / (Math.tan(GE_FOV_RADIANS / 2.0));
  var map = {};

  // We didn't really need to do it that way, but we're extrapolating back
  // to level 0, and will run down the levels by adding a factor of 2.
  // This may not be very "realistic" at levels 0-3, but that's ok for our
  // purposes.
  altitudeAtLevel *= twoPowerLevel;
  for (var level = 0; level <= 32; ++level) {
    map[level] = altitudeAtLevel;
    altitudeAtLevel /= 2.0;
  }
  return map;
}

/**
 * Resize the map and left panel to fill all available space below the header
 * and to right of the left panel.
 * @param {Object} divIds  a dictionary of the following divs that are needed
 *                 for resizing the left panel and map.
 *   header, map, leftPanel, searchTitle, searchResults, layersTitle, layers.
 */
function resizeMap(divIds) {
  var headerDiv = findElement(divIds.header);
  var mapDiv = findElement(divIds.map);
  var leftPanelDiv = findElement(divIds.leftPanel);
  var headerHeight = getElementHeight(headerDiv);
  var browserDimensions = browserInnerDimensions();
  var leftPanelBorderWidths = getElementVerticalBorderWidths(leftPanelDiv);

  var leftPanelWidth = geeLeftPanelWidth;  // Set on startup by css.
  if (!geeIsLeftPanelOpen) {
    leftPanelWidth = 0;
  }

  // Subtract 5 pixels from the newHeight to remove scroll bar.
  var newHeight = browserDimensions.height - headerHeight;
  var newWidth = browserDimensions.width - leftPanelWidth;
  if (mapDiv) {
    mapDiv.style.height = newHeight + 'px';
    mapDiv.style.width = newWidth + 'px';
    // headerHeight replaced with 0
    mapDiv.style.top = 0 + 'px';
  }

  var collapsePanelDiv = findElement(divIds.collapsePanel);
  if (collapsePanelDiv) {
    var top = headerHeight - 12;
    var left = leftPanelWidth - 1;
    collapsePanelDiv.style.top = top + 'px';
    collapsePanelDiv.style.left = left + 'px';
    collapsePanelDiv.setAttribute('frameborder', '1');
    // Make sure it's set...css isn't good enough for some browsers.
    collapsePanelDiv.style.zIndex = geeShimZIndex;
  }
  var leftPanelParent = findElement(divIds.leftPanelParent);
  if (leftPanelDiv) {
    if (geeIsLeftPanelOpen) {
      leftPanelParent.style.display = 'block';
    } else {
      leftPanelParent.style.display = 'none';
    }
    // Need to adjust left panel height by borders.
    newHeight = newHeight - leftPanelBorderWidths;

    // Need to adjust layerDiv height and the rest will fill in.
    var layersTitleDiv = document.getElementById(divIds.layersTitle);
    var layersTitleHeight = getElementHeight(layersTitleDiv);

    var searchTitleDiv = document.getElementById(divIds.searchTitle);
    var searchTitleHeight = getElementHeight(searchTitleDiv);

    var searchDiv = document.getElementById(divIds.searchResults);
    var searchHeight = getElementHeight(searchDiv);

    var globesTitleDiv = document.getElementById(divIds.globesTitle);
    if (globesTitleDiv) {
      var globesTitleHeight = getElementHeight(globesTitleDiv);
      var globesDiv = document.getElementById(divIds.globes);
      var globesHeight = getElementHeight(globesDiv);
    } else {
      var globesTitleHeight = 0;
      var globesHeight = 0;
    }

    var layerDiv = document.getElementById(divIds.layers);
    var layerHeight = newHeight - layersTitleHeight -
        searchTitleHeight - searchHeight -
        globesTitleHeight - globesHeight;
    if (layerHeight < 0) {
      layerHeight = 0;
    }
    layerDiv.style.height = layerHeight + 'px';
  }
}

/**
 * Toggle the expand/collapse state of the expand/collapse button.
 * @param {boolean}
 *          open whether the left panel is open or not.
 * @param {boolean}
 *          resize whether to force a resize or not.
 */
function geeToggleLeftPanelOpen(open, resize) {
  if (open != geeIsLeftPanelOpen) {
    var collapsePanelDiv = findElement(geeDivIds.collapsePanel);
    geeIsLeftPanelOpen = open;
    var button;
    if (open) {
      button =
        '<img id="' + geeDivIds.collapsePanel + '_button" src="' +
        geeIconUrls.collapse + '" title = "Collapse Panel" />';
    } else {
      button =
        '<img id="' + geeDivIds.collapsePanel + '_button" src="' +
        geeIconUrls.expand + '" title = "Expand Panel" />';
    }
    // Set to zoom in and open balloon on click.
    collapsePanelDiv.onclick = function(toggleValue) {
      return function() {
        geeToggleLeftPanelOpen(toggleValue, true);
      };
    }(!open);

    collapsePanelDiv.innerHTML = button;
    // Resize the map to collapse/expand the leftpanel.
    if (resize) {
      resizeMap(geeDivIds);
    }
  }
}

/**
 * Submit to a new page to change which globe is displayed.
 */
function geeChangeGlobe() {
  var full_globe_name = document.forms.change_globe.globe.value;
  var globe_name_size = full_globe_name.length;
  var suffix = full_globe_name.substr(globe_name_size - 3);

  if ((suffix == 'glb') || (suffix == 'glc')) {
    document.forms.change_globe.action = '/earth/earth_local.html';
  } else if (suffix == 'glm') {
    document.forms.change_globe.action = '/maps/map_v3.html';
  } else {
    alert('Unknown suffix: ' + suffix);
  }

  document.forms.change_globe.submit();
}

/**
 * Fill in the GlobesDiv with a pull-down menu of globes and maps.
 * @param {string}
 *          globesDivId Id of div where available globes are listed.
 */
function geeInitGlobesList(globesDivId) {
  var globesDiv = document.getElementById(globesDivId);
  if (!globesDiv) {
    return;
  }

  var formHtml = '<form id="change_globe" action="" method="GET">' +
                 '<select name="globe"' +
                 ' onChange="geeChangeGlobe()">';

  if (typeof(geeServerDefs.globes) == 'undefined') {
    formHtml += '<option value="">default</option>';
  } else {
    for (var i = 0; i < geeServerDefs.globes.length; i++) {
      var globe = geeServerDefs.globes[i];
      if (globe == geeServerDefs.selectedGlobe) {
        formHtml += '<option value="' + globe + '" SELECTED>' +
                    globe + '</option>';
      } else {
        formHtml += '<option value="' + globe + '">' + globe + '</option>';
      }
    }
  }

  formHtml += '</select></form>';
  globesDiv.innerHTML = formHtml;
}

/**
 * Get polygon bounds.
 * @param {array} coordinates Array of polygon lat/lng coordinates.
 * @return {object} bounds of polygon.
 */
function geePolygonBounds(coordinates) {
  var minLat = 90.0;
  var maxLat = -90.0;
  var minLng = 180.0;
  var maxLng = -180.0;
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

  return {
        south: minLat,
        north: maxLat,
        west: minLng,
        east: maxLng
      };
  }

/**
 * Get polygon coordinates from kml.
 * @param {string} polygonKml Kml containing the polygon of interest.
 * @param {function} latLngObject Constructor to create lat/lng objects.
 * @return {array} array of polygon lat/lng coordinates.
 */
function geePolygonCoordinates(polygonKml, latLngObject) {
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
        new latLngObject(parseFloat(latLng[1]), parseFloat(latLng[0]));
    }
  }

  return coordinates;
}

/**
 * Gets the server definitions of the served map/globe.
 * It is adapted from gees_utilities.js.
 * Note: function queries the server definitions using a synchronous
 * XMLHTTPRequest.
 * @param {string} url the Server URL (URL of the served database).
 * @param {boolean} is_2d whether it is a 2D database.
 * @return {object} the ServerDefs object.
 */
function getServerDefinition(url, is_2d) {
  // If a hash is found in the url, remove it & everything after it.
  url = url.split('#')[0];

  // Remove trailing slash from url if it exists.
  url = url[url.length - 1] == '/' ? url.substr(0, url.length - 1) : url;
  var request = url + '/query?request=Json&var=geeServerDefs';
  if (is_2d) {
    request += '&is2d=t';
  }
  var httpRequest = new XMLHttpRequest();
  httpRequest.open('GET', request, false);
  httpRequest.send();
  var response = httpRequest.responseText;

  // Response is object literal.  Strip unwanted characters and
  // use eval to convert to JSON string.
  response = response.trim();
  response = response.replace('var geeServerDefs = ', '');
  response = response.replace(/;$/, '');
  response = JSON.stringify(eval('(' + response + ')'));
  var serverDefs = jQuery.parseJSON(response);

  if (Object.keys(serverDefs).length == 0) {
    return;
  }

  // serverUrl comes back as /targetPath for glm files.  Always
  // update it to full url (same url we passed to function).
  serverDefs.serverUrl = url;

  // Url format is now host.name/targetPath.  Create target path
  // by removing host name.
  baseUrl = window.location.protocol + '//' + window.location.host;
  serverDefs.serverTargetPath = url.replace(baseUrl, '');

  return serverDefs;
}

// TODO: consider to deprecate is_2d-parameter.
/**
 * Gets the server definitions of the served map/globe.
 * Note: function issues an asynchronous XMLHttpRequest to server.
 * @param {string} url the Server URL (URL of the served database).
 * @param {boolean} is_2d whether it is a 2D database.
 * @param {function} callbackFunc the callback function to call on finishing
 *                   the request.
 */
function getServerDefinitionAsync(url, is_2d, callbackFunc) {
  // If a hash is found in the url, remove it & everything after it.
  var serverUrl = url.split('#')[0];

  // Remove trailing slash from url if it exists.
  var serverUrl = serverUrl[serverUrl.length - 1] == '/' ?
      serverUrl.substr(0, serverUrl.length - 1) : serverUrl;
  var queryUrl = serverUrl + '/query?request=Json&var=geeServerDefs';
  if (is_2d) {
    queryUrl += '&is2d=t';
  }

  jQuery.get(queryUrl, makeOnGetServerDefsFunc(serverUrl, callbackFunc));
}

/**
 * Builds function to handle response of ServerDefs query.
 * @param {string} serverUrl the Server URL (URL of served database).
 * @param {function} callbackFunc the callback function to call in
 *                   the onGetServerDefs-handler.
 * @return {function} the onGetServerDefs handler.
 */
function makeOnGetServerDefsFunc(serverUrl, callbackFunc) {
  function func(response, status) {
    geeServerDefs = undefined;

    if (status == 'success' && response) {
      // Response is object literal.  Strip unwanted characters and
      // use eval to convert to JSON string.
      response = response.trim();
      response = response.replace('var geeServerDefs = ', '');
      response = response.replace(/;$/, '');
      response = JSON.stringify(eval('(' + response + ')'));
      geeServerDefs = jQuery.parseJSON(response);
      updateServerDefs(geeServerDefs, serverUrl);
    }

    callbackFunc();
  }
  return func;
}

/**
 * Updates the Server Definitions.
 *
 * Updates the serverUrl and the targetPath in serverDefs.
 * @param {object} serverDefs the Server Deinitions to update.
 * @param {string} serverUrl the Server URL (URL of served database).
 */
function updateServerDefs(serverDefs, serverUrl) {
  // serverUrl comes back as /targetPath for glm files.  Always
  // update it to full url (same url we passed to function).
  serverDefs.serverUrl = serverUrl;

  // Url format is now host.name/targetPath.  Create target path
  // by removing host name.
  baseUrl = window.location.protocol + '//' + window.location.host;
  serverDefs.serverTargetPath = serverUrl.replace(baseUrl, '');
}
