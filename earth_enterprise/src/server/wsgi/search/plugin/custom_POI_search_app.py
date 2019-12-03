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

"""Module for Custom POI Search using Google Places database."""

from search.common import exceptions
from search.plugin import custom_POI_search_handler


class GeCustomPOISearchApp(object):
  """GeCustomPOISearchApp implements the callable method.

  Class will perform a POI search using the Google Places database.
  The results are returned in XML/JSON format to the caller.

  WSGI Application class interfaces with the web server and
  receives the search queries in the form of URL parameters.
  Web server calls the callable method __call__ to start the application.
  """

  def __init__(self):
    """Inits the status and response_headers for the HTTP response."""
    self._status = "%s"
    self._custom = custom_POI_search_handler.CustomPOISearch()

  def __call__(self, environ, start_response):
    """Callable method for the WSGI interface.

    Callable method which performs a custom POI search and returns the
    XML/JSON format output back in the HTTP response.

    Args:
        environ: A list which contains the WSGI environment information.
        start_response: callable for setting the HTTP headers and status.
    Returns:
        response_body: Search Results sent back to the client wrapped in a list.
    """
    response_headers = []

    status = ""
    response_body = ""

    response_type = self._custom.utils.GetResponseType(environ)

    try:
      response_body = self._custom.HandleSearchRequest(environ)
      content_type = self._custom.utils.GetContentType(response_type)
      status = self._status % ("200 OK")
    except exceptions.BadQueryException as e:
      # For empty or incorrect search queries.
      content_type = self._custom.utils.content_type % ("text/plain")
      status = self._status % ("400 Bad Request")
      self._custom.logger.debug(
          "%s, status %s sent back to client.", e, status)
      response_body = "%s" % e
    except Exception, e:
      # Any kind of Exception/Error occurs, send "500 Internal Server Error".
      content_type = self._custom.utils.content_type % ("text/plain")
      status = self._status % ("500 Internal Server Error")
      self._custom.logger.debug(
          "%s, status %s sent back to client.", e, status)
      response_body = "%s" % e

    response_headers.append(tuple(content_type.split(",")))

    start_response(status, response_headers)
    return [response_body]

application = GeCustomPOISearchApp()


def main():
  application()

if __name__ == "__main__":
  main()
