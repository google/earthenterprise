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
  This file creates the UI for the Snippet mode of the GEE Server
  Admin UI.  Snippet mode allows for the creation and management of
  Snippet profiles.  There are three other files for the different
  modes of the Server UI, all in this folder:
    - dashboard_view.js
    - database_view.js
    - search_view.js
*/

var currentSnippetListing = [];
// Array of End Snippet descriptions.
// End Snippet description is array of elements [item, rootItem, description,
// enumVals, type, defaultValue].
var availableEndSnippets = [];
// Contains first token of the snippet name, or first token + .dd for
// repeatable snippets.
var inUseEndSnippets = [];
var reusableSnippetList = [];
var editingSnippets = [];
var groupedSnippetSets = [];
// Array of pairs {name, children}, where
// name is first/root element of snippet path after 'end_snippet.'
// children is array of subsnippets (rest of snippet path after root element).
// It is built based on availableEndSnippets and used to manage list of snippets
// available for adding.
var organizedSnippets = [];
var listOfProfiles = [];
var listRadioSnippets = [];
var listSelectSnippets = [];
var requiredItems = [];
var incompleteRequiredFields = [];
var invalidDoubleSnippets = [];
var valuesSnippetHolder = {};
var listForSaving;
var currentProfileName;

// Main initializing of the Snippet view.
function snippetView() {
  gees.admin.mode = gees.admin.modes.snippet;
  gees.admin.switchViewHelper();
  gees.admin.setHeaderLinks();
  gees.dom.clearHtml('SnippetEditor');
  gees.dom.showInline('ExistingSnippets');
  getEndSnippetListing();
  existingSnippetList();
  gees.dom.showInline('CreateSnippetButton');
  loadWelcomeMessage();
  if (gees.dom.get('AvailableEndSnippets')) {
    gees.dom.get('AvailableEndSnippets').focus();
  }
}

// Set a welcome message/ initial state for the Snippet view.
function loadWelcomeMessage() {

  // Clear the UI.  Prepare to insert the welcome message.
  gees.admin.closeAllPopups();
  var editor = gees.dom.get('SnippetEditor');
  gees.dom.showInline(editor);
  gees.dom.clearHtml(editor);

  // Set a message based on whether or not there are snippets available.
  var msg;
  if (!gees.assets.snippets || !gees.assets.snippets.length) {
    msg = '<p>There were no snippet profiles found.  You can create ' +
        'a new one using the button in the upper left.</p>';
  } else {
    msg = '<p>Please select a snippet profile for editing from the menu' +
        ' on the left.';
  }

  // Set and display a welcome message.
  var welcomeDiv = gees.dom.create('div');
  welcomeDiv.className = 'welcomeMessage';
  welcomeDiv.innerHTML = '<h3>Create or update a Snippet Profile</h3>' + msg;
  editor.appendChild(welcomeDiv);
}

// Get a list of all available end snippets that can be used in profiles.
function getEndSnippetListing() {
  availableEndSnippets = [];
  requiredItems = [];
  var snippetMetaJson = '/geserve/Snippets?Cmd=Query&QueryCmd=MetaFieldsSet';
  var snippetListingUrl = GEE_BASE_URL + snippetMetaJson;
  var response = jQuery.parseJSON(gees.requests.doGet(snippetListingUrl));
  if (response.status_code != 0) {
    // TODO: report an error.
    return;
  }
  var metaInfo = response.results;
  // Get the individual items.
  for (var item in metaInfo) {
    var origItem = metaInfo[item];
    item = String(item).split('end_snippet.')[1];
    var rootItem = String(item).split('.')[0];
    var description;
    var enumVals;
    var type;
    var children;
    var defaultValue;
    // Grab some properties of the item.
    for (var prop in origItem) {
      if (prop == 'description') {
        description = origItem[prop];
      }
      if (prop == 'enum_vals') {
        enumVals = origItem[prop];
      }
      // This will determine if this item can be resused/added more than once.
      if (prop == 'empty_concrete_fieldpath') {
        updateReusableList(origItem[prop]);
      }
      if (prop == 'typ') {
        type = origItem[prop];
      }
      if (prop == 'default_value') {
        defaultValue = origItem[prop];
      }
      if (prop == 'js_validation_rule') {
        if (origItem[prop].required) {
          requiredItems.push(item);
        }
      }
    }
    // Push information to our array of available snippets.
    availableEndSnippets.push([
        item,
        rootItem,
        description,
        enumVals,
        type,
        defaultValue]);
  }
}

// Keep track of snippets than can be reused.
function updateReusableList(item) {
  // If it contains a list, it can be reused.
  var hasMults = item.indexOf('[');
  if (hasMults != -1) {
    var itemCheck = String(item).split('end_snippet.')[1];
    itemCheck = String(itemCheck).split('.')[0];
    var isIndex = reusableSnippetList.indexOf(itemCheck);
    // If it is not already in the list of reusable items, add it.
    if (isIndex == -1) {
      reusableSnippetList.push(itemCheck);
    }
  }
}

