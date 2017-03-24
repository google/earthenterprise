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
  This is a shared tool used by the Server Viewer.  Both 2D & 3D Fusion
  Databases and Portable globes have layers - this tool exposes those layers
  in the UI and allows the user to toggle them on and off.  Layers in 3D are
  more complex, as they may have child/parent relationships that need to be
  represented.  The bulk of this javascript performs 3D layer tasks.
*/

var collapsedLayers = [];
var arrowOpenImg = '/shared_assets/images/arrow_open.png';
var arrowCloseImg = '/shared_assets/images/arrow_closed.png';

// Get a list of layers for a map and display them in the ui.
function getMapLayers() {
  for (var i = 0; i < geeServerDefs.layers.length; i++) {
    var layer = geeServerDefs.layers[i];
    var layerId = layer.glm_id ?
        layer.glm_id + '-' + layer.id : '0-' + layer.id;
    var div = document.getElementById('LayerDiv');
    var checked = layer.initialState ? ' checked' : '';
    var imgPath =
        geeServerDefs.serverUrl + 'query?request=Icon&icon_path=' + layer.icon;
    div.innerHTML +=
        '<label>' +
        '<input type="checkbox" onclick="toggleMapLayer(\'' +
        layerId + '\')"' + 'id="' + layerId + '" ' + checked + '/>' +
        '<img src="' + imgPath + '" onerror="this.style.display=\'none\';" >' +
        layer.label + '</label>';
    div.innerHTML +=
        '<input type=range id=opacity_' + i + ' min=0 value=100 max=100 step=10 ' +
        'oninput="geeMap.setOpacity(\'' + layerId + '\', Number(value/100))" ' +
        'onchange="outputOpacityValue(\'' + i + '\', Number(value))">';
    div.innerHTML += '<output id=opacity_out_' + i + '>100%</output>';

  }
  togglePolygonVisibility();
}

// Update the current opacity value (as a percentage) shown on the slider.
function outputOpacityValue(id, val) {
  document.querySelector('#opacity_out_' + id).value = val + '%';
}

// If a mobile browser is detected, remove layer and zoom elements.
function checkIfMobile() {
  var isMobile = navigator.userAgent.match(/Android/i) ||
      navigator.userAgent.match(/iPhone|iPad|iPod/i);
  if (isMobile) {
    var zmControls = getElement('ZmControls');
    if (zmControls) {
      zmControls.style.display = 'none';
    }
  }
}

// Turn a map layer on/off.
function toggleMapLayer(layerId) {
  var checked = document.getElementById(layerId).checked;
  if (checked) {
    geeMap.showFusionLayer(layerId);
  } else {
    geeMap.hideFusionLayer(layerId);
  }
}

// Get earth object containing all layers and send it on to be analyzed.
function getEarthLayers() {
  // Init Iframes required for viewing elements properly in earth on Windows.
  initIframes();
  // Set up the layer list element and add the name as a header.
  var div = document.getElementById('LayerDiv');
  div.style.display = 'block';
  var name = window.location.pathname;
  div.innerHTML = '<span><h2>Viewing: ' + name + '</h2></span>';
  // Create a button for options.
  div.innerHTML += '<div id="PreviewOptionsButton"' +
      ' onclick="togglePreviewOptions()"></div>' +
      '<div id="PreviewOptions" style="display:none;">' +
      '<em onclick="toggleOverlayElements()">Toggle overlays' +
      ' (alt-shift-h)</em></div>';
  // Get all the layers and send to the layer function.
  var layers = ge.getLayerRoot().getFeatures().getChildNodes();
  earthLayerTree(layers);
  togglePolygonVisibility();
}

// Find each earth layer and insert into the UI.  Recursively look for
// child layers and do the same.
function earthLayerTree(layers, depthCounter) {
  // Keep track of how far away each layer is from the root.
  var depth = depthCounter || 0;
  depth += 1
  // Insert each layer into the UI.
  for (var i = 0; i < layers.getLength(); i++) {
    var layer = layers.item(i);
    var id = layer.getId();
    var name = layer.getName();
    var type = layer.getType();
    var url = layer.getUrl();
    var visible = layer.getVisibility();
    var layerStyle = layer.getComputedStyle().getListStyle();
    var expandable = layerStyle.getListItemType();
    var parent = layer.getParentNode().getId();
    // Type KmlLayerRoot must be handled differently than any other type.
    if (type == 'KmlLayerRoot') {
      insertEarthLayer(name, url, depth, visible, 'toggleRootLayer', parent);
    } else {
      insertEarthLayer(name, id, depth, visible, 'toggleEarthLayer', parent);
    }
    // If layer is a folder, find out more to determine if it
    // requires recursion to add child layers.
    if (type == 'KmlFolder') {
      var childNodes = layer.getFeatures().getChildNodes();
      var children = childNodes.getLength();
      if (children > 1 && expandable == 1) {
        earthLayerTree(childNodes, depth);
      }
    }
  }
}

