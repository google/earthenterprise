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

"""Module for examining contents of Portable glx.

Adapted from module of same name used in Portable Server.
"""

import os
import re

import portable_exceptions
import glc_unpacker

NON_COMPOSITE_LAYER = 0
COMPOSITE_BASE_LAYER = 1

UNKOWN_OR_UNREADABLE_GLX_ERROR_MSG = (
    "Unknown or unreadable glx: %s. Couldn't get unpacker. Check permissions.")


class Globe(object):
  """Base class for all portable server classes."""

  def __init__(self, globe_name=""):
    """Initializes globe.

    Includes getting configuration information for the server
    and setting up the initial globe file.

    Args:
      globe_name: The name of this globe.
    Raises:
      PortableException: if unknown file type or unreadable glx.
    """
    self.is_base_layer_ = {}

    # Repository of configuration data for serving the globe.
    self.file_loc_ = glc_unpacker.PackageFileLoc()

    # For cgi, look for globes relative to the cgi-bin directory.
    self.globe_base_directory_ = "."
    self.globe_name_ = globe_name

    self.glr_name_ = ""
    self.remote_address_ = ""
    self.port_ = 80

    self.remote_globe_name_ = ""
    self.remote_globe_description_ = ""
    self.unpacker_ = None
    self.is_gee_ = False
    self.has_imagery_ = False
    self.has_terrain_ = False
    self.is_proto_imagery_ = False
    # Search code removed

    # Serve initial globe specified in the config file.
    # raise Exception("path: " + self.GlobePath())
    self.ServeGlobe(self.GlobePath())

  def IsLocal(self):
    """Returns whether we are currently serving a local globe."""
    return self.is_local_

  def IsGee(self):
    """Returns whether file appears to be a legal glx."""
    return self.is_gee_

  def Is2d(self):
    """Returns whether glx can serve 2d map data."""
    return self.is_2d_

  def Is3d(self):
    """Returns whether glx can serve 3d globe data."""
    return self.is_3d_

  def HasImagery(self):
    """Returns whether glx has imagery."""
    return self.has_imagery_

  def HasTerrain(self):
    """Returns whether glx has terrain."""
    return self.has_terrain_

  def IsProtoImagery(self):
    """Returns whether glx dbroot is proto-buffer or not."""
    return self.is_proto_imagery_

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

  def _GetData(self):
    """Returns package or file content currently pointed to by unpacker."""
    assert self.unpacker_
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
    offset = (0L + os.path.getsize(self.GlobePath())
              - glc_unpacker.Package.kVersionOffset)
    fp = open(self.GlobePath(), "rb")
    fp.seek(offset)
    version = fp.read(glc_unpacker.Package.kVersionSize)
    fp.close()
    return version

  def GetCrc(self):
    """Returns crc of the globe."""
    offset = (0L + os.path.getsize(self.GlobePath())
              - glc_unpacker.Package.kCrcOffset)
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
    if not self.unpacker_:
      raise portable_exceptions.PortableException(
          UNKOWN_OR_UNREADABLE_GLX_ERROR_MSG % self.globe_name_)

    relative_file_path = relative_file_path.encode("ascii", "ignore")
    return self.unpacker_.FindFile(relative_file_path, self.file_loc_)

  def ReadFile(self, relative_file_path):
    """Returns content of file stored in globe package file.

    If file is not found, returns an empty string.

    Args:
      relative_file_path: Relative path to file within glx.
    Returns:
      Data from the file within the glx.
    Raises:
      PortableException: if unknown or unreadable glx.
      UnableToFindException: if unable to find specified file.
    """
    if not self.unpacker_:
      raise portable_exceptions.PortableException(
          UNKOWN_OR_UNREADABLE_GLX_ERROR_MSG % self.globe_name_)

    relative_file_path = relative_file_path.encode("ascii", "ignore")
    # print "FindFile", relative_file_path
    if self.unpacker_.FindFile(relative_file_path, self.file_loc_):
      # print "Found file", self.file_loc_.Offset()
      return self._GetData()
    else:
      raise portable_exceptions.UnableToFindException(
          "Unable to find file %s." % relative_file_path)

  def ReadLayerFile(self, relative_file_path, layer_id):
    """Returns content of file stored in layer package file.

    If file is not found, returns an empty string.
    Args:
      relative_file_path: relative path to file in the layer.
      layer_id: id of layer in the composite globe.
    Returns:
      The file content.
    Raises:
      PortableException: if unknown or unreadable glx.
      UnableToFindException: if unable to find the specified file.
    """
    if not self.unpacker_:
      raise portable_exceptions.PortableException(
          UNKOWN_OR_UNREADABLE_GLX_ERROR_MSG % self.globe_name_)

    if self.unpacker_.FindLayerFile(
        relative_file_path, layer_id, self.file_loc_):
      return self._GetData()
    else:
      raise portable_exceptions.UnableToFindException("Unable to find file.")

  def ReadMetaDbRoot(self):
    """Reads meta dbroot.

    Returns:
      meta dbroot if found, otherwise throws an exception.
    Raises:
      PortableException: if unknown or unreadable glx.
      UnableToFindException: if unable to find meta dbroot.
    """
    if not self.unpacker_:
      raise portable_exceptions.PortableException(
          UNKOWN_OR_UNREADABLE_GLX_ERROR_MSG % self.globe_name_)

    if self.unpacker_.FindMetaDbRoot(self.file_loc_):
      return self._GetData()
    else:
      raise portable_exceptions.UnableToFindException(
          "Unable to find meta dbroot.")

  def ReadDbRoot(self, path, layer_id):
    """Returns dbroot.

    If dbroot is not found, throws an exception.

    Args:
      path: path to dbRoot file to use. If not found uses first qtp packet.
      layer_id: id of layer in the composite globe.
    Returns:
      The dbroot.
    Raises:
      PortableException: if unknown or unreadable glx.
      UnableToFindException: if unable to find a dbroot.
    """
    if not self.unpacker_:
      raise portable_exceptions.PortableException(
          UNKOWN_OR_UNREADABLE_GLX_ERROR_MSG % self.globe_name_)

    if self.unpacker_.FindLayerFile(path, layer_id, self.file_loc_):
      data = self._GetData()
      print "path dbroot: ", path, len(data)
      return data
    elif self.unpacker_.FindQtpPacket(
        "0", glc_unpacker.kDbRootPacket, 0, layer_id, self.file_loc_):
      return self._GetData()
    else:
      print "Did not find dbRoot for: ", layer_id
      raise portable_exceptions.UnableToFindException("Unable to find dbroot.")

  def ReadDataPacket(self, qtpath, packet_type, channel, layer_id):
    """Returns packet at given address and channel.

    If packet is not found, throws an exception.

    Args:
      qtpath: the quadtree node of the data packet.
      packet_type: type of data packet.
      channel: channel of the data packet.
      layer_id: id of layer in the composite globe.
    Returns:
      The quadtree packet itself.
    Raises:
      PortableException: if unknown or unreadable glx.
      UnableToFindException: if unable to find the data packet.
    """
    if not self.unpacker_:
      raise portable_exceptions.PortableException(
          UNKOWN_OR_UNREADABLE_GLX_ERROR_MSG % self.globe_name_)

    if self.unpacker_.FindDataPacket(
        qtpath, packet_type, channel, layer_id, self.file_loc_):
      return self._GetData()
    else:
      raise portable_exceptions.UnableToFindException("Unable to find packet.")

  def ReadQtPacket(self, qtpath, layer_id):
    """Returns quadtree packet at given address.

    If quadtree packet is not found, throws an exception.
    Args:
      qtpath: the quadtree node of the quad set packet.
      layer_id: id of layer in the composite globe.
    Returns:
      The quadtree packet itself.
    Raises:
      PortableException: if unknown or unreadable glx.
      UnableToFindException: if unable to find the quad set packet.
    """
    if not self.unpacker_:
      raise portable_exceptions.PortableException(
          UNKOWN_OR_UNREADABLE_GLX_ERROR_MSG % self.globe_name_)

    if self.unpacker_.FindQtpPacket(
        qtpath, glc_unpacker.kQtpPacket, 0, layer_id, self.file_loc_):
      return self._GetData()
    else:
      raise portable_exceptions.UnableToFindException(
          "Unable to find quadtree packet.")

  def ReadImageryPacket(self, qtpath, layer_id):
    """Returns imagery packet at given address.

    If imagery packet is not found, throws an exception.

    Args:
      qtpath: the quadtree node of the imagery packet.
      layer_id: id of layer in the composite globe.
    Returns:
      The imagery packet itself.
    Raises:
      PortableException: if unknown or unreadable glx.
      UnableToFindException: if unable to find the imagery data packet.
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
    Raises:
      PortableException: if unknown or unreadable glx.
      UnableToFindException: if unable to find the terrain data packet.
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
    Raises:
      PortableException: if unknown or unreadable glx.
      UnableToFindException: if unable to find the vector data packet.
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
    Raises:
      PortableException: if unknown or unreadable glx.
      UnableToFindException: if unable to find the map data packet.
    """
    if not self.unpacker_:
      raise portable_exceptions.PortableException(
          UNKOWN_OR_UNREADABLE_GLX_ERROR_MSG % self.globe_name_)

    if self.unpacker_.FindMapDataPacket(
        qtpath, packet_type, channel, layer_id, self.file_loc_):
      return self._GetData()
    else:
      raise portable_exceptions.UnableToFindException("Unable to find packet.")

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

  def ServeGlobe(self, globe_path):
    """Sets local or remote globe to be served."""
    if globe_path[-4:] == ".glb" or globe_path[-4:] == ".glm":
      self.is_local_ = True
      self.ServeLocalGlobe(globe_path, False)
    elif globe_path[-4:] == ".glc":
      self.is_local_ = True
      self.ServeLocalGlobe(globe_path, True)
    elif globe_path[-4:] == ".glr":
      self.is_local_ = False
      self.LoadGlr(globe_path)
    else:
      raise portable_exceptions.PortableException(
          "Unknown file type: %s" % globe_path)

    if not self.unpacker_:
      raise portable_exceptions.PortableException(
          UNKOWN_OR_UNREADABLE_GLX_ERROR_MSG % globe_path)

    self.SetType(globe_path)

  def SetType(self, globe_name):
    """Sets is2d based on the given globe name."""
    self.is_gee_ = self.unpacker_.IsGee()
    if globe_name[-4:] == ".glb":
      self.is_2d_ = False
      self.is_3d_ = True
      self.is_composite_ = False
      self.SetDbrootInfo()
    elif globe_name[-4:] == ".glm":
      self.is_2d_ = True
      self.is_3d_ = False
      self.is_composite_ = False
    elif globe_name[-4:] == ".glc":
      self.is_2d_ = self.unpacker_.Is2d()
      self.is_3d_ = self.unpacker_.Is3d()
      self.is_composite_ = True
      if self.is_3d_:
        self.SetDbrootInfo()
    else:
      raise portable_exceptions.PortableException(
          "Unknown file type: %s" % globe_name)

  def SetDbrootInfo(self):
    """Set attributes of dbroot by reading/parsing dbroot-file from globe."""
    assert self.is_3d_
    dbroot_reader = glc_unpacker.DbRootInfoReader(self.reader_,
                                                  self.unpacker_)
    self.has_imagery_ = dbroot_reader.HasImagery()
    self.has_terrain_ = dbroot_reader.HasTerrain()
    self.is_proto_imagery_ = dbroot_reader.IsProtoImagery()

  def CleanJson(self, json_text):
    """Clean up json to allow old server defs to be parsed."""
    index = json_text.find("geeServerDefs =")
    json_text = json_text[index + len("geeServerDefs ="):]
    json_text = re.sub("\r", "\n", json_text)
    # Quote fields and remove pre-colon spaces
    json_text = re.sub(r'\n\s*"([\w-]+)"\s+:', r'\n"\1":', json_text)
    json_text = re.sub(r'\n\s*([\w-]+)\s+:', r'\n"\1":', json_text)
    # Remove extraneous commas
    json_text = re.sub(r",[\s\n]*\]", "\n]", json_text)
    json_text = re.sub(r",[\s\n]*\}", "\n}", json_text)
    # Remove semicolon
    json_text = re.sub(r"}[\s\n]*;", "}", json_text)
    return json_text

  def ServeLocalGlobe(self, globe_path, is_composite):
    """Sets which globe is being served."""
    self.reader_ = glc_unpacker.PortableGlcReader(globe_path)
    if not self.reader_.IsOpen():
      self.unpacker_ = None
      self.is_gee_ = False
      return

    self.unpacker_ = glc_unpacker.GlcUnpacker(self.reader_,
                                              is_composite,
                                              False)
    self.SetGlobePath(globe_path)
    # self.search_db_.ClearSearchTables()
    # Keep search separate and specific to glc.
    # print "Serving globe:", globe_path

  def ServeLocalGlobeFiles(self, globe_path):
    """Sets which globe is being served, but will only serve top-level files."""
    self.reader_ = glc_unpacker.PortableGlcReader(globe_path)
    if not self.reader_.IsOpen():
      self.unpacker_ = None
      self.is_gee_ = False
      return

    self.unpacker_ = glc_unpacker.GlcUnpacker(self.reader_,
                                              globe_path[-4:] == ".glc",
                                              True)
    self.is_2d_ = self.unpacker_.Is2d()
    self.is_3d_ = self.unpacker_.Is3d()
    self.SetGlobePath(globe_path)

  def Info(self):
    """Tries to get globe info from glb file.

    Returns:
      Info file from the globe.
      If no info file is found in the globe,
      it returns "Old globe (no info.txt)."
    """
    try:
      return self.ReadFile("earth/info.txt")

    # Don't fail if old globe with no info.txt file.
    except portable_exceptions.UnableToFindException:
      return "Old globe (no info.txt)."

  def Description(self):
    """Tries to get globe description from glb file.

    Returns:
      The globe description.
      If no description is available in the globe,
      it returns "No description given."
    """
    info = self.Info()
    # Divide entries by date lines.
    entries = re.split(r"[\r\n]*\d\d\d\d-\d\d-\d\d \d+:\d\d:\d\d[\r\n]+",
                       info)
    if len(entries) > 1 and entries[1][:19] == "Globe description: ":
      return entries[1][19:]

    return "No description given."

  def Timestamp(self):
    """Tries to get globe timestamp from glb file.

    Returns:
      The GMT timestamp for when the globe build started.
      If no timestamp is found in the globe,
      it returns "No timestamp."
    """
    info = self.Info()
    # Find the GMT line
    match = re.match(r".*[\r\n](\d\d\d\d-\d\d-\d\d \d+:\d\d:\d\d\s+GMT).*",
                     info, re.DOTALL)
    if match:
      return match.group(1)
    else:
      return "No timestamp."

  def Polygon(self):
    """Tries to get globe polygon from glb file.

    Returns:
      The polygon for the globe.
      If no polygon is found in the globe,
      it returns "No polygon."
    """
    try:
      return self.ReadFile("earth/polygon.kml")

    # Don't fail if old globe with no polygon file.
    except portable_exceptions.UnableToFindException:
      return "No polygon."

  def GlobeBaseDirectory(self):
    return self.globe_base_directory_

  def GlobeName(self):
    return self.globe_name_

  def MaxGlobes(self):
    return 1

  def GlobeShortName(self):
    return self.globe_name_[:-4]

  def GlobePath(self):
    return "%s%s%s" % (self.globe_base_directory_, os.sep, self.globe_name_)

  def LayerChannelId(self, layer_id, channel_id):
    if type(layer_id) != type(1):
      layer_id = int(layer_id)
    if type(channel_id) != type(1):
      channel_id = int(channel_id)
    return "%d-%d" % (layer_id, channel_id)

  def SetGlobePath(self, globe_directory):
    name_start = globe_directory.rfind(os.sep)
    self.globe_base_directory_ = globe_directory[0:name_start]
    self.globe_name_ = globe_directory[1 + name_start:]

  def Database(self):
    return self.database_

  def Debug(self):
    return self.debug_

  def Port(self):
    return self.port_