// Get a list of existing snippet profiles.
function existingSnippetList() {
  currentSnippetListing = [];
  listOfProfiles = [];
  gees.dom.clearHtml('ExistingSnippets');
  // Creating heading for display.
  createSnippetHeader();
  // Iterate over profiles.
  for (var i = 0; i < gees.assets.snippets.length; i++) {
    var item = gees.assets.snippets[i];
    var setName = item.set_name;
    var endSnippet = item.end_snippet.end_snippet;
    var countProps = 0;
    listOfProfiles.push(setName);
    // Create a list of the different snippets used in the profile.
    for (var prop in endSnippet) {
      discoverSnippetItem(setName, prop, endSnippet[prop]);
      countProps += 1;
    }
  }
  fillExistingProfiles();
}

// Create a small alphabetized list of snippet profiles, insert into UI.
function fillExistingProfiles() {
  listOfProfiles.sort();
  for (var i = 0; i < listOfProfiles.length; i++) {
    var setName = listOfProfiles[i];
    // Create display element for each profile.
    gees.dom.get('ExistingSnippets').innerHTML +=
        '<span><a onclick="populateSnippetEditor(\'' + setName +
        '\');" id="Profile_' + setName + '">' + setName +
        '</a><label onclick="deleteSnippetProfile(\'' + setName + '\')">' +
        'delete</delete></span>';
  }
}

// For all existing profiles, push individual snippets
// to an object, or send snippets that belong to a group
// to a function that will handle groups differently.
function discoverSnippetItem(name, prop, value) {
  var valueType = typeof(value);
  if (valueType != 'object') {
    currentSnippetListing.push({
      'parent_set': name,
      'item_key': prop,
      'value': value,
      'status': 'old',
      'isGroup': false,
      'children': ''
    });
  } else {
    var isReusable = reusableSnippetList.indexOf(prop) != -1;
    if (isReusable) {
      discoverReusableSnippetGroup(name, prop, value);
    } else {
      discoverSingleSnippetGroup(name, prop, value);
    }
  }
}

// Find all values within a snippet set, and add an object
// to the current list of snippet profiles.
function discoverSingleSnippetGroup(name, prop, value) {
  var children = [];
  for (var id in value) {
    var groupProp = prop;
    children.push({'name': id, 'value': value[id]});
  }
  currentSnippetListing.push({
    'parent_set': name,
    'item_key': groupProp,
    'value': null,
    'status': 'old',
    'isGroup': true,
    'children': children
  });
}

// Find all values within a snippet set that can be used multiple times.
// Add an object to the list of snippet profiles.
function discoverReusableSnippetGroup(name, prop, value) {
  for (var id in value) {
    var isReusable = reusableSnippetList.indexOf(prop) != -1;
    var groupProp;
    var children = [];
    groupProp = prop + '.' + id;
    var childArray = returnChildItem(value[id]);
    for (var i = 0; i < childArray.length; i++) {
      var childName = childArray[i][0];
      var childValue = childArray[i][1];
      children.push({'name': childName, 'value': childValue});
    }
    currentSnippetListing.push({
      'parent_set': name,
      'item_key': groupProp,
      'value': null,
      'status': 'old',
      'isGroup': true,
      'children': children
    });
  }
}

// Iterate over child items recursively and return an array of the
// names and values.
function returnChildItem(item, parent, array) {
  var array = array || [];
  var parent = parent == undefined ? '' : parent + '.';
  for (var value in item) {
    var type = typeof(item[value]);
    if (type != 'object') {
      array.push([parent + value, item[value]]);
    } else {
      returnChildItem(item[value], parent + value, array);
    }
  }
  return array;
}

// Count the number of times a snippet set appears
// and return the number to determine if a key belongs
// to a group.
function checkIfGroup(key) {
  var rootKey = String(key).split('.')[0];
  var count = 0;
  for (var i = 0; i < availableEndSnippets.length; i++) {
    if (availableEndSnippets[i][1] == rootKey) {
      count += 1;
    }
  }
  return count;
}

// Fill the editor with items to edit/view.
function populateSnippetEditor(profileName, editing) {
  // If editing is passed it will be true.  If undefined, set to false.
  editing = editing || false;
  if (profileName != currentProfileName) {
    editingSnippets = [];
  }
  // Update header display element.
  snippetEditorHeader(profileName);
  // Clear array of snippets we are currently using.
  inUseEndSnippets = [];
  // Iterate over our previously created array to create the form.
  createEditorDisplayElements(
        fillEditorObject(profileName), profileName, editing);
  finishPopulateSnippetEditor(profileName);
}

