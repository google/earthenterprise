// Copyright 2017 Google Inc.
// Copyright 2019, Open GEE Contributors
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
  File used by glc_assembly.html and glc_disassembly.html
  for creating glc files.
*/

// Global variables used to keep track of selections.
var glcIs2d = false;
var availableGlobes = [];
var selectedGlobes = [];
var availableGlmObject = [];
var damagedGlmList = [];
var layerIdsForJson = [];
var selectedGlms = [];
var memorizedNames = [];
var layersSelect = gees.dom.get('AvailableLayersSelect');
var polygonSuccess = true;
var SELECT_STRING = '-- select --';
var ALL_LAYER_IN_USE = '(All available layers in use)';
var currentBaseGlobe = SELECT_STRING;
var GLC_SIZE_UPDATE_TIME = 1500;  // Time between updates of size.
var GLC_job_info = {};
var GLC_taskcheck_delay = 0;
var GLC_TASKCHECK_MAX_DELAY = 6;
var GLC_ASSEMBLER_APP = '/cgi-bin/glc_assembler_app.py?';

// Set a type (glm/glb) for the glc that is about to be created.
function checkGlcSourceType() {
  glcIs2d = gees.dom.isChecked('radio2d');
  var glcSourceType = glcIs2d ? 'glm' : 'glb';
  resetElements();
  getAvailableLayers(glcSourceType);
  gees.dom.setHtml('ErrorMessage', '');
  gees.dom.hide('cancel_button');
  currentBaseGlobe = SELECT_STRING;
}

// Reset all display elements to initial state.
function resetElements() {
  availableGlobes = [];
  selectedGlobes = [];
  availableGlmObject = [];
  damagedGlmList = [];
  gees.dom.clearHtml('ChooseType');
  gees.dom.setHtml('SelectedLayers',
      '<em id="NoSelectedLayers">No layers selected</em>');
  DisableCommit();
}

// Create list of available globes based on the type of glc being created.
function getAvailableLayers(sourceType) {
  var isCollectingGlm = sourceType == 'glm';
  for (var i = 0; i < gees.assets.portables.length; i++) {
    var item = gees.assets.portables[i];
    if (item.type == sourceType) {
      if (isCollectingGlm) {
        get2dLayers(item);
      } else {
        availableGlobes.push(item);
      }
    }
  }
  // Show or hide the base globe option based on 2D/3D option.
  checkBaseGlobeDisplay();
  // Put the list of available layers into the 'layersSelect' dropdown.
  displayAvailableLayers(layersSelect);
}

// Show (3D) or hide (2D) the base globe option.
function checkBaseGlobeDisplay() {
  // Hide or show base globe element.
  gees.dom.get('baseglobe_span').style.display = glcIs2d ? 'none' : 'block';
  // If 3D, display items available for use as base globes.
  if (!glcIs2d) {
    var select = gees.dom.get('BaseGlobeSelect');
    displayAvailableLayers(select);
  }
}

// 2D layers are collected differently than 3D layers.  3D layers
// are simply globes.  2D layers are the individual layers of
// a map.  This function grabs the layers of a single glm
// on each iteration.
function get2dLayers(item) {
  var name = item.name;
  var query = name + '/query?request=Json';
  var script = '/cgi-bin/globe_info_app.py/serve/';
  var url = GEE_BASE_URL + script + query;
  discoverGlmLayers(url, name);
}

// Get jobInfo, providing uid, report error if uid is invalid.
function getJobInfo(uid) {
  console.assert(uid in GLC_job_info);
  return GLC_job_info[uid];
}

// Add layers and make them available as options for building a glc.
// This function takes the list of layers belonging to a glm and
// places them into the list of available layers.
function add2dLayerAsOption(layers, sourceMap) {
  for (var i = 0; i < layers.length; i++) {
    var item = {};
    var layerName = layers[i].label;
    item.name = sourceMap + ' [' + layerName + ']';
    availableGlobes.push(item);
  }
  damagedGlmMessage();
}

