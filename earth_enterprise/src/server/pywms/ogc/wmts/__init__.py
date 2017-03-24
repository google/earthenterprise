import StringIO
import urllib2

import ogc
import utils
import xml.capabilities
import xml.exception

class ExceptionReport(ogc.XMLBase):
  """
  Represents an OGC exception.
  """
  def __init__(self, code = None, message = 'Unspecified Error', locator = None):
    self.code = code
    self.message = message
    self.locator = locator

  def getXML(self):
    """
    Generates and returns the XML response.
    """
    xmlText = StringIO.StringIO()
    xmlText.write(self.getXMLHeader())

    exceptionReport = xml.exception.ExceptionReport('en', '1.0.0')
    exceptionType = xml.exception.ExceptionType(self.locator, self.code)
    exceptionType.add_ExceptionText(self.message)
    exceptionReport.add_Exception(exceptionType)

    exceptionReport.export(xmlText, 0)

    return xmlText.getvalue()

class GetCapabilitiesRequest(ogc.GetCapabilitiesRequest):
  """
  Represents WMTS GetCapabilities request.
  """
  def getXML(self):
    """
    Generates and returns the XML response.
    """
    utils.logger.debug('start big scary xml object building')

    xmlText = StringIO.StringIO()
    xmlText.write(self.getXMLHeader())

    capabilities = xml.capabilities.Capabilities(
        None,
        '1.0.0',
        xml.capabilities.ServiceIdentification(
            [xml.capabilities.LanguageStringType(None, 'OGC:WMTS')],
            [xml.capabilities.LanguageStringType(
                None,
                'Google Earth provided WMTS service.'
            )],
            None,
            xml.capabilities.CodeType(None, 'OGC WMTS'),
            ['1.0.0'],
            None,
            'none'
        ),
        xml.capabilities.ServiceProvider(
            'Google Earth Enterprise',
            xml.capabilities.OnlineResourceType(
                None,
                None,
                None,
                self.parameters['server-obj'].getServerURL()
            ),
            xml.capabilities.ResponsiblePartySubsetType()
        ),
        xml.capabilities.OperationsMetadata([
            xml.capabilities.Operation(
                'GetCapabilities',
                [xml.capabilities.DCP(xml.capabilities.HTTP([
                    xml.capabilities.RequestMethodType(
                        None,
                        None,
                        None,
                        self.parameters['this-endpoint'],
                        None,
                        None,
                        None,
                        [xml.capabilities.DomainType(
                            xml.capabilities.AllowedValues([
                                xml.capabilities.ValueType('KVP')
                            ]),
                            None,
                            None,
                            None,
                            None,
                            None,
                            None,
                            None,
                            None,
                            None,
                            'GetEncoding'
                        )]
                    )
                ]))]
            ),
            xml.capabilities.Operation(
                'GetTile',
                [xml.capabilities.DCP(xml.capabilities.HTTP([
                    xml.capabilities.RequestMethodType(
                        None,
                        None,
                        None,
                        self.parameters['this-endpoint']
                    )
                ]))]
            )
        ]),
        xml.capabilities.ContentsType(
            None,
            None,
            [xml.capabilities.TileMatrixSet(
                None,
                None,
                None,
                xml.capabilities.CodeType(None, 'GE'),
                None,
                'urn:ogc:def:crs:OGC:1.3:CRS84'
            )]
        )
    )
    '''
    if self.serverDefinitions['isAuthenticated'] != None and self.serverDefinitions['isAuthenticated'] == True:
        capabilities.get_ServiceIdentification().add_AccessConstraints(
            'http'
        )
    else:
        capabilities.get_ServiceIdentification().add_AccessConstraints(
            'none'
        )
    '''
    serverLayers = self.parameters['server-obj'].getLayers()
    for i in range(len(serverLayers)):
      layer = serverLayers[i]

      tmp = xml.capabilities.LayerType(
          [xml.capabilities.LanguageStringType(None, layer['label'])],
          None,
          None,
          [xml.capabilities.WGS84BoundingBoxType(
              None,
              None,
              '-180 -90',
              '180 90'
          )],
          xml.capabilities.CodeType(None, str(layer['id'])),
          None,
          None,
          None,
          [xml.capabilities.Style(
              [xml.capabilities.LanguageStringType(
                  None,
                  's-' + str(layer['id'])
              )],
              None,
              None,
              True,
              xml.capabilities.CodeType(None, 's-' + str(layer['id'])),
              [xml.capabilities.LegendURL(
                  None,
                  None,
                  None,
                  self.parameters['server-obj'].getServerURL(True) + 'query?request=Icon&icon_path=' + layer['icon'],
                  None,
                  None,
                  None,
                  None,
                  None,
                  None,
                  None,
                  'image/png'
              )]
          )],
          None,
          None,
          None,
          [xml.capabilities.TileMatrixSetLink('GE')]
      )

      if layer['isPng'] == True:
        tmp.add_Format('image/png')
      else:
        tmp.add_Format('image/jpeg')

      capabilities.get_Contents().add_DatasetDescriptionSummary(tmp)

    for i in range(24):
      capabilities.get_Contents().get_TileMatrixSet()[0].add_TileMatrix(
          xml.capabilities.TileMatrix(
              None,
              None,
              None,
              xml.capabilities.CodeType(None, str(i + 1)),
              2 ** (i + 1),
              '-180 90',
              256,
              256,
              2 ** (i + 1),
              2 ** (i + 1)
          )
      )

    capabilities.export(xmlText, 0, '')

    utils.logger.debug('end big scary xml object building')

    return xmlText.getvalue()


