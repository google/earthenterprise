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


"""Entrypoint for WMS-family services."""

import logging
import logging.config

import wms.ogc.common.utils as utils
import wms.ogc.service.wms_request_handler as wms

# Create logger
logging.config.fileConfig("/opt/google/gehttpd/conf/ge_logging.conf",
                          disable_existing_loggers=False)
logger = logging.getLogger("wms_maps")


class WMS(object):
  """class for the WMS service application."""

  def __init__(self):
    self.handler_obj = wms.WMSRequestHandler()

  def __call__(self, environ, start_response):
    """Callable method for the WMS service.

    Args:
      environ: A list which contains the WSGI environment information.
      start_response: callable for setting the HTTP headers and status.
    Returns:
      response_body: WMS response sent back to the client wrapped in a list.
    """
    response_headers = []

    # Fetch all the attributes provided by the user
    # in the WMS request like SERVICE,REQUEST,VERSION etc
    parameters = utils.GetParameters(environ)

    status, header_pairs, output = self.handler_obj.GenerateResponse(parameters)

    for content_type in header_pairs:
      response_headers.append(content_type)

    response_body = output
    start_response(status, response_headers)

    return [response_body]

application = WMS()


def main():
  app = WMS()
  app()

if __name__ == "__main__":
  main()
