#!/usr/bin/env python3.8
#
# Copyright 2017 Google Inc.
# Copyright 2021 the Open GEE Contributors
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


"""Publisher application.

WSGI application that accepts, parses publish-serving HTTP requests and
delegates them to publish processor.
"""

import cgi
import logging
import logging.config

from common import utils
from serve import constants
from serve import http_io
from serve.publish import publish_servlet


logging.config.fileConfig("/opt/google/gehttpd/conf/ge_logging.conf",
                          disable_existing_loggers=False)

logger = logging.getLogger("ge_stream_publisher")


class Application(object):
  """Publisher application class.

  Implements WSGI application interface, parses HTTP requests into internal
  request object and delegates processing to PublishServlet.
  """

  def __init__(self):
    logger.info("Publish service initializing...")
    self._publish_servlet = publish_servlet.PublishServlet()

  def __call__(self, environ, start_response):
    status = "200 OK"
    # TODO: put headers into response
    response_headers = [("Content-type", "text/html")]

    form = cgi.FieldStorage(fp=environ["wsgi.input"],
                            environ=environ)

    request = http_io.Request()
    for key in list(form.keys()):
      request.SetParameter(key, form.getvalue(key, ""))

    origin_request_host = utils.GetSchemeHostPort(environ)
    request.SetParameter(constants.ORIGIN_REQUEST_HOST, origin_request_host)
    logger.debug("Origin request host: %s", origin_request_host)

    response = http_io.Response()
    if request.parameters:
      self._publish_servlet.DoGet(request, response)
    else:
      logger.error("Internal Error - Request has no parameters")
      http_io.ResponseWriter.AddBodyElement(
          response,
          constants.HDR_STATUS_MESSAGE,
          "Internal Error - Request has no parameters")
      http_io.AddBodyElement(response, constants.HDR_STATUS_CODE,
                             constants.STATUS_FAILURE)
    response_headers = [(x.encode('ascii'), y.encode('ascii') for (x,y) in response_headers]
    start_response(status.encode('ascii'), response_headers)
    return response.body


application = Application()
