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
    globesTitle: 'globes_title',
    globes: 'globes_container',
    collapsePanel: 'collapsePanel',
    collapseShim: 'collapseShim'
};
if (typeof(GEE_URL_PREFIX) == 'undefined') {
  GEE_URL_PREFIX = '';
}
// Need the static URL for absolute addresses
var GEE_STATIC_URL = window.location.protocol + '//'
    + window.location.host + GEE_URL_PREFIX;
// Static image paths in this example are relative to the earth server host.
var GEE_EARTH_IMAGE_PATH = GEE_STATIC_URL + '/earth/images/';
var GEE_MAPS_IMAGE_PATH = GEE_STATIC_URL + '/maps/api/icons/';
// Default channel for 3d.
var GEE_UNUSED_CHANNEL = 0;

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
 * the getServerDefs request.
 *
 * @param {boolean} showGlobesList (optional)
 *    Whether to show globes list for switching globe being served.
 *    Default is false.
 */
function geeInit(showGlobesList) {
  // Issue an asynchronous HTTP request to get the ServerDefs.
  // Set the doInitEarth() as a callback-function which will be triggered on
  // finishing the request.
  getServerDefinitionAsync(GEE_SERVER_URL, false,
                           makeDoInitEarthCallback(showGlobesList));
}

/**
 * Makes doInitEarth() callback function.
 *
 * @param {boolean} showGlobesList (optional)
 *    Whether to show globes list for switching globe being served.
 *    Default is false.
 * @return {function} callback-function.
 */
function makeDoInitEarthCallback(showGlobesList) {
  function func() {
    doInitEarth(showGlobesList);
  }
  return func;
}

/**
 * Does the actual initialization of the Google Earth plugin.
 * It initializes: 1) the earth plugin 2) the layer panel 3) the search tabs.
 *
 * @param {boolean} showGlobesList (optional)
 *    Whether to show globes list for switching globe being served.
 *    Default is false.
 */