// Iterate over snippet items and prepare to insert then into the UI.
function createEditorDisplayElements(snippets, profileName, editing) {
  listForSaving = snippets;
  if (snippets.length == 0) {
    gees.dom.get('SnippetEditor').innerHTML +=
        '<span id="NoSnippetsFound">There are currently no snippets in' +
        ' this profile</span>';
  }
  // If currently editing, place newly added snippets at the top of the
  // display.  If loading or saving snippet set (!editing), sort the
  // list of snippets alphabetically by name (item_key).
  if (!editing) {
    snippets.sort(gees.tools.sort.asc('item_key'));
  }
  for (var i = 0; i < snippets.length; i++) {
    // Give spans even/odd class types.
    var spanClass = i & 1 ? 'OddSnippet' : 'EvenSnippet';
    var item = snippets[i];
    spanClass = item.status == 'new' ? 'NewClass' : spanClass;
    var profile = item.parent_set;
    var key = item.item_key;
    var value = item.value;
    var isGroup = item.isGroup;
    var children = item.children;
    // We are iterating over an array of all profile snippets.  This grabs
    // only those that belong to the profile we are editing.
    if (profile == profileName) {
      insertEditorDisplayElements(key, isGroup, value, children, spanClass);
    }
  }
}

// Determine if an item belongs to a group or not, and insert into UI.
function insertEditorDisplayElements(key, isGroup, val, children, spanClass) {
  var storageKey = GetStorageKey(key);
  // For each snippet item, add to an array of items that are
  // being used.  Do not add if it has already been recorded.
  if (!isUsedEndSnippet(storageKey)) {
    inUseEndSnippets.push(storageKey);
  }
  // Create a 'clean key' that can be used to search for description
  // and enum values.  If enumVal is not null, it will be used to create
  // a dropdown as opposed to a text input in the snippet editor.
  var cleanKey = cleanKeyName(key);
  var detailsArray = getDetailsArray(cleanKey);
  var description = detailsArray[0];
  var enumVals = detailsArray[1];
  var type = detailsArray[2];
  var defaultVal = detailsArray[3];
  var deleteLink;
  if (isGroup) {
    deleteLink = '';
    createSnippetEditorGroup(
      spanClass,
      key,
      children,
      deleteLink);
  } else {
    var div = gees.dom.get('SnippetEditor');
    deleteLink = '<a href="#" onclick="deleteEditingSnippet(\'' +
        key + '\')">delete</a>';
    createSnippetEditorItem(
        spanClass,
        description,
        key,
        val,
        enumVals,
        type,
        div,
        deleteLink,
        defaultVal);
  }
}

// Create a main display element to hold a group of snippets.
function createSnippetEditorGroup(spanClass, key, children, deleteLink) {
  gees.dom.get('SnippetEditor').innerHTML +=
        '<span class="' + spanClass + '" title="">' +
        '<label class="GroupHeading">' + key + ' (group)</label>' +
        '<a href="#" onclick="deleteEditingSnippet(\'' +
        key + '\')">delete</a>' +
        '<div id="' + key + '"> </div>' +
        '</span>';
  getChildrenContent(children, key, spanClass, deleteLink);
}

// Delete a snippet from a profile that is being edited.
function deleteEditingSnippet(key) {
  // We have two arrays of snippets: those that are already in the
  // profile, and those that have been added while editing.
  deleteSnippetListing(key, currentSnippetListing);
  deleteSnippetListing(key, editingSnippets);
}

// Find and remove a snippet from a profile.  Update the screen
// to show the updated profile.
function deleteSnippetListing(key, array) {
  for (var i = 0; i < array.length; i++) {
    var item = array[i];
    if (key == item.item_key && currentProfileName == item.parent_set) {
      array.splice(i, 1);
      break;
    }
  }
  updateSnippetEditorValues();
  populateSnippetEditor(currentProfileName);
  holdSnippetEditorValues();
}

// Get the content of a specific child element and prepare
// to append it to the display.
function getChildrenContent(children, key, spanClass, deleteLink) {
  var div = gees.dom.get(key);
  // Sort children belonging to a snippet group by name.
  children.sort(gees.tools.sort.asc('name'));
  for (var i = 0; i < children.length; i++) {
    var name = children[i].name;
    var value = children[i].value;
    var cleanKey = cleanKeyName(key + '.' + name);
    var detailsArray = getDetailsArray(cleanKey);
    var description = detailsArray[0];
    var enumVals = detailsArray[1];
    var type = detailsArray[2];
    var defaultVal = detailsArray[3];
    createSnippetEditorItem(spanClass, description, name, value, enumVals,
        type, div, deleteLink, defaultVal);
  }
}

// Add a snippet editor item to the display.  The element will either be added
// to the main list, or as a sub-item to a group, depending on which 'div'
// argument is passed.
function createSnippetEditorItem(
    spanClass, desc, key, value, val, type, div, deleteLink, defaultVal) {
  if (val != null && type != 'bool') {
    createSnippetSelect(
        spanClass, desc, key, value, val, div, deleteLink, defaultVal);
  } else if (type == 'bool') {
    createSnippetRadio(
        spanClass, desc, key, value, div, deleteLink, defaultVal);
  } else {
    createSnippetInput(
        spanClass, desc, key, div, value, deleteLink, defaultVal);
  }
}

