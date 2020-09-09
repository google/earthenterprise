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
// Global Namespace for GEE Server 5.0.
var gees = {

  initUrls: function() {
    this.baseUrl = window.location.protocol + '//' + window.location.host;
    this.globesBase = this.baseUrl + '/cutter/globes';
    delete this.initUrls;
    return this;
  },

  // Namespace for commonly used DOM manipulation functions.
  dom: {

    // Basic DOM manipulation, create/find elements.
    create: function(type) {
      return document.createElement(type);
    },
    get: function(id) {
      return document.getElementById(id);
    },
    getByClass: function(className) {
      return document.getElementsByClassName(className);
    },
    getByName: function(name) {
      return document.getElementsByName(name);
    },

    // Set or add to the class name of a DOM element.
    addClass: function(div, className) {
      div = (typeof div == 'string') ? this.get(div) : div;
      div.className += ' ' + className;
    },

    setClass: function(div, className) {
      div = (typeof div == 'string') ? this.get(div) : div;
      div.className = className;
    },

    // Set or clear content of a div.
    appendHtml: function(div, content) {
      div = (typeof div == 'string') ? this.get(div) : div;
      div.innerHTML += content;
    },
    clearHtml: function(div) {
      div = (typeof div == 'string') ? this.get(div) : div;
      div.innerHTML = '';
    },
    setHtml: function(div, content) {
      div = (typeof div == 'string') ? this.get(div) : div;
      div.innerHTML = content;
    },

    // Get or set the display properties of an element.
    getDisplay: function(div) {
      div = (typeof div == 'string') ? this.get(div) : div;
      return div.style.display;
    },
    setDisplay: function(div, display) {
      div = (typeof div == 'string') ? this.get(div) : div;
      div.style.display = display;
    },
    hide: function(div) {
      div = (typeof div == 'string') ? this.get(div) : div;
      div.style.display = 'none';
    },
    show: function(div) {
      div = (typeof div == 'string') ? this.get(div) : div;
      div.style.display = 'block';
    },
    showInline: function(div) {
      div = (typeof div == 'string') ? this.get(div) : div;
      div.style.display = 'inline-block';
    },

    // Get or set the status of a checkable element.
    isChecked: function(box) {
      box = (typeof box == 'string') ? this.get(box) : box;
      return box.checked;
    },
    setCheckbox: function(box, state) {
      box = (typeof box == 'string') ? this.get(box) : box;
      box.checked = state;
    },

    // Other dom utilities.
    itemHasValue: function(item) {
      item = (typeof item == 'string') ? gees.dom.get(item) : item;
      return item.value != '';
    }
  },

  assets: {
    searchDefs: [],
    snippets: [],
    virtualServers: [],
    numberOfObjects: null,
    timerHolder: [],
    setItems: function() {
      this.databases = [];
      this.portables = [];
      this.cutterItems = [];
      this.missingItems = [];
      this.tempMemory = [];
      this.fusionDatabaseCount = 0;
      this.totalPublishedFusion = 0;
      this.totalPortableDatabases = 0;
      this.totalPublishedPortables = 0;
      this.fusionData = 0;
      this.portableData = 0;
      return this;
    },
    getBaseUrl: function(targetPath) {
      targetPath = targetPath[0] == '/' ? targetPath : '/' + targetPath;
      for (var i = 0; i < this.databases.length; i++) {
        if (this.databases[i].target_path == targetPath) {
          return this.databases[i].target_base_url;
        }
      }
      return '';
    }
  }.setItems(),

  requests: {
    databaseCmd: '/geserve/Publish?Cmd=Query&QueryCmd=ListAllAssets',
    searchDefsCmd: '/geserve/SearchPublish?Cmd=Query&QueryCmd=ListSearchDefs',
    snippetCmd: '/geserve/Snippets?Cmd=Query&QueryCmd=ListSnippetSets',
    endSnippetCmd: '/geserve/Snippets?Cmd=Query&QueryCmd=MetaFieldsSet',
    virtualServersCmd: '/geserve/Publish?Cmd=Query&QueryCmd=ListVss',
    init: function() {
      this.doPost = function(url, params) {
        var httpRequest = new XMLHttpRequest();
        httpRequest.open('POST', url, false);
        httpRequest.send(params);
        return httpRequest.responseText;
      };
      // Make a POST request. Params are in url unlike above post function.
      this.doSimplePost = function(url) {
        var httpRequest = new XMLHttpRequest();
        httpRequest.open('POST', url, false);
        httpRequest.send();
        var response = httpRequest.responseText;
        return response;
      };
      this.doGet = function(url) {
        var httpRequest = new XMLHttpRequest();
        httpRequest.open('GET', url, false);
        httpRequest.send();
        return httpRequest.responseText;
      };
      // Make an asynchronous GET request.
      this.asyncGetRequest = function(url, callback, errorCb) {

        // Build request.
        var httpRequest = new XMLHttpRequest();
        httpRequest.open("GET", url, true);

        // Check state of request and perform success/error functions.
        httpRequest.onload = function (e) {
          if (httpRequest.readyState === 4) {
            if (httpRequest.status === 200) {
              if (callback) {
                callback(httpRequest.responseText);
              }
            } else {
              if (errorCb) {
                errorCb(httpRequest.statusText, e);
              }
            }
          }
        };

        // Perform error function if request fails completely.
        httpRequest.onerror = function (e) {
          if (errorCb) {
            errorCb(httpRequest.statusText, e);
          }
        };

        // Send request.
        httpRequest.send(null);
      };
      this.base = window.location.protocol + '//' + window.location.host;
      this.databases = this.base + this.databaseCmd;
      this.getDatabases = function() {
        return this.doGet(this.databases);
      };
      this.searchDefs = this.base + this.searchDefsCmd;
      this.getSearchDefs = function() {
        return this.doGet(this.searchDefs);
      };
      this.snippets = this.base + this.snippetCmd;
      this.getSnippets = function() {
        return this.doGet(this.snippets);
      };
      this.endSnippets = this.base + this.endSnippetCmd;
      this.getEndSnippets = function() {
        return this.doGet(this.endSnippets);
      };
      this.virtualServers = this.base + this.virtualServersCmd;
      this.getVirtualServers = function() {
        return this.doGet(this.virtualServers);
      };
      // Get server defs JSON for serving a map/globe.
      this.getServerDefs = function(url) {
        // If a hash is found in the url, remove it & everything after it.
        url = url.split('#')[0];
        // Remove trailing slash from url if it exists.
        url = url[url.length - 1] == '/' ? url.substr(0, url.length - 1) : url;
        var request = url + '/query?request=Json&var=geeServerDefs';
        var response = this.doGet(request);
        // Response is object literal.  Strip unwanted characters and
        // use eval to convert to JSON string.
        response = response.trim();
        response = response.replace('var geeServerDefs = ', '');
        response = response.replace(/;$/, '');
        response = JSON.stringify(eval('(' + response + ')'));
        var serverDefs = jQuery.parseJSON(response);
        // serverUrl comes back as /targetPath for glm files.  Always
        // update it to full url (same url we passed to function).
        serverDefs.serverUrl = url;
        // Url format is now host.name/targetPath.  Create target path
        // by removing host name.
        serverDefs.serverTargetPath = url.replace(gees.baseUrl, '');
        return serverDefs;
      };
      delete this.init;
      return this;
    }
  }.init(),

  tools: {

    isFusionDb: function(item) {
      return item.type == 'map' || item.type == 'ge';
    },

    isPortableDb: function(item) {
      return !this.isFusionDb(item);
    },

    isRemoteFusion: function(item) {
      return item.remote;
    },

    is2dDatabase: function(item) {
      return item.is_2d;
    },

    is3dDatabase: function(item) {
      return item.is_3d;
    },

    isGlcPortable: function(item) {
      return item.type == 'glc';
    },

    isValidName: function(name) {
      // Valid characters are: 'a-z', 'A-Z', '0-9', '-', '_', '.', and ' '.
      var patt = new RegExp(/^[a-zA-Z0-9\-\_\.\ ]*$/);
      if (patt.test(name)) {
        // If valid, replace all ' ' with '_' and return [true, new name].
        return [true, name.replace(/ /g, '_')];
      } else {
        // If invalid, return [false, error message].
        msg = 'Please use valid characters:' +
               ' a-z, A-Z, 0-9, \'\-\', \'\_\', \'\ \', or \'\.\'' +
               ' in name.';
        return [false, msg];
      }
    },

    geEncodeURIComponent: function(str) {
        if (typeof encodeURIComponent == 'function') {
            return encodeURIComponent(str);
        } else {
            return escape(str);
        }
    },

    getDisplayStatus: function(item) {
      return item.target_path == '' ? 'available' : 'published';
    },

    getDbDisplayType: function(item) {
      if (this.isFusionDb(item)) {
        return this.isRemoteFusion(item) ? 'Remote' : 'Fusion';
      } else {
        return 'Portable';
      }
    },

    sanitizeStringForDatabase: function(string) {
      return string.replace(/"/g, '\\"');
    },

    isCuttableDb: function(item) {
      return ((window.location.hostname == 'localhost' ||
               window.location.hostname == '127.0.0.1' ||
               item.virtual_host_name != 'local') &&
              item.target_path != '' && item.type != 'glc');
    },

    getTargetPathForCutter: function() {
      var targetPath = window.location.search.split('?server=')[1];
      if (targetPath) {
        return targetPath.replace(/%2F/g, '/');
      } else {
        return false;
      }
    },

    dbIsServing2d: function(serverDefs) {
      // Note: 2D databases have projection property, while 3D
      // databases do not.  This helps determine serving type.
      return serverDefs.projection;
    },

    buildServerUrl: function(baseUrl, targetPath) {
      baseUrl = !baseUrl || baseUrl == '' ? gees.baseUrl : baseUrl;
      targetPath = targetPath[0] == '/' ? targetPath : '/' + targetPath;
      return baseUrl + targetPath;
    },

    // Get the full server url giving a targetPath.
    getServerUrl: function(targetPath) {
      var baseUrl = gees.assets.getBaseUrl(targetPath);
      return this.buildServerUrl(baseUrl, targetPath);
    },

    // Replace the hostname of url with browser's current hostname. This could
    // help conform to "same origin" security policy enforced by browsers.
    replaceHost: function(url) {
      var parser = document.createElement('a');
      parser.href = url;
      var new_url = parser.protocol + '//' + window.location.hostname;
      if (parser.pathname.length > 1 ||
          parser.pathname == url[url.length - 1]) {
        new_url += parser.pathname;
      }
      return new_url;
    },

    getLocalDateFromIso8601String: function(timestamp) {

      // Return a string to be displayed in place of timestamp
      // if it is empty/null.
      if (!timestamp || timestamp == '') {
        return 'none';
      }

      // Create a Javascript Date object.
      var newTime = new Date(timestamp);

      if (isNaN(newTime)) {

        // If newTime is not a number, user is likely using an older browser.
        // In this case, display the string date that was received.
        timestamp = timestamp.split('T')[0];
        return timestamp.split(' ')[0];
      }

      // If newTime is valid, user is in a modern browser.  Return value can be
      // constructed from a valid ISO-8601 string with time zone info.
      var day = ('0' + newTime.getDate()).slice(-2);
      var month = ('0' + (newTime.getMonth() + 1)).slice(-2);
      var year = newTime.getFullYear();

      return [year, month, day].join('-');
    },

    setAdditionalDatabaseDetails: function(db) {
      db.projection = db.is_2d ? '2D' : '3D';
      db.displayType = this.getDbDisplayType(db) + ' ' + db.projection;
      // Make sure name and timestamp have the same format.
      db.name = db.name[0] == '/' ? db.name.replace('/', '') : db.name;
      db.displayTimestamp = this.getLocalDateFromIso8601String(db.timestamp);
      // Some stored variables that we can sort with.
      db.sortName = db.name.toUpperCase().replace(/\//, '');
      db.sortDesc = db.description.toUpperCase();
      db.sortPath = db.target_path.toUpperCase().replace(/\//, '');
      // Make sortable name of undefined paths '~', so they will sort
      // last in a list.  A blank path sorts first, which is not helpful.
      db.sortPath = db.sortPath == '' ? '~' : db.sortPath;
      return db;
    },

    // Keep a count of total assets and data.
    collectDashboardData: function(db) {
      var undiscovered = gees.assets.tempMemory.indexOf(db.name) == -1;
      // Keep a running total of data for our Dashboard view.
      if (this.isFusionDb(db)) {
        if (undiscovered) {
          gees.assets.fusionDatabaseCount += 1;
          gees.assets.fusionData += parseInt(db.size);
        }
        if (db.target_path != '') {
          gees.assets.totalPublishedFusion += 1;
        }
      } else {
        if (undiscovered) {
          gees.assets.totalPortableDatabases += 1;
          gees.assets.portableData += parseInt(db.size);
        }
        if (db.target_path != '') {
          gees.assets.totalPublishedPortables += 1;
        }
      }
      gees.assets.tempMemory.push(db.name);
    },

    isSystemTab: function(name) {
      if (name == 'POISearch' ||
          name == 'GeocodingFederated' ||
          name == 'Coordinate' ||
          name == 'Places' ||
          name == 'ExampleSearch' ||
          name == 'SearchGoogle') {
        return true;
      } else {
        return false;
      }
    },

    // Function to show & hide notification div, used for many notifications.
    toggleNotification: function(title, contents, buttons) {
      if (gees.admin) {
        gees.admin.closeAllPopups();
      }
      var notification = gees.dom.get('NotificationModal');
      var overlay = gees.dom.get('WhiteOverlay');
      var style = 'block';
      if (gees.dom.getDisplay(notification) == style) {
        style = 'none';
        title = '';
        contents = '';
        buttons = '';
      }
      gees.dom.setDisplay(notification, style);
      gees.dom.setDisplay(overlay, style);
      gees.dom.setHtml('NoteTitle', title);
      gees.dom.setHtml('NoteContents', contents);
      gees.dom.setHtml('NoteButtons', buttons);
    },

    clearAllTimers: function() {
      for (var i = 0; i < gees.assets.timerHolder.length; i++) {
        clearTimeout(gees.assets.timerHolder[i]);
      }
    },

    notificationBar: function(element, message, timeout) {
      timeout = timeout || true;
      this.clearAllTimers();
      gees.dom.show(element);
      gees.dom.setHtml(element + 'Message', message);
      if (timeout) {
        gees.assets.timerHolder.push(setTimeout(function() {
            gees.dom.hide(element);
          }, 5000));
      }
    },

    butterBar: function(message, timeout) {
      this.notificationBar('ButterBar', message, timeout);
    },

    errorBar: function(message, timeout) {
      this.notificationBar('ErrorBar', message, timeout);
    },

    // Highlight a form input field that is incomplete/incorrect.
    errorInput: function(input) {
      input = (typeof input == 'string') ? gees.dom.get(input) : input;
      input.style.backgroundColor = '#f7c9c4';
      setTimeout(function() {
          input.style.backgroundColor = '';
        }, 2000);
    },

    internetExplorerVersion: function() {
      var rv = -1; // Return value assumes failure.
      if (navigator.appName == 'Microsoft Internet Explorer') {
        var ua = navigator.userAgent;
        var re = new RegExp('MSIE ([0-9]{1,}[\.0-9]{0,})');
        if (re.exec(ua) !== null) {
          rv = parseFloat(RegExp.$1);
        }
      }
      return rv;
    },

    // Clean a string of unwanted characters and encode it to prepare
    // to convert it to stringified json.
    cleanAndEncodeString: function(string) {
      string = string.replace(/"/g, '');
      string = string.replace(/'/g, '');
      string = encodeURIComponent(string);
      return string;
    },

    // From size (in bytes), returns string, example '23 MB'.
    getDisplaySize: function(size, forDashboard) {
      size = (size / 1048576);
      var minDisplaySize = forDashboard ? '99' : '999';
      var displayUnits;
      if (size >= 0 && size <= minDisplaySize) {
        displayUnits = 'MB';
      } else if (size > minDisplaySize && size <= 100000) {
        size /= 1024;
        displayUnits = 'GB';
      } else if (size > 100000) {
        size /= 1048576;
        displayUnits = 'TB';
      }
      if (forDashboard) {
        size = Math.round(size);
        // This value is for display only at this point.  If it is 0,
        // we'll call it 'less than one'.
        if (size == 0) {
          size = '<1';
        }
        return size + '<span>' + displayUnits + '</span>';
      } else {
        size = String(size).substr(0, 5);
        return size + ' ' + displayUnits;
      }
    },

    // Given a name and an array of items, find the item that contains
    // the name, and return the item's location.
    getIndexOfName: function(name, theArray) {
      var x = -1;
      for (var i = 0; i < theArray.length; i++) {
        if (theArray[i] && theArray[i].name == name) {
          x = i;
        }
      }
      return x;
    },

    // Updates the display of all elements in an aray with arg displayType.
    setElementDisplay: function(array, displayType) {
      for (var i = 0; i < array.length; i++) {
        if (gees.dom.get(array[i])) {
          gees.dom.setDisplay(gees.dom.get(array[i]), displayType);
        }
      }
    },

    removeElementChildren: function(divName) {
      var div = document.getElementById(divName);
      if (div) {
        while (div.lastChild) {
          div.removeChild(div.lastChild);
        }
      }
    },

    // Set all sort icons to an initial, 'unsorted' state.
    resetSortIcons: function() {
      var sortImages = gees.dom.getByName('sortimg');
      for (var i = 0; i < sortImages.length; i++) {
        gees.dom.setClass(sortImages[i], 'unsorted');
      }
    },

    // Style the sort icons based on current sort method.
    setSortIcons: function(sortBy) {
      this.resetSortIcons();
      var currentSortMethod = 'sort' + sortBy;
      gees.dom.addClass(currentSortMethod, 'sorted');
    },

    launch: {

      newWindow: function(url) {
        window.open(url, '', 'titlebar=no,toolbar=no,width=750,height=700');
      },

      // Launch glc assembly in a new window.
      glcAssembly: function() {
        this.newWindow('/cutter/glc_assembly.html');
      },

      // Launch glc disassembly in a new window.
      glcDisassembly: function() {
        this.newWindow('/cutter/glc_disassembly.html');
      }
    },

    sort: {

      // Helper function to sort a dictionary by property in descending order.
      dsc: function(prop) {
        return function(a, b) {
          if (b[prop] == a[prop]) return 0;
          return b[prop] < a[prop] ? -1 : 1;
        }
      },

      // Helper function to sort a dictionary by property in ascending order.
      asc: function(prop) {
        return function(a, b) {
          if (a[prop] == b[prop]) return 0;
          return a[prop] < b[prop] ? -1 : 1;
        }
      }
    }
  }
}.initUrls();
