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
 * Namespace for all functions used for the database view
 * within the admin mode of server.
 */
gees.admin.databaseView = {

  // Get the list of Databases from the server, then refresh
  // the list of items on the page.
  onRefresh: function() {
    gees.initialize.databases();
    this.itemList.sortAndLoad();
  },

  // Keep track of which items are checked.
  checkedItems: {

    reset: function() {
      this.targetPaths = [];
      return this;
    },

    add: function(path) {
      this.targetPaths.push(path);
    },

    remove: function(index) {
      this.targetPaths.splice(index, 1);
    },

    // Add/remove from list of checked items.  Return its list location.
    toggleAndGetLocation: function(path) {
      var index = this.targetPaths.indexOf(path);
      var isChecked = index != -1;
      if (!isChecked && path != '') {
        this.add(path);
      } else {
        this.remove(index);
      }
      return index;
    }
  },

  // Functions for displaying all databases in the UI.
  itemList: {

    init: function() {
      this.currentSortMethod = 'sortName';
      this.currentSortDirection = gees.admin.sort.ASCENDING_SORT;
      return this;
    },

    // Displays primary list of available Databases and Portables.
    display: function() {

      // Reset pagination and buttons.
      gees.admin.paging.setButtons();
      gees.admin.buttons.clearAll();
      gees.admin.buttons.updateItemOptions();

      // Clear out the div that will hold the list of items.
      var databaseListElement = gees.dom.get('ArrayContentDiv');
      gees.dom.clearHtml(databaseListElement);

      // Loop through databases and fill the div.
      for (var i = gees.admin.paging.min; i < gees.admin.paging.max; i++) {
        var db = gees.assets.databases[i];
        var dbDisplayInfo = this.getDbDisplayInfo(db);
        var status = gees.tools.getDisplayStatus(db);
        var innerListElement = this.element.createHolder(i, db.name);
        var checkbox = this.element.createCheckbox(i, db.name, innerListElement);
        this.element.createHolderSpans(i, db, dbDisplayInfo, innerListElement);
        databaseListElement.appendChild(innerListElement);
      }
      // Log the last update of the list.
      this.element.createUpdateNote(databaseListElement);
    },

    // Given a database, return an object containing display friendly values.
    getDbDisplayInfo: function(db) {
      var display = {};
      display.size = gees.tools.getDisplaySize(db.size);
      display.wms =  db.serve_wms ? this.wmsHoverText(db.target_path) : '';
      display.def_db = db.default_database ? '<em class="default_db_label">Default</em>' : '';
      // Print string 'unpublished' in place of target path and virtual host
      // if the portable/databse in question is not published.
      var unpublished = '<text class=\'unpublished\'>unpublished</text>';
      var targetUrl =
          gees.tools.buildServerUrl(db.target_base_url, db.target_path);
      var targetPathLink = '<a target="_blank" href="' + targetUrl + '">' +
          db.target_path + '</a>';
      display.target_path =
          db.target_path == '' ? unpublished : targetPathLink;
      display.target_path_hover = db.target_path == '' ? '' : targetUrl;
      display.displayType = gees.tools.isRemoteFusion(db) ?
          this.remoteHostHoverText(db) : db.displayType;
      display.virtual_host_name =
          db.virtual_host_name == '' ? unpublished : db.virtual_host_name;
      return display;
    },

    wmsHoverText: function(targetPath) {
      return '<em class="wms_label">WMS' +
          '<label class="holderlabel">' +
          '<label class="url">' + gees.baseUrl + targetPath + '/wms</label>' +
          '<label class="note">click ctrl+c to copy wms link</label>' +
          '</label></em>';
    },

    remoteHostHoverText: function(db) {
      return '<em>' + db.displayType + '<label>Host: ' + db.host +
          '</label></em>';
    },

    // A function to highlight a wms url string when it is visible.
    selectWmsLink: function(mouseEvent) {

      // Get the wms element that contains the wms url string.
      var elementToSelect = mouseEvent.target.children[0].children[0];

      // Select the wms url string so that it can be easily copied by the user.
      var range;
      var selection;
      if (document.body.createTextRange) {
          range = document.body.createTextRange();
          range.moveToElementText(elementToSelect);
          range.select();
      } else if (window.getSelection) {
          selection = window.getSelection();
          range = document.createRange();
          range.selectNodeContents(elementToSelect);
          selection.removeAllRanges();
          selection.addRange(range);
      }
    },

    // Sorts the item list by the sortMethod passed as an argument.
    sortAndLoad: function(sortMethod) {
      var list = gees.assets.databases;
      gees.admin.sort.sortItemList(list, sortMethod, this);

      // Refresh the list and set the sort icons.
      this.refresh();
    },

    // Remember the current sort state in the listView object.
    setSortProperties: function(sortMethod, sortDirection) {
      this.currentSortMethod = sortMethod;
      this.currentSortDirection = sortDirection;
    },

    // Refreshes item list by loading data from database list. It is performed
    // after a database is un/published or un/registered; after the list of
    // databases has been altered in any way.
    refresh: function() {
      // Update paging controls in case there are more/fewer databases, then
      // display the current list of databases.
      gees.admin.paging.update();
      this.display();

      // Reset buttons and checkboxes.
      gees.admin.checkboxes.resetAllHolders();
      gees.admin.databaseView.checkedItems.reset();
      gees.admin.buttons.updateItemOptions();

      // Set sort icons.
      gees.tools.setSortIcons(this.currentSortMethod);

      // Attach a listener to a published item's wms icon, if it exists.
      gees.admin.databaseView.addWmsHoverListeners();
    },

    // Manages the creation of the database list header element
    // and all of its sub-elements.
    header: {

      // Creates the header element for the list of databases.
      create: function() {
        var menu = gees.dom.get('MenuRight');
        gees.dom.clearHtml(menu);

        var innerMenu = gees.dom.create('div');
        innerMenu.className = 'ItemInlineLegend';
        innerMenu.id = 'legend';

        this.createMasterCheckbox(innerMenu);
        this.createIndividualHeaderItems(innerMenu);
        menu.appendChild(innerMenu);
        this.createListHolder(menu);
      },

      createMasterCheckbox: function(innerMenu) {
        var checkbox = gees.dom.create('input');
        checkbox.id = 'masterCheckBox';
        checkbox.type = 'checkbox';
        checkbox.className = 'frontPageCheck';
        var onclick = 'gees.admin.checkboxes.toggleAll()';
        checkbox.setAttribute('onclick', onclick);
        innerMenu.appendChild(checkbox);
      },

      createListHolder: function(menu) {
        var listHolder = gees.dom.create('div');
        listHolder.id = 'ArrayContentDiv';
        menu.appendChild(listHolder);
      },

      createIndividualHeaderItems: function(div) {
        this.headerSpan('gName', 'sortName', 'Database name', div);
        this.headerSpan('gPublishPoint', 'sortPath', 'Publish point', div);
        this.headerSpan(
            'gVirtualHost', 'virtual_host_name', 'Virtual host', div);
        this.headerSpan('gDescription', 'sortDesc', 'Description', div);
        this.headerSpan('gType', 'displayType', 'Type', div);
        this.headerSpan('gDate', 'timestamp', 'Date', div);
        this.headerSpan('gSize', 'size', 'Size', div);
      },

      headerSpan: function(spanId, sortType, title, parent) {
        var span = gees.dom.create('span');
        span.id = spanId;
        span.className = 'legend';

        var label = gees.dom.create('label');
        label.className = 'legend';
        var onclick = 'gees.admin.databaseView.itemList.sortAndLoad' +
            '(\'' + sortType + '\')';
        label.setAttribute('onclick', onclick);
        gees.dom.setHtml(label, title);

        var img = this.createSortImage(sortType);
        label.appendChild(img);
        span.appendChild(label);
        parent.appendChild(span);
      },

      createSortImage: function(sortType) {
        var img = gees.dom.create('img');
        img.id = 'sort' + sortType;
        img.name = 'sortimg';
        img.className = 'unsorted';
        img.src = 'images/sort_arrow.png';
        return img;
      }
    },

    // Create UI elements needed to display the database item.
    element: {

      // Create a div to hold a database list element.
      createHolder: function(index, name) {
        var div = gees.dom.create('div');
        var id = index + name;
        div.id = 'db' + id;
        div.className = 'ItemInline';
        var click = 'gees.admin.checkboxes.clickBox(\'' + 'cb' + id + '\')';
        div.setAttribute('onclick', click);
        return div;
      },

      // Create the various span elements that will be placed into a
      // database list element.
      createHolderSpans: function(index, db, display, parentDiv) {
        this.createInnerSpan('gName', db.name, parentDiv, db.name);
        var pathHover = display.target_path_hover;
        var typeClass = gees.tools.isRemoteFusion(db) ? 'remote-host' : '';
        var desc = display.def_db + display.wms + db.description;
        this.createInnerSpan(
            'gPublishPoint', display.target_path, parentDiv, pathHover);
        this.createInnerSpan(
            'gVirtualHost', display.virtual_host_name, parentDiv);
        this.createInnerSpan('gDescription', desc, parentDiv);
        this.createInnerSpan(
            'gType', display.displayType, parentDiv, false, typeClass);
        this.createInnerSpan('gDate', db.displayTimestamp, parentDiv);
        this.createInnerSpan('gSize', display.size, parentDiv);
      },

      // Create a span item for within a database list element.
      createInnerSpan: function(id, content, parent, title, className) {
        var span = gees.dom.create('span');
        span.id = id;
        span.title = title ? title : '';
        span.innerHTML = content;
        span.className = className ? className : '';
        parent.appendChild(span);
      },

      // Create a checkbox.
      createCheckbox: function(index, name, parent) {
        var clickFunction = 'gees.admin.databaseView.toggleDatabaseItem' +
            '(\'' + index + '\');';
        var checkbox = gees.dom.create('input');
        checkbox.className = 'dbCheck frontPageCheck';
        checkbox.name = 'dbCheck';
        checkbox.type = 'checkbox';
        checkbox.id = 'cb' + index + name;
        checkbox.setAttribute('onclick', clickFunction);
        parent.appendChild(checkbox);
      },

      createUpdateNote: function(parent) {
        var div = gees.dom.create('div');
        var lastUpdate = (new Date()).toString();
        div.innerHTML = 'Last Updated: ' + lastUpdate;
        div.id = 'LastUpdate';
        parent.appendChild(div);
      }
    }
  },

  // Creates Portable manager, which allows users to register,
  // unregister and download all Portables in one place.
  portableManager: {

    // Open the portable manager.
    init: function() {
      gees.admin.updateItemList()
      // If there are no portables, there is nothing to manage!
      if (gees.assets.portables.length == 0) {
        var message = 'There are currently no globes.';
        gees.tools.errorBar(message);
      } else {
        gees.admin.popupOpen('RegisterPopup');
        gees.dom.clearHtml('PortableGlobeRegistry');
        gees.assets.portables.sort(gees.tools.sort.asc('sortName'));
        for (var i = 0; i < gees.assets.portables.length; i++) {
          var portable = gees.assets.portables[i];
          this.insertPortableListItem(portable, i);
        }
      }
      var portableCount = gees.assets.portables.length + ' Portable globes';
      gees.dom.get('PortableGlobeSummary').innerHTML = portableCount;
      gees.admin.paging.update();
    },

    // Creates a list item that will appear in the Portable manager.
    insertPortableListItem: function(item, index) {
      var name = item.name;
      var registered = item.registered;
      var displaySize = gees.tools.getDisplaySize(item.size);
      var description = '(' + item.projection + ')' + item.description;

      var listDiv = gees.dom.get('PortableGlobeRegistry');
      var span = gees.dom.create('span');
      var link = this.makeLink(name, item);
      var button = this.makeButton(registered, index);

      span.appendChild(this.makeLabel('large_label', '', link, name));
      span.appendChild(this.makeLabel('large_label', description));
      span.appendChild(this.makeLabel('small_label', displaySize));
      span.appendChild(this.makeLabel('small_label', item.displayTimestamp));
      span.appendChild(button);

      listDiv.appendChild(span);
    },

    makeLink: function(name, item) {
      var link = gees.dom.create('a');
      link.className = 'link';
      link.download = name;
      link.href = gees.globesBase + '/' + name;
      link.innerHTML = name;
      return link;
    },

    makeButton: function(registered, dbIndex) {
      var buttonClass = 'StandardButton ManageButton';
      buttonClass = registered ? buttonClass : buttonClass + ' BlueButton';
      var pushFunc = 'gees.admin.databaseView.register.init';
      var unpushFunc = 'gees.admin.databaseView.unregister.init';
      var buttonFunction = registered ? unpushFunc : pushFunc;
      buttonFunction += '(\'' + dbIndex + '\')';
      var buttonStatus = registered ? 'Unregister' : 'Register';

      var button = gees.dom.create('button');
      button.className = buttonClass;
      button.innerHTML = buttonStatus;
      button.setAttribute('onclick', buttonFunction);
      return button;
    },

    makeLabel: function(className, content, child, hover) {
      hover = hover || content;
      var label = gees.dom.create('label');
      label.className = className;
      label.innerHTML = content;
      label.title = hover;
      if (child) {
        label.appendChild(child);
      }
      return label;
    },

    // Refresh Portable manager and notify user.
    refresh: function() {
      gees.dom.hide('ButterBar');
      gees.admin.databaseView.portableManager.init();
      var message = 'Portable listing refreshed.';
      // Intentionally delay it so it is clear that refresh has occurred.
      setTimeout(function() {
          gees.tools.butterBar(message);
        }, 200);
    }
  },

  // Publish Fusion and Portable databases.
  publish: {

    // Begin the publish process for a database.
    init: function() {
      var index = gees.admin.checkboxes.checkedItems[0];
      var item = gees.assets.databases[index];
      this.ui.determinePublishOptions(item);
      var displayName = item.name.replace(/\//g, '');
      var headerContent = 'Publish from <i>' + displayName + '</i>';
      gees.dom.setHtml('PublishHead', headerContent);

      // Use the name of the item as a suggested name for the target path.
      var targetPath = this.suggestTargetPath(item.target_path, displayName);
      gees.dom.get('NewGlobePublishPoint').value = targetPath;
      gees.dom.setCheckbox('default_db_radio_off', true);
      gees.dom.setCheckbox('wms_radio_off', true);
      gees.dom.hide('NoPublishName');
      gees.dom.get('NewGlobePublishPoint').focus();
    },

    suggestTargetPath: function(path, name) {
      var suggestion = path == '' ? name : path.substr(1);
      suggestion = suggestion.replace(/.glc/g, '');
      suggestion = suggestion.replace(/.glm/g, '');
      suggestion = suggestion.replace(/.glb/g, '');
      suggestion = this.confirmSuggestionIsUnique(suggestion);
      return suggestion;
    },

    // If a suggested mount point is not unique, add '_copy' at the end of
    // the suggested name to make it unique.
    confirmSuggestionIsUnique: function(suggestion) {

        // Remove '_copy' from the initial suggestion if it exists.
        suggestion = suggestion.split('_copy')[0];

        // Keep track of how many published items are using the same database
        // name within their mount point.
        var count = 0;
        for (var i = 0; i < gees.assets.databases.length; i++) {
          var targetPath = gees.assets.databases[i].target_path;

          // Remove the leading slash and trailing '_copy' text, if it exists.
          targetPath = targetPath.substr(1, targetPath.length - 1);
          targetPath = targetPath.split('_copy')[0];
          if (suggestion == targetPath) {
            count += 1;
          }
        }

        // If this mount point is in use, add the word '_copy' to the new
        // suggested mount point.
        if (count >= 1) {
          suggestion += '_copy';

          // If mount point is in use more than twice, give the copy a number.
          if (count > 1) {
            suggestion += '_' + count;
          }
        }
        return suggestion;
    },

    validateForm: function() {
      var pathField = gees.dom.get('NewGlobePublishPoint');
      var pathName = pathField.value;
      if (pathName.length == 0) {
        gees.dom.show('NoPublishName');
        gees.tools.errorInput(pathField);
      } else {
        pathName = pathName.replace(/ /g, '_');
        pathName = pathName.replace(/\?/g, '');
        this.request(pathName);
      }
    },

    // Publish a database / assign target path.
    request: function(target) {
      var index = gees.admin.checkboxes.checkedItems[0];
      var item = gees.assets.databases[index];
      var url = this.args.assembleUrl(item, target);
      var publishResponse = gees.requests.doGet(url);

      var response = publishResponse.split('StatusCode:');
      var successCode = response[1][0];
      if (successCode == 0) {
        this.onSuccess(target);
      } else {
        var errorMsg = 'An error occurred: ' + publishResponse;
        gees.tools.errorBar(errorMsg);
      }
    },

    // Handle a successfully published database/portable.  Instead of
    // completely reloading the list of dbs, this simply updates the
    // values that have changed as a result of the publish.
    onSuccess: function(target) {
      var index = gees.admin.checkboxes.checkedItems[0];
      var highlightedItemIndex = index - gees.admin.paging.min;

      var newItem = this.getDatabaseDetails(target);
      var existingItem = gees.assets.databases[index];
      this.updateDatabaseListAfterPublish(existingItem, newItem, index);

      gees.admin.closeAllPopups();
      this.displaySuccessMessage(target);
      this.highlight(highlightedItemIndex);
      gees.admin.databaseView.itemList.refresh();

      // Attach a listener to a published item's wms icon, if it exists.
      gees.admin.databaseView.addWmsHoverListeners();

      // Refresh the database list if this database was published as the default
      if( newItem.default_database ){
        gees.admin.databaseView.onRefresh();
      }
    },

    // Get the details of a published database, based on its target path.
    getDatabaseDetails: function(target) {
      var url =
          '/geserve/Publish?Cmd=Query&QueryCmd=PublishedDbDetails&TargetPath=';
      var request = gees.baseUrl + url + target;
      var newItem = gees.requests.doGet(request);
      newItem = eval('(' + newItem.split('Gepublish-Data:')[1] + ')');
      newItem = gees.tools.setAdditionalDatabaseDetails(newItem);
      return newItem;
    },

    // Highlight a div containing a recently updated item.
    highlight: function(index) {
      var dbList = gees.dom.get('ArrayContentDiv');
      var listItems = dbList.getElementsByTagName('div');

      // Highlight div.
      setTimeout(function() {
          gees.dom.setClass(listItems[index], 'ItemInline highlightPublished');
        }, 200);

      // Remove highlight from div.
      setTimeout(function() {
          var className = listItems[index].className;
          className = className.replace('highlightPublished', '');
          gees.dom.setClass(listItems[index], className);
        }, 1500);
    },

    displaySuccessMessage: function(target) {
      var link = GEE_HOST + '/' + target;
      var message = 'Your database has been published to:<b><a href="/' +
          target + '" target="new">' + link + '</a></b>';
      gees.tools.butterBar(message, true);
    },

    updateDatabaseListAfterPublish: function(existingItem, newItem, index) {
      // A database (existingItem) has just been published.  If this database
      // does not already have a target path, replace it with newItem.  If it
      // does have a target path, that means newItem is an additional publish
      // point on the same database.  In this case, do not remove any items
      // from the list - simply insert the newItem into the list.
      var itemsToRemove = existingItem.target_path == '' ? 1 : 0;
      gees.assets.databases.splice(index, itemsToRemove, newItem);
    },

    // Functions that build the arguments for a url within a publish request.
    args: {

      virtualHost: function() {
        var selectMenu = gees.dom.get('VirtualHostSelect');
        var option = selectMenu.options[selectMenu.selectedIndex].value;
        var virtualHost = option || 'public';
        return '&VirtualHostName=' + virtualHost;
      },

      hostName: function(db) {
        return db.host == '' ? '' : '&Host=' + db.host;
      },

      filePath: function(db) {
        return gees.tools.isPortableDb(db) ? '/' + db.name : db.path;
      },

      wms: function() {
        return gees.dom.isChecked('wms_radio') ? '&ServeWms=1' : '';
      },

      default_database: function() {
        return gees.dom.isChecked('default_db_radio') ? '&EcDefaultDb=1' : '';
      },

      searchDefs: function(name) {
        var array =
            gees.admin.databaseView.publish.ui.updateSearchSelect(name, true);

        // If search services have been selected, add them to the request.
        if (array.length > 0) {
          if (name == 'Search') {
            return '&SearchDefName=' + array.join(',');
          } else if (name == 'SupplementalSearch') {

            // If it is for supplemental search, also add the supplemental
            // search label.
            return '&SupSearchDefName=' + array.join(',') +
                this.supplementalSearchLabel();
          }
        } else {
          return '';
        }
      },

      supplementalSearchLabel: function() {
        var label = gees.dom.get('SupplementalSearchLabelInput').value;
        label = label == '' ? '' : '&SupUiLabel=' + label;
        return label;
      },

      poiOptions: function() {
        var poiSearchArgs = '';
        if (gees.dom.isChecked('poi_radio')) {
          var enhanced = gees.dom.isChecked('enhanced_search_radio');
          var federated = enhanced ? '&PoiFederated=1' : '&PoiFederated=0';
          poiSearchArgs += federated + '&PoiSuggestion=' +
              encodeURIComponent(gees.dom.get('poi_suggestion').value);
        }
        return poiSearchArgs;
      },

      snippets: function() {
        var value = gees.dom.get('SnippetSelect').value;
        value = value == '-- no snippet profile selected --' ? '' : value;
        var snippetArg = value == '' ? value : '&SnippetSetName=' + value;
        return snippetArg;
      },

      fusionOnlyOptions: function(item) {
        var fusionOptions = '';
        if (gees.tools.isFusionDb(item)) {

          // Get search and poi info for all Fusion DBs.
          fusionOptions = this.searchDefs('Search') + this.poiOptions();

          // Get snippet and supplemental search info for 3D Fusion.
          if (gees.tools.is3dDatabase(item)) {
            fusionOptions +=
                this.searchDefs('SupplementalSearch') + this.snippets();
          }
        }
        return fusionOptions;
      },

      assembleUrl: function(item, target) {
        return '/geserve/Publish?Cmd=PublishDb&DbName=' +
            this.filePath(item) + '&TargetPath=' + target +
            this.virtualHost() + this.hostName(item) +
            this.wms() + this.default_database() + 
            this.fusionOnlyOptions(item);
      }
    },

    // Functions that control UI elements used during the publish process.
    ui: {

      countSelectedSearchTabs: function(name) {
        return gees.dom.getByName(name + 'Select').length;
      },

      // Toggle elements available only when publishing Fusion databases.
      toggleFusionOnlyElements: function(display) {
        var fusionOnlyElements = gees.dom.getByName('FusionOnlyPublish');
        for (var i = 0; i < fusionOnlyElements.length; i++) {
          gees.dom.setDisplay(fusionOnlyElements[i], display);
        }
      },

      // Add a new dropdown for selecting a search tab.
      addSearchTabSelect: function(selectType) {
        var selectId = this.countSelectedSearchTabs(selectType);
        var select = this.createSearchSelect(selectType);
        select.id = selectType + '_search_select_' + selectId;
        var deleteLink = this.createSearchTabDeleteLink(selectId, selectType);
        var div = gees.dom.get(selectType + 'TabsInset');
        div.appendChild(select);
        div.appendChild(deleteLink);
      },

      createSearchSelect: function(searchType) {
        var select = gees.dom.create('select');
        select.setAttribute('name', searchType + 'Select');
        select.setAttribute('onchange',
            'gees.admin.databaseView.publish.ui.updateSearchSelect(\'' +
            searchType + '\')');
        var option = gees.dom.create('option');
        gees.dom.setHtml(option, '-- add a search tab --');
        select.appendChild(option);
        gees.assets.searchDefs.sort(gees.tools.sort.asc('sortName'));
        for (var i = 0; i < gees.assets.searchDefs.length; i++) {
          var def = gees.assets.searchDefs[i];
          var name = def.name;
          // POI Search is handled elsewhere, don't include it here.
          if (name != 'POISearch') {

            // Do not list services with > 1 fields for the search tabs menu.
            if (searchType == 'Search' && def.fields.length > 1) {
              continue;
            }
            option = gees.dom.create('option');
            option.setAttribute('id', 'option_' + name);
            gees.dom.setHtml(option, name);
            select.appendChild(option);
          }
        }
        return select;
      },

      // Keep track of the selected search tabs.  Also returns a list of
      // the tabs that are currently in use.
      updateSearchSelect: function(name, noNewTabs) {
        var searchDrops = gees.dom.getByName(name + 'Select');
        var count = 0;
        var tabsInUse = [];

        // Check for POI Search first.  Only insert POI to list of options
        // if it is the 'Search' dropdown; do not include POI in list of
        // options for 'Supplemental Search' services.
        if (name == 'Search' && gees.dom.isChecked('poi_radio')) {
          tabsInUse.push('POISearch');
        }

        for (var i = 0; i < searchDrops.length; i++) {
          var defaultMsg = 'add a search tab';
          var select = searchDrops[i];
          var searchValue = select.options[select.selectedIndex].value;
          if (searchValue.indexOf(defaultMsg) == -1) {
            var alreadySelected = tabsInUse.indexOf(searchValue) != -1;
            if (!alreadySelected) {
              count += 1;
              tabsInUse.push(searchValue);
            }
          }
        }

        // If not all of the available search tabs have been selected, append
        // a new select menu so additional search tabs can be selected.
        if (count == searchDrops.length && !noNewTabs) {
          this.addSearchTabSelect(name);
        }

        if (name == 'SupplementalSearch') {
          if (count > 0) {
            gees.dom.show('SupplementalSearchLabel');
          } else {
            gees.dom.hide('SupplementalSearchLabel');
          }
        }

        this.refreshDeleteLinks(tabsInUse, name);
        return tabsInUse;
      },

      // Given the list of search tabs in use, make a delete link avaiable for
      // all but the last one.
      refreshDeleteLinks: function(array, name) {
        var links = gees.dom.getByName(name + 'DeleteLink');
        for (var i = 0; i < links.length - 1; i++) {
          gees.dom.showInline(links[i].id);
        }
      },

      // Create a corresponding link for each search tab select menu,
      // allowing the user to easily remove a selected search tab.
      createSearchTabDeleteLink: function(id, name) {
        var deleteLink = gees.dom.create('label');
        deleteLink.id = name + '_delete_tab_' + id;
        deleteLink.title = name;
        gees.dom.setClass(deleteLink, name + 'TabDelete');
        gees.dom.setHtml(deleteLink, 'delete');
        deleteLink.setAttribute('name', name + 'DeleteLink');
        deleteLink.onclick = function(event) {
            gees.admin.databaseView.publish.ui.unstageSearchTab(event);
        };
        return deleteLink;
      },

      // Remove a search tab from a list of staged tabs.
      unstageSearchTab: function(event) {
        var type = event.target.title;
        var id = event.target.id.split(type + '_delete_tab_')[1];
        var select = gees.dom.get(type + '_search_select_' + id);
        var link = gees.dom.get(type + '_delete_tab_' + id);
        var div = gees.dom.get(type + 'TabsInset');
        div.removeChild(select);
        div.removeChild(link);
        this.updateSearchSelect(type);

        // Update the id of each search select dropdown and each search tab
        // delete button so that they are consecutive.  For instance, if the
        // 3rd tab in a list of 4 is removed/unstaged, the ids will read
        // 0, 1, 2 instead of 0, 1, 3.
        var searchSelectList = gees.dom.getByName(type + 'Select');
        for (var i = 0; i < searchSelectList.length; i++) {
          var searchSelect = searchSelectList[i];
          searchSelect.id = type + '_search_select_' + i;
        }
        var deleteButtonList = gees.dom.getByName(type + 'DeleteLink');
        for (i = 0; i < deleteButtonList.length; i++) {
          var deleteButton = deleteButtonList[i];
          deleteButton.id = type + '_delete_tab_' + i;
          deleteButton.onclick = function(event) {
            gees.admin.databaseView.publish.ui.unstageSearchTab(event);
          };
        }
      },

      // Show or hide the Enhanced POI radio option.
      toggleEnhancedPoi: function() {
        if (gees.dom.isChecked('poi_radio')) {
          gees.dom.showInline('poi_suggestion');
          gees.dom.showInline('poi_suggestion_label');
          gees.dom.showInline('EnhancedSearchOption');
        } else {
          gees.dom.hide('poi_suggestion');
          gees.dom.hide('poi_suggestion_label');
          gees.dom.hide('EnhancedSearchOption');
        }
      },

      determinePublishOptions: function(item) {
        this.addVirtualServerSelectMenu();
        this.determineWmsOptions(item);
        this.determineSearchAndSnippetOptions(item);
        this.determinePoiOptions(item.has_poi);
      },

      // Hide the Serve WMS radio option for 3d Portable databases
      determineWmsOptions: function(item) {
        if (gees.tools.isPortableDb(item) && gees.tools.is3dDatabase(item)) {
          gees.dom.hide('WmsOption');
        } else {
          gees.dom.show('WmsOption');
        }
      },

      determinePoiOptions: function(itemHasPoi) {
        var display = itemHasPoi ? 'inline-block' : 'none';
        gees.dom.setDisplay('POIDiv', display);
        gees.dom.setDisplay('poi_suggestion', display);
        gees.dom.setDisplay('EnhancedSearchOption', display);
        gees.dom.setCheckbox('poi_radio_off', !itemHasPoi);
        gees.dom.setCheckbox('poi_radio', itemHasPoi);
      },

      determineSearchAndSnippetOptions: function(item) {
        if (gees.tools.isFusionDb(item)) {
          gees.dom.clearHtml('SearchTabsInset');
          gees.dom.clearHtml('SupplementalSearchTabsInset');
          // Display search options.
          this.addSearchTabSelect('Search');
          this.toggleFusionOnlyElements('block');

          if (gees.tools.is2dDatabase(item)) {
            gees.dom.hide('DefaultDatabase');
            gees.dom.hide('SnippetPublishOptions');
            gees.dom.hide('SupplementalSearchOptions');
            gees.dom.hide('SupplementalSearchLabel');
          } else if (gees.tools.is3dDatabase(item)) {
            gees.dom.show('SupplementalSearchLabel');
            this.addSearchTabSelect('SupplementalSearch');
            this.addSnippetProfileSelectMenu();
          }
        } else {
          this.toggleFusionOnlyElements('none');
        }
      },

      addVirtualServerSelectMenu: function() {
        var div = gees.dom.get('PublishVirtualHost');
        gees.dom.clearHtml(div);
        gees.assets.virtualServers.sort();
        // Put "public" on top of the list.
        gees.assets.virtualServers.sort(function(a, b) {
          return b.indexOf('public') - a.indexOf('public');
        });
        var select = this.createSelectMenu(
            'VirtualHostSelect', gees.assets.virtualServers);
        div.appendChild(select);
      },

      addSnippetProfileSelectMenu: function() {
        var div = gees.dom.get('PublishMenuSnippets');
        gees.dom.clearHtml(div);
        gees.assets.snippets.sort(gees.tools.sort.asc('sortName'));
        var select = this.createSelectMenu(
            'SnippetSelect', gees.assets.snippets);
        div.appendChild(select);
      },

      createSelectMenu: function(selectType, nameArray) {
        var select = gees.dom.create('select');
        select.setAttribute('name', selectType);
        select.setAttribute('id', selectType);
        if (selectType == 'SnippetSelect') {
          var option = gees.dom.create('option');
          gees.dom.setHtml(option, '-- no snippet profile selected --');
          select.appendChild(option);
        }
        for (var i = 0; i < nameArray.length; i++) {
          var name = nameArray[i];
          if (selectType == 'SnippetSelect') {
            name = name.set_name;
          }
          option = gees.dom.create('option');
          gees.dom.setHtml(option, name);
          select.appendChild(option);
        }
        return select;
      }
    }
  },

  // Functions used when registering/pushing a Portable database.
  register: {

    init: function(index) {
      gees.admin.resetCheckboxesAndButtons();
      var portable = gees.assets.portables[index];
      var requestUrl = this.request(portable);

      // Try to push and get the request response.
      var response = gees.requests.doGet(requestUrl);
      var successCode = response[response.length - 3];
      this.finalize(successCode, portable);
    },

    request: function(portable) {
      var requestUrl = gees.baseUrl + '/geserve/StreamPush?Cmd=AddDb' +
          '&DbName=/' + gees.tools.geEncodeURIComponent(portable.name) +
          '&DbPrettyName=' +
          gees.tools.geEncodeURIComponent(portable.description) +
          '&FilePath=/opt/google/gehttpd/htdocs/cutter/globes/' +
          gees.tools.geEncodeURIComponent(portable.name) +
          '&FileSize=' + portable.size + '&DbSize=' + portable.size;
      if (portable.timestamp != '' && portable.timestamp != 'No timestamp.') {
        requestUrl += '&DbTimestamp=' + portable.timestamp;
      }
      return requestUrl;
    },

    finalize: function(successCode, portable) {
      if (successCode == 0) {
        var message = 'Portable<b> ' + portable.name + ' </b> registered.';
        gees.tools.butterBar(message, true);
        gees.admin.databaseView.portableManager.init();
        gees.admin.databaseView.itemList.refresh();
      } else {
        popupTitle = 'An error occurred';
        popupContents = 'An error occurred while processing the request.' +
            ' Please try again.';
        popupButtons = '<a href="javascript:void(0)" class="button blue"' +
            'onclick="gees.admin.closeAllPopups()">Ok</a>';
        gees.tools.toggleNotification(popupTitle, popupContents, popupButtons);
      }
    }
  },

  // Functions used when unregistering/unpushing a Portable database.
  unregister: {

    init: function(index) {
      gees.admin.resetCheckboxesAndButtons();
      var portable = gees.assets.portables[index];
      var unpushResponse = this.request(portable.name);
      var errorMsg = unpushResponse[0];
      var successCode = unpushResponse[1];
      this.finalize(successCode, portable, errorMsg);
    },

    request: function(portableName) {
      var requestUrl = gees.baseUrl;
      requestUrl += '/geserve/StreamPush?Cmd=CleanupDb&DbName=/';
      requestUrl += portableName;
      var response = gees.requests.doGet(requestUrl);
      var successCode = response[response.length - 3];
      // TODO: implement universal function to get status message.
      var errorLog = response.split('HandleUnregisterPortableRequest: ');
      return [errorLog[1], successCode];
    },

    finalize: function(successCode, portable, errorMsg) {
      if (successCode == 0) {
        var message = 'Portable<b> ' + portable.name + ' </b> unregistered';
        gees.tools.butterBar(message, true);
        gees.admin.databaseView.portableManager.init();
        gees.admin.databaseView.itemList.refresh();
      } else {
        var message = 'An error occurred: ' + errorMsg;
        gees.tools.errorBar(message, true);
      }
    }
  },

  // Functions for removing databases.  This does not delete, but
  // unpushes a Fusion DB back to Fusion, or unregisters a Portable.
  // Only available for items that are not currently published.
  remove: {

    init: function() {
      // Build a list of successfully removed items.
      var listOfItems = [];

      // Remove all checked items.
      for (var i = 0; i < gees.admin.checkboxes.checkedItems.length; i++) {
        var index = gees.admin.checkboxes.checkedItems[i];
        var db = gees.assets.databases[index];

        // Handle portables and fusion dbs differently.
        if (gees.tools.isPortableDb(db)) {
          listOfItems = this.portableDb(db, listOfItems);
        } else {
          listOfItems = this.fusionDb(db, listOfItems);
        }
      }
      this.successMessage(listOfItems);
    },

    fusionDb: function(db, list) {
      var url = gees.baseUrl + '/geserve/StreamPush?Cmd=DeleteDb' +
          '&DbName=' + db.path + '&Host=' + db.host;
      var deleteResponse = gees.requests.doGet(url);
      deleteResponse = deleteResponse.split('Gepublish-StatusCode:')[1];
      if (deleteResponse == 0) {
        list.push(db.name);
      }
      return list;
    },

    portableDb: function(db, list) {
      if (db.name[0] == '/') {
        db.name = db.name.substring(1);
      }
      var request = gees.admin.databaseView.unregister.request(db.name);
      var successCode = request[1];
      if (successCode == 0) {
        list.push(db.name);
      }
      return list;
    },

    successMessage: function(listOfItems) {
      var printedList = listOfItems.join(',');
      var message = printedList.length > 1 ? 'Items' : 'Item';
      message += ' removed: <b>' + printedList + '</b>';
      this.removeDbFromList();
      gees.tools.butterBar(message);
    },

    removeDbFromList: function() {
      // Sort gees.admin.checkboxes.checkedItems in descending order which
      // then allows to delete elements from gees.assets.databases by index.
      gees.admin.checkboxes.checkedItems.sort(function(a, b) {return b - a});
      for (var i = 0; i < gees.admin.checkboxes.checkedItems.length; i++) {
        gees.assets.databases.splice(gees.admin.checkboxes.checkedItems[i], 1);
      }
      gees.admin.databaseView.itemList.refresh();
    }
  },

  // Functions for unpublishing a database.
  unpublish:  {

    init: function() {
      var successHolder = [];
      var failureHolder = [];

      // Multiple items can be unpublished at once.
      var items = gees.admin.databaseView.checkedItems.targetPaths;
      for (var i = 0; i < items.length; i++) {
        var path = items[i];
        var p1 = '/geserve/Publish?Cmd=UnPublishDb&TargetPath=';
        var url = gees.baseUrl + p1 + path;
        var publishResponse = gees.requests.doGet(url);
        var successCode = publishResponse.split('Gepublish-StatusCode:')[1];
        // Log success/failure.
        if (successCode == 0) {
          successHolder.push(path);
        } else {
          failureHolder.push(path);
        }
      }
      if (successHolder.length > 0) {
        this.onSuccess(successHolder);
      }
      if (failureHolder.length > 0) {
        this.failure(failureHolder);
      }
    },

    failure: function(list) {
      var failureList = list.join(',');
      var title = 'Items failed';
      var msg = 'Unable to unpublish the following items: ' + failureList;
      var buttons = '<input type="submit" class="StandardButton' +
          ' BlueButton" onclick="gees.admin.closeAllPopups();"' +
          ' value="Close">';
      gees.tools.toggleNotification(title, msg, buttons);
    },

    onSuccess: function(list) {
      var successList = list.join(',');
      var msg = 'The following items were successfully unpublished: <b>' +
          successList + '</b>';
      gees.tools.butterBar(msg);
      this.finalize();
    },

    // Determine if there are multiple publish points associated with this
    // database (item).  If so, this now unpublished item is an erroneous
    // duplicate in the list and should be removed.
    checkIfDuplicate: function(item, itemIndex) {
      var count = 0;
      for (var i = 0; i < gees.assets.databases.length; i++) {
        var db = gees.assets.databases[i];
        var index = db.host + db.path;
        var locator = item.host + item.path;
        if (index == locator) {
          count += 1;
        }
      }
      if (count > 1) {
        gees.assets.databases.splice(itemIndex, 1);
        gees.admin.paging.update();
      }
    },

    // Helper function for refreshing database list after an unpublish.
    finalize: function () {
      // Sort checkedItems in descending order which then allows to delete
      // duplicated elements from gees.assets.databases by index.
      gees.admin.checkboxes.checkedItems.sort(function(a, b) {return b - a});
      for (var i = 0; i < gees.admin.checkboxes.checkedItems.length; i++) {
        var index = gees.admin.checkboxes.checkedItems[i];
        var item = gees.assets.databases[index];
        item.target_path = '';
        item.status = 'available';
        item.virtual_host_name = '';
        item.serve_wms = false;
        item.default_database = false;
        // Make sortable name of undefined paths '~', so they will sort
        // last in a list.  A blank path sorts first, which is not helpful.
        item.sortPath = '~';
        this.checkIfDuplicate(item, gees.admin.checkboxes.checkedItems[i]);
      }
      gees.admin.databaseView.itemList.refresh();
    },
  },

  // Toggling items on, keeping track of what is currently toggled.
  toggleDatabaseItem: function(index) {
    var db = gees.assets.databases[index];
    var status = gees.tools.getDisplayStatus(db)

    // Check if it resides in the array of checked items.
    var localLocation = gees.admin.checkboxes.checkedItems.indexOf(index);
    var isChecked = db.target_path == '' ?
        localLocation : this.checkedItems.toggleAndGetLocation(db.target_path);

    // Add or remove from list of checked items.
    if (isChecked != -1) {
      gees.admin.checkboxes.checkedItems.splice(localLocation, 1);
      gees.admin.checkboxes.uncheckItem(status, 'db' + index + db.name);
    } else {
      // Store the global location so that we can remember it is checked.
      gees.admin.checkboxes.checkedItems.push(index);
      gees.admin.checkboxes.checkItem(status, 'db' + index + db.name);
    }
    // Reset option buttons based on which items are selected.
    gees.admin.buttons.updateItemOptions();
  },

  // Send browser to preview interface, ex: server.com/TargetPath.
  previewDatabase: function() {
    var itemIndex = gees.admin.checkboxes.checkedItems[0];
    var database = gees.assets.databases[itemIndex];
    var previewUrl = gees.tools.buildServerUrl(
        database.target_base_url, database.target_path);

    // This will open in a new tab.
    window.open(previewUrl, '_blank');
  },

  // Set a hover listener on every wms icon found in the current view.
  addWmsHoverListeners: function() {
    var wmsLabels = gees.dom.getByClass('wms_label');
    for (var i = 0; i < wmsLabels.length; i++) {
      var item = wmsLabels[i];
      item.addEventListener('mouseenter', function(e) {
        gees.admin.databaseView.itemList.selectWmsLink(e);
      });
    }
  },

  // Beginning function that loads list of DBs, sorts, and refreshes
  // pagination if necessary.
  init: function() {
    gees.admin.mode = gees.admin.modes.database;
    this.checkedItems.reset();
    gees.admin.switchViewHelper();
    this.itemList.header.create();
    gees.admin.setHeaderLinks();
    this.itemList.init();
    this.itemList.sortAndLoad();
  }
};
