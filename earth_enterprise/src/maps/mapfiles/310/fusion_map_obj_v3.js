/* Copyright 2010 Google.
 * All rights reserved.
 *
 * Description: Wrapper objects for using the Google
                Maps API with Google Earth Enterprise
 * Version: 1.01
 */

var MAX_ZOOM_LEVEL = 23;
var TILE_WIDTH = 256;
var TILE_HEIGHT = 256;
var map = null;

// Options passed to the v3 map object.
var GEE_MAP_OPTS = {
    zoom: 4,
    center: new google.maps.LatLng(37, -122.1),
    streetViewControl: false,
    navigationControl: true,
    mapTypeControl: false,
    scaleControl: true
};

/**
 * @constructor Class for storing vector layer with its index. The index gives
 * position in array of all vector layers in the map. Allows the layer to be
 * toggled on and off.
 * @param {Number} index Index into array of vector layers.
 * @param {object} overlay Map overlay for the vector layer.
 */
function GFusionLayer(index, overlay) {
  this.index = index;
  this.overlay = overlay;
}

/**
 * @constructor Class for wrapping a map to be served with the given server
 * definitions (from JSON) in a div named divName.
 * @param {string} divName Name of div in which map will be placed.
 * @param {object} serverDefs Struct of map layers, base url, etc.
 * @param {object} mapOpts (optional)
 *    Parameters for setting up the map. If this  parameter is not passed
 *    in, default parameters are used.
 */
function GFusionMap(divName, serverDefs, mapOpts) {
  if (typeof(mapOpts) == 'undefined') {
    mapOpts = GEE_MAP_OPTS;
  }

  // The map div must exist!
  var mapDiv = document.getElementById(divName);
  if (!mapDiv) {
    alert('Cannot initialize map!  Missing: ' + divName);
    return;
  }

  // Create a map based on the imagery layer's channel and version.
  var projection = serverDefs.projection;
  var serverUrl = serverDefs.serverUrl;

  // If we are using Google for the base map, get map from the api.
  if (serverDefs.useGoogleLayers) {
    if (projection == 'flat') {
      alert('Expected Mercator projection.');
    }
    this.map = new google.maps.Map(mapDiv, mapOpts);

  // If not, create the map with the imagery layer in the requested projection.
  } else {
    // Find the imagery layer.
    var layerDefs = serverDefs.layers;
    var imagery_idx;
    for (imagery_idx = 0; imagery_idx < layerDefs.length; ++imagery_idx) {
      if (layerDefs[imagery_idx].requestType == 'ImageryMaps') {
        break;
      }
    }

   if (imagery_idx == layerDefs.length) {
     alert('Unable to find an imagery layer.');
     return;
    }
    // Use layer glm_id if it is defined. Otherwise, set it to 0.
    var glmId = layerDefs[imagery_idx].glm_id;
    if (typeof(glmId) == 'undefined') {
      glmId = 0;
    }
    channel = layerDefs[imagery_idx].id;
    version = layerDefs[imagery_idx].version;
    if (projection == 'flat') {
      this.map = this.createFlatMap(mapDiv,
                                    serverUrl,
                                    glmId,
                                    channel,
                                    version,
                                    mapOpts);
    } else {
      this.map = this.createMercatorMap(mapDiv,
                                        serverUrl,
                                        glmId,
                                        channel,
                                        version,
                                        mapOpts);
    }
  }

  // Global map needs to be set (argh!).
  map = this.map;

  // Initialize the non-base layers layers.
  this.nonBaseLayerMap = {};
  this.nonBaseLayerVisible = {};
  this.nonBaseLayerName = {};
  this.layerIds = [];
  this.initializeLayers(imagery_idx, serverDefs);
}

/**
 * Initialize all of the vector layers defined in the server defs.
 * @param {number} skipLayer Layer to treat as base layer.
 * @param {object} serverDefs Struct of map layers, base url, etc.
 */
GFusionMap.prototype.initializeLayers = function(skipLayer, serverDefs) {
  var layerDefs = serverDefs.layers;
  var serverUrl = serverDefs.serverUrl;

  if (layerDefs == undefined || layerDefs.length == 0) {
    alert('Error: No Layers are defined for this URL.');
    return;
  }

  // Create tile layers for the remaining vector layers
  var numLayers = 0;
  for (var i = 0; i < layerDefs.length; ++i) {
    this.map.overlayMapTypes.push(null);
    if (i != skipLayer) {
      var name = layerDefs[i].label;
      var channel = layerDefs[i].id;
      // Use layer glm_id if it is defined. Otherwise, set it to 0.
      var glmId = layerDefs[i].glm_id;
      if (typeof(glmId) == 'undefined') {
        glmId = 0;
      }
      var version = layerDefs[i].version;
      var enabled = layerDefs[i].initialState
      if (layerDefs[i].requestType == 'VectorMapsRaster') {
        this.addLayer(name, numLayers, map, true,
                      this.geeMapVectorFunc(serverUrl, glmId, channel, version),
                      glmId, channel, enabled);
      } else if (layerDefs[i].requestType == 'ImageryMaps') {
        this.addLayer(name, numLayers, map, true,
                      this.geeMapImageryFunc(
                          serverUrl, glmId, channel, version),
                      glmId, channel, enabled);
      }
      this.layerIds[numLayers++] = glmId + '-' + channel;
    }
  }
};

