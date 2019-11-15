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

"""The stream_pusher module.

Contains classes for dispatching stream push requests to handlers.
"""
import logging

import psycopg2

from common import exceptions
from serve import constants
from serve import http_io
from serve.push.stream import stream_push_manager

# Get logger
logger = logging.getLogger("ge_stream_publisher")


class StreamPushServlet(object):
  """Class for delegating stream push requests to handlers."""

  def __init__(self):
    """Inits StreamPusher."""
    try:
      logger.info("Stream Pusher initializing...")
      self._stream_push_manager = stream_push_manager.StreamPushManager()

    except exceptions.StreamPushServeException as e:
      self._stream_push_manager = None
      logger.error(e)
    except psycopg2.Warning as w:
      self._stream_push_manager = None
      logger.error(w)
    except psycopg2.Error as e:
      self._stream_push_manager = None
      logger.error(e)
    except Exception as e:
      self._stream_push_manager = None
      logger.error(e)

  def DoGet(self, request, response):
    """Handles GET request by delegating it to stream push manager.

    Args:
      request: request object.
      response: response object.
    Raises:
      StreamPushServeException, psycopg2.Warning/Error
    Returns:
      response object.
    """
    # Check for init failure and return an error status and appropriate message.
    if not self._stream_push_manager:
      http_io.ResponseWriter.AddBodyElement(
          response,
          constants.HDR_STATUS_MESSAGE,
          ("Server-side Internal Error: Failure to init StreamPusher"))
      http_io.ResponseWriter.AddBodyElement(response,
                                            constants.HDR_STATUS_CODE,
                                            constants.STATUS_FAILURE)
      return

    try:
      cmd = request.GetParameter(constants.CMD)
      if not cmd:
        raise exceptions.StreamPushServeException(
            "Internal Error - Missing Request Command.")

      # Handle different query requests: ListDbs, PublishedDbs, Dbdetails...
      if cmd == constants.CMD_QUERY:
        self._stream_push_manager.HandleQueryRequest(request, response)
      # Ping gestream database.
      elif cmd == constants.CMD_PING:
        self._stream_push_manager.HandlePingRequest(request, response)
      # Register Fusion database/portable globe.
      elif cmd == constants.CMD_ADD_DB:
        self._stream_push_manager.HandleAddDbRequest(request, response)
      # Unregister Fusion database/ Delete portable globe.
      # TODO: consider to just unregister portable globe here,
      # while delete it in garbage collect.
      elif cmd == constants.CMD_DELETE_DB:
        self._stream_push_manager.HandleDeleteDbRequest(request, response)
      # Unregister portable globe.
      elif cmd == constants.CMD_CLEANUP_DB:
        self._stream_push_manager.HandleUnregisterPortableRequest(
            request, response)
      # Handle sync database request.
      elif cmd == constants.CMD_SYNC_DB:
        self._stream_push_manager.HandleSyncRequest(request, response)
      # Handle local transfer request.
      elif cmd == constants.CMD_LOCAL_TRANSFER:
        self._stream_push_manager.HandleLocalTransferRequest(request, response)
      # Handle garbage collect request.
      elif cmd == constants.CMD_GARBAGE_COLLECT:
        self._stream_push_manager.HandleGarbageCollectRequest(request, response)
      # Handle cleanup request.
      elif cmd == constants.CMD_CLEANUP:
        self._stream_push_manager.HandleCleanupRequest(request, response)
      else:
        raise exceptions.StreamPushServeException(
            "Internal Error - Invalid Request Command: %s." % cmd)
      return
    except exceptions.StreamPushServeException as e:
      logger.error(e)
      http_io.ResponseWriter.AddBodyElement(response,
                                            constants.HDR_STATUS_MESSAGE,
                                            e)
    except psycopg2.Warning as w:
      logger.error(w)
      http_io.ResponseWriter.AddBodyElement(response,
                                            constants.HDR_STATUS_MESSAGE,
                                            w)
    except psycopg2.Error as e:
      logger.error(e)
      http_io.ResponseWriter.AddBodyElement(response,
                                            constants.HDR_STATUS_MESSAGE,
                                            e)
    except Exception as e:
      logger.error(e)
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
