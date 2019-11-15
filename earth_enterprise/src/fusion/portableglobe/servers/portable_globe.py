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


import json
import os
import re
import urllib

import glc_unpacker

import file_search
import portable_config
import postgres_search
try:
  import sqlite3
  MBTILE_SUPPORT = True
except ImportError:
  print "Mbtiles are not supported."
  MBTILE_SUPPORT = False
import stub_search

NON_COMPOSITE_LAYER = 0
COMPOSITE_BASE_LAYER = 1


class UnableToFindException(Exception):
  """Exception when unable to find packet or file."""

  def __init__(self, message=""):
    self._set_message(message)

  def _get_message(self):
    return self._message

  def _set_message(self, message):
    self._message = message

  message = property(_get_message, _set_message)


class Globe(object):
  """Base class for all portable server classes."""

  def __init__(self, globe_name=""):
    """Initializes globe.

    Includes getting configuration information for the server
    and setting up the initial globe file.
    """
    self.is_base_layer_ = {}

    # Repository of configuration data for serving the globe.
    self.config_ = portable_config.PortableConfig()
    self.file_loc_ = glc_unpacker.PackageFileLoc()

    self.globe_base_directory_ = self.config_.GlobeBaseDirectory()
    if not globe_name:
      self.globe_name_ = self.config_.GlobeName()

    self.port_ = self.config_.Port()

    # Decide on the type of search to use.
    # TODO: Allow non-postgres choice in config file.
    database = self.config_.Database()
    if not database or database == "None":
      self.search_db_ = stub_search.StubDatabase()
    elif database.lower() == "file":
      self.search_db_ = file_search.FileDatabase()
    else:
      self.search_db_ = postgres_search.PostgresDatabase(database)

    # Serve initial globe specified in the config file.
    self.ServeGlobe(self.GlobePath())

  def IsGee(self):
    """Returns whether file appears to be a legal glx."""
    return self.is_gee_

  def Is2d(self):
    """Returns whether glx can serve 2d map data."""
    return self.is_2d_

  def Is3d(self):
    """Returns whether glx can serve 3d globe data."""
    return self.is_3d_

  def IsMbtiles(self):
    """Returns if the map is stored as mbtiles (sqlite)."""
    return self.is_mbtiles_

  def IsComposite(self):
    """Returns whether the globe is a composite of multiple glbs."""
    return self.is_composite_

  def IsBaseLayer(self, layer_id, channel):
    """Returns whether the layer is a base layer (ie show 'no data' tiles)."""
    glm_layer_id = self.LayerChannelId(layer_id, channel)
    if glm_layer_id in self.is_base_layer_:
      return self.is_base_layer_[glm_layer_id]

    print "Unknown layer (IsBaseLayer)."
    return False

  def GetData(self):
    """Returns package or file content currently pointed to by unpacker."""
    offset = 0L + (self.file_loc_.HighOffset() & 0xffffffff) << 32
    offset += (self.file_loc_.LowOffset() & 0xffffffff)
    fp = open(self.GlobePath(), "rb")
    fp.seek(offset)
    # We should never be requesting files whose size doesn't fit in the
    # lower 32 bits.
    content = fp.read(self.file_loc_.LowSize())
    fp.close()
    return content

  def GetVersion(self):
    """Returns format version of the globe."""
    offset = (0L + os.path.getsize(self.GlobePath()) -
              glc_unpacker.Package.kVersionOffset)
    fp = open(self.GlobePath(), "rb")
    fp.seek(offset)
    version = fp.read(glc_unpacker.Package.kVersionSize)
    fp.close()
    return version

  def GetCrc(self):
    """Returns crc of the globe."""
    offset = (0L + os.path.getsize(self.GlobePath()) -
              glc_unpacker.Package.kCrcOffset)
    fp = open(self.GlobePath(), "rb")
    fp.seek(offset)
    crc_str = fp.read(glc_unpacker.Package.kCrcSize)
    fp.close()
    crc = [ord(crc_str[0]), ord(crc_str[1]), ord(crc_str[2]), ord(crc_str[3])]
    return crc

  def CalculateCrc(self):
    """Returns calculated crc for the globe."""
    size = ((os.path.getsize(self.GlobePath())
             - glc_unpacker.Package.kCrcOffset)
            / glc_unpacker.Package.kCrcSize)
    fp = open(self.GlobePath(), "rb")

    crc = [0, 0, 0, 0]
    for unused_i in xrange(size):
      for j in xrange(glc_unpacker.Package.kCrcSize):
        ch = fp.read(1)
        crc[j] ^= ord(ch)

    fp.close()
    return crc

  def FileExists(self, relative_file_path):
    """Returns whether file exists in current glx."""
    relative_file_path = relative_file_path.encode("ascii", "ignore")
    return self.unpacker_.FindFile(relative_file_path, self.file_loc_)

  def ReadFile(self, relative_file_path):
    """Returns content of file stored in globe package file.

    If file is not found, returns an empty string.
    """
    relative_file_path = relative_file_path.encode("ascii", "ignore")
    if self.unpacker_.FindFile(relative_file_path, self.file_loc_):
      return self.GetData()
    else:
      raise UnableToFindException("Unable to find file %s." %
                                  relative_file_path)

  def ReadLayerFile(self, relative_file_path, layer_id):
    """Returns content of file stored in layer package file.

    If file is not found, returns an empty string.
    Args:
      relative_file_path: relative path to file in the layer.
      layer_id: id of layer in the composite globe.
    Returns:
      The file content.
    Raises:
      UnableToFindException
    """
    if self.unpacker_.FindLayerFile(
        relative_file_path, layer_id, self.file_loc_):
      return self.GetData()
    else:
      raise UnableToFindException("Unable to find file.")

  def ReadMetaDbRoot(self):
    """Returns meta dbroot if found, otherwise throws an exception."""
    if self.unpacker_.FindMetaDbRoot(self.file_loc_):
      return self.GetData()
    else:
      raise UnableToFindException("Unable to find meta dbroot.")

  def ReadDbRoot(self, path, layer_id):
    """Returns dbroot.

    If dbroot is not found, throws an exception.
    """
    if self.unpacker_.FindLayerFile(path, layer_id, self.file_loc_):
      data = self.GetData()
      print "path dbroot: ", path, len(data)
      return data
    elif self.unpacker_.FindQtpPacket(
        "0", glc_unpacker.kDbRootPacket, 0, layer_id, self.file_loc_):
      data = self.GetData()
      return data
    else:
      print "Did not find:", path, layer_id
      raise UnableToFindException("Unable to find dbroot.")

  def ReadDataPacket(self, qtpath, packet_type, channel, layer_id):
    """Returns packet at given address and channel.

    If packet is not found, throws an exception.
    """
    if self.unpacker_.FindDataPacket(
        qtpath, packet_type, channel, layer_id, self.file_loc_):
      return self.GetData()
    else:
      raise UnableToFindException("Unable to find packet.")

  def ReadQtPacket(self, qtpath, layer_id):
    """Returns quadtree packet at given address.

    If quadtree packet is not found, throws an exception.
    Args:
      qtpath: the quadtree node of the imagery packet.
      layer_id: id of layer in the composite globe.
    Returns:
      The quadtree packet itself.
    Raises:
      UnableToFindException
    """
    if self.unpacker_.FindQtpPacket(
        qtpath, glc_unpacker.kQtpPacket, 0, layer_id, self.file_loc_):
      return self.GetData()
    else:
      raise UnableToFindException("Unable to find quadtree packet.")

  def ReadImageryPacket(self, qtpath, layer_id):
    """Returns imagery packet at given address.

    If imagery packet is not found, throws an exception.

    Args:
      qtpath: the quadtree node of the imagery packet.
      layer_id: id of layer in the composite globe.
    Returns:
      The imagery packet itself.
    Raises:
      UnableToFindException
    """
    return self.ReadDataPacket(
        qtpath, glc_unpacker.kImagePacket, 0, layer_id)

  def ReadTerrainPacket(self, qtpath, layer_id):
    """Returns terrain packet at given address.

    If terrain packet is not found, throws an exception.

    Args:
      qtpath: the quadtree node of the terrain packet.
      layer_id: id of layer in the composite globe.
    Returns:
      The terrain packet itself.
    """
    return self.ReadDataPacket(
        qtpath, glc_unpacker.kTerrainPacket, 1, layer_id)

  def ReadVectorPacket(self, qtpath, channel, layer_id):
    """Returns vector packet at given address and channel.

    If vector packet is not found, throws an exception.

    Args:
      qtpath: the quadtree node of the vector packet.
      channel: the channel of the vector packet.
      layer_id: id of layer in the composite globe.
    Returns:
      The vector packet itself.
    """
    return self.ReadDataPacket(
        qtpath, glc_unpacker.kVectorPacket, channel, layer_id)

  def ReadMapDataPacket(self, qtpath, packet_type, channel, layer_id):
    """Returns packet at given address and channel.

    If packet is not found, throws an exception.

    Args:
      qtpath: the quadtree node of the map packet.
      packet_type: the type of data in the map packet.
      channel: the channel of the map packet.
      layer_id: id of layer in the composite globe.
    Returns:
      The map packet itself.
    """
    if self.unpacker_.FindMapDataPacket(
        qtpath, packet_type, channel, layer_id, self.file_loc_):
      return self.GetData()
    else:
      raise UnableToFindException("Unable to find packet.")

  def ReadMapImageryPacket(self, qtpath, channel, layer_id):
    """Returns imagery packet at given address.

    If imagery packet is not found, throws an exception.

    Args:
      qtpath: the quadtree node of the map imagery packet.
      channel: the channel of the map imagery packet.
      layer_id: id of layer in the composite globe.
    Returns:
      The map imagery packet itself.
    """
    return self.ReadMapDataPacket(
        qtpath, glc_unpacker.kImagePacket, channel, layer_id)

  def ReadMapVectorPacket(self, qtpath, channel, layer_id):
    """Returns vector packet at given address and channel.

    If vector packet is not found, throws an exception.

    Args:
      qtpath: the quadtree node of the map vector packet.
      channel: the channel of the map vector packet.
      layer_id: id of layer in the composite globe.
    Returns:
      The map vector packet itself.
    """
    return self.ReadMapDataPacket(
        qtpath, glc_unpacker.kVectorPacket, channel, layer_id)

  def ReadMapImageryTile(self, x, y, z):
    """Returns tile from map tiles db."""
    # flip y
    y = (1 << z) - 1 - y
    sql = ("select tile_data"
           " from tiles"
           " where"
           "  tile_column = %d"
           "  and tile_row = %d"
           "  and zoom_level = %d;") % (x, y, z)
    if not self.db_:
      print "No tiles database."
      return ""

    result = self.db_.execute(sql).fetchone()
    return "%s" % result[0]

  def ServeGlobe(self, globe_path):
    """Sets local or remote globe to be served."""
    globe_path = os.path.normpath(globe_path)
    if not os.path.exists(globe_path):
      self.SetGlobePath(globe_path)
      print "Unable to find", globe_path
      return False

    if globe_path[-4:] == ".glb" or globe_path[-4:] == ".glm":
      self.ServeLocalGlobe(globe_path, False)
    elif globe_path[-4:] == ".glc":
      self.ServeLocalGlobe(globe_path, True)
    elif MBTILE_SUPPORT and globe_path[-4:] == ".mbt":
      self.ServeLocalTiles(globe_path)
    else:
      print "Unknown file type: ", globe_path
      return False

    print "Globe served!"
    self.SetType(globe_path)
    return True

  def ServeLocalTiles(self, globe_path):
    """Sets which tile db file is being served."""
    self.SetGlobePath(globe_path)
    self.db_ = None
    try:
      print "sqlite connection to ", globe_path
      self.db_ = sqlite3.connect(globe_path)
    except Exception:
      print "Failed to connect to %s." % globe_path

  def SetType(self, globe_name):
    """Sets is2d based on the given globe name."""
    self.is_gee_ = self.unpacker_.IsGee()
    if globe_name[-4:] == ".glb":
      self.is_2d_ = False
      self.is_3d_ = True
      self.is_composite_ = False
      self.is_mbtiles_ = False
    elif globe_name[-4:] == ".glm":
      self.is_2d_ = True
      self.is_3d_ = False
      self.is_composite_ = False
      self.is_mbtiles_ = False
    elif globe_name[-4:] == ".glc":
      self.is_2d_ = self.unpacker_.Is2d()
      self.is_3d_ = self.unpacker_.Is3d()
      self.is_composite_ = True
      self.is_mbtiles_ = False
    elif MBTILE_SUPPORT and globe_name[-4:] == ".mbt":
      self.is_local_ = True
      self.is_2d_ = True
      self.is_composite_ = False
      self.is_mbtiles_ = True
      return
    else:
      print "Unknown file type: ", globe_name

    if self.is_2d_:
      self.ParseJson(self.ReadFile("maps/map.json"))

  def ParseJson(self, json_text):
    """Extract needed information from json."""
    try:
      self.server_defs_ = json.loads(self.CleanJson(json_text))
      if "layers" in self.server_defs_:
        layers = self.server_defs_["layers"]
        for layer in layers:
          non_base_layer = False
          if "non_base_layer" in layer:
            non_base_layer = layer["non_base_layer"]
          glm_id = 0
          if "glm_id" in layer:
            glm_id = layer["glm_id"]

          if self.config_.Debug():
            print glm_id, layer["id"]
          glm_layer_id = self.LayerChannelId(glm_id, layer["id"])
          if self.config_.Debug():
            print "glm_layer_id:", glm_layer_id
          self.is_base_layer_[glm_layer_id] = not non_base_layer

    except Exception, e:
      print self.CleanJson(json_text)
      print "Unable to decode json."
      print e.message
      self.server_defs_ = None

  def CleanJson(self, json_text):
    """Clean up json to allow old server defs to be parsed."""
    index = json_text.find("geeServerDefs =")
    json_text = json_text[index + len("geeServerDefs ="):]
    json_text = re.sub(r"\r", r"\n", json_text)
    # Quote fields and remove pre-colon spaces
    json_text = re.sub(r'\n\s*"([\w-]+)"\s+:', r'\n"\1":', json_text)
    json_text = re.sub(r'\n\s*([\w-]+)\s+:', r'\n"\1":', json_text)
    # Remove extraneous commas
    json_text = re.sub(r",[\s\n]*\]", r"\n]", json_text)
    json_text = re.sub(r",[\s\n]*\}", r"\n}", json_text)
    # Remove semicolon
    json_text = re.sub(r"}[\s\n]*;", r"}", json_text)
    return json_text

  def ServeLocalGlobe(self, globe_path, is_composite):
    """Sets which globe is being served."""
    self.reader_ = glc_unpacker.PortableGlcReader(globe_path)
    self.unpacker_ = glc_unpacker.GlcUnpacker(self.reader_,
                                              is_composite,
                                              False)
    self.SetGlobePath(globe_path)
    self.search_db_.ClearSearchTables()
    # Keep search separate and specific to glc.
    self.LoadSearchDb()
    print "Serving globe:", globe_path

  def ServeLocalGlobeFiles(self, globe_path):
    """Sets which globe is being served, but will only serve top-level files."""
    globe_path = os.path.normpath(globe_path)
    if globe_path[-4:] == ".mbt":
      self.is_2d_ = True
      self.is_composite_ = False
      self.is_mbtiles_ = True
    else:
      self.reader_ = glc_unpacker.PortableGlcReader(globe_path)
      self.unpacker_ = glc_unpacker.GlcUnpacker(self.reader_,
                                                globe_path[-4:] == ".glc",
                                                True)
      self.is_2d_ = self.unpacker_.Is2d()
      self.is_3d_ = self.unpacker_.Is3d()
      self.is_mbtiles_ = False
      self.SetGlobePath(globe_path)

  def LoadSearchTable(self, table_file):
    """Create new table in Postgres and load it with content from file."""
    table_name = table_file[10:]  # Strip off "search_db/"
    print "   Loading search table from %s ..." % table_file
    content = self.ReadFile(table_file)
    if not content:
      print table_name, "not loaded."
      return

    self.search_db_.LoadSearchTable(table_name, content)

  def LoadSearchDb(self):
    """Load all search tables into Postgres."""
    print "Loading search db ..."
    table_files = []
    for i in xrange(self.unpacker_.IndexSize()):
      index_file = self.unpacker_.IndexFile(i)
      if index_file[0:10] == "search_db/":
        table_files.append(index_file)

    # Add creation and filling of all tables to the script.
    for table_file in table_files:
      self.LoadSearchTable(table_file)

  def GetRemoteGlobeInfo(self):
    """Gets info about remote globe being served."""
    self.remote_globe_name_ = ""
    self.remote_globe_description_ = ""
    try:
      f = urllib.urlopen("%s/?cmd=globe_info" % self.remote_address_)
      content = f.read()
      idx = content.find("\n")
      self.remote_globe_name_ = content[0:idx]
      self.remote_globe_description_ = content[idx+1:]
      self.SetType(self.remote_globe_name_)
      f.close()
      self.SetConnected(True)
    except IOError:
      self.SetConnected(False)
      return
    return content

  def ServingGlobe(self):
    return self.serving_globe_

  def GlobeBaseDirectory(self):
    return self.globe_base_directory_

  def GlobeName(self):
    return self.globe_name_

  def MaxGlobes(self):
    return 1

  def GlobeShortName(self):
    return self.globe_name_[:-4]

  def GlobePath(self):
    return os.path.join(self.globe_base_directory_, self.globe_name_)

  def LayerChannelId(self, layer_id, channel_id):
    if type(layer_id) != type(1):
      layer_id = int(layer_id)
    if type(channel_id) != type(1):
      channel_id = int(channel_id)
    return "%d-%d" % (layer_id, channel_id)

  def SetGlobePath(self, globe_path):
    globe_path = os.path.normpath(globe_path)
    self.globe_base_directory_ = os.path.dirname(globe_path)
    self.globe_name_ = os.path.basename(globe_path)

  def Database(self):
    return self.database_

  def Debug(self):
    return self.debug_

  def Port(self):
    return self.port_