/**
 * Add a vector layer as an overlay to the map.
 * @param {string} name Name of the layer.
 * @param {Number} index Index of vector layer in array of overlays.
 * @param {google.maps.ImageMapType} map The map we are adding the layer to.
 * @param {boolean} isPng Whether vector tiles are png files.
 * @param {string} urlFunction Function to get vector tiles urls.
 * @param {Number} glmId Id for this glm (set of channels).
 * @param {Number} channel Channel for this layer.
 * @param {boolean} enabled Whether vector layer is visible initially.
 */
GFusionMap.prototype.addLayer = function(
    name, index, map, isPng, urlFunction, glmId, channel, enabled) {
  var options = {
       'getTileUrl': urlFunction,
       'tileWidth': TILE_WIDTH,
       'tileHeight': TILE_HEIGHT,
       'isPng': isPng
       }

  options.maxZoom = MAX_ZOOM_LEVEL;
  options.tileSize = new google.maps.Size(
      options.tileWidth, options.tileHeight);

  var overlay = new google.maps.ImageMapType(options);
  if (enabled) {
    map.overlayMapTypes.setAt(index, overlay);
  }
  var id = glmId + '-' + channel;
  this.nonBaseLayerMap[id] = new GFusionLayer(index, overlay);
  this.nonBaseLayerVisible[id] = enabled;
  this.nonBaseLayerName[id] = name;
};


/**
 * Make a vector layer visible on the map.
 * @param {string} id Layer id for the vector layer to show.
 */
GFusionMap.prototype.showFusionLayer = function(id) {
  if (typeof(this.nonBaseLayerMap[id]) != 'undefined') {
    this.map.overlayMapTypes.setAt(this.nonBaseLayerMap[id].index,
                                   this.nonBaseLayerMap[id].overlay);
    this.nonBaseLayerVisible[id] = true;
  } else {
    alert('Unknown layer: ' + id);
  }
};

/**
 * Hide a vector layer on the map.
 * @param {string} id Layer id for the vector layer to hide.
 */
GFusionMap.prototype.hideFusionLayer = function(id) {
  if (typeof(this.nonBaseLayerMap[id]) != 'undefined') {
    this.map.overlayMapTypes.setAt(this.nonBaseLayerMap[id].index, null);
    this.nonBaseLayerVisible[id] = false;
  } else {
    alert('Unknown layer: ' + id);
  }
};

/**
 * @param {string} id Layer id for the layer to check.
 * @return {bool} whether layer is visible.
 */
GFusionMap.prototype.isFusionLayerVisible = function(id) {
  return this.nonBaseLayerVisible[id];
};

/**
 * @param {string} id Layer id for the layer to get name from.
 * @return {string} name of layer.
 */
GFusionMap.prototype.fusionLayerName = function(id) {
  return this.nonBaseLayerName[id];
};

/**
 * @param {number} index Index of the layer.
 * @return {number} id of indexed layer.
 */
GFusionMap.prototype.fusionLayerId = function(index) {
  return this.layerIds[index];
};

/**
 * @return {number} number of layers.
 */
GFusionMap.prototype.layerCount = function() {
  return this.layerIds.length;
};

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
GFusionMap.prototype.panTo = function(lat, lng, zoomLevel) {
  if (zoomLevel == null) {
    zoomLevel = DEFAULT_SINGLE_CLICK_ZOOM_LEVEL;
  }

  var latLng = new google.maps.LatLng(parseFloat(lat), parseFloat(lng));
  this.panToLatLng(latLng, zoomLevel);
};

/**
 * Pan and Zoom the Earth viewer to the specified lat, lng and zoom level.
 * @param {google.maps.LanLng}
 *          latLng Latitude and longitude to pan to.
 * @param {Number}
 *          zoomLevel [optional] the zoom level (an integer between 1 : zoomed
 *          out all the way, and 32: zoomed in all the way) indicating the zoom
 *          level for the view.
 */
GFusionMap.prototype.panToLatLng = function(latLng, zoomLevel) {
  if (zoomLevel == null) {
    zoomLevel = DEFAULT_SINGLE_CLICK_ZOOM_LEVEL;
  }

  this.map.panTo(latLng);
  this.map.setZoom(zoomLevel);
};

/**
 * Open the info window at the given location with the given content.
 * @param {google.maps.LatLng} position Position at which to draw the window.
 * @param {string} content The content to put into the info window.
 */
GFusionMap.prototype.openInfoWindow = function(position, content) {
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

  this.infoWindow.open(this.map);
};

/**
 * Close the info window if it is open.
 */
GFusionMap.prototype.closeInfoWindow = function() {
  if ((typeof(this.infoWindow) == 'undefined') ||
      (this.infoWindow == null)) {
    return;
  }

  this.infoWindow.close();
  this.infoWindow = null;
};