// Gets and returns the layers of a glm file.
function discoverGlmLayers(url, glm) {
  var serverDefs = gees.requests.doGet(url);
  serverDefs = serverDefs.split('geeServerDefs =')[1];
  if (serverDefs) {
    serverDefs = serverDefs.split(';')[0];
    serverDefs = eval('(' + serverDefs + ')');
    var item = {};
    item.name = glm;
    item.layers = serverDefs.layers;
    availableGlmObject.push(item);
    add2dLayerAsOption(item.layers, glm);
  } else {
    damagedGlmList.push(glm);
  }
}

// Deliver a UI message if damaged GLMs exist.
function damagedGlmMessage() {
    if (damagedGlmList.length == 0) {
      return;
    }
    var note = damagedGlmList.length > 1 ? 'items appear' : 'item appears';
    var msg = 'The following ' + note + ' to be damaged, and cannot ' +
        'be used for GLC assembly: <b>' + damagedGlmList.join(', ') + '</b>';
    gees.dom.get('ErrorBar').innerHTML = msg;
    gees.dom.show('ErrorBar');
    setTimeout(function() {
        gees.dom.hide('ErrorBar');
      }, 4500);
}

// Using a previously assembled list of available layers, populate
// a dropdown menu with layer each as an option.
function displayAvailableLayers(select) {
  var currentlyAvailableGlobes = 0;
  var div = gees.dom.get('AvailableLayers');
  gees.dom.clearHtml(select);
  gees.dom.show(div);
  createOption(SELECT_STRING, select);
  for (var i = 0; i < availableGlobes.length; i++) {
    var item = availableGlobes[i];
    var name = item.name;
    var notSelected = selectedGlobes.indexOf(name) == -1;
    if (notSelected) {
      createOption(name, select);
      currentlyAvailableGlobes += 1;
    }
  }
  confirmGlobeCount(currentlyAvailableGlobes, select);
}

// If all globes are selected, dropdown should display a unique message.
function confirmGlobeCount(globes, select) {
  if (globes == 0) {
    gees.dom.clearHtml(select);
    createOption(ALL_LAYER_IN_USE, select);
  }
}

// Helper function - adds an option element to a select menu.
function createOption(optionContent, div) {
  var option = gees.dom.create('option');
  gees.dom.setHtml(option, optionContent);
  div.appendChild(option);
}

// Store names and check statuses of each selected layer
// so they can be restored when the list is refreshed.
function rememberNames() {
  memorizedNames = [];
  getAllLayerElements();
  for (var i = 0; i < layerIdsForJson.length; i++) {
    var item = layerIdsForJson[i];
    var nameLabel = item[0];
    var nameValue = gees.dom.get(nameLabel).value;
    memorizedNames.push([nameLabel, nameValue, 'name']);
    var checkLabel = item[1];
    memorizedNames.push([checkLabel, gees.dom.isChecked(checkLabel), 'check']);
  }
}

// Take a list of memorized display elements and restore them
// the their most recent state.
function restoreNames() {
  for (var i = 0; i < memorizedNames.length; i++) {
    var item = memorizedNames[i];
    var id = item[0];
    var value = item[1];
    var type = item[2];
    if (gees.dom.get(id)) {
      if (type == 'name') {
        gees.dom.get(id).value = value;
      } else {
        gees.dom.setCheckbox(id, value);
      }
    }
  }
}

// Add a layer to a tentative list of selected layers.
function stageGlcLayer() {
  rememberNames();
  var select = layersSelect;
  var globe = select.options[select.selectedIndex].value;
  if (globe != SELECT_STRING && globe != ALL_LAYER_IN_USE) {
    selectedGlobes.push(globe);
    displayAvailableLayers(layersSelect);
  }
  stageLayers();
}


