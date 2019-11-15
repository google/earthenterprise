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

"""Module for POI Search WSGI application."""

import logging
import sys
import traceback

from search.common import exceptions
from search.plugin import poi_search_handler

logging.config.fileConfig("/opt/google/gehttpd/conf/ge_logging.conf",
                          disable_existing_loggers=False)


class GePoiSearchApp(object):
  """GePoiSearchApp implements the callable method.

  WSGI Application class interfaces with the web server and
  receives the search queries in the form of URL parameters.
  Web server calls the callable method __call__ to start the application.
  """

  def __init__(self):
    """Inits the status and response_headers for the HTTP response."""

    self._status = "%s"
    self._poi = poi_search_handler.POISearch()

  def __call__(self, environ, start_response):
    """Callable method for the WSGI application.

    Callable method which performs a POI search and returns the
    KML/JSONP format output back in the HTTP response.

    Args:
        environ: A list which contains the WSGI environment information.
        start_response: callable for setting the HTTP headers and status.
    Returns:
        response_body: Search Results sent back to the client wrapped in a list.
    """
    response_headers = []

    content_type = ""
    status = ""
    response_body = ""
    response_type = ""

    try:
      response_body, response_type = self._poi.HandleSearchRequest(environ)
      content_type = self._poi.utils.GetContentType(response_type)
      status = self._status % ("200 OK")

    except exceptions.BadQueryException as e:
      content_type = self._poi.utils.content_type % ("text/plain")
      status = self._status % ("400 Bad Request")
      self._poi.logger.error(
          "%s Status %s sent back to client.", e, status)
      response_body = "%s" % e
    except exceptions.PoolConnectionException as e:
      # Catch PoolConnectionException when connections
      # are not available in the pool
      content_type = self._poi.utils.content_type % ("text/plain")
      status = self._status % ("500 Internal Server Error")
      self._poi.logger.error(
          "%s Status %s sent back to client.", e, status)
      response_body = "Client Connections exceeded"
    except Exception as e:
      # Any kind of Exception/Error occurs, send "500 Internal Server Error".
      content_type = self._poi.utils.content_type % ("text/plain")
      status = self._status % ("500 Internal Server Error")
      self._poi.logger.error(
          "%s Status %s sent back to client.", e, status)
      _, _, tb = sys.exc_info()
      stack_trace = "".join(traceback.format_tb(tb))
      self._poi.logger.debug("Stack trace:\n%s", stack_trace)
      response_body = "%s" % e

    response_headers.append(tuple(content_type.split(",")))

    start_response(status, response_headers)

    return [response_body]


application = GePoiSearchApp()


def main():
  application()

if __name__ == "__main__":
  main()
