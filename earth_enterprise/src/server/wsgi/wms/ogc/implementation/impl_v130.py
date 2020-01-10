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

"""Implementation of everything that's particular to version 1.3.0."""
import logging
import StringIO

import wms.ogc.common.image_specs as image_specs
import wms.ogc.common.utils as utils
import wms.ogc.implementation.common as common
import wms.ogc.xml.v130.capabilities_wms as capabilities_wms
import wms.ogc.xml.v130.exceptions_wms as exceptions_wms

_WMS_VERSION = "1.3.0"
_XML_CONTENT_TYPE = "text/xml"
_XML_SERVICE_EXCEPTION_TYPE = "application/vnd.ogc.se_xml"
_XML_HEADER = '<?xml version="1.0" encoding="UTF-8"?>\n'
_NAME = "WMS"
_TITLE = "Google Earth WMS service."

# required wms parameters
# This crs instead of srs is the only diff. in this list from 1.1.1
_REQUIRED_PARAMS = [
    "version",
    "request",
    "layers",
    "styles",
    "crs",
    "bbox",
    "width",
    "height",
    "format"
    ]


_HEADERS_EXCEPTION = [
    ("Content-Disposition",
     'inline; filename="service-exception-%s-google.xml"' % _WMS_VERSION),
    ("Content-Type", _XML_SERVICE_EXCEPTION_TYPE)]

# Get logger
logger = logging.getLogger("wms_maps")


class ServiceException(Exception):
  """Represents a 1.3.0 WMS service exception."""

  def __init__(self, code=None, message="Unspecified Error"):
    super(ServiceException, self).__init__(code, message)
    self.code = code
    self.message = message

  def Xml(self):
    """Produces the XML response.

    Returns:
        The XML response.
    """
    xml_text = StringIO.StringIO()
    xml_text.write(_XML_HEADER)

    service_exception = exceptions_wms.ServiceExceptionType(
        self.code,
        self.message
    )
    service_exception_report = exceptions_wms.ServiceExceptionReport(
        _WMS_VERSION, [service_exception])

    service_exception_report.export(xml_text, 0)

    return xml_text.getvalue()