//Retrieve the dbroot information (imagery, terrain, proto) of base globe.
function UpdateBaseGlobeDbroot() {
  var select = gees.dom.get('BaseGlobeSelect');
  var globe = select.options[select.selectedIndex].value;

  // In 3D, if no base globe has been selected, user cannot proceed.
  if (globe == SELECT_STRING) {
    DisableCommit();
    gees.dom.setHtml('ErrorMessage', 'Please select a base globe.');
    return;
  }

  // Check whether user has selected a new base-globe.
  if (globe == currentBaseGlobe) {
    return;
  }
  currentBaseGlobe = globe;

  //Update base-globe dbroot information.
  var query = '/cgi-bin/globe_info_app.py/dbroot_info/' + globe;
  $.ajax({
    url: GEE_BASE_URL + query,
    dataType: 'json',
    success: successUpdateBaseGlobeDbroot,
    error: failUpdateBaseGlobeDbroot
  });
}

//Update the html GUI once dbroot query succeeds.
function successUpdateBaseGlobeDbroot(response) {
  // Update imagery radio button
  if (response['has_imagery']) {
    // Set default to "yes"
    gees.dom.setCheckbox('imagery_radio', true);
    gees.dom.show('ImageryRadioOptions');
  } else {
    // Set default to "no" and hide
    gees.dom.setCheckbox('imagery_radio_no', true);
    gees.dom.hide('ImageryRadioOptions');
  }

  // Update terrain radio button
  if (response['has_terrain']) {
    // Set default to "yes"
    gees.dom.setCheckbox('terrain_radio', true);
    gees.dom.show('TerrainRadioOptions');
  } else {
    // Set default to "no" and hide
    gees.dom.setCheckbox('terrain_radio_no', true);
    gees.dom.hide('TerrainRadioOptions');
  }

  // If a globe has no imagery and terrain, then it is not a valid base globe.
  if (!(response['has_imagery'] || response['has_terrain'])) {
    DisableCommit();
    gees.dom.setHtml('ErrorMessage', 'Please select a valid globe with ' +
                     'imagery and/or terrain.');
  }

  // Update the hidden proto_imagery variable.
  if (response['is_proto_imagery']) {
   gees.dom.get('is_proto_imagery').value = 1;
  } else {
   gees.dom.get('is_proto_imagery').value = 0;
  }
}

//Print out error information if the dbroot query fails.
function failUpdateBaseGlobeDbroot(xhr) {
  DisableCommit();
  gees.dom.setHtml('ErrorMessage', 'Please select a valid base globe.');
}

// Display selected layers as they are selected/removed.
function stageLayers() {
  var div = gees.dom.get('SelectedLayers');
  gees.dom.clearHtml('ErrorMessage');
  gees.dom.clearHtml(div);
  // Iterate over selected layers and stage to display.
  for (var i = 0; i < selectedGlobes.length; i++) {
    var layerNumber = i + 1;
    stageLayerToDisplay(selectedGlobes[i], div, layerNumber);
  }
  // Make the 'Assemble Glc' button available or unavailable based
  // on whether or not a valid globe selection has been made.
  if (selectedGlobes.length == 0) {
    DisableCommit();
    gees.dom.setHtml('SelectedLayers',
        '<em id="NoSelectedLayers">No layers selected</em>');
  } else {
    EnableCommit();
  }

  if (!glcIs2d) {
    UpdateBaseGlobeDbroot();
  }

  restoreNames();
  // TODO: this is a good place to run a future function that
  // estimates the size of the glc being built.
}

// Enable commit button and cancel button.
function EnableCommit() {
  gees.dom.setClass('commit_button', 'button blue');
  gees.dom.showInline('cancel_button');
}

// Disable commit button and cancel button.
function DisableCommit() {
  gees.dom.setClass('commit_button', 'button blue Muted');
  gees.dom.hide('cancel_button');
}

