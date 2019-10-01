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
/**
 * Namespace for all functions used for the search view
 * within the admin mode of server.
 */
gees.admin.searchView = {

  // Get the list of Search Defs from the server, then refresh
  // the list of items on the page.
  onRefresh: function(newItem) {
    gees.initialize.searchDefs();
    this.itemList.sortAndLoad(false, true, newItem);
  },

  // List the available Search defs in the UI.
  itemList: {

    // Hold the last update time.
    timeOfLastUpdate: (new Date()).toString(),

    init: function() {
      this.currentSortMethod = 'sortName';
      this.currentSortDirection = gees.admin.sort.ASCENDING_SORT;
      return this;
    },

    // Refresh the list of items in the Search View.
    refresh: function(refreshLastUpdate, newItem) {
      // Reset UI elements and create the header for the list.
      gees.admin.resetUI();
      this.header.create();

      // Create a list item for each search definition.
      for (var i = gees.admin.paging.min; i < gees.admin.paging.max; i++) {
        var searchDef = gees.assets.searchDefs[i];
        this.createItem(searchDef, i, newItem);
      }

      // Make a note of the last update time & set sort icons.
      this.createLastUpdate(refreshLastUpdate);
      gees.tools.setSortIcons(this.currentSortMethod);
    },

    // Sorts the item list by the sortMethod passed as an argument.
    sortAndLoad: function(sortMethod, refresh, newItem) {
      var list = gees.assets.searchDefs;
      gees.admin.sort.sortItemList(list, sortMethod, this);

      // Refresh the list and set the sort icons.
      this.refresh(refresh, newItem);
    },

    // Remember the current sort state in the listView object.
    setSortProperties: function(sortMethod, sortDirection) {
      this.currentSortMethod = sortMethod;
      this.currentSortDirection = sortDirection;
    },

    // Create a UI element for a Search Definition item.
    createItem: function(searchDef, index, recentlyAddedDef, itemMenu) {
      // Add some properties to the searchDef.
      searchDef = this.appendSearchDefDisplayProperties(searchDef);

      // Create the list item.
      this.element.createHolder(index, searchDef);

      // If the recentlyAddedDef is true, it is because a Search Definition has
      // just been edited or created. If so, highlight that element in the UI.
      if (recentlyAddedDef == searchDef.name) {
        this.highlightItem(index);
      }
    },

    // Add display properties to the Search Definition item.
    appendSearchDefDisplayProperties: function(searchDef) {
      // Determine if search tab has query or config parameters.
      var hasQuery = searchDef.query != null && searchDef.query != '';
      var hasConfig = searchDef.config != null && searchDef.config != '';

      // Display properties that will be used by the item list.
      searchDef.hasQueryParams = hasQuery ? 'Yes' : 'No';
      searchDef.hasConfigParams = hasConfig ? 'Yes' : 'No';
      searchDef.isSystemTab = searchDef.isSystem ? 'Yes' : 'No';
      return searchDef;
    },

    // Highlight a list item.
    highlightItem: function(id) {
      setTimeout(function() {
        gees.dom.addClass('div_' + id, 'item_hold');
      }, 300);
      setTimeout(function() {
        gees.dom.setClass('div_' + id, 'ItemInline');
      }, 1500);
    },

    // Create a UI element that shows the last time the list was udpated.
    createLastUpdate: function(refresh) {
      if (refresh) {
        this.timeOfLastUpdate = (new Date()).toString();
      }
      var parent = gees.dom.get('MenuRight');
      var date = this.timeOfLastUpdate;
      var lastUpdate = gees.dom.create('div');
      lastUpdate.id = 'LastUpdate';
      lastUpdate.innerHTML = 'Last Updated: ' + date;
      parent.appendChild(lastUpdate);
    },

    // Functions for creating elements that together compose one list item.
    element: {

      // Create a holder that contains all elements of a list item.
      createHolder: function(index, searchDef) {
        var listItem = gees.dom.create('div');
        var clickArg = 'checkbox_' + index;
        var onclick = 'gees.admin.checkboxes.clickBox(\'' + clickArg + '\')';
        listItem.setAttribute('onclick', onclick);
        listItem.className = 'ItemInline';
        listItem.id = 'div_' + index;

        // Add child elements to holder and append it to the main item menu.
        this.createCheckbox(listItem, index);
        this.createSpans(listItem, searchDef);

        // Append the list item to the list element.
        gees.dom.get('MenuRight').appendChild(listItem);
      },

      // Create a checkbox for the list item.
      createCheckbox: function(parent, index) {
        var checkbox = gees.dom.create('input');
        checkbox.name = 'dbCheck';
        checkbox.className = 'dbCheck frontPageCheck';
        checkbox.type = 'checkbox';
        checkbox.id = 'checkbox_' + index;
        var onclick =
            'gees.admin.searchView.toggleItem(\'' + index + '\')';
        checkbox.setAttribute('onclick', onclick);
        parent.appendChild(checkbox);
      },

      // Create the spans for a list item that contain the various properties
      // of the Search Definition.
      createSpans: function(parent, searchDef) {
        this.createPropertySpan(parent, searchDef.name);
        this.createPropertySpan(parent, searchDef.url);
        this.createPropertySpan(parent, searchDef.label);
        this.createPropertySpan(parent, searchDef.isSystemTab);
        this.createPropertySpan(parent, searchDef.hasQueryParams);
        this.createPropertySpan(parent, searchDef.hasConfigParams);
      },

      // Create a span for a property of a Search Definition list item.
      createPropertySpan: function(parent, content) {
        var span = gees.dom.create('span');
        span.id = 'gName';
        span.innerHTML = content;
        parent.appendChild(span);
      }
    },

    // Controls functions for creating the header element of the item list.
    header: {

      // Create the header element of the item list.
      create: function() {
        var listItem = gees.dom.create('div');
        listItem.className = 'ItemInlineLegend';
        listItem.id = 'legend';

        // Add child items to the header.
        this.createMasterCheckbox(listItem);
        this.createSpans(listItem);
        gees.dom.get('MenuRight').appendChild(listItem);
      },

      // Create a master checkbox for the header that toggles all list items.
      createMasterCheckbox: function(parent) {
        var checkbox = gees.dom.create('input');
        checkbox.id = 'masterCheckBox';
        checkbox.type = 'checkbox';
        checkbox.className = 'frontPageCheck';
        var onclick = 'gees.admin.checkboxes.toggleAll()';
        checkbox.setAttribute('onclick', onclick);
        parent.appendChild(checkbox);
      },

      // Create the spans that fill the header item.
      createSpans: function(parent) {
        this.createIndividualSpan(parent, 'Name', 'sortName');
        this.createIndividualSpan(parent, 'Url');
        this.createIndividualSpan(parent, 'Label');
        this.createIndividualSpan(parent, 'System tab');
        this.createIndividualSpan(parent, 'Query parameters?');
        this.createIndividualSpan(parent, 'Config parameters?');
      },

      // Get a sort icon for a heading/property in the item list.
      getSortIcon: function(sortProperty) {
        var icon = gees.dom.create('img');
        icon.name = 'sortimg';
        icon.id = 'sort' + sortProperty;
        icon.className = 'unsorted sorted';
        icon.src = 'images/sort_arrow.png';
        icon.setAttribute('onclick', onclick);
        return icon;
      },

      // Create the individual spans that make up the header.
      createIndividualSpan: function(parent, content, sortProperty) {
        var span = gees.dom.create('span');
        span.id = 'gName';
        span.className = 'legend';

        var label = gees.dom.create('label');
        label.className = 'legend';
        gees.dom.setHtml(label, content);

        if (sortProperty) {
          var onclick = 'gees.admin.searchView.itemList.sortAndLoad' +
              '(\'' + sortProperty + '\')';
          label.setAttribute('onclick', onclick);
          var sortIcon = this.getSortIcon('sortName');
          label.appendChild(sortIcon);
        }

        span.appendChild(label);
        parent.appendChild(span);
      }
    }
  },

  // Functions for deleting a Search Definition.
  deleteSearchDefDialog: {

    // Create a modal dialog so the user can confirm they want to delete
    // Search Definition(s).
    create: function() {
      var popupTitle = 'Confirm Delete';
      var popupContents = 'You are about to delete the following tabs: ';
      popupContents += '<b>' + this.getListForDelete() + '</b>';
      var popupButtons = '<a href="#" class="button standard"' +
          'onclick="gees.admin.closeAllPopups()">Cancel</a>' +
          '<a href="#" class="button blue"' +
          'onclick="gees.admin.searchView.deleteSearchDefDialog.commit()">Ok</a>';
      gees.tools.toggleNotification(popupTitle, popupContents, popupButtons);
    },

    // User has confirmed they want to delete a Search Definition.
    commit: function() {
      var checkedTabs = gees.admin.checkboxes.checkedItems.length;
      if (checkedTabs > 0) {
        var deleted = [];
        while (--checkedTabs >= 0) {
          deleted = this.requestDeleteSearchDef(checkedTabs, deleted);
        }
        gees.admin.searchView.onRefresh();
        this.displaySuccessMessage(deleted);
      }
    },

    // Make the request to the server to delete the Search Definition.
    // Provide a message if any items fail to delete, and return with a list
    // of successfully deleted tabs.
    requestDeleteSearchDef: function(checkedTabs, deletedItems) {
      var defIndex = gees.admin.checkboxes.checkedItems[checkedTabs];
      var defName = gees.assets.searchDefs[defIndex].name;
      defName = gees.tools.sanitizeStringForDatabase(defName);

      var requestUrl =
          '/geserve/SearchPublish?Cmd=DeleteSearchDef&SearchDefName=' +
          defName;
      var response = jQuery.parseJSON(gees.requests.doGet(requestUrl));
      if (response.status_code == 0) {
        deletedItems.push(defName);
      } else {
        var message = 'Unable to delete Search Definition: ' + defName;
        gees.tools.errorBar(message);
      }
      gees.admin.checkboxes.checkedItems.splice(checkedTabs, 1);
      return deletedItems;
    },

    // Create a readable list of items that are about to be deleted.
    getListForDelete: function() {
      var list = [];
      for (var i = 0; i < gees.admin. checkboxes.checkedItems.length; i++) {
        var index = gees.admin. checkboxes.checkedItems[i];
        var item = gees.assets.searchDefs[index];
        list.push(item.name);
      }
      return list.join(', ');
    },

    // Display a message informing the user about the results of the deletion.
    displaySuccessMessage: function(deletedSearchDefs) {
      var message = 'The following tabs were successfully deleted: ';
      if (deletedSearchDefs.length) {
        message += '<b>' + deletedSearchDefs.join(', ') + '</b>';
      } else {
        message = 'No tabs were deleted';
      }
      gees.admin.closeAllPopups();
      gees.tools.butterBar(message);
    }
  },

  // Functions for controlling the UI elements and functionality of the
  // Search Definition form.  Form can be used for either creating a new
  // Search Definition or editing and existing one.
  searchDefForm: {

    // Search form constants.
    EDIT_MODE: 'Edit',
    CREATE_MODE: 'Create',

    // Limit search tabs to have no more than 5 field definitions.
    ALLOWED_SEARCH_DEF_FIELDS: 5,

    // Object of key value pairs where key is parameter that will be sent
    // as part of json, and where value is the ID of a form element where
    // the parameter's value can be found.  Eg, pass to JSON
    // &label={value of element with ID SearchDefLabel}.
    FORM_FIELDS_FOR_JSON: {
        'additional_config_param': 'SearchDefConfigParams',
        'label': 'SearchDefLabel',
        'additional_query_param': 'SearchDefQueryParams',
        'service_url': 'SearchDefUrl'
    },

    // These are advanced options for 3D databases.  They are just like
    // the FORM_FIELDS_FOR_JSON above, except that they are hidden in the UI by
    // default, and are assigned default values if they are not used.
    ADVANCED_3D_OPTION_FORM_FIELDS: {
        'result_type': {
          id: '3DSearchOptionResultType',
          default_value: 'KML',
          input_type: 'radio'
        },
        'html_transform_url': {
          id: '3DSearchOptionHtmlTransformUrl',
          default_value: 'about:blank',
          input_type: 'text'
        },
        'kml_transform_url': {
          id: '3DSearchOptionKmlTransformUrl',
          default_value: 'about:blank',
          input_type: 'text'
        },
        'suggest_server': {
          id: '3DSearchOptionSuggestServer',
          default_value: 'about:blank',
          input_type: 'text'
        }
    },

    // Constant list for required fields in the Search Def Form.
    REQUIRED_FORM_FIELDS: ['SearchDefName',
                           'SearchDefLabel',
                           'SearchDefUrl'],

    // Current form mode, and the different mode options.
    currentFormMode: '',

    // Holder for incomplete form fields.
    incompleteFormFields: [],

    // Close the form - cancel editing/creating the current item.
    close: function() {
      gees.admin.popupClose('CreateSearchPopup');
      this.reset();
    },

    // Opens the Search Def form and focus the cursor on the name field.
    create: function() {
      gees.admin.popupOpen('CreateSearchPopup');
      gees.admin.searchView.searchDefForm.toggle3DSearchOptions(true);
      gees.admin.searchView.searchDefForm.reset3DSearchOptions();
      gees.dom.get('SearchDefName').focus();
    },

    // Populate the Search Definition form with values of an existing item.
    populate: function(item) {
      gees.dom.get('SearchDefName').value = item.name;
      gees.dom.get('SearchDefLabel').value = item.label;
      gees.dom.get('SearchDefUrl').value = item.url;
      gees.dom.get('SearchDefQueryParams').value = item.query;
      gees.dom.get('SearchDefConfigParams').value = item.config;

      // Look for advanced 3D options and fill those values as well.
      for (var option in this.ADVANCED_3D_OPTION_FORM_FIELDS) {
        if (item[option]) {
          var el = this.ADVANCED_3D_OPTION_FORM_FIELDS[option];

          // Either fill a text input with a value, or select the appropriate
          // radio button for the advanced 3D options.
          if (el.input_type == 'text') {
            gees.dom.get(el.id).value = item[option];
          } else if (el.input_type == 'radio') {
            var radioId = el.id + item[option];
            gees.dom.get(radioId).checked = true;
          }
        }
      }

      this.searchFieldDefinitionList.create(item.fields);
    },

    // Clear the search form of all values - reset to original state.
    reset: function() {
      gees.dom.get('SearchDefName').value = '';
      gees.dom.get('SearchDefLabel').value = '';
      gees.dom.get('SearchDefUrl').value = '';
      gees.dom.get('SearchDefQueryParams').value = '';
      gees.dom.get('SearchDefConfigParams').value = '';
      gees.dom.clearHtml(
          this.searchFieldDefinitionList.FIELD_DEF_LIST_ELEMENT);
      this.searchFieldDefinitionList.newElement();
    },

    // Submit Search Definition form to create/edit a Search Def.
    submit: function() {
      var searchDef = this.assembleJson();
      if (this.validateSearchDefName(searchDef.name)) {
        // Search Def name is not duplicate/invalid, make request to server.
        this.requestCreateSearchDef(searchDef);
      } else {
        // The SearchDef already exists, need to change the name.
        gees.tools.errorInput('SearchDefName');
        var message = 'The Search definition <b>' + searchDef.name +
            '</b> already exists. Please change the name to create' +
            ' a new definition.';
        gees.tools.errorBar(message);
      }
    },

    // Toggle the visiblity of advanced 3D Options for new/existing search
    // tabs. These are hidden by default.  A true 'hide' argument will
    // force the state to hidden.
    toggle3DSearchOptions: function(hide) {
      var el = gees.dom.get('advanced3DSearchTabOptions');
      var toggleButton = gees.dom.get('advanced3DOptionsToggle');
      if (el.style.display != 'block' && !hide) {
        el.style.display = 'block';
        toggleButton.innerHTML = 'hide advanced 3D options';
      } else {
        el.style.display = 'none';
        toggleButton.innerHTML = 'show advanced 3D options';
      }
    },

    // Reset the value of the form fields for advanced 3D search tab options.
    reset3DSearchOptions: function() {
      for (var option in this.ADVANCED_3D_OPTION_FORM_FIELDS) {
        var el = this.ADVANCED_3D_OPTION_FORM_FIELDS[option];
        if (el.input_type == 'text') {
          gees.dom.get(el.id).value = '';
        } else if (el.input_type == 'radio') {
          gees.dom.get(el.id + el.default_value).checked = true;
        }
      }
    },

    // Confirms that all required Search Def Form elements have been filled
    // out (have a value).  Submits form if valid, delivers message and
    // highlights invalid elements if not valid.
    validate: function() {
      // Get required elements and determine if they are valid.
      var requiredElements = this.getRequiredElements();
      var invalidElements = [];
      for (var i = 0; i < requiredElements.length; i++) {
        var element = requiredElements[i];
        if (!gees.dom.itemHasValue(element)) {
          invalidElements.push(element);
          gees.tools.errorInput(element);
        }
      }

      // Deliver a message if invalid elements are found.  Sumbit form
      // if no invalid elements are found.
      if (invalidElements.length > 0) {
        this.displayIncompleteFieldMessage(invalidElements);
      } else {
        this.submit();
      }
    },

    // Gets the required elements of the Search Def Form.
    getRequiredElements: function() {
      // Concat the constant list of Required fields to the list of
      // required elements from Search Field Definitions.
      return this.REQUIRED_FORM_FIELDS.concat(
          this.searchFieldDefinitionList.getRequiredElements());
    },

    // Launch the form.  Create mode means the form will be completely blank
    // in order to create a new Search Definition.
    openInCreateMode: function() {
      this.currentFormMode = this.CREATE_MODE;
      this.create();

      // Reset form, update text elements, and set the mode.
      this.reset();
      gees.dom.setHtml('SearchDefHead', 'Create Search Definition');
    },

    // Launch the form.  Edit mode means the form will be populated with
    // the values of the currently selected Search Definition.
    openInEditMode: function() {
      this.currentFormMode = this.EDIT_MODE;
      this.create();

      // Populate form with currently selected item.
      var item = gees.assets.searchDefs[gees.admin.checkboxes.checkedItems[0]];
      this.populate(item);
      gees.dom.setHtml('SearchDefHead', 'Edit Search Definition');
    },

    // Form elements within the Search Def Form for Field Defintions.
    // Contains functions for creating form elements and validating
    // them on form submit.
    searchFieldDefinitionList: {

      // Constant to identify Field Defintion List dom element.
      FIELD_DEF_LIST_ELEMENT: 'SearchDefFieldListing',

      // List existing field definitions in form when editing a
      // search definition.
      create: function(fields) {
        // Clear out area of the form used for listing fields.
        var searchFieldDiv = gees.dom.get(this.FIELD_DEF_LIST_ELEMENT);
        gees.dom.clearHtml(searchFieldDiv);

        // Create a UI element for each field definition, or deliver a message
        // that no fields exist for this item.
        if (fields.length > 0) {
          for (var i = 0; i < fields.length; i++) {
            var label = fields[i]['label'];
            label = label || '';
            var suggestion = fields[i]['suggestion'];
            var key = fields[i]['key'];
            var fieldsUsed = this.count();
            this.createElement(label, suggestion, key, searchFieldDiv);
          }
          this.checkAlottedFields();
        } else {
          gees.dom.setHtml(searchFieldDiv, 'No fields found');
        }
      },

      // Creates Search Definition Field List item.
      createElement: function(label, suggestion, key, searchFieldDiv) {
        this.element.create(
            label, suggestion, key, searchFieldDiv, this.count());
      },

      // Return the number of fields currently in use.
      count: function() {
        var fieldsId = this.element.FIELD_DEF_ID;
        return gees.dom.getByName(fieldsId).length;
      },

      // Gets required form elements for all Field Defintions.  Key is
      // the only required element of a Field Definition, so this returns
      // the id of the Key element for every Field Definition found.
      getRequiredElements: function() {
        var requiredElements = [];
        var fieldDefCount = this.count();
        for (var i = 0; i < fieldDefCount; i++) {
          requiredElements.push('key_' + i);
        }
        return requiredElements;
      },

      // Add the form values for field definitions to the object that will be
      // converted to encoded json.
      assembleJson: function(searchDefContent) {
        searchDefContent['fields'] = [];
        for (var i = 0; i < this.count(); i++) {
          // For each Field Def, get the label, suggestion and key value.
          var labelVal = gees.dom.get('label_' + i).value;
          var sugVal = gees.dom.get('suggestion_' + i).value;
          var keyVal = gees.dom.get('key_' + i).value;

          // Build a object for each Field Defintion.
          var collection = {};
          collection.label = gees.tools.cleanAndEncodeString(labelVal);
          collection.suggestion = gees.tools.cleanAndEncodeString(sugVal);
          collection.key = gees.tools.cleanAndEncodeString(keyVal);

          // Push Field Definition object to a list of Field Definitions
          // within the parent Search Definition object.
          searchDefContent['fields'].push(collection);
        }
        return searchDefContent;
      },

      // Create new form elements to allow for creation of a new Field
      // Definitions for a Search Definition.
      newElement: function() {
        var searchFieldDiv = gees.dom.get(this.FIELD_DEF_LIST_ELEMENT);
        this.createElement('', '', '', searchFieldDiv);
        this.checkAlottedFields();
      },

      // Remove the form elements for the last/newest Field Definition.
      removeLastElement: function() {
        // Get a list of Field Definition elements.
        var elements = gees.dom.getByName(this.element.FIELD_DEF_ID);
        var targetId = elements.length - 1;

        // Identify last element, its corresponding header, and their parent.
        var lastElement = elements[targetId];
        var header = gees.dom.get('field_label_' + targetId);
        var parent = lastElement.parentNode;

        // Remove the element and its header, and check how many fields are
        // used to be sure the appropriate buttons are visible.
        parent.removeChild(lastElement);
        parent.removeChild(header);
        this.checkAlottedFields();
      },

      // Checks the number of fields being used for a Search Def against the
      // limit of allowed fields.  Update links that appear near field
      // definitions depending on how many are visible.
      checkAlottedFields: function() {
        var fieldsUsed = this.count();
        // Allow addition of more fields until limit is reached.
        if (fieldsUsed >= this.ALLOWED_SEARCH_DEF_FIELDS) {
          gees.dom.setHtml('add_field_link', 'field limit reached');
          gees.dom.addClass('add_field_link', 'inactive_link');
        } else {
          gees.dom.setHtml('add_field_link', 'add field');
          gees.dom.setClass('add_field_link', 'search_link');
        }
        // Allow user to remove all but first set of field definition inputs.
        if (fieldsUsed > 1) {
          gees.dom.showInline('remove_field_link');
        } else {
          gees.dom.hide('remove_field_link');
        }
      },

      // Contains functions that create elements for a Search Field
      // Definition item in the Search Definition Form.
      element: {

        // Constant for identifying Field Definition form items within
        // the Search Definition Form.
        FIELD_DEF_ID: 'SearchDefField',

        // Creates Search Field Definition element.
        create: function(label, suggestion, key, holder, fieldIndex) {
          // Create a header div for the Field Def.
          var fieldLabelHeader = gees.dom.create('div');
          fieldLabelHeader.className = 'setting-label';
          fieldLabelHeader.innerHTML = 'Field definition:';
          fieldLabelHeader.id = 'field_label_' + fieldIndex;
          holder.appendChild(fieldLabelHeader);

          // Create a holder for the Field Def.
          var defHolder = gees.dom.create('div');
          defHolder.id = this.FIELD_DEF_ID;
          defHolder.setAttribute('name', this.FIELD_DEF_ID);

          // Create spans and make up the Field Def form.
          var labelId = 'label_' + fieldIndex;
          this.createSpan(labelId, label, defHolder, 'Label');
          var suggestionId = 'suggestion_' + fieldIndex;
          this.createSpan(suggestionId, suggestion, defHolder, 'Suggestion');
          var keyId = 'key_' + fieldIndex;
          this.createSpan(keyId, key, defHolder, 'Key');
          holder.appendChild(defHolder);
        },

        // Create a span to hold a Field Definition element.  Each Field
        // Definition requires three of these: one each for Label,
        // Suggestion and Key.
        createSpan: function(id, value, parent, labelText) {
          var span = gees.dom.create('span');

          // Create a label for the span.
          var label = gees.dom.create('label');
          label.innerHTML = labelText;
          span.appendChild(label);

          // Create a text input for the span.
          var input = gees.dom.create('input');
          input.id = id;
          input.type = 'text';
          input.value = value;
          span.appendChild(input);

          // Append the span to the div containing the field definition.
          parent.appendChild(span);
        }
      }
    },

    // Validates search definition name.
    // Return true if search definition with specific name doesn't exist, or if
    // it is a search definition that is being edited, otherwise return false.
    validateSearchDefName: function(name) {
      var id = gees.tools.getIndexOfName(name, gees.assets.searchDefs);
      return (id == -1 || (this.currentFormMode == this.EDIT_MODE &&
                           gees.admin.checkboxes.checkedItems.length && id ==
                           gees.admin.checkboxes.checkedItems[0]));
    },

    // Create Search Definition object by assembling values from Search
    // Definition form.
    assembleJson: function() {
      // Set tab name; it is its own param aside from our json object.
      var searchDefName = gees.dom.get('SearchDefName').value;
      // Create a json object that will become the search definition.
      var searchDefContent = {};

      // Add properties to Search Def Json for each field in the form.
      for (var param in this.FORM_FIELDS_FOR_JSON) {
        var id = this.FORM_FIELDS_FOR_JSON[param];
        var value = gees.dom.get(id).value;
        value = gees.tools.cleanAndEncodeString(value);
        searchDefContent[param] = value;
      }

      // Add advanced 3D options to the Search Def Json.
      for (var advancedOption in this.ADVANCED_3D_OPTION_FORM_FIELDS) {

        // Get the 3D option and its default value.
        var el = this.ADVANCED_3D_OPTION_FORM_FIELDS[advancedOption];
        var optionValue = el.default_value;

        // If the form element is text, grab the text value. If it is a radio
        // input, determine which radio button is selected.
        if (el.input_type == 'text') {
          if (gees.dom.get(el.id).value) {
            optionValue = gees.dom.get(el.id).value;
          }
        } else if (el.input_type == 'radio') {

          // Build an array of radio buttons for this option.
          var radioClass = el.id + 'Radio';
          var radiosArray = gees.dom.getByName(radioClass);

          // Iterate over this option's radio buttons, and get the value of
          // the checked button.
          for (var i = 0; i < radiosArray.length; i++) {
            var radio = radiosArray[i];
            if (radio.checked) {
              optionValue = radio.value;
            }
          }
        }

        // Add the advanced option parameter and value to the search def json.
        searchDefContent[advancedOption] = optionValue;
      }

      // Add Field Defintions objects to the Search Def object.
      searchDefContent =
          this.searchFieldDefinitionList.assembleJson(searchDefContent);
      return {name: searchDefName, content: searchDefContent};
    },

    // Show a an error bar if the form contains incomplete fields.
    displayIncompleteFieldMessage: function(incompleteFormFields) {
      var errorFields = incompleteFormFields.join(', ');
      // A form element is incomplete, double check.
      var message = 'Please complete the following required fields: ' +
          errorFields;
      gees.tools.errorBar(message);
    },

    // Builds the url to be used in the post request for
    // managing search definitions and attempts to create/update tab.
    requestCreateSearchDef: function(searchDef) {
      var requestUrl = '/geserve/SearchPublish?';
      var encodedJson = JSON.stringify(searchDef.content);

      // Escape quotes in the search def name to make sure it is safe
      // for insertion into the database.
      var name = gees.tools.sanitizeStringForDatabase(searchDef.name);

      var params = 'Cmd=AddSearchDef&SearchDefName=' +
          name + '&SearchDef=' + encodedJson;
      var response =
          jQuery.parseJSON(gees.requests.doPost(requestUrl, params));
      if (response.status_code != 0) {
        this.onCreateSearchDefRequestFailure(response);
      } else {
        this.onCreateSearchDefRequestSuccess(response, searchDef);
      }
    },

    // On a successful request for a new Search Definition, remove the form,
    // deliver a UI message, refresh the item list and highlight the newly
    // created (or edited) item.
    onCreateSearchDefRequestSuccess: function(response, searchDef) {
      var searchDef = response.results;
      gees.admin.closeAllPopups();
      gees.admin.checkboxes.resetAllHolders();
      var successVerb = this.getSuccessVerb(searchDef);
      var message = 'Your search tab <b>' + searchDef.search_def_name +
          '</b> was successfully ' + successVerb;
      gees.admin.searchView.onRefresh(searchDef.search_def_name);
      gees.tools.butterBar(message);
    },

    // A user can be in 'edit' mode, change the name of the tab, and in doing
    // so create a new tab.  Determine if the success request has updated a
    // tab or created a new one, so that the success message is correct.
    getSuccessVerb: function(searchDef) {
      var name = searchDef.search_def_name;
      var id = gees.tools.getIndexOfName(name, gees.assets.searchDefs);
      return id == -1 ? 'created' : 'updated';
    },

    // If a request to create a new Search Def fails, get the message from the
    // server response and deliver a message to the user.
    onCreateSearchDefRequestFailure: function(response) {
      var message = 'An error occurred: ';
      message += response.status_message;
      message += '<br><br><i>Note: you can change the name of a system' +
          ' search definition to create a new definition.</i>';
      gees.tools.errorBar(message);
    }
  },

  // Toggle a checkbox on and off and keep track of which boxes are checked.
  toggleItem: function(id) {
    // gees.admin.stopEvent(e);
    var isSystem = gees.assets.searchDefs[id].isSystem;
    var status = isSystem == true ? 'system_search' : 'search';
    if (gees.dom.isChecked('checkbox_' + id)) {
      gees.admin.checkboxes.checkedItems.push(id);
      gees.admin.checkboxes.checkItem(status, 'div_' + id);
    } else {
      var localLocation = gees.admin.checkboxes.checkedItems.indexOf(id);
      gees.admin.checkboxes.checkedItems.splice(localLocation, 1);
      gees.admin.checkboxes.uncheckItem(status, 'div_' + id);
    }
    gees.admin.buttons.updateItemOptions();
  },

  // Initializing function for the Search mode of the admin panel.
  init: function() {
    gees.admin.mode = gees.admin.modes.search;
    gees.admin.switchViewHelper();
    gees.admin.setHeaderLinks();
    gees.dom.showInline('CreateSearchDefButton');
    gees.admin.buttons.setMenu();
    this.itemList.init();
    this.itemList.sortAndLoad();
  }
};
