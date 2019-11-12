#!/usr/bin/env python2.7
#
# Copyright 2017 Google Inc, 2019 Open GEE Contributors
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

import os
import re
import json
import StringIO
import sys

try:
  pil_enabled = True
  from PIL import Image
  from PIL import ImageFile
except ImportError:
  pil_enabled = False
  print "PIL is not available."
import tornado.httpserver
import tornado.ioloop
import tornado.web

import portable_globe
import portable_web_interface

# Python file that defines object that serves requests to /ext/...
EXT_SERVICE_FILE = "ext_service.py"
# Alternate search key for Google Earth 6.2.2+.
ALT_SEARCH_SERVICE_KEY = "q"
MBTILES_JSON = """
var geeServerDefs = {
isAuthenticated : false,
layers :
[
{
icon : "icons/773_l.png",
id : 1000,
initialState : true,
isPng : false,
label : "Imagery",
lookAt : "none",
opacity : 1,
requestType : "ImageryMaps",
version : 3
}
]
,
serverUrl : "http://localhost:9335",
useGoogleLayers : false
}
"""


class LocalServer(object):
  """Class for handling requests to the local server."""

  def __init__(self):
    # Load the "empty tile" jpg for Maps.
    fp = open("./local/maps/mapfiles/out_of_cut.jpg", "rb")
    self.empty_tile_ = fp.read()
    fp.close()

    # Load any additional search services that may be available.
    self.search_services_ = {}
    self.ext_service_ = None
    self.LoadSearchServices()
    self.LoadExtService()

  def LoadSearchServices(self):
    """Load user defined search services."""
    search_services_directory = (
        tornado.web.globe_.config_.SearchServicesDirectory())
    search_service_regex = re.compile("^(search_service.*)\.py$")
    if search_services_directory:
      sys.path.append(search_services_directory)
      print "Looking for search services in %s ..." % search_services_directory
      for file_name in os.listdir(search_services_directory):
        match = search_service_regex.match(file_name)
        if match:
          print "Found search service:", match.group(1)
          module = __import__(match.group(1))
          try:
            service = module.RegisterSearchService(self.search_services_)
          except Exception, e:
            print e.message
            print "Unable to  register service."
            print "Make sure RegisterSearchService is implemented in your"
            print "search service module."
            return

          try:
            service.SetWorkingDirectory(search_services_directory)
          except:
            print "Unable to set working directory."

    else:
      print "Search services directory is not defined."

  def LoadExtService(self):
    """Load externally defined service for extending server functionality."""
    ext_service_directory = tornado.web.globe_.config_.ExtServiceDirectory()
    if ext_service_directory:
      sys.path.append(ext_service_directory)
      print "Looking for ext service in %s ..." % ext_service_directory
      path = "%s%s%s" % (ext_service_directory, os.sep, EXT_SERVICE_FILE)
      if os.path.isfile(path):
        print "Found ext service."
        module = __import__(EXT_SERVICE_FILE[:-3])
        try:
          self.ext_service_ = module.ExtService()
        except:
          print "Unable to load ext service object."
          return
        try:
          self.ext_service_.SetWorkingDirectory(ext_service_directory)
        except:
          print "Unable to set ext working directory."

    else:
      print "Ext service directory is not defined."

  def GetIcon(self, handler, path, use_local=False):
    """Return icon from a file."""
    if use_local:
      handler.WriteLocalFile(path)
    else:
      handler.WriteFile(path)

  def LocalFlatFileHandler(self, handler, layer_id):
    """Handle GET request for a flatfile request."""
    if not handler.IsValidRequest():
      raise tornado.web.HTTPError(404)

    argument_str = handler.request.uri.split("?")[1]
    arguments = argument_str.split("-")
    tag = arguments[0]
    qtnode = arguments[1]
    request_type = arguments[2][0]
    version = arguments[2][2:]

    try:
      if tag == "q2" and request_type == "q":
        # Read quadtree packet from globe package.
        tornado.web.globe_.ReadQtPacket(qtnode, layer_id)

      elif (tag == "f1c" or tag == "f1") and request_type == "i":
        # Debugging tool for checking an imagery packet.
        if tornado.web.globe_.config_.single_imagery_tile_:
          handler.WriteFile("imagery_tile")
          return

        # Read imagery packet from globe package.
        tornado.web.globe_.ReadImageryPacket(qtnode, layer_id)

      elif (tag == "f1c" or tag == "f1") and request_type == "t":
        # Read terrain packet from globe package.
        tornado.web.globe_.ReadTerrainPacket(qtnode, layer_id)

      elif (tag == "f1c" or tag == "f1") and request_type == "d":
        # Read vector packet.
        channel = int(version.split(".")[0])
        # Read terrain packet from globe package.
        tornado.web.globe_.ReadVectorPacket(qtnode, channel, layer_id)

      elif tag == "lf":
        # Read icon packet.
        icon_args = "-".join(arguments[2:]).split("&")
        self.GetIcon(handler, icon_args[0])
        return

      else:
        print "unknown packet request: %s" % handler.request.uri
        return

      # Get the data and send it.
      handler.write(tornado.web.globe_.GetData())

    except portable_globe.UnableToFindException, e:
      print e.message

  def LocalDbRootHandler(self, handler, layer_id):
    """Handle GET request for the dbroot."""
    if not handler.IsValidRequest():
      raise tornado.web.HTTPError(404)

    # First try to read a dbroot built for the current port.
    if layer_id == portable_globe.COMPOSITE_BASE_LAYER:
      handler.write(tornado.web.globe_.ReadMetaDbRoot())
      return

    else:
      path = "dbroot/dbroot_localhost_%s" % tornado.web.globe_.Port()

    if (layer_id == portable_globe.NON_COMPOSITE_LAYER and
        handler.WriteFile(path)):
      # If we found it, we already wrote it.
      pass

    else:
      # Otherwise, try write out the default dbroot.
      try:
        # Write dbRoot from globe package.
        handler.write(tornado.web.globe_.ReadDbRoot(path, layer_id))

      except portable_globe.UnableToFindException, e:
        print e.message

  def LocalLayerVectorFileHandler(self, handler, path, layer_id):
    """Handle GET request for a file from a particular layer."""
    if not handler.IsValidRequest():
      raise tornado.web.HTTPError(404)

    if path == "root.kml":
      path = "skel.kml"
    elif path == "eb_balloon":
      ftid = handler.request.arguments["ftid"][0].replace(":", "-")
      path = "balloon_%s.html" % ftid

    try:
      handler.write(tornado.web.globe_.ReadLayerFile(
          "earth/vector_layer/%s" % path, layer_id))
    except:
      print "Unable to find", path

  def LocalLayerDocsHandler(self, handler, path, layer_id):
    """Handle GET request for a file from a particular layer."""
    if not handler.IsValidRequest():
      raise tornado.web.HTTPError(404)

    try:
      handler.write(tornado.web.globe_.ReadLayerFile(path, layer_id))
    except Exception as e:
      print "Unable to find", path, e

  def LocalDocsHandler(self, handler, path):
    """Handle GET request for some document.

    For example it is used for javascript files for
    the Google Earth web browser plugin.

    Args:
      handler: Handler processing current web request.
      path: Path to file to be returned.
    """
    if not handler.IsValidRequest():
      raise tornado.web.HTTPError(404)

    # See if the request includes changing the globe.
    try:
      if handler.request.arguments["globe"][0]:
        globe_path = "%s%s%s" % (
            tornado.web.globe_.GlobeBaseDirectory(),
            os.sep, handler.request.arguments["globe"][0])
        tornado.web.globe_.ServeGlobe(globe_path)
    except:
      pass

    handler.WriteFile(path)

  def ConvertIconPath(self, icon_path):
    """Strips away prefix and indicates if it is local to server.

    Paths are of the form:
      /icons/some_path returns (some_path, False, False)
        - Look in top-level files of the glc
      icons/some_path returns (some_path, True, False)
        - Look in specified or default layer of the glc
      local/icons/some_path returns (some_path, True, True)
        - Look in Portable server
    Args:
      icon_path: Raw path to icon
    Returns:
      (stripped_path, use_layer, use_local)
    """
    use_layer = True
    if icon_path[0] == "/":
      use_layer = False
      icon_path = icon_path[1:]
    use_local = False
    if icon_path[0:6] == "local/":
      icon_path = icon_path[6:]
      use_local = True
    if icon_path[0:6] == "icons/":
      icon_path = icon_path[6:]
    return (icon_path, use_layer, use_local)

  def LocalIconHandler(self, handler, icon, layer_id, use_local=False):
    """Handle GET request for icon."""
    if not handler.IsValidRequest():
      raise tornado.web.HTTPError(404)

    if layer_id > 0:
      try:
        if use_local:
          handler.WriteLocalFile("icons/%s" % icon)
        else:
          handler.write(tornado.web.globe_.ReadLayerFile(
              "icons/%s" % icon, layer_id))
      except:
        print "Unable to find", icon
    else:
      self.GetIcon(handler, "icons/%s" % icon, use_local)

  def LocalKmlSearchHandler(self, handler):
    """Handle GET request for kml search results."""
    if not handler.IsValidRequest():
      raise tornado.web.HTTPError(404)

    try:
      version = handler.request.arguments["cv"][0]
      if float(version[:3]) >= 7.1:
        self.LocalOneBoxKmlSearchHandler(handler)
        return
    except KeyError:
      pass  # Unknown version.

    try:
      service = handler.request.arguments["service"][0]
      self.search_services_[service].KmlSearch(handler)
      return
    except KeyError:
      pass  # Search service has not been defined; use default.

    if "displayKeys" in handler.request.arguments.keys():
      key = handler.request.arguments["displayKeys"][0]
      if key in handler.request.arguments.keys():
        try:
          search_term = handler.request.arguments[key][0].lower()
        except KeyError:
          # Earth 6.2.2+ will use "q" instead.
          search_term = handler.request.arguments[ALT_SEARCH_SERVICE_KEY][0]
        handler.write(tornado.web.globe_.search_db_.KmlSearch(search_term))

  def LocalOneBoxKmlSearchHandler(self, handler):
    """Handle GET request for kml search results from search box."""
    search_term = handler.request.arguments[ALT_SEARCH_SERVICE_KEY][0]
    # Look for POI prefix match.
    results = tornado.web.globe_.search_db_.KmlSearch(search_term).strip()
    if results:
      if "Placemark" in results:
        handler.write(results)
        return
    # Then try POI anywhere.
    results = tornado.web.globe_.search_db_.KmlSearch(
        ".*%s" % search_term).strip()
    if results:
      if "Placemark" in results:
        handler.write(results)
        return
    # If no POI matches, try GEPlaces.
    if "GEPlacesPlugin" in self.search_services_.keys():
      self.search_services_["GEPlacesPlugin"].KmlSearch(handler)

  def LocalJsonSearchHandler(self, handler):
    """Handle GET request for json search results."""
    if not handler.IsValidRequest():
      raise tornado.web.HTTPError(404)

    cb = handler.request.arguments["cb"][0]
    service = handler.request.arguments["service"][0]
    try:
      self.search_services_[service].JsonSearch(handler, cb)
      return
    except KeyError:
      pass  # Search service has not been defined; use default.

    if "displayKeys" in handler.request.arguments.keys():
      key = handler.request.arguments["displayKeys"][0]
      if key in handler.request.arguments.keys():
        search_term = handler.request.arguments[key][0].lower()
        handler.write(tornado.web.globe_.search_db_.JsonSearch(search_term, cb))

  def ParseGlobeReqName(self, req):
    """Ascertain whether requested globe name exists in data directory."""
    globe_name = ""
    if "/" in req:
      try:
        globe_name = req.split("/")[1]
      except:
        globe_name = req.split("/")[0]

    # If the globe requested is not already selected
    if globe_name != tornado.web.globe_.GlobeName():
      if globe_name in portable_web_interface.SetUpHandler.GlobeNameList(
          tornado.web.globe_.GlobeBaseDirectory(),[".glc", ".glb", ".glm"]):
        # Globe requested is in the list of globes
        return globe_name
      else:
        # Invalid globe name
        return -1
    else:
      # Globe requested is the current selectedGlobe
      return 1

  def JStoJson(self, js_string):
    """Converts a JS server definition string to valid JSON."""
    # Remove "var geeServerDefs = " or similar from start.
    # Then add quotes to JSON keys that don't have them.
    # Strip out the trailing ';'
    # Finally, push it through json.dumps to ensure consistently-formatted output.
    out_string = re.sub(r"^var \w+ ?= ?", "", js_string)
    out_string = re.sub(r"([,{]\s+)(\w+):", r'\1"\2":', out_string)
    out_string = out_string.strip(";")
    return json.dumps(json.loads(out_string))

  def LocalJsonHandler(self, handler, is_2d=False, json_version=1):
    """Handle GET request for JSON file for plugin."""
    if not handler.IsValidRequest():
      raise tornado.web.HTTPError(404)

    current_globe = ""
    globe_request_name = self.ParseGlobeReqName(handler.request.uri)
    if globe_request_name != -1 and globe_request_name != 1:
      # Requested globe name is valid, so select it
      current_globe = tornado.web.globe_.GlobeName()
      globe_path = "%s%s%s" % (
          tornado.web.globe_.GlobeBaseDirectory(),
          os.sep, globe_request_name)
      tornado.web.globe_.ServeGlobe(globe_path)

    # Get to end of serverUrl so we can add globe name.
    # This will fail if serverDefs are requested for a glc file
    try:
      if is_2d:
        # TODO: Add real layer support for mbtiles.
        if tornado.web.globe_.IsMbtiles():
          json = MBTILES_JSON
        else:
          # Portable seems to believe that 2D files are 3D when they
          # are not actively being viewed by a client, so handle
          # both possibilities in either case.
          try:
            json = tornado.web.globe_.ReadFile("maps/map.json")
          except:
            json = tornado.web.globe_.ReadFile("earth/earth.json")
      else:
        try:
          json = tornado.web.globe_.ReadFile("earth/earth.json")
        except:
          json = tornado.web.globe_.ReadFile("maps/map.json")

    except:
      handler.write("var geeServerDefs = {};")
      return

    host = handler.request.host
    json = json.replace("localhost:9335", host)
    json = json.replace(" : ", ": ")

    WHITE_SPACE_ALLOWED = 3
    index0 = json.find("serverUrl")
    if index0 == -1:
      print "Non-standard 2d map json."
      handler.write(json)
      return
    else:
      index0 += 9

    index1 = json.find(":", index0)
    if index1 == -1 or index1 > index0 + WHITE_SPACE_ALLOWED:
      print "Non-standard 2d map json."
      handler.write(json)
      return
    else:
      index1 += 1

    index2 = json.find('"', index1)
    if index2 == -1 or index2 > index1 + WHITE_SPACE_ALLOWED:
      print "Non-standard 2d map json."
      handler.write(json)
      return
    else:
      index2 += 1

    index3 = json.find('"', index2)
    if index3 == -1:
      print "Non-standard 2d map json."
      handler.write(json)
      return

    json_start = json[:index3].strip()
    json_end = json[index3:]

    # Get rid of the end of structure, so we can add to it.
    json_end = json_end[:json_end.rfind("}")].strip()

    # If not from a remote server, show available globes.
    if handler.IsLocalhost():
      # Add information about available globes
      json_end = (("%s,\n\"selectedGlobe\": \"%s\"\n, "
                   "\"globes\": [\"%%s\"]};") %
                  (json_end, tornado.web.globe_.GlobeName()))
      json_end %= "\", \"".join(
          portable_web_interface.SetUpHandler.GlobeNameList(
              tornado.web.globe_.GlobeBaseDirectory(),
              [".glc", ".glb", ".glm"]))
    else:
      json_end += "};"

    # Adding globe name helps ensure clearing of cache for new globes.
    json_text = "%s/%s%s" % (
      json_start, tornado.web.globe_.GlobeShortName(), json_end)

    if json_version == 2:
      json_text = self.JStoJson(json_text)

    handler.write(json_text)

    # If we switched globes, switch back
    if len(current_globe):
      globe_path = "%s%s%s" % (
          tornado.web.globe_.GlobeBaseDirectory(),
          os.sep, globe_request_name)
      tornado.web.globe_.ServeGlobe(globe_path)

  def ConvertToQtNode(self, col, row, level):
    """Converts col, row, and level to corresponding qtnode string."""
    qtnode = "0"
    half_ndim = 1 << (level - 1)
    for unused_ in xrange(level):
      if row >= half_ndim and col < half_ndim:
        qtnode += "0"
        row -= half_ndim
      elif row >= half_ndim and col >= half_ndim:
        qtnode += "1"
        row -= half_ndim
        col -= half_ndim
      elif row < half_ndim and col >= half_ndim:
        qtnode += "2"
        col -= half_ndim
      else:
        qtnode += "3"

      half_ndim >>= 1

    return qtnode

  def LocalMapTileHandler(self, handler, is_imagery, layer_id):
    """Handle requests for map imagery or vector jpeg tiles."""
    if not handler.IsValidRequest():
      raise tornado.web.HTTPError(404)

    x = int(handler.request.arguments["x"][0])
    y = int(handler.request.arguments["y"][0])
    z = int(handler.request.arguments["z"][0])
    if tornado.web.globe_.IsMbtiles() and is_imagery:
      handler.write(
          tornado.web.globe_.ReadMapImageryTile(x, y, z))
      return

    qtnode = self.ConvertToQtNode(x, y, z)
    channel = int(handler.request.arguments["channel"][0])
    try:
      if is_imagery:
        handler.write(
            tornado.web.globe_.ReadMapImageryPacket(qtnode, channel, layer_id))
        return
      else:
        handler.write(
            tornado.web.globe_.ReadMapVectorPacket(qtnode, channel, layer_id))
        return
    except:
      # Super-sample tiles where there is missing imagery data.
      if (not is_imagery or
          not tornado.web.globe_.IsBaseLayer(layer_id, channel)):
        return

      if (not pil_enabled or
          not tornado.web.globe_.config_.FillMissingMapTiles()):
        if tornado.web.globe_.IsBaseLayer(layer_id, channel):
          handler.write(self.empty_tile_)
        return

      length = len(qtnode)
      max_index = min(length,
                      tornado.web.globe_.config_.MaxMissingMapTileAncestor())
      index = 1
      size = 128
      xoff = 0
      yoff = 0
      # Use shift since we are moving right to left off address.
      shift = 1
      while index < max_index:
        try:
          if size > 0:
            last_node = qtnode[length - index]
            if last_node == "0":
              yoff += shift
            elif last_node == "1":
              yoff += shift
              xoff += shift
            elif last_node == "2":
              xoff += shift
            elif last_node == "3":
              pass
            else:
              raise(
                  Exception("Unexpected qtnode: %s" %  qtnode[:length - index]))

          parser = ImageFile.Parser()
          parser.feed(
              tornado.web.globe_.ReadMapImageryPacket(
                  qtnode[:length - index], channel, layer_id))
          image = parser.close()
          # Skip 1 x 1 placeholders
          if image.size[0] == 1:
            if tornado.web.globe_.IsBaseLayer(layer_id, channel):
              handler.write(self.empty_tile_)
            return

          xoffset = xoff * size
          yoffset = yoff * size
          image = image.crop((xoffset,
                              yoffset,
                              xoffset + size,
                              yoffset + size)).resize((256, 256))
          output = StringIO.StringIO()
          # If has palette, use PNG.
          if image.mode == "P":
            image.save(output, "PNG")
          else:
            image.save(output, "JPEG")

          content = output.getvalue()
          output.close()
          handler.write(content)
          return
        except portable_globe.UnableToFindException, e:
          index += 1
          if size > 1:
            size >>= 1   # divide by 2
            shift <<= 1  # multiply by 2
          else:
            # If we get down to a single pixel,
            # keep cutting the resolution of the
            # offset in half to grab the "best"
            # pixel.
            xoff >>= 1  # divide by 2
            yoff >>= 1  # divide by 2

      # Unable to find a near enough ancestor; show "no data".
      if tornado.web.globe_.IsBaseLayer(layer_id, channel):
        handler.write(self.empty_tile_)

  def LocalPingHandler(self, handler):
    """Handle GET request for ping."""
    if not handler.IsValidRequest():
      handler.write(handler.request.remote_ip)
      return
    handler.write("Server is live.")

  def LocalInfoHandler(self, handler):
    """Handle GET request for information about current globe."""
    handler.write("<p><b>Globe version:</b> %s"
                  % tornado.web.globe_.GetVersion())
    crc = tornado.web.globe_.GetCrc()
    handler.write("<p><b>Internal globe crc:</b> %s %s %s %s"
                  % (hex(crc[0]), hex(crc[1]), hex(crc[2]), hex(crc[3])))
    crc = tornado.web.globe_.CalculateCrc()
    handler.write("<br><b>Calcluated globe crc:</b> %s %s %s %s"
                  % (hex(crc[0]), hex(crc[1]), hex(crc[2]), hex(crc[3])))
    handler.write("<br><b>Info file:</b><pre>")
    try:
      handler.write(tornado.web.globe_.ReadFile("earth/info.txt"))
      handler.write("</pre>")
    except portable_globe.UnableToFindException:
      print "no info file"