/**
 * Return a function that can be used to grab imagery tiles for the map.
 * @param {string} server The address of the server providing vector tiles.
 * @param {string} glmId Id of the glm.
 * @param {string} channel The channel that vectors are served on.
 * @param {string} version The version of vectors being served.
 * @return {function} function for grabbing vector tiles.
 */
GFusionMap.prototype.geeMapVectorFunc = function(
    server, glmId, channel, version) {
  return this.geeMapTileFunc(
      'VectorMapsRaster', server, glmId, channel, version);
};

/**
 * Return a function that can be used to grab imagery tiles for the map.
 * @param {string} server The address of the server providing imagery tiles.
 * @param {string} glmId Id of the glm.
 * @param {string} channel The channel that imagery is served on.
 * @param {string} version The version of imagery being served.
 * @return {function} function for grabbing imagery tiles.
 */
GFusionMap.prototype.geeMapImageryFunc = function(
    server, glmId, channel, version) {
  return this.geeMapTileFunc(
      'ImageryMaps', server, glmId, channel, version);
};

/**
 * Return a function that can be used to grab imagery tiles for the map.
 * @param {string} request The type of tile being requested.
 * @param {string} server The address of the server providing tiles.
 * @param {string} glmId Id of the glm.
 * @param {string} channel The channel that tiles are served on.
 * @param {string} version The version of tiles being served.
 * @return {function} function for grabbing tiles.
 */
GFusionMap.prototype.geeMapTileFunc = function(
    request, server, glmId, channel, version) {
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
      var url = server + glmPath + '/query?request=' + request +
                '&channel=' + channel + '&version=' + version + '&x=' + x +
                '&y=' + coord.y + '&z=' + zoom;
      return url;
    };
};

/**
 * Create a map with a flat (Platte-Carre) projection.
 * @param {object} mapDiv The div that the map will be placed in.
 * @param {string} server The address of the server providing imagery tiles.
 * @param {string} glmId Id of the glm.
 * @param {string} channel The channel that imagery is served on.
 * @param {string} version The version of imagery being served.
 * @param {object} mapOpts
 *    Parameters for setting up the map. If this  parameter is not passed
 *    in, default parameters are used.
 * @return {google.maps.ImageMapType} flat map.
 */
GFusionMap.prototype.createFlatMap = function(
    mapDiv, server, glmId, channel, version, mapOpts) {
  var flatMapType = new google.maps.ImageMapType({
    getTileUrl: this.geeMapImageryFunc(server, glmId, channel, version),
    tileSize: new google.maps.Size(256, 256),
    isPng: false,
    minZoom: 0,
    maxZoom: 23,
    name: 'flat'
  });

  flatMapType.projection = new FlatProjection();
  var flatMap = new google.maps.Map(mapDiv, mapOpts);

  flatMap.mapTypes.set('flatMap', flatMapType);
  flatMap.setMapTypeId('flatMap');
  flatMap.overlayMapTypes.insertAt(0, flatMapType);
  return flatMap;
};

/**
 * Create a map with a Mercator projection. This is the default project
 * for the Maps api so we don't have to define the projection.
 * @param {object} mapDiv The div that the map will be placed in.
 * @param {string} server The address of the server providing imagery tiles.
 * @param {string} glmId Id of the glm.
 * @param {string} channel The channel that imagery is served on.
 * @param {string} version The version of imagery being served.
 * @param {object} mapOpts
 *    Parameters for setting up the map. If this  parameter is not passed
 *    in, default parameters are used.
 * @return {google.maps.ImageMapType} Mercator map.
 */
GFusionMap.prototype.createMercatorMap = function(
    mapDiv, server, glmId, channel, version, mapOpts) {
  var mercatorMapType = new google.maps.ImageMapType({
    getTileUrl: this.geeMapImageryFunc(server, glmId, channel, version),
    tileSize: new google.maps.Size(256, 256),
    isPng: false,
    minZoom: 0,
    maxZoom: 23,
    name: 'mercator'
  });

  var mercatorMap = new google.maps.Map(mapDiv, mapOpts);

  mercatorMap.mapTypes.set('mercatorMap', mercatorMapType);
  mercatorMap.setMapTypeId('mercatorMap');
  mercatorMap.overlayMapTypes.insertAt(0, mercatorMapType);
  return mercatorMap;
};

/**
 * @constructor Class for a flat (Platte-Carree) projection. In Maps api v3, we
 * only have to convert coordinates for zoom level 0 and it infers the rest.
 */
function FlatProjection() {
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
FlatProjection.prototype.fromLatLngToPoint = function(latLng) {
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
FlatProjection.prototype.fromPointToLatLng = function(point) {
  var y = point.y;
  var x = point.x;

  var lng = (x - this.pixelOrigin.x) / this.pixelsPerDegree;
  var lat = (this.pixelOrigin.y - y) / this.pixelsPerDegree;
  return new google.maps.LatLng(lat, lng);
};