function doInitEarth(showGlobesList) {
  // check that geeServerDefs are loaded.
  if (geeServerDefs == undefined) {
    alert('Error: The Google Earth Enterprise server does not recognize the ' +
          'requested database.');
    return;
  }

  // Don't show globes list by default.
  if (typeof showGlobesList == 'undefined') {
    showGlobesList = false;
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

  geeInitLeftPanelDivs(showGlobesList);  // The left panel needs some sub-divs for layers etc.
  geeResizeDivs();  // Resize the map to fill the screen.

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
 * The Earth Plugin init callback.
 * @param {GEPlugin}
 *          object the just created Google Earth plugin object.
 */
function geeEarthPluginInitCb(object) {
  ge = object;
  ge.getOptions().setStatusBarVisibility(true);  // show lat,lon,height,eye_alt
  ge.getOptions().setScaleLegendVisibility(true);  // show scale legend
  ge.getWindow().setVisibility(true);
  ge.getNavigationControl().setVisibility(ge.VISIBILITY_SHOW);

  // On successful init, we can now load the layers and initialize the search
  // tabs.
  geeInitLayerList(ge, geeDivIds.layers);

  // Set up select for changing to another globe or map.
  geeInitGlobesList(geeDivIds.globes);

  // Check for a search_timeout override.
  if (getPageURLParameter('search_timeout')) {
    geeSearchTimeout = getPageURLParameter('search_timeout');
  }
  initializeSearch(geeDivIds.searchTabs, geeServerDefs.searchTabs,
                   geeSearchTimeout);
  geeCreateShims();
  geeResizeDivs();

  geeInitView(geeShowPolygon);
}

/**
 * The Earth Plugin failure callback.
 * @param {string}
 *          message Error message describing failure.
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
 * Fill in the given list with list items for each child layer/folder. This is a
 * recursive call to handle folders of folders.
 * @param {string}
 *          parentFeatureId the GE feature id of the parent layer folder.
 * @param {Element}
 *          parentList the DOM list element to add these layers to.
 * @param {Array}
 *          childLayers the child layers to be added to this list element.
 */
function geeCreateLayerElements(parentFeatureId, parentList, childLayers) {
  for (var i = 0; i < childLayers.getLength(); ++i) {
    var feature = childLayers.item(i);
    if (feature) {
      var layerId = feature.getId();
      var layerName = feature.getName();
      var isVisible = false;
      if (feature.getVisibility()) {
        isVisible = true;
      }
      var type = feature.getType();
      // This is a setting that can be set in GEE Fusion but is not exposed
      // by the GE Plugin API.
      var isOpenable = (layerId in geeSupplementalLayerInfo) ?
                          geeSupplementalLayerInfo[layerId].isExpandable : true;
      if (!layerId) {
        // KmlLayers sometimes do not have a feature ID due to a plugin bug!!!
        layerId = parentFeatureId + '_' + i;
      }
      // We need to track the children of each parent folder so we can turn on
      // and off entire folders.
      geeFolderLayerChildren[parentFeatureId].push(layerId);
      var item = geeCreateLayerItem(parentList, feature, layerId,
                                    isVisible, isOpenable);

      if (type == 'KmlLayer') {
        // Do nothing.
      } else if (type == 'KmlFolder' && isOpenable) {
        // It's an openable folder. Create a sublist.
        var features = feature.getFeatures();
        var folderChildLayers = features.getChildNodes();
        // Init the array for this folder.
        geeFolderLayerChildren[layerId] = [];

        // Create a sublist for the children.
        var folderListId = 'folder_contents_' + layerId;
        var folderList = createElement(item, 'ul', folderListId);
        folderList.className = 'layer_list';
        geeCreateLayerElements(layerId, folderList, folderChildLayers);

        // Hide/show child folders on click...and swap the folder icon!
        item.onclick = function(theFolder, theList, theFolderName,
                                openFolderIconUrl, closedFolderIconUrl) {
          return function(e) {
            var iconId = theFolderName + '_icon';
            var iconElement = document.getElementById(iconId);
            var iconUrl = openFolderIconUrl;
            if (theList.style.display != 'none') {
              theList.style.display = 'none';
              iconUrl = closedFolderIconUrl;
            } else {
              theList.style.display = 'block';
            }
            iconElement.innerHTML = '<image src="' + iconUrl + '" \>';
            cancelEvent(e);
          };
        }(item, folderList, layerName, geeIconUrls.openedFolder,
            geeIconUrls.closedFolder);
      }
    }
  }
}

/**
 * Create a single layer item entry with a checkbox and appropriate hover and
 * click behaviors.
 * @param {Element}
 *          layerList the list element container for the item being created.
 * @param {KmlLayer}
 *          layer the layer object.
 * @param {string}
 *          layerId the layer object.
 * @param {boolean}
 *          isChecked true if the layer is default on.
 * @param {boolean}
 *          isOpenable true if the layer is allowed to open/expand (only
 *                     meaningful for folders.
 * @return {Element} the list item for the layer.
 */
function geeCreateLayerItem(layerList, layer, layerId,
                            isChecked, isOpenable) {
  var type = layer.getType();
  var layerName = layer.getName();
  var checkboxId = 'checkbox_' + layerId;
  var layerIconUrl = geeFeatureIconUrl(layer);

  if ((type == 'KmlFolder') && isOpenable) {  // Give folders a 'folder icon'.
    layerIconUrl = geeIconUrls.openedFolder;
  }

  // Handle network links specially. They are loaded on demand and the
  // layer id will swap out from under us when they are loaded.
  geIsNetworkLink[layerId] = false;
  if (type == 'KmlNetworkLink') {
    geNetworkLinks[layerId] = layer;
    geIsNetworkLink[layerId] = true;
  } else if (type == 'KmlDocument') {
    // KmlDocument is a network link that's already loaded!
    geIsNetworkLink[layerId] = true;
    geNetworkLinks[layerId] = layer;
    geLoadedNetworkLinks[layerId] = { childObject: layer };
  }

  // Create the list item for the layer.
  var item = geeCreateLayerItemBasic(layerList,
                                     checkboxId,
                                     GEE_UNUSED_CHANNEL,
                                     layerId,
                                     layerName,
                                     layerIconUrl,
                                     isChecked,
                                     'geeToggleLayer');

  // Set to zoom in on layer on click
  item.onclick = function(layerObject) {
    return function(e) {
      var view = layerObject.getAbstractView();
      if (view) {
        ge.getView().setAbstractView(view);
      }
      cancelEvent(e);
    };
  }(layer);

  // Make sure checkbox state matches layer visibility.
  geeToggleLayer(null, checkboxId, GEE_UNUSED_CHANNEL, layerId, layerName);
  return item;
}

/**
 * Toggle the visibility of the specified layer. Assumes the 'ge',
 * 'geNetworkLinks', 'geIsNetworkLink', and 'geLoadedNetworkLinks' objects
 * are initialized.
 * @param {Object}
 *          e  the event argument.
 * @param {string}
 *          checkBoxId the name of the checkbox which maintains the layer's
 *          visibility state.
 * @param {string}
 *          channel the channel of the layer to toggle.
 * @param {string}
 *          layerId the id of the layer to toggle.
 * @param {string}
 *          layerName the name of the layer to toggle only used for printing
 *          error message.
 */
function geeToggleLayer(e, checkBoxId, channel, layerId, layerName) {
  try {
    var cb = document.getElementById(checkBoxId);
    if (cb) {
      try {
        var isEnabled = true;
        if (!cb.checked)
          isEnabled = false;
        // We have to handle network links separately.
        // They are loaded on demand.
        if (geIsNetworkLink[layerId]) {
          var feature = geNetworkLinks[layerId];
          if (feature.getType() == 'KmlNetworkLink') {
            // Load up a new network link and replace the array entry with
            // the KmlObject.
            try {
              // Pull out the URL for the network link.
              var link = feature.getLink();
              var href = GEE_SERVER_URL + link.getHref();
              if (geLoadedNetworkLinks[layerId]) {
                if (geLoadedNetworkLinks[layerId].childObject) {
                  geLoadedNetworkLinks[layerId].childObject.setVisibility(
                    isEnabled);
                }
              } else if (isEnabled) {  // Need to load the network link.
                geLoadedNetworkLinks[layerId] = { childObject: null };
                // Fetch the KML and add a callback to record the KML object for
                // future toggling.
                window.google.earth.fetchKml(ge,
                  href,
                  function(obj) {
                    geLoadedNetworkLinks[layerId].childObject = obj;
                    ge.getFeatures().appendChild(obj);
                    // Update the checkbox with the new name.
                    var spanId = layerId + '_name';
                    var span = document.getElementById(spanId);
                    if (span) {
                      span.innerHTML = obj.getName();
                    }
                  });
              }
            } catch (err3) {
              alert('Failed attempt to enable/disable network link: ' +
                    layerName + '\n' + err3);
            }
          }
        } else {  // A normal streamed layer.
          // For folders, we need to update the checkboxes of all children
          geeSetFolderLayersVisibility(layerId, isEnabled);
        }
      } catch (err2) {
        alert('Failed attempt to enable/disable layer: ' + layerName +
              '\n' + err2);
      }
    } else {
      alert('unknown: ' + checkBoxId);
    }
  } catch (err) {
    alert('Failed attempt to get checkbox for layer: ' + layerName +
          '\n' + err);
  }

  if (e) {
    cancelEvent(e);
  }
}

 /**
  * Recursively update the checkbox status for all children of the specified
  * layer (i.e., all the child layers of a folder layer).
  * @param {string}
  *             layerId  the id of the layer. If it's not a folder,
  *             it is ignored.
  * @param {boolean}
  *             isEnabled whether to set the children checkboxes as checked.
  */
function geeSetFolderLayersVisibility(layerId, isEnabled) {
  ge.getLayerRoot().enableLayerById(layerId, isEnabled);
  if (layerId in geeFolderLayerChildren) {
    var childLayerIds = geeFolderLayerChildren[layerId];
    for (var i = 0; i < childLayerIds.length; ++i) {
      var childLayerId = childLayerIds[i];
      var checkboxId = 'checkbox_' + childLayerId;
      var cb = document.getElementById(checkboxId);
      cb.checked = isEnabled;
      geeSetFolderLayersVisibility(childLayerId, isEnabled);
    }
  }
}

/**
 * Extract the predefined icon from the KmlFeature.
 * This is not trivial.
 * @param {KmlFeature}
 *          feature the kml feature object.
 * @return {string} the kml feature object.
 */
function geeFeatureIconUrl(feature) {
  var kml = feature.getKml();
  if (!kml) {
    return geeIconUrls.defaultLayerIcon;
  }
  // Pull the substring between <ItemIcon> and </ItemIcon>.
  var itemIconText = extractElementFromXml(kml, 'ItemIcon');
  // From that pull the contents between the href.
  var href = extractElementFromXml(itemIconText, 'href');
  return href;
}

/*
 * Interface for search_tab.js: The following code implements the GEE Methods
 * required by search_tabs.js.
 */

/**
 * Create a balloon with no content.
 * @return {KmlFeatureBalloon} the kml balloon object.
 */
function geeCreateBalloon() {
  var balloon = ge.createFeatureBalloon('');
  balloon.setMaxWidth(DEFAULT_BALLOON_MAX_WIDTH_PIXELS);
  return balloon;
}

/**
 * Create a placemark at the specified location.
 * @param {string}
 *          name the name of the placemark.
 * @param {string}
 *          description the description of the placemark.
 * @param {Object}
 *          latlng an object that contains the 'lat' and 'lon' of
 *          the position of the placemark.
 * @param {string}
 *          iconUrl the URL of the placemark's icon.
 * @param {KmlFeatureBalloon}
 *          balloon the balloon for the placemark.
 * @return {KmlPlacemark} the placemark object.
 */
function geeCreateMarker(name, description, latlng, iconUrl, balloon) {
  // Create icon style for the placemark
  var icon = ge.createIcon('');
  icon.setHref(iconUrl);
  var style = ge.createStyle('');
  style.getIconStyle().setIcon(icon);

  // create a point geometry
  var point = ge.createPoint('');
  point.setLatitude(parseFloat(latlng.lat));
  point.setLongitude(parseFloat(latlng.lon));

  // create the point placemark and add it to Earth
  var pointPlacemark = ge.createPlacemark('');
  pointPlacemark.setName(name);
  pointPlacemark.setGeometry(point);
  pointPlacemark.setStyleSelector(style);
  pointPlacemark.setDescription(description);

  // Init the balloon.
  balloon.setFeature(pointPlacemark);

  return pointPlacemark;
}

/**
 * Add the specified feature to the earth viewer.
 * @param {KmlFeature}
 *          feature the feature to add.
 */
function geeAddOverlay(feature) {
  ge.getFeatures().appendChild(feature);
}

/**
 * Remove the specified feature from the earth viewer.
 * @param {KmlFeature}
 *          feature the feature to remove.
 */
function geeRemoveOverlay(feature) {
  ge.getFeatures().removeChild(feature);
}

/**
 * Close the open info windows. This is a no-op for the earth plugin.
 */
function geeCloseInfoWindow() {
  // For Earth, do nothing.
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

function geeShowPolygon(polygonKml) {
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
    ge.getFeatures().appendChild(kmlObject);

    if (kmlObject.getAbstractView() !== null) {
      ge.getView().setAbstractView(kmlObject.getAbstractView());
    }

    var bounds = geePolygonBounds(geePolygonCoordinates(polygonKml, geeLatLng));
    latDiff = bounds.north - bounds.south;
    lngDiff = bounds.east - bounds.west;
    if (latDiff > lngDiff) {
      maxDist = latDiff;
    } else {
      maxDist = lngDiff;
    }
    zoomLevel = Math.floor(Math.log(360 / maxDist) / Math.log(2));
    geePanTo((bounds.north + bounds.south) / 2,
             (bounds.east + bounds.west) / 2, zoomLevel);

    setTimeout('geeClearFeatures()', geePolygonDisplayTime);
  }
}

function geeClearFeatures() {
    gex = new GEarthExtensions(ge);
    gex.dom.clearFeatures();
}


/**
 * Open a balloon in the plugin.
 * @param {Object}
 *          marker the marker that is attached to the balloon
 *          (ignored for Earth Plugin).
 * @param {Object}
 *          balloon the balloon object (this is all we need for Earth Plugin).
 * @param {string}
 *          title the title of the balloon contents.
 *          (ignored for Earth Plugin).
 * @param {string}
 *          innerText the inner text of the balloon text.
 *          (ignored for Earth Plugin).
 */
function geeOpenBalloon(marker, balloon, title, innerText) {
  ge.setBalloon(balloon);
}

/**
 * Callback for Search Results Manager after results are cleared.
 * We simply clear any active balloon.
 */
function geeClearSearchResults() {
  ge.setBalloon(null);
}

/**
 * Simple lat/lng constructor that behaves similarly to the Maps API lat/lng
 * object.
 * @param {number} lat Latitude.
 * @param {number} lng Longitude.
 * @constructor
 */
function geeLatLng(lat, lng) {
  this.lat_ = lat;
  this.lng_ = lng;
}

/**
 * @return {number} the latitude.
 */
geeLatLng.prototype.lat = function() {
  return this.lat_;
};

/**
 * @return {number} the longitude.
 */
geeLatLng.prototype.lng = function() {
  return this.lng_;
};
