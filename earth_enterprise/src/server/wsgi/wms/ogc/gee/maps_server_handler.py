#!/usr/bin/env python2.7
#
# Copyright 2017 Google Inc.
# Copyright 2019 Open GEE Contributors
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

"""Code and objects that know about the servers."""

import json
import logging
import re
import ssl
from socket import gethostname
import urlparse
from pprint import pformat

import wms.ogc.common.wms_connection as wms_connection
import wms.ogc.common.projections as projections

# Example 'serverDefs' from Maps/Portable. The parts we use are the same
# for GEMap server.

# var geeServerDefs = {
# isAuthenticated : false,
# layers :
# [
# {
# icon : "icons/773_l.png",
# id : 1004,
# initialState : true,
# isPng : false,
# label : "Imagery",
# lookAt : "none",
# opacity : 1,
# requestType : "ImageryMaps",
# version : 8
# }
# ,
# {
# icon : "icons/773_l.png",
# id : 1002,
# initialState : true,
# isPng : true,
# label : "SF info",
# lookAt : "none",
# opacity : 1,
# requestType : "VectorMapsRaster",
# version : 4
# }
# ]
# ,
# projection : "mercator",
# ...
# url : "http://localhost:9335/SearchServlet/MapsAdapter?\
#        service=GEPlacesPlugin&DbId=42"
# ...


_LAYER_ARG_NAMES = {
    "ImageryMaps": {
        "x": "x",
        "y": "y",
        "z": "z"},
    "VectorMapsRaster": {
        "x": "col",
        "y": "row",
        "z": "level"},
    "ImageryMapsMercator": {
        "x": "x",
        "y": "y",
        "z": "z"}
    }

_SERVER_DEF_URL = "query?request=Json&var=geeServerDefs&is2d=t"
_TILE_BASE_URL = "%s/query?request=%s"
_IMAGE_FMT = "&format=%s"
_CHANNEL_VERSION = "&channel=%s&version=%s"
_TILE_ARGS = "&%s=%d&%s=%d&%s=%d"

# TODO: Support for "glc" type database should be added.
_SUPPORTED_DB_TYPES = ("gemap", "gedb", "glm", "glb")

# Get logger
logger = logging.getLogger("wms_maps")


class WmsLayer(object):
  """Represents everything a client needs to deal with a server layer."""

  def __init__(
      self,
      target_url,
      name,
      layer_id,
      label,
      projection,
      request_type,
      db_type,
      version,
      tile_arg_names):
    self.target_url = target_url
    self.name = name
    self.layer_id = layer_id
    self.label = label
    self.projection = projection
    self.request_type = request_type
    self.db_type = db_type
    self.version = str(version)
    self.tile_arg_names = tile_arg_names

    # Add a variable to store the requested
    # image format for an image.
    # This would be of type "jpeg" or "png".
    # Make "jpeg" as default value.
    # This variable will be updated with the
    # user request format at a later stage.
    self.image_format = "image/jpeg"

  @staticmethod
  def Make(target_url, server_layer_def):
    """Make a WmsLayer object from a serverDefs layer.

    Args:
      target_url: The server's target url, after which we'll append '?query...'
      server_layer_def: JSON layer spec.
    Returns:
        A WmsLayer object.
    """

    # 'layer_ns' is namespace to distinguish layers of the same target path.
    target_path = urlparse.urlsplit(target_url).path[1:]
    layer_ns = "[%s]:%s" %(target_path, str(server_layer_def["id"]))

    if server_layer_def["projection"] == "mercator":
      projection = projections.Mercator()
    else:
      projection = projections.Flat()

    # Note: By default, the requestType is set to 'ImageryMaps' since the
    # requestType-property doesn't present in 3D Fusion Database.
    request_type = server_layer_def.get("requestType", "ImageryMaps")

    tile_arg_names = _LAYER_ARG_NAMES[request_type]

    # TODO: support multi-layered maps (contain glm_id in json).
    version_info = server_layer_def.get("version", None)

    layer = WmsLayer(
        target_url=target_url,
        name=layer_ns,
        layer_id=str(server_layer_def["id"]),
        label=server_layer_def["label"],
        projection=projection,
        request_type=request_type,
        db_type=server_layer_def["db_type"],
        version=version_info,
        tile_arg_names=tile_arg_names
        )

    return layer

  def GetMapBaseUrl(self):
    """Prepares the base URL for fetching tiles."""

    # Add 'requestType' parameter to the URL.
    base_url = _TILE_BASE_URL % (self.target_url, self.request_type)

    # Add 'format' parameter to the URL.
    base_url += _IMAGE_FMT % self.image_format

    if self.db_type == "gemap":
      # 'channel' and 'version' information is
      # required only for 'gemap' database types.
      base_url += _CHANNEL_VERSION % (self.layer_id, self.version)

    return base_url

  def GetTileArgs(self, x, y, z):
    """Prepares the URL for fetching tiles."""
    tile_args = _TILE_ARGS % (
        self.tile_arg_names["x"], x,
        self.tile_arg_names["y"], y,
        self.tile_arg_names["z"], z
        )

    return tile_args


