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

"""Implementation of everything that's particular to version 1.1.1."""

import logging
import StringIO

import wms.ogc.common.image_specs as image_specs
import wms.ogc.common.utils as utils
import wms.ogc.implementation.common as common
import wms.ogc.xml.v111.capabilities as capabilities_wms
import wms.ogc.xml.v111.exception as exceptions_wms

_WMS_VERSION = "1.1.1"
_XML_CONTENT_TYPE = "text/xml"
_XML_SERVICE_EXCEPTION_TYPE = "application/vnd.ogc.se_xml"
_XML_HEADER = '<?xml version="1.0" encoding="UTF-8"?>\n'
_NAME = "OGC:WMS"
_TITLE = "Google Earth WMS service."

# required wms parameters
# This crs instead of srs is the only diff. in this list from 1.1.1
_REQUIRED_PARAMS = [
    "version",
    "request",
    "layers",
    "styles",
    "srs",
    "bbox",
    "width",
    "height",
    "format"
    ]


_HEADERS_EXCEPTION = [
    ("Content-Disposition",
     'inline; filename="service-exception-%s-google.xml"' % _WMS_VERSION),
    ("Content-Type", _XML_SERVICE_EXCEPTION_TYPE)
    ]

# Get logger
logger = logging.getLogger("wms_maps")


class ServiceException(Exception):
  """Represents a WMS 1.1.1 service exception."""

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
    xml_text.write(
        "<!DOCTYPE ServiceExceptionReport SYSTEM\n"
        '"http://www.digitalearth.gov/wmt/xml/exception_1_1_1.dtd">'
        )

    service_exception = exceptions_wms.ServiceException(self.code, self.message)

    service_exception_report = (exceptions_wms.
                                ServiceExceptionReport(_WMS_VERSION))
    service_exception_report.add_ServiceException(service_exception)

    service_exception_report.export(xml_text, 0)

    return xml_text.getvalue()


