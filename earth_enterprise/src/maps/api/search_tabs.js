/* Copyright 2009 Google.
 * All rights reserved.
 *
 * Description: Search Tab javascript utilities for Google Earth Enterprise
 *  Search:
 *  SearchManager: a class to manage search functionality/UI
 *  Geocoder: a geocoder class
 *  Tab: a class to create a tabbed list of search tab entries.
 * Version: 1.01
 *
 * Requires:
 *  fusion_utils.js : defines helper classes for accessing the DOM.
 *  The following other functions are assumed to be defined externally by
 *  the caller:
 *    geeCreateMarker(name, description, latlng, iconUrl, balloon) :
 *      name of the marker, description, latlng position , icon URL,
 *      and KmlBalloon object
 *    geeCreateSearchListItem(listDiv, result, balloon, iconUrl)
 *    geePanTo(lat, lng, zoomLevel) : zoom into the specified lat and lng
 *                                       and zoom level (1-32)
 *    geeSearchMarkerIconUrl(index) : return the icon url for the index'th
 *                                       search result
 *    geeCloseInfoWindow() : close the info window
 *    geeAddOverlay(marker) : add the marker from maps or earth
 *    geeRemoveOverlay(marker) : remove the marker from maps or earth
 *    geeCreateBalloon() : create a balloon object for maps or earth
 *    geeUpdateSearchResults() : called when the search results return.
 *    geeClearSearchResults() : called when the search results are cleared.
 *
 * The SearchManager class expects that the following divs are defined:
 *   search_results_title: the title of the search results list,
 *   search_results_cancel: the cancel button for the search results list,
 *   search_results_container: the container of the search results,
 * and the following optional div:
 *   search_results_loader : defines a div for the loading search results
 *                           indicator
 */

/******************************************************************************
 * Search
 ******************************************************************************/
var SEARCH_TIMEOUT_MILLISECONDS = 5000;  // 5 seconds by default.
var searchManager = null;  // The SearchManager object.

/**
 * Initialize the search tabs in the specified search div and set up
 * the searchManager which will process and manage the results.
 * @param {string}
 *           searchDivId  is the div id for the search tabs form.
 * @param {Array.<Object>}
 *           searchTabDefs  is the array of search tab definitions.
 * @param {number}
 *           opt_searchTimeoutMilliseconds  is optional search timeout override
 *           in milliseconds.
 */
function initializeSearch(searchDivId, searchTabDefs,
                          opt_searchTimeoutMilliseconds) {
  // only configure searching if the search div can be found
  var searchBoxDiv = findElement(searchDivId);
  if (!searchBoxDiv || !searchTabDefs || searchTabDefs.length == 0)
    return;

  // Override the search timeout if specified.
  if (opt_searchTimeoutMilliseconds != undefined) {
    SEARCH_TIMEOUT_MILLISECONDS = opt_searchTimeoutMilliseconds;
  }

  // install 'loading...' status message object
  var loadingMessage = createElement(document.body, "div", "loader");
  loadingMessage.style.visibility = 'hidden';
  loadingMessage.appendChild(document.createTextNode("loading..."));

  // install search tabs here
  var searchTabs = new Tab(searchBoxDiv);

  for (t = 0; t < searchTabDefs.length; ++t) {
    var searchDef = searchTabDefs[t];
    var tabBody = searchTabs.addTab(searchDef.tabLabel);

    // assemble search boxes
    var formElement = createElement(tabBody, "form");

    // Create elements with id's that are unique for each tab
    // by appending the tab id.
    var table = createElement(formElement, "table", "search_input_table");
    var tbody = createElement(table, "tbody");
    var inputRow = createElement(tbody, "tr", "search_input_row");
    var labelRow = createElement(tbody, "tr", "search_label_row");

    // Used if there would be more than one input per form
    // 2 are possible now, but maybe more in the future?
    var argNum = 0;
    while (searchDef.args[argNum]) {
      var label = createElement(labelRow, "td");
      // Use innerHTML to avoid escaping html tags.
      label.innerHTML = searchDef.args[argNum].screenLabel;
      var td = createElement(inputRow, "td");
      var textInput = createInputElement(td, "text");
      textInput.className = "search_text";
      searchDef.args[argNum].searchElement_ = textInput;

      ++argNum;
    }

    var rowElement = createElement(inputRow, "td");
    var submitButton = document.createElement("input");
    submitButton.setAttribute("type", "submit");
    submitButton.setAttribute("value", "Search");
    rowElement.appendChild(submitButton);

    // Set the on submit callback to refer to this searchDef.
    formElement.onsubmit = function(tabDef) {
        return function() {
          submitSearch_(tabDef);
          return false;
        };
      }(searchDef);
  }

  // Initialize the search manager.
  searchManager = new SearchManager();
}


