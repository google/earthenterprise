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
// Flag indicating if sequence is in the process of being run.
var GEE_running_get_data_sequence = false;
// List of commands to run in sequence.
var GEE_sequence = [];
// Index of next command to run in sequence.
var GEE_current_sequence_index = 0;
// Interval for updating screen while waiting for command to complete.
var GEE_interval = '';
// Current globe UID.
var GEE_uid = '';
// Current globe name.
var GEE_globe_name = '';
// Time at which cutting began.
var GEE_start_time;
// Delay before returning task status.
var GEE_delay = 0;
// Values to be forced in the cutter (e.g. the tmp directory).
var GEE_force_arguments = '';
// Indicates whether globe being cut is a 2d Map.
var GEE_is_2d = '';
// Place holder for customized cleanup.
var GEE_cleanUp = function() {};
// Indicates if the current build has already been cancelled
var GEE_cancelled = '';

/**
 * Cancel current build.
 */
function GEE_cancelBuild() {
  var div = gees.dom.get('globe');
  gees.dom.setHtml(div, 'Cancelling ...');
  GEE_cancelled = 1;
  uid = GEE_uid;
  GEE_cleanUp();
  url = '/cgi-bin/globe_cutter_app.py?cmd=CANCEL&uid=' + uid +
        '&globe_name=' + GEE_globe_name + GEE_force_arguments;
  jQuery.get(url, function(info) {
        gees.dom.setHtml(div, 'Cancelled.');
      });
  // Update the heading of the main build progress message.
  gees.dom.setHtml('status', 'Build Cancelled');
  setTimeout(function() {
      var heading = 'Build Cancelled' +
          '<a href="javascript:void(0)" onclick="hideBuildResponse()">-</a>';
      gees.dom.setHtml('ProgressBar', heading);
    }, 100);
}

/**
 * Execute a list commands in sequence.
 * @param {array} sequence List of commands to execute.
 * @param {number} update_time Number of msecs before updating.
 * @param {callback} showGlobeSize Show file size of generated globe.
 * @param {callback} cleanup Clean up GUI elements when done.
 */
function GEE_runAjaxSequence(sequence, update_time, showGlobeSize, cleanup) {
  if (GEE_running_get_data_sequence) {
    alert('Sequence is already running.');
    return;
  }
  GEE_running_get_data_sequence = true;
  GEE_cleanUp = cleanup;

  if ((update_time > 0) && (GEE_interval == '')) {
    GEE_interval = setInterval(showGlobeSize, update_time);
  }

  // Initialize sequence
  GEE_start_time = new Date();
  GEE_cancelled = 0;
  var div = gees.dom.get('globe');
  gees.dom.setHtml(div, '<hr>Working ...');
  GEE_sequence = [];
  for (var i = 0; i < sequence.length; i++) {
    GEE_sequence[i] = sequence[i];

  }

  GEE_current_sequence_index = 0;
  GEE_runNextAjaxSequenceItem();
}


function GEE_clearUid() {
  GEE_uid = '';
}


function GEE_setUid(uid) {
  GEE_uid = uid;
}


function GEE_getUid() {
  return GEE_uid;
}


function GEE_setGlobeName(globe_name) {
  GEE_globe_name = globe_name;
}


function GEE_setIs2d() {
  GEE_is_2d = 't';
}


function GEE_getGlobeName() {
  return GEE_globe_name;
}


function GEE_setForceArguments(force_arguments) {
  GEE_force_arguments = force_arguments;
}


/**
 * Execute the next command in the sequence.
 */
function GEE_runNextAjaxSequenceItem() {
  if (GEE_running_get_data_sequence) {
    if (GEE_current_sequence_index < GEE_sequence.length) {
      GEE_current_sequence_index += 1;
      // Being extra cautious here about race conditions.
      eval(GEE_sequence[GEE_current_sequence_index - 1]);
    }
  }
}


/**
 * Convert time to string.
 * @param {number} Number of msec for cut.
 */
function GEE_convertTimeToString(time_in_msec) {
  var MSEC_PER_MINUTE = 60000;
  var MSEC_PER_HOUR = MSEC_PER_MINUTE * 60;
  var time_str = 'Time taken: ';
  if (time_in_msec > MSEC_PER_HOUR) {
    var num_hours = Math.floor(time_in_msec / MSEC_PER_HOUR);
    time_str += num_hours + ' hr ';
    time_in_msec -= (MSEC_PER_HOUR * num_hours);
  }

  if (time_in_msec > MSEC_PER_MINUTE) {
    var num_minutes = Math.floor(time_in_msec / MSEC_PER_MINUTE);
    time_str += num_minutes + ' min ';
    time_in_msec -= (MSEC_PER_MINUTE * num_minutes);
  }

  var num_seconds = Math.floor(time_in_msec / 1000);
  time_str += num_seconds + ' sec';

  return time_str;
}


/**
 * Make POST call to server. If a divId is given, set up routine
 * to asynchronously fill the div with the results returned
 * by the server. Otherwise, ignore the returned data.
 * @param {string} dataSource URL to server.
 * @param {string} postData Data to be sent to the server.
 * @param {string} divId HTML div tag id for div to receive returned HTML.
 * @param {string} mode How result should be added to output.
 */
function GEE_postData(dataSource, postData, divId, mode) {
  var div;
  var text;

  if (divId != '') {
    div = gees.dom.get(divId);
  }

  callBack =
    function(msg) {
      // Put response in the div
      if (divId != '') {
        // Change line-returns to "<br>".
        if (mode == 'SET_CR' || mode == 'APPEND_CR') {
          msg = msg.trim().split('\n').join('<br>');
        }
        if (mode == 'SET' || mode == 'SET_CR') {
          end_time = new Date();
          cut_time = end_time.getTime() - GEE_start_time.getTime();
          msg += '<br>' + GEE_convertTimeToString(cut_time);
          gees.dom.setHtml(div, msg);
        } else if (mode == 'APPEND' || mode == 'APPEND_CR') {
          var html = div.innerHTML;
          html += '<br/>' + msg;
          if (msg.substring(msg.length - 3, msg.length - 1) != 'Ok' &&
              msg.substring(msg.length - 7, msg.length) != 'SUCCESS') {
            html += '<br/><span class="text_status_warn">FAILED</span>';
            var globe_div = gees.dom.get('globe');
            var failMsg = '<hr><span class="text_status_warn">FAILED</span>';
            gees.dom.setHtml(globe_div, failMsg);
            GEE_cleanUp();
          }
          gees.dom.setHtml(div, html);
        } else if (mode == 'EXECUTE') {
          eval(msg);
        }

        GEE_runNextAjaxSequenceItem();
      }
    }

  jQuery.post(dataSource, postData, callBack);
}


/**
 * Append status message.
 * @param {string} msg Message to be added to the status.
 */
function GEE_appendStatus(msg) {
  var div = gees.dom.get('status');
  var html = div.innerHTML;
  gees.dom.setHtml(div, html + '<br/>' + msg);
}
