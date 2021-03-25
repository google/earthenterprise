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

"""Contains WmsGetMapRequest."""

import logging
import StringIO

import wms.ogc.common.geom as geom
import wms.ogc.common.image_specs as image_specs
import wms.ogc.common.tiles as tiles
import wms.ogc.common.utils as utils
from pprint import pformat

OPERATIONS = set(["GetMap", "GetCapabilities"])

# These are all of the 1.3.0 official codes. 1.1.1's are the same
# except with 2 fewer, as noted below. (Any other errors are fine,
# they just don't get a code.)
_INVALID_FORMAT = "InvalidFormat"
_INVALID_CRS = "InvalidCRS"
_LAYER_NOT_DEFINED = "LayerNotDefined"
_STYLE_NOT_DEFINED = "StyleNotDefined"
_LAYER_NOT_QUERYABLE = "LayerNotQueryable"
# not 1.1.1
_INVALID_POINT = "InvalidPoint"
_CURRENT_UPDATE_SEQUENCE = "CurrentUpdateSequence"
_INVALID_UPDATE_SEQUENCE = "InvalidUpdateSequence"
_MISSING_DIMENSION_VALUE = "MissingDimensionValue"
_INVALID_DIMENSION_VALUE = "InvalidDimensionValue"
# not 1.1.1
# exported, for wms_130
OPERATION_NOT_SUPPORTED = "OperationNotSupported"

# Get logger
logger = logging.getLogger("wms_maps")