// Insert an earth layer into the UI.
function insertEarthLayer(name, id, depth, visibility, func, parent) {
  var div = document.getElementById('LayerDiv');
  var collapse = createExpandableFolder(id);
  // Create indentation elements based on depth.
  var ems = checkLayerDepth(depth);
  div.innerHTML += '<label title="' + name + '" name="' +
      parent + '">' + ems + makeEarthCheckbox(id, visibility, func) +
      name + collapse + '</label>';
}

// Create a checkbox for an earth layer.
function makeEarthCheckbox(id, checkedStatus, func) {
  var checked = checkedStatus ? 'checked' : '';
  var checkbox = '<input type="checkbox"' +
      'id="cb_' + id + '" ' +
      'onclick="' + func + '(\'' + id + '\')" ' +
      checked + '/>';
  return checkbox;
}

// If the item is an expandable folder, create an element that
// will allow the user to expand/collapse the tree of layers.
function createExpandableFolder(id) {
  var string = '';
  var layer = ge.getLayerRoot().getLayerById(id);
  var type = layer.getType();
  var name = layer.getName();
  var layerStyle = layer.getComputedStyle().getListStyle();
  var expandable = layerStyle.getListItemType();
  // If it meets these conditions, user should be able to toggle
  // the layer in order to expand and collapse its children.
  if (expandable == 1 && type == 'KmlFolder') {
    string = '<a href="#" class="CollapseIcon" id="collapse_' +
        id + '" onclick="collapseExpandLayers(\'' + id + '\')">' +
        getArrowImg('open') + '</a>';
  }
  return string;
}

// Collapse or expand a layer and all of its children.  Update
// the label for the link.
function collapseExpandLayers(id, parentDisplayType) {
  var elements = document.getElementsByName(id);
  var displayType = getDisplayType(id, parentDisplayType);
  var layerLabel = displayType == 'block' ?
      getArrowImg('open') : getArrowImg('close');
  document.getElementById('collapse_' + id).innerHTML = layerLabel;
  for (var i = 0; i < elements.length; i++) {
    elements[i].style.display = displayType;
  }
  collapseChildLayers(id, displayType);
}

// Helper function to update the display of a layer and its children.
// When parentDisplayType is passed, it is done so in order to be sure
// its children inherit the same behavior.
function getDisplayType(id, parentDisplayType) {
  var location = collapsedLayers.indexOf(id);
  if (location == -1) {
    displayType = 'none';
    if (parentDisplayType != 'block') {
      collapsedLayers.push(id);
    }
  } else {
    displayType = 'block';
    if (parentDisplayType != 'none') {
      collapsedLayers.splice(location, 1);
    }
  }
  displayType =
      parentDisplayType == undefined ? displayType : parentDisplayType;
  return displayType;
}

// Get information about all child layers, and continue expanding
// or collapsing if they too are expandable folders.
function collapseChildLayers(id, displayType) {
  var layer = ge.getLayerRoot().getLayerById(id);
  var children = layer.getFeatures().getChildNodes();
  for (var j = 0; j < children.getLength(); j++) {
    var child = children.item(j);
    var childId = child.getId();
    var type = child.getType();
    var name = child.getName();
    var layerStyle = child.getComputedStyle().getListStyle();
    var expandable = layerStyle.getListItemType();
    if (type == 'KmlFolder' && expandable == 1) {
      collapseExpandLayers(childId, displayType);
    }
  }
}

// Create an arrow image for expanding/collapsing the layer tree.
function getArrowImg(arg) {
  var img = arg == 'open' ? arrowOpenImg : arrowCloseImg;
  return '<img src="' + img + '"/>';
}

// Check a layer's distance from the root layer and create indentation
// elements based on how far away it is.  Indenting the layers helps to
// to illustrate the child/parent relationships within the layer tree.
function checkLayerDepth(num) {
  var string = '';
  // Start counting at 1 - base layers are 1, and do
  // not need to be indented.
  for (var i = 1; i < num; i++) {
    // A clean way to indent layers based on depth is to add an 'em'
    // element for each step a layer is in relation to the root.  The
    // 'em' elements have a fixed width of 12px, so the layer will be
    // indented (12px * distance from root layer).
    string += '<em></em>';
  }
  return string;
}

// Toggle an earth layer on/off.  Toggle its parents/children appropriately.
function toggleEarthLayer(layerId) {
  // Define layer and whether or not it is expandable.
  var layer = ge.getLayerRoot().getLayerById(layerId)
  var layerStyle = layer.getComputedStyle().getListStyle();
  var expandable = layerStyle.getListItemType();
  var action;
  if (layer.getVisibility() == false) {
    // If off, turn on.
    action = true
    ge.getLayerRoot().enableLayerById(layerId, action);
    // Also turn on parent layers and check their checkboxes. Only
    // if toggling layers on.
    toggleParentCheckbox(layer);
  } else {
    // If on, turn off.
    action = false
    ge.getLayerRoot().enableLayerById(layerId, action);
  }
  // Whether toggling on or off, make sure all child layers mimic the
  // behavior by turning them on/off as well.
  if (layer.getType() == 'KmlFolder' && expandable == 1) {
    var childLayers = layer.getFeatures().getChildNodes();
    toggleChildLayers(childLayers, action);
  }
}