class GetCapabilitiesRequest(object):
  """Represents a WMS 1.3.0 GetCapabilities request."""

  def __init__(self, layer_obj, parameters):
    self.parameters = parameters
    self.layer_obj = layer_obj
    self.capabilities_xml = ""

  def _MakeOnlineResourceXml(self, url):
    return capabilities_wms.OnlineResource(
        # xlink type
        type_="simple",
        # xlink_href
        href=url)

  def _GetMapLimitsForEpsg(self, limits, epsg_name):
    """Fetch the map limits based on the EPSG name."""

    # As per OGC specifications for EPSG:4326(Plate Carree projection),
    # the bounding box coordinates should be in
    # latitude, longitude format for WMS 1.3.0 version,
    # which otherwise are in longitude, latitude order for other projections.

    if epsg_name == "EPSG:4326":
      return (limits.y0, limits.x0, limits.y1, limits.x1)

    return (limits.x0, limits.y0, limits.x1, limits.y1)

  def GetOnlineResource(self):
    if "proxy-endpoint" in self.parameters:
      return self._MakeOnlineResourceXml(self.parameters["proxy-endpoint"])
    return self._MakeOnlineResourceXml(self.parameters["this-endpoint"])

  def GetDCPTypeInfo(self):
    resource_info = self.GetOnlineResource()

    http_info = capabilities_wms.HTTP(
        Get=capabilities_wms.Get(OnlineResource=resource_info))

    dcptype_info = [capabilities_wms.DCPType(HTTP=http_info)]

    return dcptype_info

  def GetCapabilities(self):
    dcptype_info = self.GetDCPTypeInfo()

    capabilities = capabilities_wms.OperationType(
        Format=[_XML_CONTENT_TYPE], DCPType=dcptype_info)

    return capabilities

  def GetMap(self):
    formats = [spec.content_type
               for spec in image_specs.IMAGE_SPECS.values()]

    dcptype_info = self.GetDCPTypeInfo()

    map_info = capabilities_wms.OperationType(
        Format=formats, DCPType=dcptype_info)

    return map_info

  def GetExceptionInfo(self):
    exp_info = capabilities_wms.Exception(
        Format=[_XML_SERVICE_EXCEPTION_TYPE])
    return exp_info

  def GetRequestInfo(self):
    """Process request information.

    Returns:
        request_info: request information in the capabilities xml.
    """
    capabilities_info = self.GetCapabilities()
    map_info = self.GetMap()

    request_info = capabilities_wms.Request(
        GetCapabilities=capabilities_info,
        GetMap=map_info,
        GetFeatureInfo=None)

    return request_info

  def SetCapability(self):
    request_info = self.GetRequestInfo()
    exp_info = self.GetExceptionInfo()

    self.capabilities_xml.set_Capability(
        capabilities_wms.Capability(Request=request_info, Exception=exp_info))

    self.SetLayers()

  def SetService(self):
    resource_info = self.GetOnlineResource()

    self.capabilities_xml.set_Service(
        capabilities_wms.Service(
            Name=_NAME,
            Title=_TITLE,
            Abstract=None,
            KeywordList=None,
            OnlineResource=resource_info))

  def SetLayers(self):
    """Set the layers for the capabilities xml."""

    # This outer, inaccessible layer is to give information just once;
    # the sub-layers inherit it.
    outer_layer = capabilities_wms.Layer(
        # 7.2.4.7.4 The layer is area-filling => opaque
        opaque=True,
        # 7.2.4.7.5 -- we can subset.
        noSubsets=False,
        # whether we support GetFeatureInfo
        queryable=False,
        # -- can't request it, this is just a container.
        Name=None,
        Title=_TITLE)

    server_layers_by_name = self.layer_obj.GetLayers(
        utils.GetValue(self.parameters, "server-url"),
        utils.GetValue(self.parameters, "TargetPath"))

    if not server_layers_by_name:
      # Raise ServiceException here.
      raise ServiceException(None, "Database type is not supported.")

    for layer_name, server_layer in server_layers_by_name.iteritems():
      proj = server_layer.projection
      wms_layer = capabilities_wms.Layer(
          # 7.2.4.7.4 - Even for vector maps we always get data from
          # the server, even if it's just a transparent tile.  By
          # jeffdonner's reading, this means that even the vector
          # layers are 'opaque'.
          opaque=True,
          # 7.2.4.7.5 - we can subset.
          noSubsets=False,
          queryable=False,
          Name=layer_name,
          Title=server_layer.label,
          # ex geo bounding box is required.
          EX_GeographicBoundingBox=
          capabilities_wms.EX_GeographicBoundingBox(
              westBoundLongitude=-proj.MAX_LONGITUDE,
              eastBoundLongitude=proj.MAX_LONGITUDE,
              southBoundLatitude=-proj.MAX_LATITUDE,
              northBoundLatitude=proj.MAX_LATITUDE))

      for epsg_name in proj.EPSG_NAMES:
        wms_layer.add_CRS(epsg_name)

      map_limits = proj.AdvertizedLogOuterBounds()
      bounding_boxes = []

      for epsg_name in proj.EPSG_NAMES:
        (min_x, min_y, max_x, max_y) = self._GetMapLimitsForEpsg(
            map_limits, epsg_name)
        bounding_box_object = capabilities_wms.BoundingBox(
            CRS=epsg_name,
            minx=min_x,
            miny=min_y,
            maxx=max_x,
            maxy=max_y)

        bounding_boxes.append(bounding_box_object)

      wms_layer.set_BoundingBox(bounding_boxes)

      outer_layer.add_Layer(wms_layer)

    self.capabilities_xml.get_Capability().set_Layer(outer_layer)

  def _Xml(self):
    """Generates the xml response.

    Returns:
       The XML response.
    """
    logger.debug("Begin XML response for GetCapabilities for WMS v1.3.0")

    xml_text = StringIO.StringIO()
    xml_text.write(_XML_HEADER)

    self.capabilities_xml = capabilities_wms.WMS_Capabilities(
        version=_WMS_VERSION)

    self.SetService()
    self.SetCapability()

    self.capabilities_xml.export(xml_text, 0)

    logger.debug("End XML response for GetCapabilities for WMS v1.3.0")

    return xml_text.getvalue()

  def GenerateOutput(self):
    """Generate response for GetCapabilities request.

    Returns:
      headers: List of headers for the response.
      response: GetCapabilities response.
    """

    logger.debug("Processing GetCapabilities response for WMS v1.3.0")

    # If "TargetPath" query parameter doesn't exist in the
    # input parameters, then send back "ServiceException"
    # to the client.

    target_path = utils.GetValue(self.parameters, "TargetPath")

    if not target_path:
      headers = _HEADERS_EXCEPTION
      response = ServiceException(None, "Target path is not specified.").Xml()
    else:
      try:
        headers = [
            ("Content-Disposition",
             'inline; filename="wmsCapabilities-%s-google.xml"' % _WMS_VERSION),
            ("Content-Type", _XML_CONTENT_TYPE)]
        response = self._Xml()
      except ServiceException, e:
        headers = _HEADERS_EXCEPTION
        return headers, e.Xml()

    return headers, response