// Given a layer, a target div element and a layer id number,
// create a span element and insert it into the target div.
function stageLayerToDisplay(layer, div, layerNumber) {
  // Returns array [layerName, layerId].
  var displayArray = layerDisplayValues(layer);
  var layerName = displayArray[0];
  var layerId = displayArray[1];
  var checkbox = '<input id="' + layerId +
      '_searchbox" type="checkbox"><em>Use Search?</em>';
  var nameInput =
      '<input id="' + layerId + '_name"' +
      ' type="text" value="' + layerName + '">';
  var label = '<label>' + layerName + '</label>';
  var deleteLink =
      '<a href="#" onclick="unstageLayer(\'' + layer + '\')">delete</a>';
  var span =
      '<span><label>Layer ' + layerNumber + ': </label>' + nameInput +
      checkbox + deleteLink + '</span>';
  div.innerHTML += span;
}

// Creates display label and hidden id for layers.  Glb layers are simple;
// they have unique names.  For glms, we have to create internal labels that
// look like 'SourceGlm_LayerName' in order to keep them uniquely identified.
function layerDisplayValues(layer) {
  if (glcIs2d) {
    var glm = layer.split('[')[0].replace(/ /g, '');
    var layerName = layer.split('[')[1].replace(/]/g, '');
    var label = glm + '?' + layerName;
    return [layerName, label];
  } else {
    return [layer, layer];
  }
}

// Remove a layer from the list of selected layers and from display list.
function unstageLayer(layer) {
  rememberNames();
  var index = selectedGlobes.indexOf(layer);
  selectedGlobes.splice(index, 1);
  displayAvailableLayers(layersSelect);
  stageLayers();
}

// Gets the display elements for all selected for creating the glc.
// If form is valid, this function will kick off the build process for the glc.
function initializeGlcAssembly() {
  var responseDiv = gees.dom.get('status');
  gees.dom.clearHtml(responseDiv);
  gees.dom.clearHtml('ErrorMessage');
  var validForm = validateGlcForm();
  if (validForm) {
    getAllLayerElements();
    gees.dom.setClass('commit_button', 'button blue Muted');
    gees.dom.show('BuildProgress');

    // If polygon looks good, move ahead with the assembly process.
    if (polygonSuccess) {
      var name = gees.dom.get('glc_name_field').value;

      // Create the http request.
      var url = '/cgi-bin/glc_assembler_app.py?cmd=ASSEMBLE_JOBINFO&name=' +
                name;
      jQuery.get(url,
         function(job_info_str) {
           var job_info = JSON.parse(job_info_str);
           GLC_job_info[job_info.uid] = job_info;
           GEE_setUid(job_info.uid);

           var base_url = '/cgi-bin/glc_assembler_app.py", "cmd=';
           var prefix = 'GEE_postData("' + base_url;
           var globe = '&globe=' + job_info.path;
           var name = '&name=' + job_info.name;
           var base = '&base=' + job_info.base;
           var clean = '&cleanup=t';
           var append = ('", "status", "APPEND_CR");');
           var overwrite = ('", "globe", "SET_CR");');

           var sequence = [];
           sequence[0] = 'glc_assembleGlc(\'' + job_info.uid + '\')';
           sequence[1] = 'glc_checkTaskDone(\'' + job_info.uid + '\');';
           sequence[2] = prefix + 'CLEAN_UP' + globe + base + clean + append;
           sequence[3] = prefix + 'GLOBE_INFO' + name + overwrite;
           sequence[4] = 'glc_cleanUp();';
           glc_clearGlobeInfo();

           GEE_runAjaxSequence(sequence, GLC_SIZE_UPDATE_TIME,
                               glc_showGlobeSize, glc_cleanUp);
        });
    }
  }
  // Resest polygon success message, whatever happens.
  polygonSuccess = true;
}

// Clear the "globe" bar in GUI.
function glc_clearGlobeInfo() {
  gees.dom.clearHtml('globe');
}

