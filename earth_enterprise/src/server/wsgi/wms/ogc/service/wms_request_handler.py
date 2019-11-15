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

"""Utils for when the request is partially correct."""

import logging

import wms.ogc.common.utils as utils
import wms.ogc.gee.maps_server_handler as gms
import wms.ogc.implementation.impl_v111 as v111_impl
import wms.ogc.implementation.impl_v130 as v130_impl


# Others could be WFS (features) and WMTS (tiles)
_SERVICES = set(["WMS"])

_SERVICE_HANDLERS = {
    "1.1.1": {
        "GetCapabilities": v111_impl.GetCapabilitiesRequest,
        "GetMap": v111_impl.GetMapRequest,
        "bad_service_handler": v111_impl.BadWmsRequest
        },
    "1.3.0": {
        "GetCapabilities": v130_impl.GetCapabilitiesRequest,
        "GetMap": v130_impl.GetMapRequest,
        "bad_service_handler": v130_impl.BadWmsRequest
        }
    }

_TEXT_CONTENT_TYPE = ("Content-Type", "text/html")
_VERSION_V130 = "1.3.0"

# Get logger
logger = logging.getLogger("wms_maps")


class WMSRequestHandler(object):
  """Class for handling the WMS requests."""

  def __init__(self):
    self.layer_obj = gms.GEELayer()

  def GenerateResponse(self, parameters):
    """The workhorse of servicing the WMS call.

    Args:
      parameters: All of the query parameters (SERVICE, FORMAT etc), with
        the additions of 'this-endpoint'.
    Returns:
      A tuple of status, other headers, and the output, whether an image or
        some xml representing the Capabilities or a ServiceException.
    """
    good_request = "200 OK"
    bad_request = "400 Bad request"

    headers = []
    output = ""

    logger.debug("Generating response for the WMS request.")

    is_valid_service, msg_to_client = self.IsValidService(parameters)

    if not is_valid_service:
      status = bad_request
      headers.append(_TEXT_CONTENT_TYPE)
      output = msg_to_client
      return status, headers, output

    is_valid_version, msg_to_client, version_handler = self.IsValidVersion(
        parameters)

    if not is_valid_version:
      status = bad_request
      headers.append(_TEXT_CONTENT_TYPE)
      output = msg_to_client
      return status, headers, output

    request_handler = self.GetRequestHandler(parameters, version_handler)
    utils.logger.debug("Generating output for the WMS service request")
    status = good_request

    # A successful ServiceError would count as a status 200.
    headers, output = request_handler.GenerateOutput()

    return status, headers, output

  def GetRequestHandler(self, parameters, version_handler):
    """Get the request handler appropriate to the WMS request.

    Args:
      parameters: All of the query parameters (SERVICE, FORMAT etc), with
        the additions of 'this-endpoint'.
      version_handler: Version handler appropriate to the WMS version.
    Returns:
      req_hdlr_obj: Request handler.
    """
    req_hdlr_obj = None

    request_name = utils.GetValue(parameters, "request")
    request_handler = version_handler.get(request_name, None)

    if request_handler is None:
      req_hdlr_obj = version_handler["bad_service_handler"](request_name)
    else:
      req_hdlr_obj = request_handler(self.layer_obj, parameters)

    logger.debug("Type of WMS request: %s", type(req_hdlr_obj))

    return req_hdlr_obj

  def IsValidService(self, parameters):
    """Checks if the service is valid.

    Args:
      parameters: All of the query parameters (SERVICE, FORMAT etc), with
        the additions of this-endpoint.
    Returns:
      is_valid_service: If the service is valid.
    msg_to_client: Message sent back to the client.
    """
    logger.debug("Checking if the service is a valid one.")

    is_valid_service = False

    service = utils.GetValue(parameters, "service")
    request_type = utils.GetValue(parameters, "request")

    if service not in _SERVICES:
      if service is None:
        if request_type == "GetCapabilities":
          # SERVICE parameter is mandatory for GetCapabilities request.
          logger.error("No service parameter in the WMS request")
          msg_to_client = ("MISSING SERVICE PARAMETER "
                           "(expected something like WMS)")
        elif request_type == "GetMap":
          # SERVICE parameter is optional for GetMap request.
          is_valid_service = True
          msg_to_client = "VALID SERVICE PARAMETER RECEIVED"
        else:
          # Missing or no parameters in the WMS request.
          logger.error("Invalid WMS request received")
          msg_to_client = "INVALID WMS REQUEST RECEIVED"
      else:
        logger.error("Unknown service in the WMS request: \'%s\'", service)
        msg_to_client = "BAD SERVICE PARAMETER (expected something like WMS)"
    else:
      is_valid_service = True
      msg_to_client = "VALID SERVICE PARAMETER RECEIVED"
      logger.debug("Valid service parameter received: \'%s\'", service)

    return is_valid_service, msg_to_client

  def IsValidVersion(self, parameters):
    """Checks if the version is a valid one.

    Args:
      parameters: All of the query parameters (SERVICE, FORMAT etc), with
        the additions of this-endpoint.
    Returns:
      is_valid_version: If the version is valid.
      msg_to_client: Message sent back to the client.
      version_handler: Version handler appriopriate to the WMS version.
    """

    logger.debug("Checking if the WMS version is supported.")

    is_valid_version = False

    version = utils.GetValue(parameters, "version")

    # TODO: Add support for 1.0.7, 1.1.0 and others.
    # The MapServer code has good examples of handling the minor
    # differences between versions.
    if version is None or version.startswith("1.3."):
      # Tolerate 1.3.x for no reason.
      # If there is no version parameter, we're supposed to reply with
      # the highest we support, ie 1.3.0
      version = _VERSION_V130
      # We cheat here - we don't use this version yet in the
      # supposedly common handling, but this lets us.
      parameters["version"] = _VERSION_V130

    version_handler = _SERVICE_HANDLERS.get(version, None)

    if version_handler is None:
      # Have a version, but it's something other than we support.
      # We can't return a ServiceException because we don't know what
      # format to send it in - it changes between versions.
      msg_to_client = "Version %s not supported" % version
      logger.error("WMS Version not supported: \'%s\'", version)
    else:
      is_valid_version = True
      msg_to_client = "Version %s supported" % version
      logger.debug("WMS Version supported: \'%s\'", version)

    return is_valid_version, msg_to_client, version_handler


def main():
  obj = WMSRequestHandler()
  is_valid_service = obj.IsValidService("WXX")
  print is_valid_service

if __name__ == "__main__":
  main()
