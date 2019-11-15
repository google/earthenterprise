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

"""The AssetManager service implementation module.

The AssetManager service executes asset managing requests by dispatching
to handlers.
"""

import logging
import logging.config

from common import exceptions
from fusion.asset_manager import asset_manager_handler
from serve import constants
from serve import http_io

logger = logging.getLogger("gefusion")


class AssetManagerService(object):
  """Dispatches asset managing requests to handlers."""

  def __init__(self):
    """Initializes AssetManagerService."""
    try:
      self._handler = asset_manager_handler.AssetManagerHandler()
    except exceptions.Error, e:
      self._handler = None
      logger.error(e)

  def DoRequest(self, request, response):
    """Handles request by delegating to handler."""
    assert isinstance(request, http_io.Request)
    assert isinstance(response, http_io.Response)

    # Check for init failure and return an error status and appropriate message.
    if not self._handler:
      http_io.ResponseWriter.AddJsonFailureBody(
          response,
          "Server-side Internal Error: Failure to init AssetManagerService.")
      return

    try:
      cmd = request.GetParameter(constants.CMD)
      if not cmd:
        raise exceptions.AssetManagerServeException(
            "Internal Error - Missing request command.")

      if cmd == "ListAssets":
        self._handler.HandleListAssetsRequest(request, response)
      else:
        raise exceptions.AssetManagerServeException(
            "Invalid request command: {0}.".format(cmd))
    except exceptions.AssetManagerServeException, e:
      logger.error(e)
      http_io.ResponseWriter.AddJsonFailureBody(response, str(e))
    except Exception, e:
      logger.error(e)
      http_io.ResponseWriter.AddJsonFailureBody(
          response, "Server-side Internal Error: {0}".format(e))


def main():
  pass


if __name__ == "__main__":
  main()
