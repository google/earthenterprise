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


"""Search Publisher application.

WSGI application for managing search data publishing.
"""
import cgi
import cgitb
import logging
import logging.config
from StringIO import StringIO
import sys
import urlparse

from common import exceptions
from serve import constants
from serve import http_io
from serve.publish.search import search_publish_handler

# Load logging configuration file.
logging.config.fileConfig("/opt/google/gehttpd/conf/ge_logging.conf",
                          disable_existing_loggers=False)

# Get logger.
logger = logging.getLogger("ge_search_publisher")


class SearchPublishApp(object):
  """Search data publish application class.

  Implements WSGI application interface.
  """
  STATUS_OK = "200 OK"
  STATUS_ERROR = "500 Internal Server Error"
  RESPONSE_HEADERS = [("Content-type", "text/html")]

  def __init__(self):
    """Initialises search publish application."""
    self._publish_handler = search_publish_handler.SearchPublishHandler()

  def __call__(self, environ, start_response):
    """Executes an application.

    Parses HTTP requests into internal request object and delegates
    processing to SearchPublishHandler.

    Args:
      environ: WSGI environment.
      start_response: callable that starts response.
    Returns:
      response body.
    """
    request_method = "GET"
    if "REQUEST_METHOD" in environ:
      request_method = environ["REQUEST_METHOD"]

    # Get parameters from HTTP request.
    request = http_io.Request()
    if request_method == "GET":
      form = cgi.FieldStorage(fp=environ["wsgi.input"],
                              environ=environ)

      for key in form.keys():
        request.SetParameter(key, form.getvalue(key, ""))
    else:
      try:
        request_body_size = int(environ.get("CONTENT_LENGTH", 0))
      except ValueError:
        request_body_size = 0
      post_input = environ["wsgi.input"].read(request_body_size)
      logging.debug("POST request body: %s", post_input)
      self.__ParsePostInput(post_input, request)

    response = http_io.Response()
    if request.parameters:
      self._publish_handler.DoRequest(request, response)
    else:
      logger.error("Internal Error - Request has no parameters.")
      http_io.ResponseWriter.AddJsonFailureBody(
          response, "Internal Error - Request has no parameters")

    try:
      start_response(SearchPublishApp.STATUS_OK,
                     SearchPublishApp.RESPONSE_HEADERS)
      return response.body
    except Exception:
      exc_info = sys.exc_info()
      start_response(SearchPublishApp.STATUS_ERROR,
                     SearchPublishApp.RESPONSE_HEADERS, exc_info)
      return self.__FormatException(exc_info)

  def __FormatException(self, exc_info):
    dummy_file = StringIO()
    hook = cgitb.Hook(file=dummy_file)
    hook(*exc_info)
    return [dummy_file.getvalue()]

  def __ParsePostInput(self, post_input, request):
    post_dct = urlparse.parse_qs(post_input)
    cmd = post_dct.get(constants.CMD, [""])[0]
    if not cmd:
      return

    # Extract all the parameters for corrresponding command.
    if cmd == constants.CMD_ADD_SEARCH_DEF:
      logger.debug("%s: %s", constants.CMD,
                   post_dct.get(constants.CMD, [""])[0])
      logger.debug("%s: %s", constants.SEARCH_DEF_NAME,
                   post_dct.get(constants.SEARCH_DEF_NAME, [""])[0])
      logger.debug("%s: %s", constants.SEARCH_DEF,
                   post_dct.get(constants.SEARCH_DEF, [""])[0])
      request.SetParameter(
          constants.SEARCH_DEF_NAME,
          post_dct.get(constants.SEARCH_DEF_NAME, [""])[0])
      request.SetParameter(
          constants.SEARCH_DEF,
          post_dct.get(constants.SEARCH_DEF, [""])[0])
    else:
      raise exceptions.SearchPublishServeException(
          "Internal Error - Invalid Request Command: %s." % cmd)

    request.SetParameter(constants.CMD, cmd)


# application instance to use by server.
application = SearchPublishApp()


def main():
  pass

if __name__ == "main":
  main()
