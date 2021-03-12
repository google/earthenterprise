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

/**
 * Description: Creates and extends a Google Maps API
 *              map object for use in Google Earth Enterprise.
 */

var MAX_ZOOM_LEVEL = 23;
var TILE_WIDTH = 256;
var TILE_HEIGHT = 256;
var DEFAULT_LATITUDE = 0.0;
var DEFAULT_LONGITUDE = 0.0;
var map = null;

// Options passed to the v3 map object.
var GEE_MAP_OPTS = {
    zoom: 3,
    center: new google.maps.LatLng(DEFAULT_LATITUDE, DEFAULT_LONGITUDE),
    streetViewControl: false,
    navigationControl: true,
    mapTypeControl: false,
    scaleControl: true,
    panControl: false,
    zoomControl: false
};

/**
 * Creates an extended Maps Api map object to be served with the given server
 * definitions (from JSON) in a div named divName.
 * @param {string} divName Name of div in which map will be placed.
 * @param {object} serverDefs Struct of map layers, base url, etc.
 * @param {object} mapOpts (optional)
 *    Parameters for setting up the map. If this  parameter is not passed
 *    in, default parameters are used.
 * @return {object} extended Maps API map object or undefined object if error.
 */
function geeCreateFusionMap(divName, serverDefs, mapOpts) {
  var map;  // Undefined.
  if (typeof(mapOpts) == 'undefined') {
    mapOpts = GEE_MAP_OPTS;
  }

  // The map div must exist!
  var mapDiv = document.getElementById(divName);
  if (!mapDiv) {
    alert('Cannot initialize map!  Missing: ' + divName);
    return map;
  }

  // Create a map based on the imagery layer's channel and version.
  var projection = serverDefs.projection;
  var serverUrl = serverDefs.serverUrl;

  // If we are using Google for the base map, get map from the api.
  if (serverDefs.useGoogleLayers) {
    if (projection == 'flat') {
      alert('Expected Mercator projection.');
    }
    map = new google.maps.Map(mapDiv, mapOpts);

  // If not, create the map with the imagery layer in the requested projection.
  } else {
    if (projection == 'flat') {
      map = geeCreateFlatMap(mapDiv, mapOpts);
    } else {  // projection is 'mercator'.
      map = geeCreateMercatorMap(mapDiv, mapOpts);
    }
  }

  // Extend the map object to handle Fusion functionality.
  geeExtendMapObject(map);

  map.initializeLayers(serverDefs);
  return map;
}

/**
 * Creates a map with a flat (Plate-Carree) projection.
 * @param {object} mapDiv The div that the map will be placed in.
 * @param {object} mapOpts
 *    Parameters for setting up the map. If this  parameter is not passed
 *    in, default parameters are used.
 * @return {google.maps.ImageMapType} flat map.
 */
geeCreateFlatMap = function(
    mapDiv, mapOpts) {
  var flatMapType = new google.maps.ImageMapType({
    getTileUrl: geeMapEmptyTileFunc(),
    tileSize: new google.maps.Size(TILE_WIDTH, TILE_HEIGHT),
    isPng: true,
    minZoom: 0,
    maxZoom: MAX_ZOOM_LEVEL,
    name: 'flat'
  });

  flatMapType.projection = new geeFlatProjection();
  var flatMap = new google.maps.Map(mapDiv, mapOpts);

  flatMap.mapTypes.set('flatMap', flatMapType);
  flatMap.setMapTypeId('flatMap');
  flatMap.overlayMapTypes.insertAt(0, flatMapType);
  return flatMap;
};

/**
 * Creates a map with a Mercator projection. This is the default projection
 * for the Maps API so we don't have to define the projection.
 * @param {object} mapDiv The div that the map will be placed in.
 * @param {object} mapOpts
 *    Parameters for setting up the map. If this  parameter is not passed
 *    in, default parameters are used.
 * @return {google.maps.ImageMapType} Mercator map.
 */
