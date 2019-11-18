#!/usr/bin/env python2.7
#
# Copyright 2017 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Search Pusher application.

WSGI application for managing search data pushing.
"""
import cgi
import cgitb
import logging
import logging.config
from StringIO import StringIO
import sys

from serve import constants
from serve import http_io
from serve.push.search.core import search_push_servlet

# Load logging configuration file.
logging.config.fileConfig("/opt/google/gehttpd/conf/ge_logging.conf",
                          disable_existing_loggers=False)

# Get logger.
logger = logging.getLogger("ge_search_publisher")


class SearchPushApp(object):
  """Search data pusher application class.

  Implements WSGI application interface.
  """
  STATUS_OK = "200 OK"
  STATUS_ERROR = "500 Internal Server Error"
  RESPONSE_HEADERS = [("Content-type", "text/html")]

  def __init__(self):
    """Initialises search push application."""
    self._search_pusher = search_push_servlet.SearchPushServlet()

  def __call__(self, environ, start_response):
    """Executes an application.

    Parses HTTP requests into internal request object and delegates
    processing to SearchPushServlet.

    Args:
      environ: WSGI environment.
      start_response: callable that starts response.
    Returns:
      response body.
    """
    form = cgi.FieldStorage(fp=environ["wsgi.input"],
                            environ=environ)

    # Get parameters from HTTP request.
    request = http_io.Request()
    for key in form.keys():
      request.SetParameter(key, form.getvalue(key, ""))

    response = http_io.Response()
    if request.parameters:
      self._search_pusher.DoGet(request, response)
    else:
      logger.error("Internal Error - Request has no parameters.")
      http_io.ResponseWriter.AddBodyElement(
          response,
          constants.HDR_STATUS_MESSAGE,
          "Internal Error - Request has no parameters")
      http_io.AddBodyElement(response, constants.HDR_STATUS_CODE,
                             constants.STATUS_FAILURE)
    try:
      start_response(SearchPushApp.STATUS_OK, SearchPushApp.RESPONSE_HEADERS)
      return response.body
    except Exception:
      exc_info = sys.exc_info()
      start_response(SearchPushApp.STATUS_ERROR,
                     SearchPushApp.RESPONSE_HEADERS, exc_info)
      return self.__FormatException(exc_info)

  def __FormatException(self, exc_info):
    dummy_file = StringIO()
    hook = cgitb.Hook(file=dummy_file)
    hook(*exc_info)
    return [dummy_file.getvalue()]


# application instance to use by server.
application = SearchPushApp()


def main():
  pass

if __name__ == "main":
  main()