// Find all snippets that belong to a profile, along with any
// snippets that have been added for edit, and create a master
// list/object of all these items.
function fillEditorObject(profileName) {
  var editorObject = [];
  for (var i = 0; i < editingSnippets.length; i++) {
    if (editingSnippets[i].parent_set == profileName) {
      editorObject.push(editingSnippets[i]);
    }
  }
  for (var i = 0; i < currentSnippetListing.length; i++) {
    if (currentSnippetListing[i].parent_set == profileName) {
      editorObject.push(currentSnippetListing[i]);
    }
  }
  return editorObject;
}

// Gets storage key for inUseEndSnippets list based on information about
// snippet.
// If it is a repeatable snippet then a storage key is first/root element of
// snippet path + ".dd" ("dd" is an index of repeatable element), otherwise
// a storage key is first/root element of snippet path.
function GetStorageKey(key) {
  var keyTokens = String(key).split('.');
  var storageKey;
  if (reusableSnippetList.indexOf(keyTokens[0]) != -1) {
    storageKey = keyTokens[0] + '.' + keyTokens[1];
  } else {
    storageKey = keyTokens[0];
  }
  return storageKey;
}

// Some display tasks when the snippet editor has finished opening.
function finishPopulateSnippetEditor(profileName) {
  var message = 'You are now editing profile: <b>' + profileName + '</b>';
  currentProfileName = profileName;
  selectedLinkHelper();
  gees.dom.showInline('SaveSnippet');
  gees.dom.showInline('CancelSnippet');
  gees.dom.showInline('ExistingSnippets');
  gees.dom.showInline('SnippetEditor');
  availableSnippetDropdown();
  gees.tools.butterBar(message, false);
}

// Update the left menu (list of snippet profiles) to highlight
// the profile that is currently being edited.
function selectedLinkHelper() {
  for (var i = 0; i < listOfProfiles.length; i++) {
    gees.dom.setClass('Profile_' + listOfProfiles[i], '');
  }
  gees.dom.setClass('Profile_' + currentProfileName, 'SelectedSnippet');
}

// Send arguments to create a snippet listing with radio options.
function createSnippetRadio(
    spanClass, desc, key, value, div, deleteLink, defaultVal) {
  listRadioSnippets.push(key);
  var isTrue =
      value == true || value == 'True' || value == 'true' ? ' checked' : '';
  var isFalse = isTrue ? '' : ' checked';
  var radioName = div.id + '.' + key;
  div.innerHTML +=
      '<span class="' + spanClass + '"' +
      '" name="SnippetSpan"><label>' + key.split(':value')[0] + '</label>' +
      '<div class="DescTip">?<label>' + desc + '</label></div>' +
      '<input type="radio" name="' + radioName +
      '" id="' + radioName + '"' + isTrue + '>True' +
      '&nbsp;&nbsp;&nbsp;&nbsp;' +
      '<input type="radio" name="' + radioName +
      '"' + isFalse + '>False' +
      deleteLink + '</span>';
}

// Send arguments to create a snippet listing with a standard input.
function createSnippetInput(
    spanClass, desc, key, div, value, deleteLink, defaultVal) {
  var selectClass = div.id + '.' + key;
  var isRequired = checkIfRequired(selectClass);
  value = returnSnippetValue(value, defaultVal, key);
  // If field is required, no value has been entered, and the
  // default returned is either null or 0.0, make the field
  // empty so that the user will have to enter a value.
  if (value == 'null' || value == 0.0) {
    value = isRequired ? '' : value;
  }
  var requiredDisplayElement = isRequired ? '<em>*</em>' : '';
  div.innerHTML +=
      '<span class="' + spanClass + '"' +
      '" name="SnippetSpan"><label>' + key.split(':value')[0] +
      '</label><div class="DescTip">?<label>' +
      desc + '</label></div>' +
      '<input type="text" value="' +
      value + '" placeholder="enter value here" id="' + selectClass + '"/>' +
      deleteLink + requiredDisplayElement + '</span>';
}

// Determine if an item is required or not and return boolean.
function checkIfRequired(item) {
  var itemCheck = item.split('SnippetEditor.');
  if (itemCheck.length > 1) {
    itemCheck.splice(0, 1);
  }
  itemCheck = itemCheck[0];
  var isGroup = checkIfGroup(itemCheck);
  if (isGroup > 1) {
    itemCheck = rearrangeGroupItem(itemCheck.split('.'));
  }
  // Return true if item is found in requiredItems list.
  return requiredItems.indexOf(itemCheck) != -1;
}