geeCreateMercatorMap = function(mapDiv, mapOpts) {
  var mercatorMapType = new google.maps.ImageMapType({
    // The underlying map is created with empty tiles, then added as an overlay
    // layer same as all the other layers. This allows the visibility toggle to
    // work on the base layer.
    getTileUrl: geeMapEmptyTileFunc(),
    tileSize: new google.maps.Size(TILE_WIDTH, TILE_HEIGHT),
    isPng: true,
    minZoom: 0,
    maxZoom: MAX_ZOOM_LEVEL,
    name: 'mercator'
  });

  var mercatorMap = new google.maps.Map(mapDiv, mapOpts);

  mercatorMap.mapTypes.set('mercatorMap', mercatorMapType);
  mercatorMap.setMapTypeId('mercatorMap');
  mercatorMap.overlayMapTypes.insertAt(0, mercatorMapType);
  return mercatorMap;
};

/**
 * Returns a function that can be used to grab vector tiles for the map.
 * @param {string} server The address of the server providing vector tiles.
 * @param {string} glmId Id of the glm.
 * @param {string} channel The channel that vectors are served on.
 * @param {string} version The version of vectors being served.
 * @return {function} function for grabbing vector tiles.
 */
function geeMapVectorFunc(server, glmId, channel, version) {
  return geeMapTileFunc(
      'VectorMapsRaster', server, glmId, channel, version);
}

/**
 * Returns a function that can be used to grab imagery tiles for the map.
 * @param {string} requestType The type of request that imagery is served on.
 * @param {string} server The address of the server providing imagery tiles.
 * @param {string} glmId Id of the glm.
 * @param {string} channel The channel that imagery is served on.
 * @param {string} version The version of imagery being served.
 * @return {function} function for grabbing imagery tiles.
 */
function geeMapImageryFunc(
    requestType, server, glmId, channel, version) {
  return geeMapTileFunc(
      requestType, server, glmId, channel, version);
}

/**
 * Returns a function that will NOT grab any real tiles for the map, but
 * instead fetches a placeholder background tile.
 * @return {function} function for grabbing imagery tiles.
 */
function geeMapEmptyTileFunc() {
  return function(coord, zoom) {

    // If the map isn't hosted on a GE server, use base Url
    // to point to the GE server.
    var baseURL = GEE_BASE_URL || "";

    // This path should work on both Enterprise and Portable servers.
    // To use a plain medium gray tile instead, simply return null.
    return baseURL + "/shared_assets/images/empty4.png";
  };
}

/**
 * Returns a function that can be used to grab tiles for the map.
 * @param {string} request The type of tile being requested.
 * @param {string} server The address of the server providing tiles.
 * @param {string} glmId Id of the glm.
 * @param {string} channel The channel that tiles are served on.
 * @param {string} version The version of tiles being served.
 * @return {function} function for grabbing tiles.
 */
function geeMapTileFunc(request, server,
                        glmId, channel, version) {
  return function(coord, zoom) {
    var numTiles = 1 << zoom;

    // Don't wrap tiles vertically.
    if (coord.y < 0 || coord.y >= numTiles) {
      return null;
    }

    // Wrap tiles horizontally.
    var x = ((coord.x % numTiles) + numTiles) % numTiles;

    var glmPath = '';
    if (glmId) {
      glmPath = '/' + glmId;
    }

    // For simplicity, we use a tileset consisting of 1 tile at zoom level 0
    // and 4 tiles at zoom level 1.
    var url = server + glmPath + '/query?request=' + request;

    url += '&channel=' + channel + '&version=' + version + '&x=' + x +
        '&y=' + coord.y + '&z=' + zoom;
    return url;
  };
}


/**
 * @constructor Class for a flat (Platte-Carree) projection. In Maps api v3, we
 * only have to convert coordinates for zoom level 0 and it infers the rest.
 */
function geeFlatProjection() {
  this.projection = 'flat';

  // Only need zoom level 0, i.e. for a single tile
  var pixels = 256.0;
  this.pixelsPerDegree = pixels / 360.0;
  this.pixelOrigin = new google.maps.Point(pixels / 2.0, pixels / 2.0);
}

/**
 * Converts lat/lng coordinates to pixel coordinates at zoom level 0.
 * Note that it is important that both are pairs of floating point
 * numbers (i.e. fractional pixel values are required).
 * @param {google.maps.LanLng} latLng Latitude and longitude to be converted.
 * @return {google.maps.Point} pixel coordinates.
 */
