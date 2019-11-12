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

"""GlobeDirectory class manages information about globes in a given directory.

The class provides a means to get lists of all of the globes and maps in the
given directory. It also extracts and makes available the information about
each one, such as the description and the polygon used to cut it.
"""

import os
import portable_globe
import utils

# Minimum size before considered an actual globe (MB).
# Helps prevent listing of globes that have been reserved but
# not built yet.
GLOBE_SIZE_THRESHOLD = 1.0


class GlobeDirectory(object):
  """Manages information about globes in a given directory."""

  def __init__(self, directory, is_readable_globe=False):
    """Initializes data for each globe in the given directory."""
    self.directory_ = directory
    self.globes_ = {}

    # Create dictionary for each globe indexed by the globe's name.
    for globe in self._GetNameList():
      path = os.path.join(directory, globe)
      globe_info = {
          "short_name": globe[:-4],
          "name": "%s" % globe,
          "message": "",
          "path": path,
          "info_loaded": is_readable_globe,
          "size_in_bytes": os.path.getsize(path),
          "size": utils.FileSizeAsString(path),
          "is_gee": False
          }
      if globe_info["size_in_bytes"] > GLOBE_SIZE_THRESHOLD:
        # Add access to globe internals if requested.
        if is_readable_globe:
          try:
            globe_info["globe"] = portable_globe.Globe(globe_info["path"])
            if globe_info["globe"].IsGee():
              globe_info["is_gee"] = True
              if globe_info["globe"].Is2d():
                globe_info["is_2d"] = True
                globe_info["is_mercator"] = True
              else:
                globe_info["is_2d"] = False
                globe_info["is_mercator"] = False
              globe_info["is_3d"] = globe_info["globe"].Is3d()
              globe_info["description"] = self.EscapeJson(
                  globe_info["globe"].Description())
              globe_info["timestamp"] = globe_info["globe"].Timestamp()
              globe_info["has_polygon"] = (
                  globe_info["globe"].Polygon().find("<kml") >= 0)
              globe_info["is_being_served"] = True
            else:
              globe_info["message"] = ("Glx is corrupted or unreadable. "
                                       "Check permissions.")
          except Exception as e:
            globe_info["message"] = e.__str__()
        else:
          globe_info["message"] = "Glx read was not requested."
      else:
        globe_info["message"] = ("Glx is too small. "
                                 "May be being built or canceled.")
      self.globes_[globe] = globe_info

  def EscapeJson(self, json):
    """Returns a JSON legal string."""
    return json.replace('"', '\\"').replace("\n", "\\n").replace("\r", "\\r")

  def GlobeNames(self):
    """Returns a list of short names of available globes."""
    return self.globes_.keys()

  def _GetShortNameList(self):
    """Creates a list of short names of available globes."""
    try:
      file_list = os.listdir(self.directory_)
    except OSError:
      file_list = []

    return ["%s" % globe_file[:-4]
            for globe_file in file_list
            if os.path.isfile("%s%s%s" % (self.directory_, os.sep, globe_file))
            and globe_file[0] != "."
            and globe_file[-4:] == ".glb"]

  def _GetNameList(self):
    """Creates a list of names of available globes and maps."""
    try:
      file_list = os.listdir(self.directory_)
    except OSError:
      file_list = []

    return ["%s" % globe_file
            for globe_file in file_list
            if os.path.isfile("%s%s%s" % (self.directory_, os.sep, globe_file))
            and globe_file[0] != "."
            and globe_file[-4:] in [".glc", ".glb", ".glm"]]
