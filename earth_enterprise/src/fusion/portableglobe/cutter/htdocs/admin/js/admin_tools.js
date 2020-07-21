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
gees = gees || {};

/**
 * Namespace for all admin tools within gees namespace.
 */
gees.admin = {

  // Functions for styling and displaying buttons in admin mode.
  buttons: {

    // Display a button.
    revealButton: function(button) {
      gees.dom.showInline(button);
    },

    // Sets the appropriate menu buttons based on mode and screen width.
    setMenu: function() {
      this.hideOnScreenResize();
      this.setByAdminMode();
    },

    // Reveal settings buttons according to current mode.
    setByAdminMode: function() {
      if (gees.admin.mode != gees.admin.modes.database) {
        gees.dom.hide('RegisterButton');
        if (gees.admin.mode == gees.admin.modes.snippet ||
            gees.admin.mode == gees.admin.modes.dashboard) {
          gees.dom.hide('RefreshButton');
        }
      }
    },

    // Reveal Options buttons according to current mode.
    revealModeSpecificButtons: function() {
      if (gees.admin.mode == gees.admin.modes.snippet) {
        this.revealButton('CreateSnippetButton');
      } else if (gees.admin.mode == gees.admin.modes.search) {
        this.revealButton('CreateSearchDefButton');
        this.revealButton('RefreshButton');
      } else if (gees.admin.mode == gees.admin.modes.database) {
        gees.dom.show('RegisterButton');
        this.revealButton('RefreshButton');
      }
      this.setMenu();
    },

    // Hide certain buttons if the screen becomes small enough.
    hideOnScreenResize: function() {
      var screenWidth = $(document).width();
      gees.dom.showInline('RegisterButton');
      gees.dom.showInline('RefreshButton');
      gees.dom.showInline('SettingsButton');

      if (screenWidth < 800) {
        gees.dom.hide('RegisterButton');
        if (screenWidth < 670) {
          gees.dom.hide('RefreshButton');
          if (screenWidth < 600) {
            gees.dom.hide('SettingsButton');
          }
        }
      }
    },

    //  Clears all buttons. Helper function used when toggling databases.
    clearAll: function() {
      for (var i = 0; i < gees.admin.collections.adminButtons.length; i++) {
        gees.dom.hide(gees.admin.collections.adminButtons[i]);
      }
    },

    // Display/Hide different buttons based on which items are checked.
    updateItemOptions: function() {
      var total_checked = gees.admin.checkboxes.countChecked();
      // Clear all buttons - we're about to decide which to show.
      this.clearAll();

      var statusCount = gees.admin.checkboxes.multipleStatusCheck();
      if (total_checked == 1) {
        this.singleCheckedItemOptions();
      // If there are more than one items checked, there are
      // shared actions available.
      } else if (total_checked >= 2) {
        this.multipleCheckedItemOptions(statusCount);
      }
      this.revealModeSpecificButtons();
    },

    // Reveal buttons if one item is checked.
    singleCheckedItemOptions: function() {
      var counter = gees.admin.checkboxes.counter;
      if (counter['available'] > 0) {
        this.revealButton('PublishButton');
        this.revealButton('DeleteFusionButton');
      } else if (counter['published'] > 0) {
        this.revealButton('UnpublishButton');
        this.revealButton('PreviewButton');
        this.revealButton('PublishButton');
      } else if (counter['search'] > 0 ||
                 counter['system_search'] == 0) {
        this.revealButton('EditSearchDefButton');
        this.revealButton('DeleteSearchDefButton');
      } else if (counter['system_search'] == 1) {
        this.revealButton('EditSearchDefButton');
      } else if (counter['snippet'] > 0) {
        this.revealButton('EditSnippetButton');
      }
    },

    // Reveal buttons if multiple items are checked.
    multipleCheckedItemOptions: function(statusCount) {
      // Status count is 1, so all checked items have status of
      // either available or published.
      if (statusCount == 1) {
        // If checked items have status published, reveal unpublish button.
        if (gees.admin.checkboxes.counter['published'] > 0) {
          this.revealButton('UnpublishButton');
        }
        // If all items have status available, reveal delete button.
        if (gees.admin.checkboxes.counter['available'] > 0) {
          this.revealButton('DeleteFusionButton');
        }
      }
      // If current mode is Search and no system search tabs are checked,
      // offer user ability to delete all checked items.
      var checkedSearch = gees.admin.checkboxes.counter['system_search'];
      if (gees.admin.mode == gees.admin.modes.search && checkedSearch == 0) {
        this.revealButton('DeleteSearchDefButton');
      }
    }

  },

  // Functions for controlling checkboxes and keeping track of which
  // boxes are checked.  Helps UI decided which buttons and options to reveal
  // as different assets become checked/unchecked.
  checkboxes: {

    // Holder for checked items.
    checkedItems: [],

    // Count number of checked items.
    countChecked: function() {
     return this.checkedItems.length;
    },

    // Resets checkbox elements and information about checked items.
    resetAllHolders: function() {
      this.clickAll(true);
      this.checkedItems = [];
      this.resetCounter();
      return this;
    },

    // Set the checkbox counter to its initial state.
    resetCounter: function() {
      this.counter = {
        'available': 0,
        'published': 0,
        'search': 0,
        'system_search': 0,
        'snippet': 0
      };
      return this;
    },

    // Forces click on a checkbox when its parent div is clicked.
    clickBox: function(id) {
      var cb = gees.dom.get(id);
      cb.click();
    },

    // Toggle all checkboxes on/off.  Tied to a master checkbox with
    // select/deselect all functionality.
    toggleAll: function() {
      if (gees.dom.isChecked('masterCheckBox')) {
        this.clickAll(false);
      } else {
        this.clickAll(true);
      }
      gees.admin.buttons.updateItemOptions();
    },

    // A true arg clicks all unclicked boxes.  False arg unclicks all
    // clicked boxes.  Dashboard has no checkboxes - do not use if that
    // mode is detected.
    clickAll: function(arg) {
      if (gees.admin.mode != gees.admin.modes.dashboard &&
          gees.admin.mode != gees.admin.modes.snippet) {
        var checkboxes = gees.dom.getByName('dbCheck');
        var masterCheckBox = gees.dom.get('masterCheckBox');
        for (var i = 0; i < checkboxes.length; i++) {
          if (gees.dom.isChecked(checkboxes[i]) == arg) {
              checkboxes[i].click();
          }
        }
      }
    },

    // Uncheck a checkbox and restyle its associated div.
    uncheckItem: function(status, id) {
      if (this.counter[status] > 0) {
        this.counter[status] -= 1;
      }
      gees.dom.setClass(id, 'ItemInline');
      if (gees.dom.get('masterCheckBox')) {
        gees.dom.setCheckbox('masterCheckBox', false);
      }
    },

    // Check a checkbox and style its div.
    checkItem: function(status, id) {
      this.counter[status] += 1;
      gees.dom.addClass(id, 'item_hold');

      if (this.countChecked() == gees.admin.paging.itemsPerPage) {
        // If all boxes are checked, toggle the master checkbox on.
        gees.dom.setCheckbox('masterCheckBox', true);
      }
    },

    // Of all checked boxes, take a count of the varying statuses.
    // What we are trying to determine is if they have the same
    // 'status', or if multiple statuses are represented.
    multipleStatusCheck: function() {
      var statusCounter = 0;
      // Iterate through each possible status.
      for (var p in this.counter) {
        // If a box with this status is checked, increment our status counter.
        if (this.counter[p] > 0) {
          statusCounter += 1;
        }
      }
      return statusCounter;
      // If statusCounter is 0, no boxes are checked.
      // If statusCounter is 1, all checked boxes have same status.
    }

  },

  // Some dom element collections.
  collections: {

    // Collection of modals/popups in admin/index.html.
    popups: ['PublishDatabasePopup',
             'CreateSearchPopup',
             'SnippetPopup',
             'RegisterPopup',
             'ButterBar',
             'ErrorBar',
             'NotificationModal',
             'WhiteOverlay'],

    // Header links are any dom element with name admin-header.
    headerLinks: gees.dom.getByName('admin-header'),

    // Admin buttons are any dom element with name admin-button.
    adminButtons: gees.dom.getByName('admin-button')

  },

  // Functions for paging and keeping track of items.
  paging: {

    // Set initial values.
    init: function() {
      this.min = 0;
      this.max = undefined;
      this.itemsPerPage = 10;
      return this;
    },

    // Update paging values.  Determine if pagination is
    // still needed, or if it should be hidden.
    update: function() {
      gees.assets.numberOfObjects = this.getNumberOfObjects();

      // Re-calculate min number in case it is greater
      // than number of objects.
      if (this.min >= gees.assets.numberOfObjects) {
        var numPages =
            Math.floor((gees.assets.numberOfObjects - 1) / this.itemsPerPage);
        this.min = numPages * this.itemsPerPage;
        this.min = this.min < 0 ? 0 : this.min;
      }

      this.max = this.getMaxItemNumber();
      if (gees.assets.numberOfObjects > this.itemsPerPage) {
        this.enable();
      } else {
        this.disable();
      }
    },

    // Loads item list on pagination and updates pagination controls.
    // For example, if items 1-10 are visible, this loads items 11-20 once
    // the appropriate functions have been called by paging.right.
    loadItemListOnPaging: function() {
      this.update();
      if (gees.admin.mode == gees.admin.modes.database) {
        gees.admin.databaseView.itemList.refresh();
      } else if (gees.admin.mode == gees.admin.modes.search) {
        gees.admin.searchView.itemList.refresh();
      }
    },

    // Pagination not needed - hide elements.
    disable: function() {
      gees.dom.hide('PageRight');
      gees.dom.hide('PageLeft');
      gees.dom.hide('PageLabel');
    },

    // Pagination needed - display the elements.
    enable: function() {
      gees.dom.showInline('PageRight');
      gees.dom.showInline('PageLeft');
      gees.dom.showInline('PageLabel');
    },

    // If we are at the first/last page, disable up/down paging respectively.
    setButtons: function() {
      // Display Pagination elements.
      this.setButtonLabels();
      gees.dom.setClass('PageLeft', '');
      gees.dom.setClass('PageRight', '');
      if (this.min == 0) {
        gees.dom.setClass('PageLeft', 'disabled');
      }
      if (this.max >= gees.assets.numberOfObjects) {
        gees.dom.setClass('PageRight', 'disabled');
      }
    },

    // Set a label for the pagination buttons that reads, for example,
    // "1-10 of 35"
    setButtonLabels: function() {
      var minPage = this.min + 1;
      var total = gees.assets.numberOfObjects;
      var label = minPage + '-' + this.max + ' of ' + total;
      gees.dom.setHtml('PageLabel', label);
    },

    // Function for paging right/forward if paging is active.
    right: function() {
      // Increase items on page by itemsPerPage;
      // move to next page.
      if (this.min <= (gees.assets.numberOfObjects - this.itemsPerPage)) {
        this.min += this.itemsPerPage;
        this.loadItemListOnPaging();
      }
    },

    // Function for paging left/back if paging is active.
    left: function() {
      // Decrease items on page by itemsPerPage;
      // move to next page.
      if (this.min >= this.itemsPerPage) {
        this.min -= this.itemsPerPage;
        this.loadItemListOnPaging();
      }
    },

    // Return the highest displayed number when paging left/right.
    getMaxItemNumber: function() {
      var max = this.min + this.itemsPerPage;
      gees.assets.numberOfObjects = this.getNumberOfObjects();
      if (max > gees.assets.numberOfObjects) {
        max = gees.assets.numberOfObjects;
      }
      return max;
    },

    // Based on the current view/mode, grab the number of items (IE Databases,
    // Search tabs, etc).  This function helps other functions that
    // refresh our lists and update pagination.
    getNumberOfObjects: function() {
      if (gees.admin.mode == gees.admin.modes.database) {
        return gees.assets.databases.length;
      } else if (gees.admin.mode == gees.admin.modes.search) {
        return gees.assets.searchDefs.length;
      }
    }

  }.init(),

  // Functions that all modes require when switching views.
  switchViewHelper: function() {
    gees.dom.hide('SnippetEditor');
    gees.dom.hide('ExistingSnippets');
    gees.admin.paging.init();
    gees.admin.buttons.setMenu();
    gees.admin.buttons.clearAll();
    this.resetUI();
  },

  // Set all UI elements and checkbox holders to their default state.
  resetUI: function() {
    gees.admin.paging.update();
    gees.admin.paging.setButtons();
    gees.dom.clearHtml('MenuRight');
    gees.admin.closeAllPopups();
    gees.admin.checkboxes.resetAllHolders();
    gees.admin.buttons.updateItemOptions();
  },

  // Function to refresh the list of items, specifically databases
  // in database view, or search defs in search view.  Function will leave
  // the current sort order in tact - not like a true browser refresh.
  updateItemList: function() {
    if (gees.admin.mode == gees.admin.modes.database) {
      gees.admin.databaseView.onRefresh();
      gees.tools.butterBar('Databases have been refreshed.');
    } else if (gees.admin.mode == gees.admin.modes.search) {
      gees.admin.searchView.onRefresh();
      gees.tools.butterBar('Search tabs have been refreshed.');
    }
  },

  // Toggle the visibility of the settings menu when
  // clicking on settings button.
  settingsDropdown: function() {
    var settingsDrop = gees.dom.get('SettingsPop');
    var settingsButton = gees.dom.get('SettingsButton');
    var settingsClickListener = gees.dom.get('SettingsOverlay');
    if (gees.dom.getDisplay(settingsDrop) == 'none') {
      gees.dom.show(settingsDrop);
      gees.dom.show(settingsClickListener);
    } else if (gees.dom.getDisplay(settingsDrop) == 'block') {
      gees.dom.hide(settingsDrop);
      gees.dom.hide(settingsClickListener);
    }
  },

  // Function that looks for all popups and closes them.  Toggles all
  // dom elements with name admin-popup.
  closeAllPopups: function() {
    for (var i = 0; i < this.collections.popups.length; i++) {
      gees.dom.hide(this.collections.popups[i]);
    }
    var targePathName = gees.dom.get('NewGlobePublishPoint');
    targePathName.value = '';
  },

  // Clear all header links that represent the current mode.
  resetHeaderLinks: function() {
    for (var i = 0; i < this.collections.headerLinks.length; i++) {
      gees.dom.setClass(this.collections.headerLinks[i], '');
    }
  },

  // Set the header links to show the current mode.
  setHeaderLinks: function() {
    this.resetHeaderLinks();
    gees.admin.paging.disable();
    gees.dom.hide('DashboardPanel');
    gees.dom.setClass(this.mode + 'HEADER', 'LinkSelected');
  },

  // Open a popup by name, and place a clear overlay behind it to
  // allow it to be hidden on offclick.  Perform selected actions
  // depending of which popup was opened.
  popupOpen: function(popup) {
    gees.admin.closeAllPopups();
    var popupBox = gees.dom.get(popup);
    var overlay = gees.dom.get('WhiteOverlay');
    gees.dom.show(popupBox);
    gees.dom.show(overlay);
    if (popup == 'PublishDatabasePopup') {
      gees.admin.databaseView.publish.init();
    }
    if (popup == 'SnippetPopup') {
      gees.dom.get('NewProfileName').value = '';
    }
  },

  // Close a popup and hide related items.
  popupClose: function(popup) {
    gees.dom.hide(popup);
    gees.dom.hide('WhiteOverlay');
  },

  // Grab a timestamp and store it globally to log last refresh of a list.
  displayLastUpdate: function() {
    var updateMsg = 'Last Updated: ' + (new Date()).toString();
    gees.dom.setHtml('LastUpdate', updateMsg);
  },

  // Stop event propagation. Browser independent.
  stopEvent: function(event) {
    event = event || window.event;
    if (event.stopPropagation) {
      event.stopPropagation();
    } else {
      event.cancelBubble = true;
    }
  },

  // Make ajax request to determine if cutter is available or not.
  getCutterStatus: function() {
    // If cutter directory is reachable, cutter is on.  If not, it is off.
    $.ajax({
      url: GEE_BASE_URL + '/cutter/',
      success: function(e) {
        gees.admin.cutterStatus = 'On';
      },
      error: function(e) {
        gees.admin.cutterStatus = 'Off';
      }
    });
  },

  getDocStatus: function() {
    // Show documentation link if docs are installed
    $.ajax({
      url: GEE_BASE_URL + '/shared_assets/docs/manual/',
      success: function(e) {
        gees.dom.show('DocumentationItem');
      }
    });
  },

  setAdminDefaults: function() {
    // Status globals for UI.
    this.cutterStatus = 'On';
    // Default mode.  Options: DASHBOARD, DATABASE, SEARCH, SNIPPET.
    this.mode = this.modes.database;
    // Determine if cutter is enabled or disabled.
    this.getCutterStatus();
    // Determine if docs are installed
    this.getDocStatus();
    return this;
  },

  checkForUserSubmittedMode: function(mode) {
    var newMode = window.location.search;
    if (newMode != '') {
      if (newMode == '?dash') {
        this.mode = this.modes.dashboard;
      } else if (newMode == '?search') {
        this.mode = this.modes.search;
      } else if (newMode == '?snippet') {
        this.mode = this.modes.snippet;
      }
    }
  },

  // Mode constants for admin page.
  modes: {

    dashboard: 'DASHBOARD',
    database: 'DATABASE',
    search: 'SEARCH',
    snippet: 'SNIPPET'

  },

  sort: {

    ASCENDING_SORT: 'ASCENDING',
    DESCENDING_SORT: 'DESCENDING',

    // Sorts items (a multi dimensional array/dictionary) by a property
    // (sortMethod) in ascending or descending order.  listView is an object
    // located at gees.admin.{current view}.listItems, which will help to
    // remember the current sort state.
    sortItemList: function(items, sortMethod, listView) {
      // Set sort direction, method and function.
      var sortDirection =
          this.getDirection(sortMethod, listView.currentSortDirection);
      sortMethod = sortMethod || listView.currentSortMethod;
      var sortFunction = this.getFunction(sortDirection);

      // Remember the current sort method and direction.
      listView.setSortProperties(sortMethod, sortDirection);

      // Sort the list of items.
      items.sort(sortFunction(sortMethod));
    },

    // Given a list's sort method, as well as previous sort direction,
    // get the string value of what the current sort direction should be.
    getDirection: function(sortMethod, currentSortDirection) {
      if (!sortMethod) {
        return currentSortDirection;
      } else {
        return currentSortDirection == this.ASCENDING_SORT ?
            this.DESCENDING_SORT : this.ASCENDING_SORT;
      }
    },

    // Given a string value for sort direction, get the corresponding
    // sort function.
    getFunction: function(sortDirection) {
      return sortDirection == this.ASCENDING_SORT ?
          gees.tools.sort.asc : gees.tools.sort.dsc;
    }
  },

  // Reset checked items on push/unpush of a portable.
  resetCheckboxesAndButtons: function() {
    // Reset checkbox elements.
    this.checkboxes.resetAllHolders();
    this.checkboxes.resetCounter();
    this.buttons.updateItemOptions();
  },

  // Determine if certain popups are currently visible.
  currentModalDialog: function() {
    if (gees.dom.getDisplay('PublishDatabasePopup') == 'block') {
      return 'Publish';
    } else if (gees.dom.getDisplay('CreateSearchPopup') == 'block') {
      return 'Search';
    } else {
      return false;
    }
  },

  enterButtonListener: function() {
    // Let enter key submit forms if they are open.
    $(document).keydown(function(e) {
        var currentPopup = gees.admin.currentModalDialog();
        var enterKey = e.keyCode == 13;
        if (enterKey) {
          if (currentPopup == 'Publish') {
            gees.admin.databaseView.publish.validateForm();
          } else if (currentPopup == 'Search') {
            gees.admin.searchView.searchDefForm.validate();
          }
        }
    });
  }(),

  // Load the admin page.
  init: function() {

    // Initialize Server.
    gees.initialize.init();

    // Set UI defaults.
    this.setAdminDefaults();

    // Add a listener to a window resizing event.
    window.onresize = function(event) {
      gees.admin.buttons.setMenu();
    };

    // Determine if user is overriding the default mode.
    this.checkForUserSubmittedMode();

    // Initailize the admin UI to the correct view.
    if (this.mode == gees.admin.modes.dashboard) {
      gees.admin.dashboardView.init();
    } else if (this.mode == gees.admin.modes.search) {
      gees.admin.searchView.init();
    } else if (this.mode == gees.admin.modes.snippet) {
      snippetView();
    } else {
      gees.admin.databaseView.init();
    }
  }

};
