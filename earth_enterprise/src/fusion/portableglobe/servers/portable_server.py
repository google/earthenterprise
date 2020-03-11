#!/usr/bin/env python2.7
#
# Copyright 2017 Google Inc, 2019 Open GEE Contributors.
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

"""Serves a specified globe to Google Earth EC.

Google Earth EC can be running on the same machine
or can be accessing this server through a proxy servers
sharing the same key.
"""

import tornado.httpserver
import tornado.ioloop
import tornado.web

import local_server
import portable_globe
import portable_server_base
import portable_web_interface


from platform_specific_functions import prepare_for_io_loop

class FlatFileHandler(portable_server_base.BaseHandler):
  """Class for handling flatfile requests."""

  @tornado.web.asynchronous
  def get(self):
    """Handle GET request for packets."""
    argument_str = self.request.uri.split("?")[1]
    arguments = argument_str.split("-")
    if arguments[0] == "lf":
      self.set_header("Content-Type", "image/png")
    else:
      self.set_header("Content-Type", "application/octet-stream")

    if tornado.web.globe_.IsComposite():
      tornado.web.local_server_.LocalFlatFileHandler(
          self, portable_globe.COMPOSITE_BASE_LAYER)
    else:
      tornado.web.local_server_.LocalFlatFileHandler(
          self, portable_globe.NON_COMPOSITE_LAYER)
    self.finish()


class CompositeFlatFileHandler(portable_server_base.BaseHandler):
  """Class for handling flatfile requests to glc layers."""

  @tornado.web.asynchronous
  def get(self, layer_id):
    """Handle GET request for packets."""
    argument_str = self.request.uri.split("?")[1]
    arguments = argument_str.split("-")
    if arguments[0] == "lf":
      self.set_header("Content-Type", "image/png")
    else:
      self.set_header("Content-Type", "application/octet-stream")

    tornado.web.local_server_.LocalFlatFileHandler(self, int(layer_id))
    self.finish()


class DbRootHandler(portable_server_base.BaseHandler):
  """Class for returning the dbRoot."""

  @tornado.web.asynchronous
  def get(self):
    """Handle GET request for the dbroot."""
    self.set_header("Content-Type", "application/octet-stream")

    if not tornado.web.globe_.Is3d():
      print "Bad request: dbRoot from non-3D globe."
    else:
      if tornado.web.globe_.IsComposite():
        tornado.web.local_server_.LocalDbRootHandler(
            self, portable_globe.COMPOSITE_BASE_LAYER)
      else:
        tornado.web.local_server_.LocalDbRootHandler(
            self, portable_globe.NON_COMPOSITE_LAYER)
      self.finish()


class CompositeDbRootHandler(portable_server_base.BaseHandler):
  """Class for returning the meta dbRoot of a glc or dbRoots of its layers."""

  @tornado.web.asynchronous
  def get(self, layer_id):
    """Handle GET request for the dbroot."""
    self.set_header("Content-Type", "application/octet-stream")
    if not tornado.web.globe_.Is3d():
      print "Bad request: dbRoot from non-3D globe."
    elif not tornado.web.globe_.IsComposite():
      print "Bad request: composite request for glb."
    else:
      tornado.web.local_server_.LocalDbRootHandler(self, int(layer_id))
      self.finish()


class CompositeVectorLayerHandler(portable_server_base.BaseHandler):
  """Class for returning vector layer data."""

  @tornado.web.asynchronous
  def get(self, layer_id, path):
    """Handle GET request for vector layer data."""
    path = path.encode("ascii", "ignore")
    self.set_header("Content-Type", "text/html")
    if not tornado.web.globe_.IsComposite():
      print "Bad request: composite request for glb."
    else:
      tornado.web.local_server_.LocalLayerVectorFileHandler(
          self, path, int(layer_id))
      self.finish()


