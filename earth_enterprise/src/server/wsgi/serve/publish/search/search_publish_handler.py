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

"""Module for serving search data publishing requests."""

import logging
import logging.config

import psycopg2     # No lint.

from common import exceptions
from serve import constants
from serve import http_io
from serve.publish.search import search_publish_manager

# Get logger.
logger = logging.getLogger("ge_search_publisher")


class SearchPublishHandler(object):
  """Class for delegating search publishing requests to handlers."""

  def __init__(self):
    """Inits SearchPublishServlet."""
    try:
      self._search_publish_manager = (
          search_publish_manager.SearchPublishManager())
    except exceptions.Error as e:
      self._search_publish_manager = None
      logger.error(e)

  def DoRequest(self, request, response):
    """Handles request by delegating it to search publish manager."""
    assert isinstance(request, http_io.Request)
    assert isinstance(response, http_io.Response)
    # Check for init failure and return an error status and appropriate message.
    if not self._search_publish_manager:
      http_io.ResponseWriter.AddJsonFailureBody(
          response,
          "Server-side Internal Error: Failure to init SearchPublisher")
      return

    try:
      cmd = request.GetParameter(constants.CMD)

      if not cmd:
        raise exceptions.StreamPublisherServletException(
            "Internal Error - Missing Request Command.")

      if cmd == constants.CMD_QUERY:
        self._search_publish_manager.HandleQueryRequest(request, response)
      elif cmd == constants.CMD_PING:
        self._search_publish_manager.HandlePingRequest(request, response)
      elif cmd == constants.CMD_ADD_SEARCH_DEF:
        self._search_publish_manager.HandleAddSearchDefRequest(
            request, response)
      elif cmd == constants.CMD_DELETE_SEARCH_DEF:
        self._search_publish_manager.HandleDeleteSearchDefRequest(
            request, response)
      else:
        raise exceptions.SearchPublishServeException(
            "Internal Error - Invalid Request Command: %s." % cmd)
    except exceptions.SearchPublishServeException as e:
      logger.error(e)
      http_io.ResponseWriter.AddJsonFailureBody(response, str(e))
    except (psycopg2.Warning, psycopg2.Error) as e:
      logger.error(e)
      http_io.ResponseWriter.AddJsonFailureBody(response, str(e))
    except Exception as e:
      logger.error(e)
      http_io.ResponseWriter.AddJsonFailureBody(
          response, "Server-side Internal Error: {0}".format(e))