/**
 * Submit the search query for the specified search tab.
 * @param {Object}
 *           tabDef  is the search tab definition.
 */
function submitSearch_(tabDef) {
  // Extract the searchTerm from the tabs.
  var searchTerm = '';
  for (var a = 0; a < tabDef.args.length; ++a) {
    if (tabDef.args[a].searchElement_.value != "") {
      searchTerm = tabDef.args[a].searchElement_.value;
      break;
    }
  }

  // If the tab defines a callback, use it.
  var callback = tabDef.callback;
  if (callback != null && callback != "") {
   callback(searchTerm);
    return;
  }

  // Start a search request
  startLoading_();

  var baseUrl = tabDef.url;
  if (baseUrl.indexOf('?') == -1)
    baseUrl += '?';
  baseUrl += '&' + tabDef.args[a].urlTerm + '=' +
    window.encodeURIComponent(searchTerm);

  var geocoder = new GeeGeocoder();
  geocoder.query(baseUrl,
                 function(results) {
                   endLoading_();
                   handleResults_(results)
                 });
}

/**
 * Handle the results of the search query. Add each result to the searchManager
 * which will propagate the results to the callback functions which the
 * calling application defines. After populating the results, we make
 * the callback to go to the first result.
 * @param {Object}
 *           reply  is the response to the search query.
 */
function handleResults_(reply) {
  if (reply.success == false) {
    alert("Search failed!\n\nMessage: " + reply.failureMessage);
    return;
  }

  // cleanup any previous results only if the search was successful
  searchManager.reset();

  for (var r = 0; r < reply.responses.length; ++r) {
    var response = reply.responses[r];

    if (response.success == false) {
      // Skip invalid responses.
    } else {
      if (response.data != undefined) {
        for (var d = 0; d < response.data.length; ++d) {
          var data = response.data[d];
          if (!data || !data.lon)
            break;

          var txt = "";
          if (data.name)
            txt += "Name: " + data.name + "<br>";
          if (data.description)
            txt += data.description + "<br>";

          searchManager.addMarker(data);
        }
      }
    }
  }
  searchManager.gotoFirstResult_();
}

/**
 * Callback that indicates we have begun loading the search results.
 * The calling application can optionally specify a "search_results_loader"
 * div for displaying this status. When loading completes, endLoading_() is
 * executed which will hide the "search_results_loader" div.
 */
function startLoading_() {
  var loadingMessage = findElement("search_results_loader");
  if (loadingMessage) {
    loadingMessage.style.visibility = 'visible';
  }
}

/**
 * Callback that indicates we have finished loading the search results and
 * hides the optional "search_results_loader" div.
 */
function endLoading_() {
  var loadingMessage = findElement("search_results_loader");
  if (loadingMessage) {
    loadingMessage.style.visibility = 'hidden';
  }
}

/******************************************************************************
 * SearchManager class
 ******************************************************************************/
/**
 * Constructor for the SearchManager class.
 * @constructor
 */
function SearchManager() {
  this.resultMapMarkers_ = [];
  this.firstResult_ = null;
  this.openInfoWindow_ = false;
  this.showme = null;

  // Get the Search Results Title, Cancel, and container divs.
  // Will need to be able to hide/show these depending on user actions.
  var div = document.getElementById("search_results_title");
  var cancelDiv = document.getElementById("search_results_cancel");
  var container = document.getElementById("search_results_container");
  this.searchResultsList_ = createElement(container, "ul",
      "search_results_list");

  // Callback to show the search result divs.
  this.showme = function() {
    div.style.display = "block";
    cancelDiv.style.display = "block";
    container.style.display = "block";
  };

  // When the Cancel Button is pressed, we simply hide and clear the results.
  cancelDiv.onclick = function() {
    div.style.display = "none";
    cancelDiv.style.display = "none";
    container.style.display = "none";
    searchManager.reset();
    geeUpdateSearchResults();
  };
}

