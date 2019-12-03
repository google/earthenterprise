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


"""Module for serving snippets managing requests."""

import logging
import logging.config

import psycopg2     # No lint.

from common import exceptions
from serve import constants
from serve import http_io
from serve.snippets import snippets_manager

# Get logger.
logger = logging.getLogger("ge_snippets")


class SnippetsHandler(object):
  """Class for delegating snippets managing requests to handlers."""

  def __init__(self):
    """Inits SnippetsHandler."""
    try:
      self._manager = snippets_manager.SnippetsManager()
    except exceptions.Error as e:
      self._manager = None
      logger.error(e)

  def DoRequest(self, request, response):
    """Handles request by delegating it to manager."""
    assert isinstance(request, http_io.Request)
    assert isinstance(response, http_io.Response)
    # Check for init failure and return an error status and appropriate message.
    if not self._manager:
      http_io.ResponseWriter.AddJsonFailureBody(
          response,
          "Server-side Internal Error: Failure to init SnippetsHandler")
      return

    try:
      cmd = request.GetParameter(constants.CMD)

      if not cmd:
        raise exceptions.StreamPublisherServletException(
            "Internal Error - Missing Request Command.")

      if cmd == constants.CMD_QUERY:
        self._manager.HandleQueryRequest(request, response)
      elif cmd == constants.CMD_PING:
        self._manager.HandlePingRequest(request, response)
      elif cmd == constants.CMD_ADD_SNIPPET_SET:
        self._manager.HandleAddSnippetSetRequest(
            request, response)
      elif cmd == constants.CMD_DELETE_SNIPPET_SET:
        self._manager.HandleDeleteSnippetSetRequest(
            request, response)
      else:
        raise exceptions.SnippetsServeException(
            "Invalid Request Command: {0}.".format(cmd))
    except exceptions.SnippetsServeException as e:
      logger.error(e)
      http_io.ResponseWriter.AddJsonFailureBody(response, str(e))
    except (psycopg2.Warning, psycopg2.Error) as e:
      logger.error(e)
      http_io.ResponseWriter.AddJsonFailureBody(
          response,
          "Database error: {0}".format(e))
    except Exception as e:
      logger.error(e)
      http_io.ResponseWriter.AddJsonFailureBody(
          response, "Server-side Internal Error: {0}".format(e))