// Clean up for next round of running.
function glc_cleanUp() {
  if (GEE_interval != '') {
    clearInterval(GEE_interval);
    GEE_interval = '';
  }
  GLC_taskcheck_delay = 0;
  GEE_clearUid();
  glc_enableGLC();
}

// Check whether the task has finished or not.
function glc_checkTaskDone(uid) {
  if (GEE_running_get_data_sequence) {
    if (GLC_taskcheck_delay < GLC_TASKCHECK_MAX_DELAY) {
      GLC_taskcheck_delay += 1;
    }

    var job_info = getJobInfo(uid);
    var url = GLC_ASSEMBLER_APP +
              'cmd=ASSEMBLE_DONE' +
              '&base=' + job_info.base +
              '&delay=' + GLC_taskcheck_delay;
    jQuery.get(url, function(status) {
      status = status.trim();
      // If still running, keep checking until the task is done.
      if (status == 'RUNNING') {
        glc_checkTaskDone(uid);
      // Once it is done, go to the next task in the sequence.
      } else if (status == 'SUCCEEDED') {
        GEE_runNextAjaxSequenceItem();
      } else {
        GEE_appendStatus(status);
        glc_cleanUp();
      }
    });
  } else {
    console.error('There is no running job sequence!');
  }
}

// Get size of globe built so far.
function glc_showGlobeSize() {
  if (GEE_interval == '') {
    alert('Got called but GEE interval is empty.');
  }

  if (GEE_running_get_data_sequence) {
    uid = GEE_getUid();
    var job_info = GLC_job_info[uid];
    var url = '/cgi-bin/glc_assembler_app.py?cmd=BUILD_SIZE' +
              '&base=' + job_info.base;
    jQuery.get(url, function(size) {
        div = gees.dom.get('globe');
        if (div.innerHTML.indexOf('Glc is available') < 0) {
          gees.dom.setHtml(div, 'Working... ' + size);
        }
    });
  }
}

// Enable the commit buttons.
function glc_enableGLC() {
  GEE_running_get_data_sequence = false;

  var div = gees.dom.get('status');
  gees.dom.showInline('refresh_button');
  gees.dom.hide('cancel_button');
  div.innerHTML +=
      '<a href="#" onclick="hideBuildProgress()">Close this dialog</a>';
  // Reset main 'Assemble Glc' button to active, allowing user
  // to try again after a failed assembly, or create a new glc
  // after successful assembly.
  gees.dom.setClass('commit_button', 'button blue');
}

// Start the glc assembly process.
function glc_assembleGlc(uid) {
  var job_info = getJobInfo(uid);
  var specJson = createGlcJson();
  var requestUrl = '/cgi-bin/glc_assembler_app.py';
  var params = 'cmd=ASSEMBLE_GLC' +
               '&name=' + job_info.name +
               '&glc_assembly_spec=' +
               gees.tools.geEncodeURIComponent(specJson) +
               '&base=' + job_info.base +
               '&path=' + job_info.path +
               '&overwrite=' + job_info.overwrite;
  jQuery.post(requestUrl, params, successfulBuildHelper);
}

// Deliver some messages if the build completes successfully.
function successfulBuildHelper(response) {
  var div = gees.dom.get('status');
  var response = response.split('\n').join('<br>');
  gees.dom.setHtml(div, response);

  if (GEE_running_get_data_sequence) {
    GEE_runNextAjaxSequenceItem();
  }
}

// Hide build progress dialog.
function hideBuildProgress() {
  gees.dom.hide('BuildProgress');
}

// Show build progress dialog.
function showBuildProgress() {
  gees.dom.show('BuildProgress');
}

// Use the span holding the layer UI elements to figure out which
// layers are selected, and keep track of them.
function getAllLayerElements() {
  layerIdsForJson = [];
  var layerListDiv = gees.dom.get('SelectedLayers');
  var layerList = layerListDiv.getElementsByTagName('span');
  for (var i = 0; i < layerList.length; i++) {
    makeLayerIdArray(layerList[i].childNodes);
  }
}