geeFlatProjection.prototype.fromLatLngToPoint = function(latLng) {
  var x = this.pixelOrigin.x +
                     latLng.lng() * this.pixelsPerDegree;
  var y = this.pixelOrigin.y -
                     latLng.lat() * this.pixelsPerDegree;
  return new google.maps.Point(x, y);
};

/**
 * Converts pixel coordinates to lat/lng coordinates at zoom level 0.
 * Note that it is important that both are pairs of floating point
 * numbers (i.e. fractional pixel values are required).
 * @param {google.maps.Point} point Pixel coordinates to be converted.
 * @return {google.maps.LatLng} lattitude and longitude.
 */
geeFlatProjection.prototype.fromPointToLatLng = function(point) {
  var y = point.y;
  var x = point.x;

  var lng = (x - this.pixelOrigin.x) / this.pixelsPerDegree;
  var lat = (this.pixelOrigin.y - y) / this.pixelsPerDegree;
  return new google.maps.LatLng(lat, lng);
};


/**
 * @constructor Class for storing vector layer with its index. The index gives
 * position in array of all vector layers in the map. Allows the layer to be
 * toggled on and off.
 * @param {Number} index Index into array of vector layers.
 * @param {object} overlay Map overlay for the vector layer.
 */
function geeFusionLayer(index, overlay) {
  this.index = index;
  this.overlay = overlay;
}

/**
* Determines whether layer is an imagery layer based on requestType property
* of layer definition.
* @param {layer} layer definition.
* @return {boolean} whether layer is an imagery layer.
*/
function geeIsImageryLayer(layer) {
  return (layer.requestType.search('Imagery') != -1);
}

/**
 * Extends the functionality to support Fusion 2d maps.
 * @param {object} map Maps Api map to be extended.
 */
