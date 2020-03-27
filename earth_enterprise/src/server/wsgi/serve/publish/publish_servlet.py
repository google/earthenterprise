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

"""Publish servlet.

Module for dispatching publish requests to handlers.
"""

import logging
import logging.config

import psycopg2  # NO LINT. third party package should be before own ones.

from common import exceptions
from serve import constants
from serve import http_io
from serve.publish import publish_manager


logger = logging.getLogger("ge_stream_publisher")


class PublishServlet(object):
  """Class for dispatching publish requests to handlers."""

  def __init__(self):
    """Inits publish servlet."""
    try:
      self._publish_manager = publish_manager.PublishManager()
    except exceptions.PublishServeException as e:
      self._publish_manager = None
      logger.error(e, exc_info=True)

    except psycopg2.Warning as w:
      self._publish_manager = None
      logger.error(w, exc_info=True)

    except psycopg2.Error as e:
      self._publish_manager = None
      logger.error(e, exc_info=True)

    except Exception as e:
      self._publish_manager = None
      logger.error(e, exc_info=True)

  def DoGet(self, request, response):
    """Handles request by delegating it to publish manager."""
    # Check for init failure and return an error status and appropriate message.
    if not self._publish_manager:
      if response:
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_STATUS_MESSAGE,
            ("Server-side Internal Error: Failure to init PublishServlet"))
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_STATUS_CODE, constants.STATUS_FAILURE)

      return

    try:
      logger.info(request)
      cmd = request.GetParameter(constants.CMD)

      if not cmd:
        raise exceptions.PublishServeException(
            "Internal Error - Missing request command.")

      if cmd == constants.CMD_QUERY:
        self._publish_manager.HandleQueryRequest(request, response)
      elif cmd == constants.CMD_PING:
        self._publish_manager.HandlePingRequest(request, response)
      elif cmd == constants.CMD_PUBLISH_DB:
        self._publish_manager.HandlePublishRequest(request, response)
      elif cmd == constants.CMD_REPUBLISH_DB:
        self._publish_manager.HandleRepublishRequest(request, response)
      elif cmd == constants.CMD_SWAP_TARGETS:
        self._publish_manager.HandleSwapTargetsRequest(request, response)
      elif cmd == constants.CMD_UNPUBLISH_DB:
        self._publish_manager.HandleUnpublishRequest(request, response)
      elif cmd == constants.CMD_ADD_VS:
        self._publish_manager.HandleAddVsRequest(request, response)
      elif cmd == constants.CMD_DELETE_VS:
        self._publish_manager.HandleDeleteVsRequest(request, response)
      elif cmd == constants.CMD_CLEANUP:
        self._publish_manager.HandleCleanupRequest(request, response)
      elif cmd == constants.CMD_RESET:
        self._publish_manager.HandleResetRequest(request, response)
      else:
        raise exceptions.PublishServeException(
            "Internal Error - Invalid request command: %s." % cmd)
      return

    except exceptions.PublishServeException as e:
      logger.error(e, exc_info=True)
      http_io.ResponseWriter.AddBodyElement(response,
                                            constants.HDR_STATUS_MESSAGE,
                                            e)
    except psycopg2.Warning as w:
      logger.error(w, exc_info=True)
      http_io.ResponseWriter.AddBodyElement(response,
                                            constants.HDR_STATUS_MESSAGE,
                                            w)
    except psycopg2.Error as e:
      logger.error(e, exc_info=True)
      http_io.ResponseWriter.AddBodyElement(response,
                                            constants.HDR_STATUS_MESSAGE,
                                            e)
    except Exception as e:
      logger.error(e, exc_info=True)
      http_io.ResponseWriter.AddBodyElement(response,
                                            constants.HDR_STATUS_MESSAGE,
                                            "Server-side Internal Error")
    http_io.ResponseWriter.AddBodyElement(response,
                                          constants.HDR_STATUS_CODE,
                                          constants.STATUS_FAILURE)


def main():
  pass


if __name__ == "__main__":
  main()