// Send arguments to create a snippet listing with a select/dropdown menu.
function createSnippetSelect(
    spanClass, desc, key, value, enumVals, div, deleteLink, defaultVal) {
  listSelectSnippets.push(key);
  value = returnSnippetValue(value, defaultVal, key);
  var selectClass = div.id + '.' + key;
  div.innerHTML +=
      '<span class="' + spanClass + '"' +
      '" name="SnippetSpan"><label>' + key.split(':value')[0] + '</label>' +
      '<div class="DescTip">?<label>' + desc + '</label></div>' +
      '<select id="' + selectClass + '" id="' + selectClass +
      'name="' + selectClass + '"></select>' +
      deleteLink + '</span>';
  snippetSelectHelper(key, value, enumVals, selectClass);
}

// Populate the select menu of a snippet item.
function snippetSelectHelper(key, value, enumVals, selectClass) {
  var select = gees.dom.get(selectClass);
  gees.dom.showInline(select);
  var mainOption = gees.dom.create('option');
  gees.dom.setHtml(mainOption, value || 'select a value');
  mainOption.setAttribute('selected', true);
  select.appendChild(mainOption);
  for (var prop in enumVals) {
    var option = gees.dom.create('option');
    gees.dom.setHtml(option, prop);
    select.appendChild(option);
  }
}

// Take a value and default value, and decide which should be displayed.
function returnSnippetValue(value, defaultValue, key) {
  if (value == null || value == 'select a value' ||
      value == undefined || value == '') {
    value = defaultValue;
  }
  // Default value may have been assigned, and may be null.  Do not show null.
  value = value == null || value == 'null' ? '' : value;
  return value;
}

// Create a clean key name.  This means removing erroneous content
// from the string if it belongs to a reusable Snippet group.
// Example cobrand.00000.name becomes cobrand.name.  Clean key is then
// compared to the master list to retrieve description, type, etc.
function cleanKeyName(name) {
  var isMult = reusableSnippetList.indexOf(String(name).split('.')[0]);
  var cleanName;
  if (isMult == -1) {
    cleanName = name;
  } else {
    cleanName = String(name).split('.')[0];
    var object = String(name).split('.');
    for (var i = 2; i < object.length; i++) {
      cleanName += '.' + object[i];
    }
  }
  return cleanName;
}

// Return the description and enumerated values (if they exist)
// for a particular snippet item key.
function getDetailsArray(key) {
  for (var i = 0; i < availableEndSnippets.length; i++) {
    var item = availableEndSnippets[i];
    var description;
    var enumVals;
    var type;
    var defaultVal;
    if (key == item[0]) {
      description = item[2];
      enumVals = item[3];
      type = item[4];
      defaultVal = item[5];
    }
  }
  return [description, enumVals, type, defaultVal];
}

// Set header display for snippet view.
function snippetEditorHeader(name) {
  gees.dom.get('SnippetEditor').innerHTML = '<h1>Snippet editor: ' +
      name + '</h1>' + '<input type="submit" id="CancelSnippet"' +
      ' class="StandardButton" value="Cancel" ' +
      'onclick="cancelSnippetEditing()" style="display: block;">' +
      '<input type="submit" id="SaveSnippet"' +
      ' class="StandardButton BlueButton" onclick="saveSnippetProfile()"' +
      ' style="display: block;" value="Save changes">' +
      '<select id="AvailableEndSnippets"' +
      ' onchange="addSnippetToCurrentEdit()"></select>';
  gees.dom.show('AvailableEndSnippets');
}

// Populate a dropdown of available snippet items.
function availableSnippetDropdown() {
  availableEndSnippets.sort(gees.tools.sort.asc(0));
  var dropDown = gees.dom.get('AvailableEndSnippets');
  organizeEndSnippets(availableEndSnippets);
  var firstOption = gees.dom.create('option');
  gees.dom.setHtml(firstOption, 'Add a new snippet set to the profile');
  firstOption.setAttribute('selected', true);
  dropDown.appendChild(firstOption);
  for (var i = 0; i < organizedSnippets.length; i++) {
    var item = organizedSnippets[i].name;
    var option = gees.dom.create('option');
    option.setAttribute('id', 'option_' + i);

    // Note: name in organizedSnippets is only first/root element of snippet
    // path, so repeated snippets are determined not used based on root-name.
    if (isUsedEndSnippet(item)) {
      option.setAttribute('disabled', true);
      item += '&#x2713';
    }
    // Helper function to clean names with a ':value' string at the end.
    item = sanitizeSnippetDisplayName(item);
    gees.dom.setHtml(option, item);
    dropDown.appendChild(option);
  }
}

// Changes a snippet name 'snippetname:value' to 'snippetname' and adds name
// (snippetname) to an array of items.  This array keeps track of items that
// originally had the ':value' string at the end.
function sanitizeSnippetDisplayName(name) {
  var itemArray = name.split(':value');
  name = itemArray[0];
  if (itemArray.length > 1) {
    valuesSnippetHolder[name] = true;
  }
  return name;
}

// Given object containing all possible snippets, begin to organize the list.
// End goal is to find groups, and organize as [group-name, children-snippets].
function organizeEndSnippets(object) {
  var organizedList = [];
  for (var i = 0; i < object.length; i++) {
    var snippetRoot = String(object[i][0]).split('.')[0];
    discoverChildSnippets(snippetRoot);
  }
}