// Iterate over the display items and get input IDs so the layer
// names and search options can be correctly organized.
function makeLayerIdArray(details) {
  var nameId;
  var checkboxId;
  for (var i = 0; i < details.length; i++) {
    var item = details[i];
    if (item.type == 'text') {
      nameId = item.id;
    }
    if (item.type == 'checkbox') {
      checkboxId = item.id;
    }
  }
  layerIdsForJson.push([nameId, checkboxId]);
}

// Create a json string that can be returned for the glc request.
function createGlcJson() {
  var glcJson = {};
  if (glcIs2d) {
    glcJson.glms = glmLayerInfo();
    glcJson.layers = makeLayersObjectFromGlms();
    glcJson.is_2d = true;
  } else {
    glcJson.layers = makeLayersObject();
    glcJson.base_glb = baseGlobeInfo();
    glcJson.is_2d = false;
  }
  glcJson.name = gees.dom.get('glc_name_field').value;
  glcJson.description = gees.dom.get('glc_desc_field').value;
  glcJson.polygon = gees.dom.get('glc_poly_field').value;
  return JSON.stringify(glcJson);
}

// Clean the submitted kml and bring back a string.  This will run the kml
// through the toGeoJSON library, which will ignore bad/illegal
// characters and allow us to create a kml string we have more control over.
function validateKml(kmlString) {
  var kmlString;
  var kmlObject = getKmlObject(kmlString);
  if (kmlObject && kmlObject.features[0] && kmlObject.features[0].geometry) {
    var kml = kmlObject.features[0].geometry;
    if (kml.geometries) {
      return returnKmlString(largePolygonString(kml.geometries));
    } else {
      return returnKmlString(smallPolygonString(kml));
    }
  } else {
    showBuildProgress();
    var errorMsg = 'There was a problem with ' +
        'your polygon.  Please confirm that it is valid kml.';
    gees.dom.setHtml('status', errorMsg);
    polygonSuccess = false;
    gees.dom.setClass('commit_button', 'button blue');
    gees.dom.hide('cancel_button');
  }
}

// Given a kml object with multiple polygons, return a string of polygons.
function largePolygonString(kml) {
  var polygonString = '';
  for (var i = 0; i < kml.length; i++) {
    var poly = kml[i];
    polygonString += smallPolygonString(poly);
  }
  return polygonString;
}

// Given a kml object with one polygon, return a string containing the polygon.
function smallPolygonString(kml) {
  var polygonString = '';
  var coords = kml.coordinates[0];
  for (var i = 0; i < coords.length; i++) {
    var lat = coords[i][0] + ',';
    var lon = coords[i][1] + ',0 ';
    polygonString += lat + lon;
  }
  lat = coords[0][0] + ',';
  lon = coords[0][1] + ',0';
  polygonString += lat + lon;
  return makePolygonKml(polygonString);
}

// Make a polygon kml element to insert into a kml template.
function makePolygonKml(kml) {
  return '<Polygon><outerBoundaryIs><LinearRing>' +
      '<coordinates>' + kml + '</coordinates></LinearRing>' +
      '</outerBoundaryIs></Polygon>';
}

// Given a string of one or more polygons, return a kml string.
function returnKmlString(polygonString) {
    return '<?xml version="1.0" encoding="UTF-8"?>' +
        '<kml xmlns="http://www.opengis.net/kml/2.2" ' +
        'xmlns:gx="http://www.google.com/kml/ext/2.2" ' +
        'xmlns:kml="http://www.opengis.net/kml/2.2" xmlns:' +
        'atom="http://www.w3.org/2005/Atom"><Placemark><Style>' +
        '<PolyStyle><color>c0800080</color></PolyStyle></Style>' +
        polygonString + '</Placemark></kml>';
}