/**
 * Clear the search results and close any open info windows.
 * The application callback geeClearSearchResults will also be called.
 */
SearchManager.prototype.reset = function() {
  // cleanup map markers
  for (var i = 0; i < this.resultMapMarkers_.length; i++)
    geeRemoveOverlay(this.resultMapMarkers_[i]);
  this.resultMapMarkers_.length = 0;
  if (this.searchResultsList_) {
    removeAllChildren(this.searchResultsList_);
  }
  this.firstResult_ = null;
  if (this.openInfoWindow_) {
    geeCloseInfoWindow();
    this.openInfoWindow_ = false;
  }
  geeClearSearchResults();
};

/**
 * Call the application defined callback to pan to the first search result and
 * call the update search results callback.
 * @private
 */
SearchManager.prototype.gotoFirstResult_ = function() {
  if (this.firstResult_) {
    geePanTo(this.firstResult_.lat, this.firstResult_.lon);
    geeUpdateSearchResults();
  }
};

/**
 * Add a marker to the SearchManager given a search result.
 * This makes callbacks to the application defined callbacks for creating
 * the marker and adding it to the displayable list.
 * @param {Object}
 *           result  is the search result item.
 */
SearchManager.prototype.addMarker = function(result) {
  var index = this.resultMapMarkers_.length;
  var point = result;
  if (index == 0)
    this.firstResult_ = point;

  var desc = "";
  if (result.description)
    desc = result.description;

  var iconUrl = geeSearchMarkerIconUrl(index);
  var balloon = geeCreateBalloon();
  var marker = geeCreateMarker(result.name, desc, point, iconUrl, balloon);
  geeAddOverlay(marker);
  var listItem = geeCreateSearchListItem(this.searchResultsList_,
                                         result, marker, balloon, iconUrl);
  this.resultMapMarkers_.push(marker);
  this.showme();
};

/******************************************************************************
 * GeeGeocoder class
 ******************************************************************************/
/**
 * Constructor for the GeeGeocoder class.
 * @constructor
 */
function GeeGeocoder() {
  // create a global store object where all callbacks are added/deleted
  if (!window.__gf_queryStore) {
    window.__gf_queryStore = {};
  }
}

/**
 * Execute the geocoding query given the query URL and the handler.
 * @param {string}
 *           baseUrl  is the URL for the query request.
 * @param {function(Object)}
 *           handler  is the result handler callback and takes the results
 *           object or error object.
 */
GeeGeocoder.prototype.query = function(baseUrl, handler) {
  // generate unique id for this "script"
  var id = "gf_query_" + new Date().getTime();

  // If the script never loads we need a way to cancel the request.
  var timeout = window.setTimeout(newErrorCallback_(id, handler, baseUrl, 403),
      SEARCH_TIMEOUT_MILLISECONDS);

  window.__gf_queryStore[id] = newCallback_(this, id, handler, timeout);

  var script = document.createElement("script");
  script.type = "text/javascript";
  script.id = id;
  script.charset = "UTF-8";
  script.src = baseUrl + "&cb=__gf_queryStore." + id;
  try {
    document.getElementsByTagName("head")[0].appendChild(script);
  } catch (err) {
    alert("Error!");
  }
};

/**
 * Remove the script node used to execute the query.
 * @param {string}
 *           id  is the node id.
 */
function removeScriptNode_(id) {
  var script = document.getElementById(id);
  if (script && script.nodeName == "SCRIPT") {
    script.parentNode.removeChild(script);
  }
}

