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
//

window.google = window.google || {};
google.maps = google.maps || {};

// Assumes that GEE_BASE_URL has been defined.
(function() {
  function getScript(src) {
    document.write('<script src="' + src +
                   '" type="text/javascript"></script>');
  }

  GEE_API_PATH='/maps/mapfiles/360/v3';
  FUSION_API_PATH='/maps/api';

  google.maps.Load = function(apiLoad) {
    /* http://mt0.google.com/vt */    apiLoad([,[,,,],['en-US',,,,,,GEE_BASE_URL + '/maps/api/icons/',
                                                        GEE_BASE_URL /* GE metrics */,
                                                        ,
                                                        GEE_BASE_URL /* GE metrics */],
                                                        [GEE_BASE_URL + GEE_API_PATH,'internal'],[0],0.0,,,,,], loadScriptTime);
  };

  var loadScriptTime = (new Date).getTime();

  getScript(GEE_BASE_URL + GEE_API_PATH + '/main.js');
  getScript(GEE_BASE_URL + GEE_API_PATH + '/../fusion_map_obj_v3.js');

  getScript(GEE_BASE_URL + FUSION_API_PATH + '/fusion_extended_map.js');
  getScript(GEE_BASE_URL + FUSION_API_PATH + '/fusion_utils.js');
})();