class DocsHandler(portable_server_base.BaseHandler):
  """Class for returning the content of files directly from disk."""

  @tornado.web.asynchronous
  def get(self, path):
    """Handle GET request for some document.

    For example it is used for javascript files for
    the Google Earth web browser plugin.

    Args:
      path: Path to file to be returned.
    """
    path = path.encode("ascii", "ignore")
    if path[-3:].lower() == "gif":
      self.set_header("Content-Type", "image/gif")
    elif path[-3:].lower() == "png":
      self.set_header("Content-Type", "image/png")
    else:
      self.set_header("Content-Type", "text/html")

    tornado.web.local_server_.LocalDocsHandler(self, path)
    self.finish()


class CompositeDocsHandler(portable_server_base.BaseHandler):
  """Class for returning the content of files directly from disk."""

  @tornado.web.asynchronous
  def get(self, layer_id, path):
    """Handle GET request for some document.

    For example it is used for javascript files for
    the Google Earth web browser plugin.

    Args:
      layer_id: Id of layer within the composite.
      path: Path to file to be returned.
    """
    path = path.encode("ascii", "ignore")
    if path[-3:].lower() == "gif":
      self.set_header("Content-Type", "image/gif")
    elif path[-3:].lower() == "png":
      self.set_header("Content-Type", "image/png")
    else:
      self.set_header("Content-Type", "text/html")

    tornado.web.local_server_.LocalLayerDocsHandler(
        self, path, int(layer_id))
    self.finish()


class BalloonHandler(portable_server_base.BaseHandler):
  """Class for returning the content for a balloon."""

  @tornado.web.asynchronous
  def get(self):
    """Handle GET request for FT balloon data."""
    self.set_header("Content-Type", "text/html")
    ftid = self.request.arguments["ftid"][0].replace(":", "-")
    path = "earth/vector_layer/balloon_%s.html" % ftid
    tornado.web.local_server_.LocalDocsHandler(self, path)
    self.finish()


class IconHandler(FlatFileHandler):
  """Class for returning icons."""

  @tornado.web.asynchronous
  def get(self, icon):
    """Handle GET request for icon."""
    icon = icon.encode("ascii", "ignore")
    self.set_header("Content-Type", "image/png")
    if tornado.web.globe_.IsComposite():
      tornado.web.local_server_.LocalIconHandler(
          self, icon, portable_globe.COMPOSITE_BASE_LAYER)
    else:
      tornado.web.local_server_.LocalIconHandler(
          self, icon, portable_globe.NON_COMPOSITE_LAYER)
    self.finish()


class CompositeIconHandler(FlatFileHandler):
  """Class for returning icons."""

  @tornado.web.asynchronous
  def get(self, icon, layer_id):
    """Handle GET request for icon."""
    icon = icon.encode("ascii", "ignore")
    self.set_header("Content-Type", "image/png")
    tornado.web.local_server_.LocalIconHandler(self, icon, int(layer_id))
    self.finish()


class KmlSearchHandler(portable_server_base.BaseHandler):
  """Class for returning search results as kml."""

  @tornado.web.asynchronous
  def get(self):
    """Handle GET request for kml search results."""
    self.set_header("Content-Type", "text/plain")
    tornado.web.local_server_.LocalKmlSearchHandler(self)
    self.finish()


class JsonSearchHandler(portable_server_base.BaseHandler):
  """Class for returning search results as json."""

  @tornado.web.asynchronous
  def get(self):
    """Handle GET request for json search results."""
    self.set_header("Content-Type", "text/plain")
    tornado.web.local_server_.LocalJsonSearchHandler(self)
    self.finish()