/**
 * Callback to handle an error or timeout in the query.
 * This will also alert that the search failed.
 * @param {string}
 *           id  is the id of the query.
 * @param {function(Object)}
 *           handler  is the result handler callback and takes the results
 *           object or error object.
 * @param {string}
 *           queryURL  is the query URL.
 * @param {string}
 *           errorCode  is the error code.
 * @return {function()}
 *           the error callback that removes the script node and runs the
 *           handler on the error (as well as cleanup).
 */
function newErrorCallback_(id, handler, queryURL, errorCode) {
  return function() {
    removeScriptNode_(id);
    handler(newErrorReply_(queryURL, errorCode));
    if (id && window.__gf_queryStore[id]) {
      delete window.__gf_queryStore[id];
    }
    alert("Search Failed.");
  }
}

/**
 * Callback to process the returned result and clear the timeout.
 * @param {GeeGeocoder}
 *           geocoder  is the geocoder object (ignored for this callback).
 * @param {string}
 *           id  is the id of the query.
 * @param {function(Object)}
 *           handler  is the result handler callback and takes the results
 *           object or error object.
 * @param {number}
 *           timeoutId  is the id of the timeout.
 * @return {function(Object)}
 *           the callback that handles the reply and clears the timeout.
 * @private
 */
function newCallback_(geocoder, id, handler, timeoutId) {
  return function(reply) {
    window.clearTimeout(timeoutId);
    removeScriptNode_(id);
    handler(reply);
    delete window.__gf_queryStore[id];
  };
}

/**
 * Constructs a simple return status object (appropriate for our results
 * handler) from a query and error code.
 * @param {string}
 *           queryUrl  is the query URL.
 * @param {string}
 *           errorCode  is the error code.
 * @return {Object}
 *           the response object with the failure information embedded.
 * @private
 */
function newErrorReply_(queryUrl, errorCode) {
  return {
    success: false,
    searchTerm: queryUrl,
    failureMessage: "network error",
    errorCode: errorCode
  };
}

/******************************************************************************
 * Tab class
 ******************************************************************************/
/**
 * Constructs a Tabs HTML widget which manages a set of tabs
 * within the given parent HTML Element.
 * @param {Element}
 *           parent  is the parent HTML Element.
 * @constructor
 */
function Tab(parent) {
  var mainTable = createElement(parent, "table");
  var body = createElement(mainTable, "tbody");
  var row1 = createElement(body, "tr");
  var td = createElement(row1, "td");
  var div = createElement(td, "div", "tabLabels");
  var tabTable = createElement(div, "table");
  var tabTbody = createElement(tabTable, "tbody");
  this.tab_row = createElement(tabTbody, "tr");

  var row2 = createElement(body, "tr");
  var paneData = createElement(row2, "td");
  this.paneDiv_ = createElement(paneData, "div");

  this.tabs = [];
  this.panes = [];

  this.active = null;
}

/**
 * Add a tab to the set of tabs.
 * @param {string}
 *           label  is the label for the tab.
 * @return {Element}
 *           returns the HTML element for the tab pane.
 */
Tab.prototype.addTab = function(label) {
  var td = createElement(this.tab_row, "td");
  var tabDiv = createElement(td, "div");
  this.tabs[label] = tabDiv;
  var a = createElement(tabDiv, "a");
  a.href = 'javascript:void(0)';

  // Set to zoom in on layer on click
  a.onclick = function(theTab, theLabel) {
    return function() {
      theTab.setActive_(theLabel);
    };
  }(this, label);
  a.appendChild(document.createTextNode(label));

  var paneDiv = createElement(this.paneDiv_, "div", "tabPane");
  this.panes[label] = paneDiv;

  if (!this.active) {
    this.active = label;
    tabDiv.className = 'selectedTab';
  } else {
    tabDiv.className = 'unselectedTab';
    paneDiv.style.display = 'none';
  }

  return paneDiv;
};

/**
 * Sets the tab with the specified label as active.
 * @param {string}
 *           label  is the label for the tab.
 * @private
 */
Tab.prototype.setActive_ = function(label) {
  if (this.active != label) {
    this.tabs[this.active].className = 'unselectedTab';
    this.panes[this.active].style.display = 'none';
    this.active = label;
    this.tabs[this.active].className = 'selectedTab';
    this.panes[this.active].style.display = '';
    this.active = label;
  }
};