class GetCapabilitiesRequest(object):
  """Represents WMS 1.1.1 GetCapabilities request."""

  def __init__(self, layer_obj, parameters):
    self.parameters = parameters
    self.layer_obj = layer_obj
    self.capabilities_xml = ""

  def SetService(self):
    resource_info = self.GetOnlineResource()

    self.capabilities_xml.set_Service(
        capabilities_wms.Service(
            Name=_NAME,
            Title=capabilities_wms.Title(_TITLE),
            Abstract=None,
            KeywordList=None,
            OnlineResource=resource_info))

  def _MakeOnlineResourceXml(self, url):
    return capabilities_wms.OnlineResource(
        xlink_type="simple",
        xmlns_xlink="http://www.w3.org/1999/xlink",
        xlink_href=url)

  def GetOnlineResource(self):
    if "proxy-endpoint" in self.parameters:
      return self._MakeOnlineResourceXml(self.parameters["proxy-endpoint"])
    return self._MakeOnlineResourceXml(self.parameters["this-endpoint"])

  def GetExceptionInfo(self):
    exp_info = capabilities_wms.Exception([
        capabilities_wms.Format(_XML_SERVICE_EXCEPTION_TYPE)])

    return exp_info

  def GetMap(self):
    formats = [capabilities_wms.Format(spec.content_type)
               for spec in image_specs.IMAGE_SPECS.values()]

    dcptype_info = self.GetDCPTypeInfo()

    map_info = capabilities_wms.GetMap(formats, [dcptype_info])

    return map_info

  def GetDCPTypeInfo(self):
    resource_info = self.GetOnlineResource()

    http_info = capabilities_wms.HTTP([capabilities_wms.Get(resource_info)])

    dcptype_info = capabilities_wms.DCPType(http_info)

    return dcptype_info

  def GetCapabilities(self):
    dcptype_info = self.GetDCPTypeInfo()

    capabilities = capabilities_wms.GetCapabilities(
        [capabilities_wms.Format(_XML_CONTENT_TYPE)], [dcptype_info])

    return capabilities

  def GetRequestInfo(self):
    capabilities_info = self.GetCapabilities()
    map_info = self.GetMap()

    request_info = capabilities_wms.Request(
        GetCapabilities=capabilities_info,
        GetMap=map_info)
    return request_info

  def SetCapability(self):
    request_info = self.GetRequestInfo()
    exp_info = self.GetExceptionInfo()

    self.capabilities_xml.set_Capability(
        capabilities_wms.Capability(Request=request_info, Exception=exp_info))

    self.SetLayers()

  def SetLayers(self):
    """Set the layers for the capabilities xml."""

    # This outer, inaccessible layer is to give information just once;
    # the sub-layers inherit it.
    outer_layer = capabilities_wms.Layer(
        # If this is a containing layer it should be opaque=0
        opaque=0,
        cascaded=None,
        fixedHeight=None,
        fixedWidth=None,
        # 7.1.4.6.4 - we obviously can subset.
        noSubsets=0,
        queryable=0,
        # This is a containing layer.
        Name=None,
        Title=capabilities_wms.Title(_TITLE))

    server_layers_by_name = self.layer_obj.GetLayers(
        utils.GetValue(self.parameters, "server-url"),
        utils.GetValue(self.parameters, "TargetPath"))

    if not server_layers_by_name:
      # Raise ServiceException here.
      raise ServiceException(None, "Database type is not supported.")

    for layer_name, server_layer in server_layers_by_name.iteritems():
      wms_layer = capabilities_wms.Layer(
          # 7.1.4.6.3 - Even for vector maps we always get data from
          # the server, even if it's just a transparent tile.  By
          # jeffdonner's reading, this means that even the vector
          # layers are 'opaque'.
          opaque=1,
          cascaded=0,
          fixedHeight=None,
          fixedWidth=None,
          # We can subset.
          noSubsets=0,
          queryable=0,
          Name=layer_name,
          Title=capabilities_wms.Title(server_layer.label))

      proj = server_layer.projection
      for epsg_name in proj.EPSG_NAMES:
        wms_layer.add_SRS(capabilities_wms.SRS(epsg_name))

      wms_layer.set_LatLonBoundingBox(
          capabilities_wms.LatLonBoundingBox(
              minx=-proj.MAX_LONGITUDE,
              miny=-proj.MAX_LATITUDE,
              maxx=proj.MAX_LONGITUDE,
              maxy=proj.MAX_LATITUDE))
      map_limits = server_layer.projection.AdvertizedLogOuterBounds()
      wms_layer.set_BoundingBox([
          capabilities_wms.BoundingBox(
              SRS=epsg_name,
              minx=map_limits.x0,
              miny=map_limits.y0,
              maxx=map_limits.x1,
              maxy=map_limits.y1)
          for epsg_name in proj.EPSG_NAMES])

      outer_layer.add_Layer(wms_layer)

    self.capabilities_xml.get_Capability().set_Layer(outer_layer)

  def _Xml(self):
    """Generates the xml response.

    Returns:
       The XML response.
    """
    logger.debug("Begin XML response for GetCapabilities for WMS v1.1.1")

    xml_text = StringIO.StringIO()
    xml_text.write(_XML_HEADER)
    xml_text.write("<!DOCTYPE WMT_MS_Capabilities SYSTEM")
    xml_text.write('\n"http://schemas.opengis.net/wms/'
                   '1.1.1/capabilities_1_1_1.dtd"\n')
    xml_text.write(" [ \n")
    xml_text.write(" <!ELEMENT VendorSpecificCapabilities EMPTY>\n")
    xml_text.write(" ]>\n")

    self.capabilities_xml = capabilities_wms.WMT_MS_Capabilities(
        0, _WMS_VERSION)

    self.SetService()
    self.SetCapability()

    self.capabilities_xml.export(xml_text, 0)

    logger.debug("End XML response for GetCapabilities for WMS v1.1.1")

    return xml_text.getvalue()

  def GenerateOutput(self):
    """Generate response for GetCapabilities request.

    Returns:
      headers: List of headers for the response.
      response: GetCapabilities response.
    """

    logger.debug("Processing GetCapabilities response for WMS v1.1.1")

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
            ("Content-Type", _XML_CONTENT_TYPE)
            ]
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

  def GenerateOutput(self):
    """Executes the operation and returns the image.

    Returns:
        The image composed of tiles.
    """
    logger.debug("Generating GetMapRequest response "
                 "for WMS request for version 1.1.1")

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
                 "for WMS request for version 1.1.1")

    return self.GenerateOutputCommon()


class BadWmsRequest(object):
  """Generator of the appropriate WMS-version error.

  For when we know the WMS version, so that we can and should send a
  ServiceException, yet it's not any particular REQUEST type so we
  can't handle it from within one of those.
  """

  def __init__(self, message):
    self.message = message

  def GenerateOutput(self):
    headers = _HEADERS_EXCEPTION
    response = ServiceException(None, self.message).Xml()

    return headers, response


def main():
  obj = BadWmsRequest("Not Valid Format")
  output = obj.GenerateOutput()

  print output


if __name__ == "__main__":
  main()