// Given a kmlString, use toGeoJSON to return an object containing
// each polygon found and its coordinates.
function getKmlObject(kmlString) {
  var myKML;
  var ieVersion = gees.tools.internetExplorerVersion();
  if (ieVersion > -1) {
    var xmlDoc = new ActiveXObject('Microsoft.XMLDOM');
    xmlDoc.async = false;
    xmlDoc.loadXML(kmlString);
    myKML = toGeoJSON.kml(xmlDoc, 'text/xml');
  } else {
    var parser = new DOMParser();
    myKML = toGeoJSON.kml(parser.parseFromString(kmlString, 'text/xml'));
  }
  return myKML;
}

// Helper function for making a layer object of glm layers.
function makeLayersObjectFromGlms() {
  var layers = [];
  for (var i = 0; i < layerIdsForJson.length; i++) {
    var item = layerIdsForJson[i][0];
    var glm = item.split('?')[0];
    var layer = item.split('?')[1].replace(/_name/g, '');
    layers.push(makeGlmLayer(glm, layer, item));
  }
  return layers;
}

// Helper function for making a glm object needed for json.
function makeGlmLayer(glm, layerName, layerId) {
  var layerItem = {};
  layerItem.name = gees.dom.get(layerId).value;
  for (var i = 0; i < availableGlmObject.length; i++) {
    var item = availableGlmObject[i];
    var glmName = item.name;
    if (glm == glmName) {
      var layer = locateGlmLayer(layerName, item.layers);
      layerItem.glmIndex = getGlmIndex(glm);
      layerItem.channel = layer.id;
      layerItem.requestType = layer.requestType;
      layerItem.initiallyVisible = layer.initialState;
      layerItem.isOpaque = layer.opacity != 1;
    }
  }
  return layerItem;
}

// For a layer pulled from a glc, get a layer index.  This is
// the location of a glm in a list of all the currently used glms.
function getGlmIndex(glm) {
  for (var i = 0; i < selectedGlms.length; i++) {
    if (glm == selectedGlms[i]) {
      return i;
    }
  }
}

// Return an existing glm layer to pull some information from it.
function locateGlmLayer(layer, layerList) {
  for (var i = 0; i < layerList.length; i++) {
    var item = layerList[i];
    if (item.label == layer) {
      return item;
    }
  }
}

// Return an object for base globe information.  This function is only
// called while assembling a 3D glc.
function baseGlobeInfo() {
    var select = gees.dom.get('BaseGlobeSelect');
    var baseGlobeName = select.options[select.selectedIndex].value;
    var item = {};
    item.base_glb_path = getLayerPath(baseGlobeName);
    item.has_base_imagery = gees.dom.isChecked('imagery_radio');
    // Checkbox is actually for old imagery, so checked means no proto imagery.
    item.has_proto_base_imagery = gees.dom.get('is_proto_imagery').value == 1;
    item.has_base_terrain = gees.dom.isChecked('terrain_radio');
    return item;
}

// Using an array of elements containing element ids, iterate over them
// and return their associated values.
function makeLayersObject() {
  var layersObject = [];
  // layerIdsForJson is array of items: [nameIdField, checkboxIdField].
  for (var i = 0; i < layerIdsForJson.length; i++) {
    var item = layerIdsForJson[i];
    var layerItem = {};
    layerItem.name = gees.dom.get(item[0]).value;
    layerItem.use_search = gees.dom.isChecked(item[1]);
    layerItem.path = getLayerPath(item[0]);
    layersObject.push(layerItem);
  }
  return layersObject;
}

// Figure out which glms are being used and create an object
// that lists their name, path and search options.
function glmLayerInfo() {
  var glms = [];
  getSelectedGlms();
  for (var i = 0; i < selectedGlms.length; i++) {
    var name = selectedGlms[i];
    var item = {};
    item.name = name;
    item.path = getLayerPath(name);
    item.use_search = checkGlmSearch(name);
    glms.push(item);
  }
  return glms;
}