function geeExtendMapObject(map) {
  // Initialize the layers data.
  map.layerMap = {};
  map.layerVisible = {};
  map.layerName = {};
  map.options = {};
  map.layerIds = [];

  /**
   * Initialize all layers defined in the server defs.
   * @param {object} serverDefs Struct of map layers, base url, etc.
   */
  map.initializeLayers = function(serverDefs) {
    var layerDefs = serverDefs.layers;
    var serverUrl = serverDefs.serverUrl;

    if (layerDefs == undefined || layerDefs.length == 0) {
      alert('Error: No Layers are defined for this URL.');
      return;
    }

    // Create tile layers.
    // The base map was created with an empty tile function so that we can
    // also add the "base" layer here instead of treating it as an exception.
    // By treating it uniformly, its visibility can be controlled.
    var numLayers = 0;
    for (var i = 0; i < layerDefs.length; ++i) {
      this.overlayMapTypes.push(null);
      var name = layerDefs[i].label;
      var channel = layerDefs[i].id;
      // Use layer glm_id if it is defined. Otherwise, set it to 0.
      var glmId = layerDefs[i].glm_id;
      if (typeof(glmId) == 'undefined') {
        glmId = 0;
      }
      var requestType = layerDefs[i].requestType;
      var version = layerDefs[i].version;
      var enabled = layerDefs[i].initialState;
      var isPng = layerDefs[i].isPng;
      if (layerDefs[i].requestType == 'VectorMapsRaster') {
        this.addLayer(name, numLayers, map, isPng,
                      geeMapVectorFunc(serverUrl, glmId, channel, version),
                      glmId, channel, enabled);
      } else if (geeIsImageryLayer(layerDefs[i])) {
        this.addLayer(name, numLayers, map, isPng,
                      geeMapImageryFunc(
                          requestType, serverUrl, glmId, channel, version),
                      glmId, channel, enabled);
      }
      this.layerIds[numLayers++] = glmId + '-' + channel;
    }
  };

  /**
   * Adds a layer as an overlay to the map.
   * @param {string} name Name of the layer.
   * @param {Number} index Index of vector layer in array of overlays.
   * @param {google.maps.ImageMapType} map The map we are adding the layer to.
   * @param {boolean} isPng Whether vector tiles are png files.
   * @param {string} urlFunction Function to get vector tiles urls.
   * @param {Number} glmId Id for this glm (set of channels).
   * @param {Number} channel Channel for this layer.
   * @param {boolean} enabled Whether vector layer is visible initially.
   */
  map.addLayer = function(
      name, index, map, isPng, urlFunction, glmId, channel, enabled) {
    var options = {
         'getTileUrl': urlFunction,
         'tileWidth': TILE_WIDTH,
         'tileHeight': TILE_HEIGHT,
         'isPng': isPng
         };

    options.maxZoom = MAX_ZOOM_LEVEL;
    options.tileSize = new google.maps.Size(
        options.tileWidth, options.tileHeight);

    var overlay = new google.maps.ImageMapType(options);
    if (enabled) {
      map.overlayMapTypes.setAt(index, overlay);
    }
    var id = glmId + '-' + channel;
    this.layerMap[id] = new geeFusionLayer(index, overlay);
    this.layerVisible[id] = enabled;
    this.layerName[id] = name;
    this.options[id] = options;
  };

  /**
   * Set opacity of given layer.
   * @param {number} id Index of overlay whose opacity is to be set.
   * @param {number} opacity Opacity ranging from 0 (clear) to 1 (opaque).
   */
  map.setOpacity = function(id, opacity) {
    var layer = this.layerMap[id];
    if (layer) {
      layer.overlay.setOpacity(opacity);
    }
  };

  /**
   * Make a vector layer visible on the map.
   * @param {string} id Layer id for the vector layer to show.
   */
  map.showFusionLayer = function(id) {
    if (typeof(this.layerMap[id]) != 'undefined') {
      this.overlayMapTypes.setAt(this.layerMap[id].index,
                                 this.layerMap[id].overlay);
      this.layerVisible[id] = true;
    } else {
      alert('Unknown layer: ' + id);
    }
  };

  /**
   * Hide a vector layer on the map.
   * @param {string} id Layer id for the vector layer to hide.
   */
  map.hideFusionLayer = function(id) {
    if (typeof(this.layerMap[id]) != 'undefined') {
      this.overlayMapTypes.setAt(this.layerMap[id].index, null);
      this.layerVisible[id] = false;
    } else {
      alert('Unknown layer: ' + id);
    }
  };

  /**
   * @param {string} id Layer id for the layer to check.
   * @return {bool} whether layer is visible.
   */
  map.isFusionLayerVisible = function(id) {
    return this.layerVisible[id];
  };

  /**
   * @param {string} id Layer id for the layer to get name from.
   * @return {string} name of layer.
   */
  map.fusionLayerName = function(id) {
    return this.layerName[id];
  };

  /**
   * @param {number} index Index of the layer.
   * @return {number} id of indexed layer.
   */
  map.fusionLayerId = function(index) {
    return this.layerIds[index];
  };

  /**
   * @return {number} number of layers.
   */
  map.layerCount = function() {
    return this.layerIds.length;
  };

  /**
   * Pan and Zoom the Earth viewer to the specified lat, lng and zoom level.
   * @param {string}
   *          lat the latitude of the position to pan to.
   * @param {string}
   *          lng the longitude of the position to pan to.
   * @param {Number}
   *          zoomLevel [optional] the zoom level (an integer between 1 :
   *          zoomed out all the way, and 32: zoomed in all the way) indicating
   *          the zoom level for the view.
   */
  map.panAndZoom = function(lat, lng, zoomLevel) {
    if (zoomLevel == null) {
      zoomLevel = DEFAULT_SINGLE_CLICK_ZOOM_LEVEL;
    }

    var latLng = new google.maps.LatLng(parseFloat(lat), parseFloat(lng));
    this.panTo(latLng);
    this.setZoom(zoomLevel);
  };

  /**
   * Open the info window at the given location with the given content.
   * @param {google.maps.LatLng} position Position at which to draw the window.
   * @param {string} content The content to put into the info window.
   */
  map.openInfoWindow = function(position, content) {
    if ((typeof(this.infoWindow) == 'undefined') ||
        (this.infoWindow == null)) {
      this.infoWindow = new google.maps.InfoWindow({
        content: content,
        position: position
      });
    } else {
      this.infoWindow.setPosition(position);
      this.infoWindow.setContent(content);
    }

    this.infoWindow.open(this);
  };

  /**
   * Close the info window if it is open.
   */
  map.closeInfoWindow = function() {
    if ((typeof(this.infoWindow) == 'undefined') ||
        (this.infoWindow == null)) {
      return;
    }

    this.infoWindow.close();
    this.infoWindow = null;
  };
}
