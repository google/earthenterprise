// Copyright 2005 Google Inc. All Rights Reserved.

function createXmlHttpRequest() {
  try {
    if (typeof ActiveXObject != 'undefined') {
      return new ActiveXObject('Microsoft.XMLHTTP');
    } else if (window['XMLHttpRequest']) {
      return new XMLHttpRequest();
    }
  } catch (e) {
  }

  return null;
}

function downloadUrl(url, callback) {
  var request = createXmlHttpRequest();
  if (!request) {
    return false;
  }

  request.onreadystatechange = function() {
    if (request.readyState == 4) {
      var statusAndResponseText =
          xmlHttpRequestGetStatusAndResponseText(request);
      var status = statusAndResponseText.status;
      var responseText = statusAndResponseText.responseText;

      callback(responseText, status);
      request.onreadystatechange = function() {};
    }
  };

  request.open('GET', url, true);
  request.send(null);

  return true;
}

function xmlHttpRequestGetStatusAndResponseText(request) {
  var status = -1;
  var responseText = null;

  try {
    status = request.status;
    responseText = request.responseText;
  } catch (e) {
  }

  return {
    status: status,
    responseText: responseText
  };
}