class GetMapRequest(common.WmsGetMapRequest):
  """Represents WMS GetMap request."""

  def __init__(self, layer_obj, parameters):
    common.WmsGetMapRequest.__init__(
        self,
        layer_obj,
        parameters,
        _REQUIRED_PARAMS)
    logger.debug("Initializing GetMapRequest class")
    utils.DumpParms(parameters, "GetMapRequest:")

  # These must be in the derived classes because the ServiceExceptions
  # are of different types.
  def _ServiceExceptionImpl(self, code, message):
    raise ServiceException(code, message)

  def _ProcessResponse(self):
    """Processes the GetMapRequest parameters and prepares the GEE tile URL."""
    self._ProcessCommon()
    # future 130-specific processing.

  def GenerateOutput(self):
    """Executes the operation and returns the image.

    Returns:
        The image composed of tiles.
    """
    logger.debug("Generating GetMapRequest response "
                 "for WMS request for version 1.3.0")

    # If "TargetPath" query parameter doesn't exist in the
    # input parameters, then send back "ServiceException"
    # to the client.

    target_path = utils.GetValue(self.parameters, "TargetPath")
    if not target_path:
      headers = _HEADERS_EXCEPTION
      response = ServiceException(None, "Target path is not specified.").Xml()
      return headers, response
    else:
      try:
        self._ProcessResponse()
      except ServiceException, e:
        headers = _HEADERS_EXCEPTION
        return headers, e.Xml()

    logger.debug("Done generating GetMapRequest response "
                 "for WMS request for version 1.3.0")

    return self.GenerateOutputCommon()


class BadWmsRequest(object):
  """Generator of the appropriate WMS-version error.

  For when we know the WMS version, so that we can and should send a
  ServiceException, yet it's not any particular REQUEST type so we
  can't handle it from within one of those.
  """

  def __init__(self, badRequestType):
    self.bad_request_type = badRequestType

  def GenerateOutput(self):

    headers = _HEADERS_EXCEPTION
    response = ServiceException(
        common.OPERATION_NOT_SUPPORTED,
        "WMS request type \'%s\' not supported." %
        self.bad_request_type).Xml()

    return headers, response


def main():
  obj = BadWmsRequest("Not Valid Format")
  output = obj.GenerateOutput()

  print output


if __name__ == "__main__":
  main()