// For each glm being used, iterate over the individual layers
// being used and see if search is selected.  If so, bring in
// the search options from the source glm.
function checkGlmSearch(name) {
  for (var i = 0; i < layerIdsForJson.length; i++) {
    var checkId = layerIdsForJson[i][1];
    var checked = gees.dom.isChecked(checkId);
    var isOwn = checkId.search(name) != -1;
    if (isOwn && checked) {
      return true;
    }
  }
  return false;
}

// Create a list of glm files being used.
function getSelectedGlms() {
  selectedGlms = [];
  for (var i = 0; i < layerIdsForJson.length; i++) {
    var item = layerIdsForJson[i];
    var name = item[0].split('?')[0];
    var notLogged = selectedGlms.indexOf(name) == -1;
    if (notLogged) {
      selectedGlms.push(name);
    }
  }
}

// Get the path to a globe using its name.
function getLayerPath(layerName) {
  layerName = layerName.replace(/_name/, '');
  for (var i = 0; i < gees.assets.portables.length; i++) {
    var item = gees.assets.portables[i];
    var name = item.name;
    if (name == layerName) {
      return item.path;
    }
  }
}

// Checks that all required form values are complete.
function validateGlcForm() {
  if (gees.dom.get('glc_name_field').value == '' ||
      gees.dom.get('glc_desc_field').value == '' ||
      gees.dom.get('glc_poly_field').value == '') {
    gees.dom.setHtml('ErrorMessage', 'Please complete all fields.');
    glc_clearGlobeInfo();
    return false;
  } else {
    result = gees.tools.isValidName(gees.dom.get('glc_name_field').value);
    if (!result[0]) {
      gees.dom.setHtml('ErrorMessage', result[1]);
      glc_clearGlobeInfo();
      return false;
    } else {
      gees.dom.get('glc_name_field').value = result[1];
      return true;
    }
  }
}

// Allow user to cancel their current progress and start over.
// The button featuring this option (cance_button) is only
// available once layers have been selected.
function cancelGlcBuild() {
  var title = 'Cancel building Glc?';
  var contents = 'Do you want to stop building this Glc and start over?';
  var buttons = '<a href="#" id="refresh_button" onclick="location.reload()"' +
  ' class="button blue">Ok</a><a href="#" id="refresh_button"' +
  ' onclick="closeAllPopups()" class="button standard">Go back</a>';
  gees.tools.toggleNotification(title, contents, buttons);
}

// Close all popup elements.
function closeAllPopups() {
  gees.dom.hide('BuildProgress');
  gees.dom.hide('WhiteOverlay');
  gees.dom.hide('NotificationModal');
}

// Disassemble a glc.
function disassembleGlc() {
  var select = gees.dom.get('GlcSelect');
  var path = select.options[select.selectedIndex].value;
  var outputDirectory = gees.dom.get('output_directory').value;
  var url = '/cgi-bin/glc_assembler_app.py?cmd=DISASSEMBLE_GLC';
  var request = url + '&path=' + path + '&dir=' + outputDirectory;
  var response = gees.requests.doSimplePost(request);
  gees.dom.show('BuildProgress');
  gees.dom.setHtml('status', response);
}

// Update display elements on Glc Disassembly page.
function updateDisassemblyDisplay() {
  var select = gees.dom.get('GlcSelect');
  // Create a path, and take the file extension off the end.
  var option = select.options[select.selectedIndex].value;
  var nameArray = option.split('/');
  var name = nameArray[nameArray.length - 1].split('.')[0];
  var path = '/opt/google/gehttpd/htdocs/cutter/globes/glcs/';
  var button = gees.dom.get('DisassembleButton');
  if (option == '-- select a glc to disassemble --') {
    gees.dom.setClass(button, 'button blue Muted');
    gees.dom.get('output_directory').value = '';
  } else {
    gees.dom.setClass(button, 'button blue');
    // Suggest a path for the unpacked glc files.
    gees.dom.get('output_directory').value = path + name;
  }
}