// Using a base item (ex. cobrand_info), find all of its children,
// and store it so we don't re-record its children.
function discoverChildSnippets(item) {
  var isLogged = groupedSnippetSets.indexOf(item);
  if (isLogged == -1) {
    groupedSnippetSets.push(item);
    retrieveChildSnippets(item);
  }
}

// Find all the children belonging to a snippet group. Push the group name
// and an array of all children to organizedSnippets object.
function retrieveChildSnippets(item) {
  var children = [];
  for (var i = 0; i < availableEndSnippets.length; i++) {
    var rootItem = availableEndSnippets[i][1];
    var childItem = availableEndSnippets[i][0];
    if (item == rootItem) {
      var cleanedItem = String(childItem).split(rootItem + '.')[1] || null;
      children.push(cleanedItem);
    }
  }
  children = children == null || children[0] == null ? 0 : children;
  organizedSnippets.push({'name': item, 'children': children});
}

// Add a new snippet to the current profile for editing.
function addSnippetToCurrentEdit() {
  updateSnippetEditorValues();
  var select = gees.dom.get('AvailableEndSnippets');
  var snippet = select.options[select.selectedIndex].value;
  // If snippet originally had ':value' at the end of its name, add it back
  // so its information is pulled correctly from the database.
  if (valuesSnippetHolder[snippet]) {
    snippet = snippet + ':value';
  }
  var snippetObject = returnSnippetObject(snippet);
  var name = String(snippetObject.name);
  var isGroup = snippetObject.children.length != undefined;
  var isReusable = reusableSnippetList.indexOf(name) != -1;
  var children = 0;
  if (isGroup) {
    children = getSnippetChildren(snippetObject.children);
  }
  if (isReusable) {
    name += countReusableItems(name);
  }
  // Add the snippet to the beginning of the editing list, which will
  // be merged with the list of existing snippets.
  editingSnippets.unshift({
    'parent_set': currentProfileName,
    'item_key': name,
    'value': null,
    'status': 'new',
    'isGroup': isGroup,
    'children': children
  });
  populateSnippetEditor(currentProfileName, true);
  holdSnippetEditorValues();
}

// Create an array of children name:value pairs.
function getSnippetChildren(array) {
  var children = [];
  for (var i = 0; i < array.length; i++) {
    children.push({'name': array[i], 'value': ''});
  }
  return children;
}

// Find an item in our organizedSnippets object using its name,
// return that object for parsing.
function returnSnippetObject(snippet) {
  var item;
  for (var i = 0; i < organizedSnippets.length; i++) {
    if (snippet == organizedSnippets[i].name) {
      item = organizedSnippets[i];
    }
  }
  return item;
}

// Helper function for dynamic numbering of reusable snippets.
// Example, determine if it is cobrand_info.00001,00002, etc.
function countReusableItems(key) {
  for (var i = 0; i < availableEndSnippets.length; i++) {
    if (key == availableEndSnippets[i][1]) {
      var tempKey = availableEndSnippets[i][0];
      var keyCount = countReusableKeys(key);
      var newKey = getNewSnippetKey(tempKey, keyCount);
    }
  }
  return newKey;
}

// Create a number to refer to a reusable snippet that is in use.
function getNewSnippetKey(key, count) {
  var array = String(key).split('.');
  return '.' + ('0' + count).slice(-2);
}

// Check if item is already added to snippet set.
function isUsedEndSnippet(key) {
  return inUseEndSnippets.indexOf(key) != -1;
}


// Count how many instances of a reusable item are currently in use.
function countReusableKeys(key) {
  var count = 0;
  for (var i = 0; i < inUseEndSnippets.length; i++) {
    if (key == String(inUseEndSnippets[i]).split('.')[0]) {
      count += 1;
    }
  }
  return count;
}

// Create display element for snippet view.
function createSnippetHeader() {
  gees.dom.setHtml('ExistingSnippets', '<h1>Existing snippet profiles</h1>');
}

// Cancel out of editing and start over.
function cancelSnippetEditing() {
  var previousSnippetProfile = currentProfileName;
  currentProfileName = '';
  editingSnippets = [];
  currentSnippetListing = [];
  existingSnippetList();
  loadWelcomeMessage();
}

