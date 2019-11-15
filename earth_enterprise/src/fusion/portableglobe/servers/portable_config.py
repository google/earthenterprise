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


"""Classes for loading and providing configuration data."""

import os
import re
import sys


class PortableConfig(object):
  """Class for holding the configuration data for poratable server.

  Is needed for both starting and stopping the server.
  """

  def __init__(self):
    """Loads initial values from config file."""
    fp = self.OpenConfigFile("portable.cfg")
    if not fp:
      print "Unable to open config file."
      return

    self.port_ = 9335
    self.glr_base_directory_ = ""
    self.glr_name_ = ""
    self.globe_base_directory_ = ""
    self.search_services_directory_ = ""
    self.ext_service_directory_ = ""
    self.globe_directory_ = ""
    self.globe_name_ = ""
    self.database_ = ""
    self.serving_globe_ = False
    self.local_override_ = False
    self.fill_missing_map_tiles_ = False
    self.max_missing_map_tile_ancestor_ = 24
    self.debug_ = False
    self.accept_all_requests_ = False
    self.single_imagery_tile_ = False
    self.disable_broadcasting_ = True
    self.use_plugin_ = True
    line_regex = re.compile("\s*(\w+)\s+(.*)$")
    for line in fp:
      line = line.strip()
      if not line:
        continue
      match = line_regex.match(line)
      if match:
        if match.group(1).lower() == "port":
          self.port_ = int(match.group(2))
        elif match.group(1).lower() == "globes_directory":
          self.globe_base_directory_ = match.group(2)
        elif match.group(1).lower() == "globe_name":
          self.globe_name_ = match.group(2)
        elif match.group(1).lower() == "database":
          self.database_ = match.group(2)
        elif match.group(1).lower() == "glr_directory":
          self.glr_base_directory_ = match.group(2)
        elif match.group(1).lower() == "search_services_directory":
          self.search_services_directory_ = match.group(2)
        elif match.group(1).lower() == "ext_service_directory":
          self.ext_service_directory_ = match.group(2)
        elif match.group(1).lower() == "fill_missing_map_tiles":
          self.fill_missing_map_tiles_ = match.group(2)[0].lower() == "t"
        elif match.group(1).lower() == "max_missing_map_tile_ancestor":
          self.max_missing_map_tile_ancestor_ = int(match.group(2))
        elif match.group(1).lower() == "disable_broadcasting":
          if match.group(2)[0].lower() == "f":
            self.disable_broadcasting_ = False
          else:
            print "Broadcasting is not allowed."
        elif match.group(1).lower() == "accept_all_requests":
          if match.group(2)[0].lower() == "t":
            self.accept_all_requests_ = True
            print "Allowing all requests."
        elif match.group(1).lower() == "single_imagery_tile":
          if match.group(2)[0].lower() == "t":
            self.single_imagery_tile_ = True
            print "Serving single imagery tile (./local/imagery_tile)."
        elif match.group(1).lower() == "debug":
          if match.group(2)[0].lower() == "t":
            self.debug_ = True
            print "Debug messaging is ON."
        elif match.group(1).lower() == "local_override":
          if match.group(2)[0].lower() == "t":
            self.local_override_ = True
            print "Local override is ON."
        elif match.group(1).lower() == "use_plugin":
          if match.group(2)[0].lower() == "f":
            self.use_plugin_ = False
            print "Not using plugin."
        else:
          print "Unknown configuration parameter: %s" % match.group(1)
    fp.close()
    self.CheckStartupGlobe()

  def OpenConfigFile(self, config_file):
    """Tries to open given config file.

    Tries to open the config file in the current directory. If that
    fails, it tries in the next directory up. It continues for up
    to three directories up. This is useful for the Mac where the
    working directory is nested three levels in and is hidden in the
    Finder from the user. Three levels up is where the application
    appears to reside.

    Args:
      config_file: Name of config file.
    Returns:
       file pointer or None if unable to open file.
    """
    prefix = "..%s" % os.sep
    count = 0
    fp = None
    while not fp and count < 4:
      try:
        fp = open(config_file, "r")
      except IOError:
        # Move to next directory up.
        config_file = "%s%s" % (prefix, config_file)
        print config_file
        count += 1

    return fp

  def Debug(self):
    return self.debug_

  def Port(self):
    return self.port_

  def AcceptAllRequests(self):
    return self.accept_all_requests_

  def DisableBroadcasting(self):
    return self.disable_broadcasting_

  def FillMissingMapTiles(self):
    return self.fill_missing_map_tiles_

  def MaxMissingMapTileAncestor(self):
    return self.max_missing_map_tile_ancestor_

  def CheckStartupGlobe(self):
    """If startup globe was passed, use it instead of config parameters."""
    if len(sys.argv) >= 2:
      globe = sys.argv[1]
      idx = globe.rfind(os.sep)
      if idx >= 0:
        self.globe_base_directory_ = globe[:idx]
        self.globe_name_ = globe[idx+1:]

  def GlobeBaseDirectory(self):
    return self.globe_base_directory_

  def GlobeName(self):
    return self.globe_name_

  def Database(self):
    return self.database_

  def LocalOverride(self):
    return self.local_override_

  def UsePlugin(self):
    return self.use_plugin_

  def SearchServicesDirectory(self):
    return self.search_services_directory_

  def ExtServiceDirectory(self):
    return self.ext_service_directory_
