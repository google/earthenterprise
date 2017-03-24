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

"""Contains WmsGetMapRequest.
"""

import StringIO

import geom
import image_specs
import tiles
import utils

OPERATIONS = set(['GetMap', 'GetCapabilities'])

# These are all of the 1.3.0 official codes. 1.1.1's are the same
# except with 2 fewer, as noted below. (Any other errors are fine,
# they just don't get a code.)
_INVALID_FORMAT = 'InvalidFormat'
_INVALID_CRS = 'InvalidCRS'
_LAYER_NOT_DEFINED = 'LayerNotDefined'
_STYLE_NOT_DEFINED = 'StyleNotDefined'
_LAYER_NOT_QUERYABLE = 'LayerNotQueryable'
# not 1.1.1
_INVALID_POINT = 'InvalidPoint'
_CURRENT_UPDATE_SEQUENCE = 'CurrentUpdateSequence'
_INVALID_UPDATE_SEQUENCE = 'InvalidUpdateSequence'
_MISSING_DIMENSION_VALUE = 'MissingDimensionValue'
_INVALID_DIMENSION_VALUE = 'InvalidDimensionValue'
# not 1.1.1
# exported, for wms_130
OPERATION_NOT_SUPPORTED = 'OperationNotSupported'


class WmsGetMapRequest(object):
  """Base class of WMS GetMapRequest objects particular to their version.
  """

  def __init__(self, parameters, required_param_names):
    self.server = parameters['server-obj']
    self.parameters = parameters
    self.required_param_names = required_param_names

  def GenerateOutputCommon(self):
    """Produces the WMS bitmap image. Used by both versions.

    Returns:
        The image composed of tiles.
    """
    utils.logger.debug('wms.GetMapRequest.output')
    image_spec = self.requested_layer_obj.image_spec
    im_user = tiles.ProduceImage(
        image_spec, self.requested_layer_obj.projection,
        # There's a weird border artifact that MapServer shows when we
        # ask for more than 360 laterally, and which natively we show
        # fine. It's probably not us, but there's room for doubt.  If
        # it was our calculations, it's likely the error would be most
        # extreme for a large final image, and few tiles.
        self.user_log_rect,
        self.user_width, self.user_height,
        self.requested_layer_obj.getTileUrlMaker())

    buf = StringIO.StringIO()
    im_user.save(buf, image_spec.pilFormat, **im_user.info)

    utils.logger.debug('content-type:%s' % image_spec.contentType)
    headers = [
        ('Content-Type', image_spec.contentType)
        ]
    return headers, buf.getvalue()

  def _ServiceException(self, code, message):
    # Call down to the derived class for its specific form of
    # ServiceException.
    self._serviceExceptionImpl(code, message)

  def _EnsureRequiredParameters(self, parameters, required_param_names):
    """Mechanically produces a ServiceException if a required parm. is missing.

    Args:
        parameters: the (enhanced) parameters passed in via the url.
        required_param_names: list or set of required parameter names.
    """
    for reqd in required_param_names:
      if utils.GetValue(parameters, reqd) is None:
        error_message = 'Missing required parameter: \'%s\'' % reqd
        utils.logger.debug(error_message)
        self._ServiceException(None, error_message)

  @staticmethod
  def _ExtractLayerName(layers_text):
    return layers_text.split(',')[0]

  def _CheckParameters(self, parameters, required_param_names):
    self._EnsureRequiredParameters(parameters, required_param_names)

    # presence is established - now we're looking in more detail.
    lite_checkers = {
        # version already checked by this point
        # request already checked by this point
        # layers
        'styles': self._CheckStyles,
        'crs': self._CheckCrs,
        'srs': self._CheckSrs,
        'bbox': self._CheckBbox,
        'width': self._CheckWidth,
        'height': self._CheckHeight,
        # format
        }

    for name, checker in lite_checkers.iteritems():
      parameter_value = utils.GetValue(parameters, name)
      checker(parameter_value)

    layers_text = utils.GetValue(parameters, 'layers')
    self._CheckLayers(layers_text)

    # format depends on layer too much to be standalone, ie 'lite'.
    server_layers_by_name = self.parameters['server-obj'].getLayers()
    layer_name = WmsGetMapRequest._ExtractLayerName(layers_text)

    requested_layer_obj = server_layers_by_name[layer_name]
    self._CheckFormat(utils.GetValue(parameters, 'format'), requested_layer_obj)

  def _ProcessCommon(self):
    """Processes the GetMapRequest parameters and prepares the GEE tile URL.

    Returns:
        Nothing, but might raise a ServiceException of the right version.
    """
    utils.logger.debug('wms.GetMapRequest._processCommon')
    self._CheckParameters(self.parameters, self.required_param_names)

    server_layers_by_name = self.parameters['server-obj'].getLayers()

    utils.logger.debug('find the target layer')
    # We only handle a single layer at a time (jeffdonner is not sure
    # what handling multiple ones would mean - compositing them per
    # STYLES perhaps? Sending them in sequence, somehow?)
    # Here we don't balk or warn at a list, we just silently send the
    # first item.
    layer_name = WmsGetMapRequest._ExtractLayerName(
        utils.GetValue(self.parameters, 'layers'))

    # By this point we know it's there.
    self.requested_layer_obj = server_layers_by_name[layer_name]
    utils.logger.debug('target layer: ' + layer_name)

    bbox_text = utils.GetValue(self.parameters, 'bbox')
    coords = bbox_text.split(',')
    self.user_log_rect = geom.Rect(float(coords[0]), float(coords[1]),
                                   float(coords[2]), float(coords[3]))

    # We should by rights check that CRS/SRS matches the layer's, but
    # we don't because (at least in jeffdonner's experience with
    # 'uppermidwest.map') we have to let MapServer feed us ones we
    # don't support in order to work with it. Luckily MapServer
    # produces the right result despite this confusion.

    # TODO: nice-to-have: add format flexibility (raster ->
    # png, vectorrastermap -> jpg) since PIL can transform them. At
    # least, ImageMaps could be png.

    self.user_width = int(utils.GetValue(self.parameters, 'width'))
    self.user_height = int(utils.GetValue(self.parameters, 'height'))
    utils.logger.debug('done')

  ####################################################################
  # The _CheckXXX methods raise a ServiceException if they detect
  # something wrong; otherwise they do nothing, and should have no
  # side effects (except on caching the server's layers).
  ####################################################################

  def _CheckLayers(self, layers_text):
    layer_names = layers_text.split(',')
    if 1 < len(layer_names):
      # We only handle a single layer at a time (jeffdonner is not sure
      # what handling multiple ones would mean - compositing them per
      # STYLES perhaps? Sending them in sequence, somehow?)
      utils.logger.warning(
          'Received request for multiple layers. '
          'We only handle one at a time, the first.')
    # Just check the first, that's all we'll use.
    layer_name = layer_names[0]

    server_layers_by_name = self.parameters['server-obj'].getLayers()

    requested_layer = server_layers_by_name.get(layer_name, None)
    if requested_layer is None:
      utils.logger.error('have no layer: ' + layer_name)
      self._serviceExceptionImpl(_LAYER_NOT_DEFINED,
                                 'No layer matching \'%s\' found' % layer_name)

  def _CheckStyles(self, styles_text):
    pass

  def _CheckCrs(self, crs_text):
    # we don't check, but if we did, we'd only check if it was 1.3.x
    # if self.parameters['version'].startswith('1.3.'):
    pass

  def _CheckSrs(self, srs_text):
    # we don't check, but if we did, we'd only check if it was 1.1.1
    # if self.parameters['version'].startswith('1.1.1'):
    pass

  def _CheckBbox(self, bbox_text):
    coords = bbox_text.split(',')
    if len(coords) != 4:
      self._serviceExceptionImpl(None, 'Expected 4 BBOX coordinates')
    user_log_rect = geom.Rect(float(coords[0]), float(coords[1]),
                              float(coords[2]), float(coords[3]))

    # The requirement on relative sizes of the params in BBOX, even
    # the equality part, is per the spec.
    if user_log_rect.y1 <= user_log_rect.y0:
      self._serviceExceptionImpl(None, 'BBOX.ymax <= BBOX.ymin')
    if user_log_rect.x1 <= user_log_rect.x0:
      self._serviceExceptionImpl(None, 'BBOX.xmax <= BBOX.xmin')

  def _CheckIsPositiveInt(self, maybe_numeric_text, parm_name):
    must_raise = False
    try:
      value = int(maybe_numeric_text)
      # Allowing 0 might be a good test of our math. But not negative.
      must_raise = value < 0
    except ValueError:
      must_raise = True
    if must_raise:
      self._serviceExceptionImpl(None, parm_name + ' must be positive integer')

  def _CheckWidth(self, width_text):
    self._CheckIsPositiveInt(width_text, 'WIDTH')

  def _CheckHeight(self, height_text):
    self._CheckIsPositiveInt(height_text, 'HEIGHT')

  def _CheckFormat(self, format_text, requested_layer_obj):
    if not image_specs.IsKnownFormat(format_text):
      self._serviceExceptionImpl(
          _INVALID_FORMAT,
          'Unsupported format \'%s\'' % format_text)
    if (requested_layer_obj.image_spec.contentType !=
        format_text.replace('%2F', '/')):
      self._serviceExceptionImpl(
          _INVALID_FORMAT,
          'Layer only produces %s' % requested_layer_obj.image_spec.contentType)
