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

"""Module for serving search data pushing requests."""

import logging
import logging.config

import psycopg2     # No lint.

from common import exceptions
from serve import constants
from serve import http_io
from serve.push.search.core import search_push_manager

# Get logger.
logger = logging.getLogger("ge_search_publisher")


class SearchPushServlet(object):
  """Class for delegating search pushing requests to handlers."""

  def __init__(self):
    """Inits SearchPushServlet."""
    try:
      self._search_push_manager = search_push_manager.SearchPushManager()
    except exceptions.Error as e:
      self._search_push_manager = None
      logger.error(e)

  def DoGet(self, request, response):
    """Handles request by delegating it to search push manager."""
    # Check for init failure and return an error status and appropriate message.
    if not self._search_push_manager:
      if response:
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_STATUS_MESSAGE,
            "Server-side Internal Error: Failure to init SearchPusher")
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_STATUS_CODE, constants.STATUS_FAILURE)

      return

    try:
      cmd = request.GetParameter(constants.CMD)

      if not cmd:
        raise exceptions.StreamPublisherServletException(
            "Internal Error - Missing Request Command.")

      # Handle query requests.
      if cmd == constants.CMD_QUERY:
        self._search_push_manager.HandleQueryRequest(request, response)
      # Handle gesearch DB ping request.
      elif cmd == constants.CMD_PING:
        self._search_push_manager.HandlePingRequest(request, response)
      # Handle registering DB request.
      elif cmd == constants.CMD_ADD_DB:
        self._search_push_manager.HandleAddDbRequest(request, response)
      # Handle delete DB request.
      elif cmd == constants.CMD_DELETE_DB:
        self._search_push_manager.HandleDeleteDbRequest(request, response)
      # Handle sync DB request (pushing search data and creating poi tables).
      elif cmd == constants.CMD_SYNC_DB:
        self._search_push_manager.HandleSyncRequest(request, response)
      # Deprecated.
      elif cmd == constants.CMD_PUBLISH_DB:
        # Note: Search data publishing is deprecated after 5.0.1. Return
        # success to keep compatibility between Fusion 5.0.1 and Server 5.0.2
        # and later versions.
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
      # Handle garbage collecting request - deleting not used files from
      # publish assetroot and cleaning up postgres tables.
      elif cmd == constants.CMD_GARBAGE_COLLECT:
        self._search_push_manager.HandleGarbageCollectRequest(request, response)
      # Handle file transferring request.
      elif cmd == constants.CMD_LOCAL_TRANSFER:
        self._search_push_manager.HandleLocalTransferRequest(request, response)
      else:
        raise exceptions.SearchPushServeException(
            "Internal Error - Invalid Request Command: %s." % cmd)
      return
    except exceptions.SearchPushServeException as e:
      logger.error(e)
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_MESSAGE, e)
    except (psycopg2.Warning, psycopg2.Error) as e:
      logger.error(e)
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_MESSAGE, e)
    except Exception as e:
      logger.error(e)
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_MESSAGE, "Server-side Internal Error")

    # Set failure status whether we reach this point.
    http_io.ResponseWriter.AddBodyElement(
        response, constants.HDR_STATUS_CODE, constants.STATUS_FAILURE)
