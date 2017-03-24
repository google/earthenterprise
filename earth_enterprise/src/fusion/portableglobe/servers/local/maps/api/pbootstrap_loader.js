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
// Do not clobber the list of params if they have already been defined.
// This is used in the Puppet tests.

if (typeof GEE_BASE_URL === 'undefined') {
  GEE_BASE_URL = window.location.protocol + '//' + window.location.host;
}

var params = params || {};
var experiments = [];
(function() {
  // Read cgi parameters from page url.
  var url = window.location.href;
  var paramsList = url.split('?');
  if (paramsList.length > 1) {
    var paramsString = paramsList[1];
    var terms = paramsString.split('&');
    for (var i = 0; i < terms.length; ++i) {
      var parts = terms[i].split('=');
      params[parts[0]] = unescape(parts[1]);
    }
  }

  if (params['expid']) experiments.push(params['expid']);
  if (!params['sensor']) params['sensor'] = 'false';

  // Load bootstrap.
  var bootstrap = GEE_BASE_URL + '/local/maps/api/pbootstrap.js';
  var params_list = [];
  for (var key in params) {
    params_list.push(key + '=' + params[key]);
  }
  bootstrap += '?' + params_list.join('&');
  if (!params['callback']) {
    document.write('<' + 'script src="' + bootstrap + '"' +
                   ' type="text/javascript"><' + '/script>');
  } else {
    var script = document.createElement('script');
    script.src = bootstrap;
    document.body.appendChild(script);
  }
})();