class CompositeQueryHandler(portable_server_base.BaseHandler):
  """Class for handling "query" requests."""

  @tornado.web.asynchronous
  def get(self, layer_id):
    """Handle GET request for JSON file for plugin."""
    if self.request.arguments["request"][0] == "Json":
      self.set_header("Content-Type", "text/plain; charset=utf-8")
      if ("is2d" in self.request.arguments.keys() and
          self.request.arguments["is2d"][0] == "t"):
        tornado.web.local_server_.LocalJsonHandler(self, True)
      else:
        tornado.web.local_server_.LocalJsonHandler(self, False)

    elif self.request.arguments["request"][0] == "ImageryMaps":
      if tornado.web.globe_.IsMbtiles():
        self.set_header("Content-Type", "image/png")
        tornado.web.local_server_.LocalMapTileHandler(
            self, True, portable_globe.COMPOSITE_BASE_LAYER)
      else:
        self.set_header("Content-Type", "image/jpeg")
        tornado.web.local_server_.LocalMapTileHandler(
            self, True, int(layer_id))

    elif self.request.arguments["request"][0] == "VectorMapsRaster":
      self.set_header("Content-Type", "image/png")
      tornado.web.local_server_.LocalMapTileHandler(
          self, False, int(layer_id))

    elif self.request.arguments["request"][0] == "Icon":
      self.set_header("Content-Type", "image/png")
      (icon_path, use_layer, use_local) = (
          tornado.web.local_server_.ConvertIconPath(
              self.request.arguments["icon_path"][0]))
      layer_id = int(layer_id)
      if not use_layer:
        layer_id = portable_globe.NON_COMPOSITE_LAYER
      tornado.web.local_server_.LocalIconHandler(
          self, icon_path, layer_id, use_local)

    else:
      self.set_header("Content-Type", "text/plain")
      print "Unknown query request: ", self.request.uri

    self.finish()


class QueryHandler(portable_server_base.BaseHandler):
  """Class for handling "query" requests."""

  @tornado.web.asynchronous
  def get(self):
    """Handle GET request for JSON file for plugin."""
    if self.request.arguments["request"][0] == "Json":
      if "v" in self.request.arguments:
        json_version = int(self.request.arguments["v"][0])
      else:
        json_version = 1
      self.set_header("Content-Type", "text/plain; charset=utf-8")
      # TODO: Need way to distinguish 2d/3d for
      # TODO: composite with both.
      if ("is2d" in self.request.arguments.keys() and
          self.request.arguments["is2d"][0] == "t"):
        tornado.web.local_server_.LocalJsonHandler(self, True, json_version)
      else:
        tornado.web.local_server_.LocalJsonHandler(self, False, json_version)

    elif self.request.arguments["request"][0] == "ImageryMaps":
      self.set_header("Content-Type", "image/jpeg")
      if tornado.web.globe_.IsComposite():
        tornado.web.local_server_.LocalMapTileHandler(
            self, True, portable_globe.COMPOSITE_BASE_LAYER)
      else:
        tornado.web.local_server_.LocalMapTileHandler(
            self, True, portable_globe.NON_COMPOSITE_LAYER)

    elif self.request.arguments["request"][0] == "VectorMapsRaster":
      self.set_header("Content-Type", "image/png")
      if tornado.web.globe_.IsComposite():
        tornado.web.local_server_.LocalMapTileHandler(
            self, False, portable_globe.COMPOSITE_BASE_LAYER)
      else:
        tornado.web.local_server_.LocalMapTileHandler(
            self, False, portable_globe.NON_COMPOSITE_LAYER)

    elif self.request.arguments["request"][0] == "Icon":
      self.set_header("Content-Type", "image/png")
      if tornado.web.globe_.IsComposite():
        (icon_path, use_layer, use_local) = (
            tornado.web.local_server_.ConvertIconPath(
                self.request.arguments["icon_path"][0]))
        if use_layer:
          layer_id = portable_globe.COMPOSITE_BASE_LAYER
        else:
          layer_id = portable_globe.NON_COMPOSITE_LAYER
        tornado.web.local_server_.LocalIconHandler(
            self, icon_path, layer_id, use_local)
      else:
        tornado.web.local_server_.LocalIconHandler(
            # Strips off "icons/" prefix from the path
            self, self.request.arguments["icon_path"][0][6:],
            portable_globe.NON_COMPOSITE_LAYER)

    else:
      self.set_header("Content-Type", "text/plain")
      print "Unknown query request: ", self.request.uri

    self.finish()


