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
 * Javascript that renders the results of a geecheck test.
 * source of test is /cgi-bin/run_geecheck.py, and is rendered at
 * /admin/geecheck.html.
 */

var geecheck = {

  // Load the test data.
  loadData: function() {

    // Tests may take some time, so set a loading notification for the user.
    geecheck.setLoadingNotification();

    // Delay the request for half a second to be sure the loading
    // notification has had time to render.
    setTimeout(function() {
        geecheck.requestData();
      }, 500);
  },

  // Set the loading notification to update with progress every 6/10ths of
  // a second.  Does not indicate overall progress, simply that work is
  // being done.
  setLoadingNotification: function() {
    gees.dom.show('geecheck-loader');

    // Update the progress message to indicate activity to the user.
    geecheck.progressInterval = setInterval(function() {
      var curProgress = gees.dom.get('progress-indicator').innerHTML;
      curProgress = curProgress.length == 3 ? '' : curProgress + '.';
      gees.dom.get('progress-indicator').innerHTML = curProgress;
    }, 600);
  },

  // Request the test script.  This runs the tests and returns the results.
  requestData: function() {
    var url = '/cgi-bin/run_geecheck.py';
    var httpRequest = new XMLHttpRequest();
    httpRequest.open('GET', url, false);
    httpRequest.send();

    // Test result data.
    var data = httpRequest.responseText;

    // If response was received, and is not empty, render results.  Else,
    // display an error message to the user.
    if (httpRequest.status == '200' && data.replace(/\s/g, '') != '') {

      try {

        // Convert response to JSON.
        data = JSON.parse(data);

        // If no JSON errors exist, parse test results and render them.
        if (!data.error) {
          gees.dom.show('test-summary');
          gees.dom.show('test-results');
          geecheck.renderTestResultData(data);
        } else {
          this.errorMessage(data.error);
        }

      // If this catch occurs, the json is likely invalid.  Message will tell
      // the user more.
      } catch(error) {
        this.errorMessage(error.message);
      }
    } else {

      // Deliver a default error message.
      this.errorMessage();
    }

    // Hide the loading notification and stop the progress interval.
    gees.dom.hide('geecheck-loader');
    window.clearInterval(geecheck.progressInterval);
  },

  // Deliver an error message to the user.
  errorMessage: function(message) {
    var defaultMessage =
        'No tests were run.  Please check the status of your test scripts. ';
    message = message || defaultMessage;
    gees.dom.show('error-summary');
    gees.dom.get('error-summary').innerHTML = message;
  },

  // Render the results of the test in the UI.
  renderTestResultData: function(testData) {

    // Identify holders for main results and summary results.
    var summaryDiv = gees.dom.get('test-summary-content');
    var resultsDiv = gees.dom.get('test-results-content');

    // Iterate over all the tests and create UI elements for each.
    for (var testType in testData) {
      // Check for test suite directories that weren't run:
      if (typeof testData[testType] !== 'object' || !testData[testType])
      {
        geecheck.renderResultSummary({
            description: '<i>Test suite disabled, or not found!</i>',
            status: 'SKIPPED',
            test: testType + '/'
          }, summaryDiv);
        continue;
      }

      // Report results of performed tests:
      var results = testData[testType].test_results;
      if (results.length > 0) {
        for (var i = 0; i < results.length; i++) {
          var result = results[i];

          // Render a summary of each test.
          geecheck.renderResultSummary(result, summaryDiv);

          // Render a detailed account of each test.
          geecheck.renderResultsDetail(result, resultsDiv);
        }
      }
    }
  },

  // Render a summary of the results.
  renderResultSummary: function(result, summaryDiv) {
    var holderSpan = gees.dom.create('span');

    // Update the format of the description.
    var description = result.description == null ? '' : result.description;
    description = description.split('\n')[0];
    if (result.status == 'ERROR') {
      description = '<i>There was an error performing this test.</i>';
    }

    // Create a summary item for each test that took place.
    this.createSummarylabel('test_id', result.test, holderSpan);
    this.createSummarylabel(result.status, result.status, holderSpan);
    this.createSummarylabel('desc', description, holderSpan);

    // Append the test summaries to the main summary holder.
    summaryDiv.appendChild(holderSpan);
  },

  // Create a label for a summary item.
  createSummarylabel: function(className, content, parent) {
    var label = gees.dom.create('label');
    label.className = className;
    label.innerHTML = content;
    parent.appendChild(label);
  },

  // Render information about the result of a test.
  renderResultsDetail: function(result, resultsDiv) {
    var holderSpan = gees.dom.create('span');

    // Create spans for each element of a test result.
    var status = result.status;
    this.createResultLabel('Test class:', result.class, holderSpan);
    this.createResultLabel('Description:', result.description, holderSpan);
    this.createResultLabel('Test module:', result.module, holderSpan);
    this.createResultLabel('Test status:', status, holderSpan, status);
    this.createResultLabel('Test name:', result.test, holderSpan);

    // Display selected error details if they exist.
    if (result.error_msg) {
      this.createResultLabel('Error message:', result.error_msg, holderSpan);
    }

    if (result.error_type) {
      this.createResultLabel('Error type:', result.error_type, holderSpan);
    }

    if (result.error_traceback) {
      this.createResultLabel(
          'Error traceback:', result.error_traceback, holderSpan);
    }

    if (result.skip_reason) {
      this.createResultLabel('Reason for skipping:', result.skip_reason,
          holderSpan);
    }

    // Append the test result items to a main result holder.
    resultsDiv.appendChild(holderSpan);
  },

  // Create a label for an item from a test result.
  createResultLabel: function(emContent, labelContent, parent, className) {
    var label = gees.dom.create('label');
    if (className) {
      label.className = className;
    }
    label.innerHTML = '<em>' + emContent + '</em>' +
        '<label>' + labelContent + '</label>';
    parent.appendChild(label);
  }
};

// Run the loadData function when this script is loaded.
geecheck.loadData();