def _GetServerVars(target_url):
  """Fetches the server definitions from the Maps server.

  Args:
      target_url: The url of the server with target path.
  Returns:
      The server definitions for specified target.
  """

  if target_url[-1] != "/":
    target_url += "/"

  logger.debug("Fetching server definitions over http")

  target_url = urlparse.urljoin(target_url, _SERVER_DEF_URL)

  result = wms_connection.HandleConnection(target_url)

  logger.debug("Server definitions data read, start regex")
  logger.debug("JSON vars: %s", result)

  # Clean up the JS -> JSON
  # leading line
  p = re.compile(r"var geeServerDefs =")
  result = p.sub("", result)

  # final line
  p = re.compile(r";\s*$")
  result = p.sub("", result)

  # Adds quotes to bare keywords, changing from JavaScript to
  # actual JSON. (\s* absorbs newlines, & all keys are on their own lines.)
  p = re.compile(r"([\[\{,])\s*(\w+)\s*:")
  result = p.sub(r'\g<1>"\g<2>":', result)

  return json.loads(result)


def _LayersFromServerVars(target_url):
  """Fetch layer information from ServerVars.

  Args:
    target_url: Server URL hosting the target database.
  Returns:
    layers_by_name: A dict of all the layers in a database.
  """
  layers_by_name = {}

  server_vars = _GetServerVars(target_url)

  # "server_vars" would be empty({}) if the target doesn't exist.
  # This kind of error would be handled even before reaching this point.
  # A check for server_vars being empty ({}) is not required here
  # for the same reason.

  if not server_vars.has_key("dbType"):
    # Old databases do not have the "dbType" parameter in geeServerDefs.
    # Detect type of db based on the projection field for old databases.
    # gedb has no projection specified,
    if server_vars.has_key("projection"):
      server_vars["dbType"] = "gemap"
    else:
      server_vars["dbType"] = "gedb"

  # No explicit projection, then it is gedb, default to 'flat'.
  if not server_vars.has_key("projection"):
    server_vars["projection"] = "flat"

  logger.debug("Servers projection: %s", server_vars["projection"])
  logger.debug("Server database type: %s", server_vars["dbType"])

  # Error out when db_type is not supported.
  if server_vars["dbType"] not in _SUPPORTED_DB_TYPES:
    logger.error("GEE WMS implementation doesn't support database"
                 "type '%s'", server_vars["dbType"])
    # In this scenario, return an empty({}) layers_by_name.
    return layers_by_name

  for server_layer_def in server_vars["layers"]:

    # TODO: Use layer projection for GetCapabilities.
    # This will allow the service to serve flat imagery as both 'flat'
    # and 'mercator' based projections.

    # propagate server's 'global' projection name to each layer.
    server_layer_def["projection"] = server_vars["projection"]

    # propagate server's 'global' database type to each layer.
    server_layer_def["db_type"] = server_vars["dbType"]

    layer = WmsLayer.Make(target_url, server_layer_def)
    layers_by_name[layer.name] = layer

    logger.debug("Found server layer: %s", layer.name)
  logger.debug("Layers processing done.  Layer information: %s ", pformat( layers_by_name))

  return layers_by_name


class GEELayer(object):
  """Represents a Google Earth Enterprise Layer server."""

  def __init__(self):
    logger.debug("Initializing GEELayer")

  def GetLayers(self, server_url, target_path):
    """Returns WmsLayer representations of the server's layers.

    These contain metainfo, and the ability to fetch tiles -
    everything a code client needs to know about and use, to get
    tiles from a server.


    Args:
     server_url: URL of the server on which command to be executed.
     target_path: Target published point.
    Returns:
        The layers from the server definitions.
    """
    target_url = urlparse.urljoin(server_url, target_path)
    logger.debug("Fetching layer information for target url '%s'", target_url)

    layers_by_name = _LayersFromServerVars(target_url)

    for layer_name in layers_by_name.keys():
      # Ignore Vector layers  from 3d types.
      if layers_by_name[layer_name].db_type in ("gedb", "glb"):
        if layers_by_name[layer_name].label != "Imagery":
          # Delete the non-Imagery layer.
          layers_by_name.pop(layer_name)

    return layers_by_name


def main():
  obj = GEELayer()

  hostname = gethostname()
  target_path = "merc"

  server_url = "http://%s.xxx.xxx.com" % hostname
  results = obj.GetLayers(server_url, target_path)

  print results

if __name__ == "__main__":
  main()