class MapsGen204Handler(portable_server_base.BaseHandler):
  """Class for handling /maps/gen_204 request."""

  def get(self):
    """Handle GET request for gen_204 request."""
    self.set_header("Content-Type", "text/plain")
    # TODO: Consider parsing and storing Maps API usage.
    self.finish()


class PingHandler(portable_server_base.BaseHandler):
  """Class for handling ping request to check if server is up."""

  def get(self):
    """Handle GET request for ping."""
    self.set_header("Content-Type", "text/plain")
    tornado.web.local_server_.LocalPingHandler(self)
    self.finish()


class InfoHandler(portable_server_base.BaseHandler):
  """Class for getting information about current globe."""

  def get(self):
    """Handle GET request for unknown path."""
    self.set_header("Content-Type", "text/plain")
    tornado.web.local_server_.LocalInfoHandler(self)
    self.finish()


def main():
  """Main for portable server."""
  application = tornado.web.Application([
      # Important to look for local requests first.
      (r"/local/(.*)", portable_server_base.LocalDocsHandler),
      (r"/ext/(.*)", portable_server_base.ExtHandler),
      (r".*/(\d+)/kh/flatfile/lf-(.*)", CompositeIconHandler),
      (r".*/(\d+)/kh/flatfile", CompositeFlatFileHandler),
      (r".*/(\d+)/kh/dbRoot.*", CompositeDbRootHandler),
      (r".*/(\d+)/kmllayer/(.*)", CompositeVectorLayerHandler),
      (r".*/flatfile/lf-(.*)", IconHandler),
      (r".*/flatfile", FlatFileHandler),
      (r".*/dbRoot.*", DbRootHandler),
      (r".*/MapsAdapter", JsonSearchHandler),
      (r".*/ECV4Adapter", KmlSearchHandler),
      (r".*/Portable2dPoiSearch", JsonSearchHandler),
      (r".*/Portable3dPoiSearch", KmlSearchHandler),
      (r".*/icons/(.*)", IconHandler),
      (r"/ping", PingHandler),
      (r"/info", InfoHandler),
      (r".*/(\d+)/query", CompositeQueryHandler),
      (r".*/query", QueryHandler),
      (r".*/(\d+)/(js/.*)", CompositeDocsHandler),
      (r".*/(\d+)/(kml/.*)", CompositeDocsHandler),
      (r".*/(\d+)/(license/.*)", CompositeDocsHandler),
      (r".*/(\d+)/(earth/.*)", CompositeDocsHandler),
      (r".*/(\d+)/(maps/.*)", CompositeDocsHandler),
      (r".*/(js/.*)", DocsHandler),
      (r".*/(kml/.*)", DocsHandler),
      (r".*/(license/.*)", DocsHandler),
      (r".*/(earth/.*)", DocsHandler),
      (r"/maps/gen_204", MapsGen204Handler),
      (r".*/(maps/.*)", DocsHandler),
      (r"/eb_balloon", BalloonHandler),
      (r"/(.*)", portable_web_interface.SetUpHandler),
      ])

  prepare_for_io_loop()

  tornado.web.globe_ = portable_globe.Globe()
  tornado.web.local_server_ = local_server.LocalServer()
  http_server = tornado.httpserver.HTTPServer(application)
  if tornado.web.globe_.config_.DisableBroadcasting():
    http_server.listen(tornado.web.globe_.Port(), address="127.0.0.1")
  else:
    http_server.listen(tornado.web.globe_.Port())
  tornado.ioloop.IOLoop.instance().start()


if __name__ == "__main__":
  main()