// Save (or update) a snippet profile.  This process consists of assembling
// Json for simple snippets (some_node: value) and more complex snippets
// (some_node.03.child_1.child_2 = value). The result will be an encoded
// Json object sent as a post request.
function saveSnippetProfile() {
  incompleteRequiredFields = [];
  invalidDoubleSnippets = [];
  // Create the foundation for the object.
  var profileDef = {
        end_snippet: {}
      };
  for (var i = 0; i < listForSaving.length; i++) {
    var id = listForSaving[i].item_key;
    var children = listForSaving[i].children;
    // Call additional function for items with children.
    if (typeof(children) == 'object') {
      var parentName = String(id).split('.');
      addChildNodes(profileDef.end_snippet, children, id);
    // Add items with no children to the tree.
    } else {
      var elementId = gees.dom.get('SnippetEditor.' + id);
      var value = elementId.value;
      value = checkIfRadio(id, elementId, value);
      value = checkIfSelect(id, elementId, value);
      checkSnippetRequiredness(id, value);
      confirmSnippetType(id, value);
      profileDef.end_snippet[id] = value;
    }
  }
  if (incompleteRequiredFields.length != 0 ||
      invalidDoubleSnippets.length != 0) {
    notifyIncorrectFields();
  } else {
    var encodedJson = JSON.stringify(profileDef);
    encodedJson = encodedJson.replace(/"/g, '%22');
    encodedJson = encodedJson.replace(/ /g, '%20');
    newSnippetProfile(encodedJson);
  }
}

// Notify the user if there are required fields that have been left
// incomplete, or if there are fields that contain invalid values.
function notifyIncorrectFields() {
  var msg = '';
  if (incompleteRequiredFields.length > 0) {
    msg += '<b>Please complete the following required field(s):</b>';
    msg += snippetFormErrorMessage(incompleteRequiredFields);
  }
  if (invalidDoubleSnippets.length > 0) {
    msg += '<b>Please enter a number for the following field(s):</b>';
    msg += snippetFormErrorMessage(invalidDoubleSnippets);
  }
  gees.tools.errorBar(msg);
}

// Create an error message if the Snippet editor is incomplete or contains
// invalid values.
function snippetFormErrorMessage(formElementIds) {
  var msg = '';
  for (var i = 0; i < formElementIds.length; i++) {
    var id = formElementIds[i];
    var isGroup = checkIfGroup(id);
    // isGroup returns as number of items - more than 1 is a group.
    if (isGroup == 1) {
      gees.tools.errorInput(gees.dom.get('SnippetEditor.' + id));
    } else {
      gees.tools.errorInput(gees.dom.get(id));
    }
    msg += '<br><i>' + id + '</i>';
  }
  msg += '<br><br>';
  return msg;
}

// If an item is represented with a radio button, return the 'checked'
// property instead of the 'value' property.  This boolean can then
// set the value of the radio element in the UI.
function checkIfRadio(id, radio, value) {
  var isRadio = listRadioSnippets.indexOf(id);
  var value = isRadio == -1 ? value : gees.dom.isChecked(radio);
  return value;
}

// If it is a select menu, extra steps are needed for compatability with IE8.
function checkIfSelect(id, select, value) {
  var isSelect = listSelectSnippets.indexOf(id);
  if (isSelect != -1) {
    value = select.options[select.selectedIndex].value;
  }
  return value;
}

// Get the value of a child snippet.
function addChildNodes(parentNode, children, id) {
  for (var i = 0; i < children.length; i++) {
    var name = children[i].name;
    var value = gees.dom.get(id + '.' + name).value;
    var elementId = gees.dom.get(id + '.' + name);
    value = checkIfRadio(name, elementId, value);
    value = checkIfSelect(name, elementId, value);
    checkSnippetRequiredness(id + '.' + name, value);
    confirmSnippetType(id + '.' + name, value);
    var keyName = id + '.' + name;
    var childArray = String(keyName).split('.');
    addChildHelper(childArray, value, parentNode);
  }
}

// Determine whether or not a field is required and, if so,
// determine whether or not a value has been entered.
function checkSnippetRequiredness(item, value) {
  var itemArray = item.split('.');
  var isReusable = reusableSnippetList.indexOf(itemArray[0]);
  var newItem = item;
  if (isReusable != -1) {
    newItem = rearrangeGroupItem(itemArray);
  }
  var isRequired = requiredItems.indexOf(newItem);
  // If no value for required field, push to an array of incomplete
  // items to be referenced elsewhere.
  if (isRequired != -1 && value == '') {
    incompleteRequiredFields.push(item);
  }
}

// Check for fields with type 'double'.  Cancel editing and throw error if a
// field with this type does not contain a number.  Allowing non-number values
// in this field can cause the profile to break the publish process.
function confirmSnippetType(item, value) {
  var isGroup = checkIfGroup(item) != 1;
  var newItem = isGroup ? rearrangeGroupItem(item.split('.')) : item;
  for (var i = 0; i < availableEndSnippets.length; i++) {
    var snippetItem = availableEndSnippets[i];
    if (snippetItem[0] == newItem && snippetItem[4] == 'double') {
      if (isNaN(value)) {
        invalidDoubleSnippets.push(item);
      }
    }
  }
}

// This function accepts a string similar to:
// groupname.00001.item.subitem and sanitizes it to
// groupname.item.subitem, so that we can see if this 'repeatable'
// item field is in our list of required fields.
function rearrangeGroupItem(item) {
  item.splice(1, 1);
  var newItem = '';
  var delimiter = '';
  for (var i = 0; i < item.length; i++) {
    newItem += delimiter + item[i];
    delimiter = '.';
  }
  return newItem;
}

// Add one, or several objects recursively to the tree of snippets.
function addChildHelper(childArray, value, parentNode) {
  if (childArray.length == 1) {
    parentNode[childArray[0]] = value;
  } else {
    if (!parentNode[childArray[0]]) {
      parentNode[childArray[0]] = {};
    }
    var newParentNode = parentNode[childArray[0]];
    childArray.splice(0, 1);
    addChildHelper(childArray, value, newParentNode);
  }
}

// This function can either update an existing Snippet profile (if jsonString
// is passed) or create a new one (if jsonString is not passed).  If the
// profile is new, it cannot overwrite an existing profile.
function newSnippetProfile(jsonString) {
  var profileName;
  var successTerm;
  if (jsonString) {
    profileName = currentProfileName;
    successTerm = 'updated';
  } else {
    profileName = gees.dom.get('NewProfileName').value;
    successTerm = 'created';
    // Stop the user from overwriting an existing profile.
    for (var i = 0; i < gees.assets.snippets.length; i++) {
      var currentProfiles = gees.assets.snippets[i];
      if (currentProfiles.set_name == profileName) {
        msg = 'Snippet profile name <b>' + profileName +
            '</b> is already in use.  Please choose another, or cancel.';
        gees.tools.errorBar(msg);
        return;
      }
    }
  }
  jsonString = jsonString || '{%22end_snippet%22:%20{}}';
  var requestUrl = '/geserve/Snippets?';
  var params = 'Cmd=AddSnippetSet&SnippetSetName=';
  params += profileName + '&SnippetSet=' + jsonString;
  var response = jQuery.parseJSON(gees.requests.doPost(requestUrl, params));
  var msg;
  if (response.status_code == 0) {
    msg = 'Profile <b>' + profileName + '</b> was ' + successTerm;
    gees.initialize.snippets();
    editingSnippets = [];
    snippetView();
    gees.tools.butterBar(msg);
  } else {
    msg = 'An error occurred while updating this profile.';
    gees.tools.errorBar(msg);
  }
  gees.dom.get('NewProfileName').value = '';
}

// Delete a snippet profile from the database.
function deleteSnippetProfile(name, confirmDelete) {
  if (confirmDelete) {
    var deleteRequest = GEE_BASE_URL +
        '/geserve/Snippets?Cmd=DeleteSnippetSet&SnippetSetName=' + name;
    var response = jQuery.parseJSON(gees.requests.doGet(deleteRequest));
    var msg;
    if (response.status_code == 0) {
      msg = 'Profile <b>' + name + '</b> was successfully deleted';
      gees.initialize.snippets();
      currentProfileName = undefined;
      snippetView();
      gees.tools.butterBar(msg);
    } else {
      msg = 'There was an error deleting the profile';
      gees.tools.errorBar(msg);
    }
  } else {
    popupTitle = 'Delete profile: ' + name;
    popupContents = 'Are you sure you want to delete this snippet ' +
        'profile?  This action cannot be undone';
    popupButtons = '<a class="StandardButton BlueButton"' +
        ' onclick="deleteSnippetProfile(\'' + name + '\', \'' +
          true + '\')">Yes</a>' +
        '<a class="StandardButton"' +
        ' onclick="gees.admin.closeAllPopups();snippetView()">Cancel</a>';
    gees.tools.toggleNotification(popupTitle, popupContents, popupButtons);
  }
}

// Helps retain the values in the editor when the list is refreshed.
function updateSnippetEditorValues() {
  holderForEditorValues = [];
  for (var i = 0; i < listForSaving.length; i++) {
    var parentKey = listForSaving[i].item_key;
    var children = listForSaving[i].children;
    if (typeof(children) == 'object') {
      storeSnippetChildValue(children, parentKey);
    } else {
      var id = 'SnippetEditor.' + parentKey;
      var value = gees.dom.get(id).value;
      holderForEditorValues.push({'id': id, 'value': value});
    }
  }
}

// Given a parent node and its children, store the values of all items.
function storeSnippetChildValue(children, parentKey) {
  for (var i = 0; i < children.length; i++) {
    var id = parentKey + '.' + children[i].name;
    var value = gees.dom.get(id).value;
    holderForEditorValues.push({'id': id, 'value': value});
  }
}

// Refill form fields with their previous values after a refresh.
function holdSnippetEditorValues() {
  for (var i = 0; i < holderForEditorValues.length; i++) {
    var id = holderForEditorValues[i].id;
    var value = holderForEditorValues[i].value;
    var idArray = id.split('.');
    // Check if it is a select menu.  If so, the element.value property will
    // not work in older version of IE.
    var isSelect = listSelectSnippets.indexOf(idArray[idArray.length - 1]);
    if (gees.dom.get(id)) {
      if (isSelect == -1) {
        gees.dom.get(id).value = value;
      } else {
        var select = gees.dom.get(id);
        select.options[select.selectedIndex].value = value;
      }
    }
  }
}
