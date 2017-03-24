#!/usr/bin/python
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

"""The Fusion Asset Manager application.

WSGI application for managing Fusion Assset managing requests.
"""

import cgi
import logging
import logging.config
import sys
import urlparse

from common import exceptions
from fusion.asset_manager import asset_manager_service
from serve import http_io

logger = logging.getLogger("gefusion")


class AssetManagerApp(object):
  """Asset Manager serving backend application.

  Implements WSGI application interface.
  """
  STATUS_OK = "200 OK"
  STATUS_ERROR = "500 Internal Server Error"
  RESPONSE_HEADERS = [("Content-type", "application/json")]

  def __init__(self):
    """Initializes AssetManager application."""
    self._service = asset_manager_service.AssetManagerService()

  def __call__(self, environ, start_response):
    """Executes an application.

    Parses HTTP requests into internal request object and delegates
    processing to service.

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
      logger.debug("POST request body: %s", post_input)
      self.__ParsePostInput(post_input, request)

    response = http_io.Response()
    if request.parameters:
      self._service.DoRequest(request, response)
    else:
      logger.error("Internal Error - Request has no parameters.")
      http_io.ResponseWriter.AddJsonFailureBody(
          response, "Internal Error - Request has no parameters")

    try:
      start_response(AssetManagerApp.STATUS_OK,
                     AssetManagerApp.RESPONSE_HEADERS)
      return response.body
    except Exception:
      exc_info = sys.exc_info()
      start_response(AssetManagerApp.STATUS_ERROR,
                     AssetManagerApp.RESPONSE_HEADERS, exc_info)
      return exceptions.FormatException(exc_info)

  def __ParsePostInput(self, post_input, request):
    """Parses POST request input.

    Args:
      post_input: POST request input.
      request: request object to collect exptracted parameters.
    Raises:
      Error exception.
    """
    post_dct = urlparse.parse_qs(post_input)
    # TODO: parsing POST request.

    raise exceptions.Error(
        "Internal Error - HTTP POST Request is not supported.")


# application instance to use by server.
application = AssetManagerApp()


def main():
  pass


if __name__ == "__main__":
  main()