class WmsGetMapRequest(object):
  """Base class of WMS GetMapRequest objects particular to their version."""

  def __init__(self, layer_obj, parameters, required_param_names):
    self.layer_obj = layer_obj
    self.parameters = parameters
    self.required_param_names = required_param_names

  def GenerateOutputCommon(self):
    """Produces the WMS bitmap image. Used by both versions.

    Returns:
        The image composed of tiles.
    """
    logger.debug("Generating the bitmap image")

    image_format = utils.GetValue(self.parameters, "format")
    image_spec = image_specs.GetImageSpec(image_format)
    logger.info("WMS Request image_format: %s image_spec: %s will be applied to all layers.", image_format, image_spec)
    # TRANSPARENT parameter from GIS client's.
    # It can take "TRUE"/"FALSE" values.
    # Default value is "FALSE" as per spec.

    # GIS clients either send TRANSPARENT(=TRUE) attribute or don't send it
    # at all, if it's not required.
    # If this parameter is absent, GetValue() method returns None.
    # We default it to "FALSE" if this parameter is not available in the
    # GIS client requests.
    is_transparent = utils.GetValue(self.parameters, "transparent")
    if not is_transparent:
      is_transparent = "FALSE"

    # BGCOLOR parameter is a string that specifies the colour to be
    # used as the background (non-data) pixels of the map.
    # The format is 0xRRGGBB.The default value is 0xFFFFFF
    bgcolor = utils.GetValue(self.parameters, "bgcolor")
    if bgcolor:
      # Convert HEX string to python tuple.
      # Ignore the 0x at the beginning of the hex string.
      # Otherwise str.decode("hex") will throw
      # "TypeError: Non-hexadecimal digit found" error.
      # Also catching ValueError which will be more correct
      # after we have switched to Python3.
      try:
        if bgcolor[:2] == "0x":
          bgcolor = bgcolor[2:]
        bgcolor = tuple(ord(c) for c in bgcolor.decode("hex"))
      except (TypeError, ValueError):
        bgcolor = tiles.ALL_WHITE_PIXELS
    else:
      bgcolor = tiles.ALL_WHITE_PIXELS
    layer_names = self._ExtractLayerNames(
        utils.GetValue(self.parameters, 'layers'))
    logger.info("Loop through layers: %s ", (str(layer_names)))
    composite_image = None
    for layer_name in layer_names:
        # Add the user requested image format, transparency
        # and bgcolor to the layer python object.
        layer_obj = self._ExtractLayerObj(layer_name)
        logger.debug("Processing a layer:  %s   \n%s ", layer_name, pformat( layer_obj ))
        layer_obj.image_format = image_format 
        layer_obj.is_transparent = is_transparent
        layer_obj.bgcolor = bgcolor
        # We'll assume that all WMS layers will have the same image_spec and projection 
        # for
        im_user = tiles.ProduceImage(
            layer_obj,
            # There's a weird border artifact that MapServer shows when we
            # ask for more than 360 laterally, and which natively we show
            # fine. It's probably not us, but there's room for doubt.  If
            # it was our calculations, it's likely the error would be most
            # extreme for a large final image, and few tiles.
            self.user_log_rect,
            self.user_width, self.user_height)            
        im_user = im_user.convert("RGBA")
        if composite_image is None:
          composite_image = im_user
        else:
          logger.debug( "Adding layer %s to composite image...", layer_name)
          composite_image.paste(im_user, (0, 0), im_user)
        buf = StringIO.StringIO()
        output_format = image_spec.pil_format
        composite_image.save(buf, image_spec.pil_format, **im_user.info)
    headers = [("Content-Type", image_spec.content_type)]
    return headers, buf.getvalue()

  def _ServiceException(self, code, message):
    # Call down to the derived class for its specific form of
    # ServiceException.
    self._ServiceExceptionImpl(code, message)

  def _EnsureRequiredParameters(self):
    """Mechanically produces a ServiceException if a required parm. is missing.

      Checks if the required parameter is available and is non-empty.
    """
    for reqd in self.required_param_names:
      # GIS clients send an empty value for "styles" parameter by default.
      # Empty values for styles parameter should be accepted.
      if reqd == "styles":
        continue
      if utils.GetValue(self.parameters, reqd) is None:
        error_message = "Missing required parameter: \'%s\'" % reqd
        logger.debug(error_message)
        self._ServiceException(None, error_message)

  def _CheckParameters(self):
    """Checks if required parameters are available in the request."""
    self._EnsureRequiredParameters()

    # presence is established - now we're looking in more detail.
    lite_checkers = {
        # version already checked by this point
        # request already checked by this point
        "layers": self._CheckLayers,
        "styles": self._CheckStyles,
        "crs": self._CheckCrs,
        "srs": self._CheckSrs,
        "bbox": self._CheckBbox,
        "width": self._CheckWidth,
        "height": self._CheckHeight,
        "format": self._CheckFormat
        }

    for name, checker in lite_checkers.iteritems():
      parameter_value = utils.GetValue(self.parameters, name)
      checker(parameter_value)

  def _ProcessCommon(self):
    """Processes the GetMapRequest parameters and prepares the GEE tile URL.

    Returns:
        Nothing, but might raise a ServiceException of the right version.
    """
    logger.debug("Processing common information for the GetMap request")

    self._CheckParameters()

    # We should by rights check that CRS/SRS matches the layer's, but
    # we don't because (at least in jeffdonner's experience with
    # 'uppermidwest.map') we have to let MapServer feed us ones we
    # don't support in order to work with it. Luckily MapServer
    # produces the right result despite this confusion.

    # TODO: nice-to-have: add format flexibility (raster ->
    # png, vectorrastermap -> jpg) since PIL can transform them. At
    # least, ImageMaps could be png.

    logger.debug("Done processing common information for the GetMap request")

  ####################################################################
  # The _CheckXXX methods raise a ServiceException if they detect
  # something wrong; otherwise they do nothing, and should have no
  # side effects (except on caching the server's layers).
  ####################################################################

  def _CheckLayers(self, layers_text):
    """Checks if the requested layer is available."""

    logger.debug("Processing the target layer")
    layer_names = layers_text.split(",")

    if 1 < len(layer_names):
      # We only handle a single layer at a time (jeffdonner is not sure
      # what handling multiple ones would mean - compositing them per
      # STYLES perhaps? Sending them in sequence, somehow?)
      logger.warning(
          "Received request for multiple layers. "
          "We only handle one at a time, the first.")

    # Just check the first, that's all we'll use.
    layer_name = layer_names[0]

    server_layers_by_name = self.layer_obj.GetLayers(
        utils.GetValue(self.parameters, "server-url"),
        utils.GetValue(self.parameters, "TargetPath"))

    self.requested_layer_obj = server_layers_by_name.get(layer_name, None)

    if not self.requested_layer_obj:
      logger.error("Layer %s doesn't exist", layer_name)
      self._ServiceExceptionImpl(_LAYER_NOT_DEFINED,
                                 "No layer matching \'%s\' found" % layer_name)

    # By this point we know it's there.
    logger.debug("Target layer: " + layer_name)

  def _CheckStyles(self, styles_text):
    pass

  def _CheckCrs(self, crs_text):
    if self.parameters["version"].startswith("1.3."):
      self.projection_type = crs_text

  def _CheckSrs(self, srs_text):
    if self.parameters["version"].startswith("1.1.1"):
      self.projection_type = srs_text

  def _CheckBbox(self, bbox_text):
    """Checks if the bounding box information is proper."""
    coords = bbox_text.split(",")

    if len(coords) != 4:
      self._ServiceExceptionImpl(None, "Expected 4 BBOX coordinates")

    # BBOX inputs are provided in (xy0.y, xy0.x, xy1.y, xy1.x) format
    # for a flat projection when the WMS version is 1.3.0, which
    # otherwise is in (xy0.x, xy0.y, xy1.x, xy1.y) format.
    if (self.requested_layer_obj.projection.name == "flat" and
        self.parameters["version"].startswith("1.3.")):
      self.user_log_rect = geom.Rect(float(coords[1]), float(coords[0]),
                                     float(coords[3]), float(coords[2]))
    else:
      self.user_log_rect = geom.Rect(float(coords[0]), float(coords[1]),
                                     float(coords[2]), float(coords[3]))

    # The requirement on relative sizes of the params in BBOX, even
    # the equality part, is per the spec.
    if self.user_log_rect.y1 <= self.user_log_rect.y0:
      self._ServiceExceptionImpl(None, "BBOX.ymax <= BBOX.ymin")
    if self.user_log_rect.x1 <= self.user_log_rect.x0:
      self._ServiceExceptionImpl(None, "BBOX.xmax <= BBOX.xmin")

  def _CheckIsPositiveInt(self, maybe_numeric_text, param_name):
    """Checks if the parameter is positive integer."""
    must_raise = False

    try:
      value = int(maybe_numeric_text)
      # Allowing 0 might be a good test of our math. But not negative.
      must_raise = value < 0
    except ValueError:
      must_raise = True

    if must_raise:
      self._ServiceExceptionImpl(None, param_name + " must be positive integer")

  def _CheckWidth(self, width_text):
    self._CheckIsPositiveInt(width_text, "WIDTH")
    self.user_width = int(width_text)

  def _CheckHeight(self, height_text):
    self._CheckIsPositiveInt(height_text, "HEIGHT")
    self.user_height = int(height_text)

  def _CheckFormat(self, image_format):
    if not image_specs.IsKnownFormat(image_format):
      self._ServiceExceptionImpl(
          _INVALID_FORMAT,
          "Unsupported format \'%s\'" % image_format)

  def _ExtractLayerNames(self, layers_text):
    return layers_text.split(',')
  
  def _ExtractLayerObj( self, layer_name): 
    
    server_layers_by_name = self.layer_obj.GetLayers(
        utils.GetValue(self.parameters, "server-url"),
        utils.GetValue(self.parameters, "TargetPath"))
    logger.debug( "Server layers: %s ", pformat( server_layers_by_name ) )

    layer_obj = server_layers_by_name.get(layer_name, None)
    return layer_obj

def main():
  map_req_obj = WmsGetMapRequest({"format": "image/jpeg"}, ["format"], [])
  is_valid_format = map_req_obj.GenerateOutputCommon()
  print is_valid_format

if __name__ == "__main__":
  main()