// Layers with type KmlLayerRoot need to be toggled differently than others.
function toggleRootLayer(dbName) {
  var children = ge.getLayerRoot().getFeatures().getChildNodes();
  for (var z = 0; z < children.getLength(); z++) {
    var layer = children.item(z);
    var layerUrl = layer.getUrl();
    var layerVisibility = layer.getVisibility();
    if (layerUrl == dbName) {
      if (layerVisibility == true) {
        layer.setVisibility(false);
      } else {
        layer.setVisibility(true);
      }
    }
  }
}

// Toggle all children of a particular layer.
function toggleChildLayers(children, action) {
  for (var i = 0; i < children.getLength(); i++) {
    var layer = children.item(i);
    var layerStyle = layer.getComputedStyle().getListStyle();
    var expandable = layerStyle.getListItemType();
    var name = layer.getName();
    var id = layer.getId();
    // Turn the layer on/off.
    ge.getLayerRoot().enableLayerById(id, action);
    var type = layer.getType();
    var checkBox = document.getElementById('cb_' + id);
    checkBox.checked = action;
    // If the layer is a folder, determine if it has children
    // that also need to be turned on/off.
    if (type == 'KmlFolder') {
      var childNodes = layer.getFeatures().getChildNodes();
      var newChildren = childNodes.getLength();
      if (newChildren > 1 && expandable == 1) {
        toggleChildLayers(childNodes, action, expandable);
      }
    }
  }
}

// Toggle the parents of a layer.
function toggleParentCheckbox(layer) {
  var parentId = layer.getParentNode().getId();
  // If layer is not root (root id == 80), toggle parent layers.
  if (parentId != 80) {
    document.getElementById('cb_' + parentId).checked = true;
    var parentLayer = ge.getLayerRoot().getLayerById(parentId);
    toggleParentCheckbox(parentLayer);
  }
}

// Load iframes needed for earth plugin in Windows.
function initIframes() {
  document.getElementById('IframeHeader').style.display = 'block';
  document.getElementById('IframeLayers').style.display = 'block';
}

// Toggle all (non-map) elements on the screen to allow for showing map only.
function toggleOverlayElements() {
  var elements = ['Header',
                  'ZmControls'];
  // If item has search options, include search div among the overlays.
  if (searchDefsListing && searchDefsListing.length > 0) {
    elements.push('SearchDiv');
  }
  var layerDiv = getElement('LayersHolder');
  var display = getElement(elements[0]).style.display == 'none' ?
      'block' : 'none';
  togglePreviewLayers(display);
  for (var i = 0; i < elements.length; i++) {
    if (getElement(elements[i])) {
      getElement(elements[i]).style.display = display;
    }
  }
  getElement('PreviewOptions').style.display = 'none';
  if (isServing == '3D') {
    var height = display == 'block' ? '64px' : '0';
    getElement('earth').style.top = height;
  }
  if (display == 'none') {
    layerDiv.style.left = '0';
    layerDiv.style.top = '0';
    layerDiv.style.overflow = 'visible';
  } else {
    layerDiv.style.left = '36px';
    layerDiv.style.top = '85px';
    layerDiv.style.overflow = 'hidden';
  }
}

// Hide/show the list of layers and opacity sliders belonging to a database/portable.
function togglePreviewLayers(display) {
  // Preserve inline formatting of opacity sliders & output labels in LayerDiv
  var display_inline = (display == 'block') ? 'inline' : 'none';
  var layerDiv = getElement('LayerDiv');
  var layers = layerDiv.getElementsByTagName('label');
  var sliders = layerDiv.getElementsByTagName('input');
  var slider_values = layerDiv.getElementsByTagName('output');
  for (var i = 0; i < layers.length; i++) {
    layers[i].style.display = display;
  }
  for (var i = 0; i < sliders.length; i++) {
    sliders[i].style.display = display_inline;
  }
  for (var i = 0; i < slider_values.length; i++) {
    slider_values[i].style.display = display_inline;
  }
}

// Show or hide options when options button is clicked.
function togglePreviewOptions() {
  var div = getElement('PreviewOptions');
  if (div.style.display == 'none') {
    div.style.display = 'block';
  } else {
    div.style.display = 'none';
  }
}

// Some keypress shortcuts.
$(document).keydown( function(e) {
    // ALT + SHIFT + h, hide all elements.
    if (e.keyCode == 72 && e.shiftKey && e.altKey) {
        toggleOverlayElements();
    }
    // ALT + SHIFT + p, toggle polygon.
    if (e.keyCode == 80 && e.shiftKey && e.altKey) {
        togglePolygonVisibility(false);
    }
});

// Helper function for grabbing elements.
function getElement(element) {
  return document.getElementById(element);
}