class GetTileRequest(ogc.GetGraphicRequest):
  """
  Represents WMTS GetTile request.
  """
  def process(self):
    """
    Processes the GetTile parameters and prepares the GEE tile URL.
    """
    if utils.getValue(self.parameters, 'layer') == None:
      return ExceptionReport(
          'MissingParameterValue',
          'LAYER is a required parameter.',
          'LAYER'
      )

    utils.logger.debug('find the target layer')
    layer = utils.getValue(self.parameters, 'layer')
    self.initializeLayerInfo(layer)

    if self.layerInfo == None:
      return ExceptionReport(
          'InvalidParameterValue',
          'Layer not found.',
          'LAYER'
      )

    self.z = utils.getValue(self.parameters, 'tilematrix')
    if self.z == None:
      return ExceptionReport(
          'MissingParameterValue',
          'TILEMATRIX is a required parameter.',
          'TILEMATRIX'
      )

    self.z = int(self.z)

    if self.z < 1 or self.z > 24:
      return ExceptionReport(
          'InvalidParameterValue',
          'TILEMATRIX out of range.',
          'TILEMATRIX'
      )

    self.y = utils.getValue(self.parameters, 'tilerow')
    if self.y == None:
      return ExceptionReport(
          'MissingParameterValue',
          'TILEROW is a required parameter.',
          'TILEROW'
      )

    self.y = int(self.y)

    if self.y < 0 or self.y >= (2 ** self.z):
      return ExceptionReport(
          'InvalidParameterValue',
          'TILEROW out of range.',
          'TILEROW'
      )

    self.x = utils.getValue(self.parameters, 'tilecol')
    if self.x == None:
      return ExceptionReport(
          'MissingParameterValue',
          'TILECOL is a required parameter.',
          'TILECOL'
      )

    self.x = int(self.x)

    if self.x < 0 or self.x >= (2 ** int(self.z)):
      return ExceptionReport(
          'InvalidParameterValue',
          'TILECOL out of range.',
          'TILECOL'
      )

    utils.logger.debug('generate google query url')
    self.initializeTileURL(self.x, self.y, self.z)

    return None

  def output(self):
    """
    Fetches and returns the tile.
    """
    if self.layerInfo['isPng']:
      contentType = 'image/png'
    else:
      contentType = 'image/jpeg'

    utils.logger.debug('query google for tile: ' + self.tileURL)
    fp = urllib2.urlopen(self.tileURL)
    return contentType, fp.read(), []

class Request:
  """
  Controller class to handle the workflow of a WMTS request.
  """
  def __init__(self, parameters):
    self.parameters = parameters

    self.version = utils.getValue(self.parameters, 'version')
    if self.version == None:
      self.version = '1.0.0'

    self.request = utils.getValue(self.parameters, 'request')

  def process(self):
    """
    Processes the request; validates high level parameters, determines type
    and returns a response object.
    """
    # try:
    if self.version != '1.0.0':
      return ExceptionReport(
          'VersionNegotiationFailed',
          'WMTS version not supported.'
      )

    utils.logger.debug('request type: ' + self.request)
    if self.request == 'GetCapabilities':
      return GetCapabilitiesRequest(self.parameters)
    elif self.request == 'GetTile':
      getTileRequest = GetTileRequest(self.parameters)

      result = getTileRequest.process()

      if result != None:
        return result
      else:
        return getTileRequest
    else:
      return ExceptionReport(
          'OperationNotSupported',
          'WMTS request type not supported.',
          self.request
      )
    # except:
    #     return ExceptionReport(
    #         'NoApplicableCode',
    #         'Bad Request: Check your parameters and try your request again.'
    #     )
