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


"""Handlers for web interface for managing portable server."""

import os
import posixpath
import re
import random
import sys
import tornado.web

import portable_globe
import portable_server_base

# Template for json used to return info about globe being served.
GLOBE_INFO_OBJ_TEMPLATE = """{
  "name":"%s",
  "timestamp":"%s",
  "size":"%s",
  "description":"%s",
  "path":"%s",
  "is_2d":%s,
  "is_3d":%s,
  "has_polygon":%s,
  "is_mercator":%s,
  "is_being_served":%s
}"""

# Template for json used to return info about globe being served.
GLOBE_INFO_JSON_TEMPLATE = "%%s(%s);\n" % GLOBE_INFO_OBJ_TEMPLATE

# Template for json used to return info about globe being served.
GLOBES_INFO_JSON_TEMPLATE = "[%s]"

# Bogus date/time when none is found.
NO_TIMESTAMP = None
GMT_TIMESTAMP_REGEX = (r"(\d\d\d\d-\d\d-\d\d) (\d+:\d\d:\d\d)"
                       r"\s+GMT[\r\n]")
TIMESTAMP_REGEX = r"(\d\d\d\d-\d\d-\d\d) (\d+:\d\d:\d\d)[\r\n]"
GMT_TIMESTAMP_TEMPLATE = "%sT%s+00:00"

BYTES_PER_MEGABYTE = 1024.0 * 1024.0


def FileSize(file_path):
  """Returns size of file in megabytes."""
  return os.path.getsize(file_path) / BYTES_PER_MEGABYTE


def SizeAsString(size):
  """Converts megabyte float to a string."""
  if size < 1000.0:
    return "%0.2fMB" % size
  else:
    return "%0.2fGB" % (size / 1024.0)


def FileSizeAsString(file_path):
  """Returns size of file as a string."""
  return SizeAsString(FileSize(file_path))


def PosixPath(file_path):
  """Converts the file path to posixpath format."""
  return posixpath.join(*(file_path.split(os.sep)))


class SetUpHandler(portable_server_base.BaseHandler):
  """Class for handling setting up of a globe."""

  @staticmethod
  def GlobeNameList(directory, suffixes=None):
    """Creates a list of short names of available globes."""

    if suffixes is None:
      suffixes = [".glb"]

    try:
      file_list = os.listdir(directory)
    except OSError:
      file_list = []

    return ["%s" % globe_file
            for globe_file in file_list
            if os.path.isfile(
                os.path.normpath(os.path.join(directory, globe_file)))
            and globe_file[0] != "."
            and globe_file[-4:] in suffixes]

  @staticmethod
  def GlobeList(directory, suffixes=None):
    """Creates a list of available globes."""

    if suffixes is None:
      suffixes = [".glb"]

    try:
      file_list = [os.path.normpath(os.path.join(directory, globe_file))
                   for globe_file in os.listdir(directory)]
    except OSError:
      file_list = []

    return [globe_file for globe_file in file_list
            if os.path.isfile(globe_file) and globe_file[-4:] in suffixes]

  def GlobeDescriptionAndTimestamp(self, globe):
    """Retrieves the globe description."""
    data = {
        "description": "No description given.",
        "timestamp": NO_TIMESTAMP
        }
    if globe.IsMbtiles():
      data["description"] = "mbtiles database"
    else:
      try:
        info = globe.ReadFile("earth/info.txt")
        match = re.search(GMT_TIMESTAMP_REGEX, info)
        if not match:
          # This probably won't be GMT, but better than nothing.
          match = re.search(TIMESTAMP_REGEX, info)
        if match:
          data["timestamp"] = GMT_TIMESTAMP_TEMPLATE % (
              match.group(1), match.group(2))

        # Divide entries by date lines.
        entries = re.split(r"[\r\n]*\d\d\d\d-\d\d-\d\d \d+:\d\d:\d\d[\r\n]+",
                           info)
        if entries[1][:19] == "Globe description: ":
          description = entries[1][19:]
          description = description.replace("\n", "<br>")
          description = description.replace("\"", "'")
          data["description"] = description

      # Don't fail if old globe with no info.txt file.
      except portable_globe.UnableToFindException:
        data["description"] = "Old globe (no info.txt)."

    return data

  def GlobeDescription(self, globe):
    """Retrieves the globe description."""
    return self.GlobeDescriptionAndTimestamp(globe)["description"]

  def IsMercator(self, globe):
    """Retrieves whether the globe (map) is Mercator."""
    # TODO: Use regex test.
    if globe.Is2d():
      json = globe.ReadFile("maps/map.json")
      index = json.find("projection")
      if "mercator" in json[index + 11: index + 24]:
        return True
    return False

  def GlobeSelectDiv(self, directory, globe_info):
    """Creates an HTML select for available globes."""
    globe_list = self.GlobeList(directory, [".glb", ".glm", ".glc", ".mbt"])
    template = """
<div class="span-5" style="text-align:left">
<span class="field_title">&nbsp;&nbsp;
<a class="serving_action" href="?cmd=serve_globe&globe=[#GLOBE_PATH]">
[#GLOBE_NAME]</a></span></div>
<div class="span-5 last" style="text-align:right">
<a class="serving_action" href="?cmd=show_info&globe=[#GLOBE_PATH]">
<!-- info --></a></div>
<hr/>
"""
    info_template = """
<div class="span-5" style="text-align:left">
<span class="field_title">&nbsp;&nbsp;
<a class="serving_action" href="?cmd=serve_globe&globe=[#GLOBE_PATH]">
[#GLOBE_NAME]</a></span><br/>[#GLOBE_DESCRIPTION]</div>
<hr/>
"""
    selected_template = """
<div class="span-5" style="text-align:left">
<span class="field_title">&nbsp;&nbsp;
[#GLOBE_NAME]</span></div>
<div class="span-5 last" style="text-align:right">
now serving</div>
<hr/>
"""
    if globe_list:
      select = ""
      current_globe_name = tornado.web.globe_.GlobeName()
      for globe_dir in globe_list:
        globe_name = os.path.basename(globe_dir)
        if current_globe_name == globe_name:
          next_globe = selected_template
        else:
          if globe_info == globe_dir:
            next_globe = info_template.replace("[#GLOBE_DESCRIPTION]",
                                               "description")
          else:
            next_globe = template

        next_globe = next_globe.replace("[#GLOBE_NAME]", globe_name)
        next_globe = next_globe.replace("[#GLOBE_PATH]", PosixPath(globe_dir))
        select += next_globe
    else:
      select = ("<div class='span-5' style='text-align:left'>"
                "No .glb or .glm files found.</div>")

    return select

  def WriteForm(self, globe_info=""):
    """Writes the form for controlling the server."""
    fp = open("local/setup.html")
    setup_form_template = fp.read()
    fp.close()

    served_globe_template = """
        <span class="text_callout">[#SERVED_GLOBE_NAME]</span><br/>
        <i>[#SERVED_GLOBE_DESCRIPTION]</i>
        <p>
        [#SERVED_OPTIONS]"""
    local_options = ""
    server_state = ""
    server_links = ""
    local_options = ""
    if tornado.web.globe_.Is2d():
      page = "/maps/map_v3.html?globe=%s" % tornado.web.globe_.GlobeName()
      local_options += (
          """<a class="serving_action"  href="%s">view 2d in browser</a><br>"""
          % page)

    if tornado.web.globe_.Is3d() and tornado.web.globe_.config_.UsePlugin():
      page = ("/earth/earth_local.html?globe=%s" %
              tornado.web.globe_.GlobeName())
      local_options += (
          """<a class="serving_action"  href="%s">view 3d in browser</a><br>"""
          % page)

    if self.IsBroadcasting():
      server_state = "Broadcasting"
      globe_name = tornado.web.globe_.GlobeName()
      server_links = """
          <a class="serving_action"
          href="/?cmd=local_only">local only</a> | """
    else:
      server_state = "Serving locally"
      globe_name = tornado.web.globe_.GlobeName()
      server_links = """
          <a class="serving_action" href="/?cmd=share">broadcast</a> | """

    served_globe_info = served_globe_template.replace(
        "[#SERVED_GLOBE_DESCRIPTION]",
        self.GlobeDescription(tornado.web.globe_))

    served_globe_info = served_globe_info.replace("[#SERVED_GLOBE_NAME]",
                                                  globe_name)
    served_globe_info = served_globe_info.replace("[#SERVED_OPTIONS]",
                                                  local_options)

    form = setup_form_template.replace("[#SERVED_GLOBE_INFO]",
                                       served_globe_info)
    form = form.replace("[#SERVER_STATE]", server_state)
    form = form.replace("[#SERVER_LINKS]", server_links)
    form = form.replace("[#SERVABLE_GLOBES]",
                        self.GlobeSelectDiv(
                            tornado.web.globe_.GlobeBaseDirectory(),
                            globe_info))
    self.write(form)

  def WriteGlobeInfo(self):
    """Writes info about current globe being served."""
    if tornado.web.globe_.GlobePath():
      self.write(tornado.web.globe_.GlobeName())
      self.write("\n")
      self.write(self.GlobeDescription(tornado.web.globe_))
    else:
      self.write("No globe is being served.")

  @staticmethod
  def JsBooleanString(bool_arg):
    """Returns javascript compatible boolean value."""
    if bool_arg:
      return "true"
    else:
      return "false"

  def WriteMapJson(self):
    """Writes json containing info about current globe being served."""
    json = tornado.web.globe_.ReadFile("maps/map.json")
    # TODO: Use regex
    # index = json.find("=") + 2;
    # end_index = json.rfind(";");
    # self.write(json[index:end_index])
    self.write(json)

  def WriteGlobesInfo(self):
    """Writes info about globes available for serving."""
    globe_list = self.GlobeList(tornado.web.globe_.GlobeBaseDirectory(),
                                [".glb", ".glc", ".mbt"])
    for globe_path in globe_list:
      self.write(globe_path[len(tornado.web.globe_.GlobeBaseDirectory()) + 1:])
      self.write("\n")

  def WriteGlobesInfoJson(self):
    """Writes json containing info about maps available for serving."""
    globe_list = self.GlobeList(tornado.web.globe_.GlobeBaseDirectory(),
                                [".glb", ".glm", ".glc", ".mbt"])
    delimiter = "\n"
    globe_info_objs = ""
    for globe_path in globe_list:
      globe_info_obj = self.GetGlobeInfoJson(globe_path)
      if globe_info_obj:
        globe_info_objs += delimiter + globe_info_obj
        delimiter = ",\n"

    self.write(GLOBES_INFO_JSON_TEMPLATE % globe_info_objs)

  def WriteGlobeInfoJson(self):
    """Writes json containing info about the map that is currently serving."""
    self.write(self.GetGlobeInfoJson(tornado.web.globe_.GlobePath()))

  def GetGlobeInfoJson(self, globe_path):
    """Prepare json containing info about a map."""
    globe_info_obj = ""
    file_globe = portable_globe.Globe()
    try:
      globe_name = globe_path[
          len(tornado.web.globe_.GlobeBaseDirectory()) + 1:]
      file_globe.ServeLocalGlobeFiles(globe_path)
      globe_info = self.GlobeDescriptionAndTimestamp(file_globe)
      description = globe_info["description"]
      timestamp = globe_info["timestamp"]
      is_2d = self.JsBooleanString(file_globe.Is2d())
      is_3d = self.JsBooleanString(file_globe.Is3d())
      has_polygon = self.JsBooleanString(
          file_globe.FileExists("earth/polygon.kml"))
      is_mercator = self.JsBooleanString(self.IsMercator(file_globe))
      is_being_served = self.JsBooleanString(
          globe_path == tornado.web.globe_.GlobePath())
      globe_info_obj = (GLOBE_INFO_OBJ_TEMPLATE % (
          globe_name, timestamp, FileSizeAsString(globe_path),
          description, PosixPath(globe_path), is_2d,
          is_3d, has_polygon, is_mercator, is_being_served))
    except Exception as e:
      print "Bad globe: ", globe_name
      print e
    return globe_info_obj

  def get(self, path):
    """Handles GET request for set-up."""
    print "SetupHandler get", self.request.uri

    if "cmd" not in self.request.arguments:
      self.servePortablePage(path)
      return

    try:
      cmd = self.request.arguments["cmd"][0]
      print "cmd (get): \"%s\"" % cmd

      if cmd in ["globe_info_json", "globe_info"]:
        if not self.IsValidRequest():
          self.write("Invalid request.")
          return
      elif not self.IsLocalhost():
        self.write("Setup from localhost only.")
        return

      if cmd == "pre-quit":
        self.set_header("Content-Type", "text/html")
        self.WriteLocalFile("quit.html")
      elif cmd == "share":
        self.set_header("Content-Type", "text/html")
        self.WriteLocalFile("share.html")
      elif cmd == "globe_info":
        self.set_header("Content-Type", "text/plain")
        self.WriteGlobeInfo()
      elif cmd == "map_json":
        self.set_header("Content-Type", "text/plain")
        self.WriteMapJson()
      elif cmd == "globe_info_json":
        self.set_header("Content-Type", "application/json")
        self.WriteGlobeInfoJson()
      elif cmd == "globes_info":
        self.set_header("Content-Type", "text/plain")
        self.WriteGlobesInfo()
      elif cmd == "globes_info_json":
        self.set_header("Content-Type", "application/json")
        self.WriteGlobesInfoJson()
      elif cmd == "show_info":
        globe = self.request.arguments["globe"][0]
        self.WriteForm(globe_info=globe)
      elif cmd == "serve_globe":
        self.set_header("Content-Type", "text/html")
        globe_file = self.request.arguments["globe"][0]
        tornado.web.globe_.ServeGlobe(globe_file)
        self.servePortablePage(path)
      elif cmd == "confirmation_id":
        self.set_header("Content-Type", "text/plain")
        tornado.web.globe_.config_.confirmation_id_ = random.randint(1, 100000)
        self.write(str(tornado.web.globe_.config_.confirmation_id_))
      elif cmd == "is_broadcasting":
        self.set_header("Content-Type", "text/plain")
        if tornado.web.globe_.config_.disable_broadcasting_:
          self.write("'disabled'")
        else:
          if tornado.web.globe_.config_.accept_all_requests_:
            self.write("true")
          else:
            self.write("false")
      else:
        self.write("Unknown cmd.")
    except:
      self.write("Unexpected error: %s" % str(sys.exc_info()[0]))

  def verifyConfirmationId(self, arguments):
    if ("confirmation_id" in arguments and
        hasattr(tornado.web.globe_.config_, "confirmation_id_")):
      confirmation_id = tornado.web.globe_.config_.confirmation_id_
      # Confirmation_id_ should be used only once and then be reset to prevent
      # replay attack.
      tornado.web.globe_.config_.confirmation_id_ = 0
      return int(arguments["confirmation_id"][0]) == confirmation_id
    else:
      return False

  def servePortablePage(self, path):
    if self.IsValidRequest():
      if path == "list":
        self.WriteForm()
      else:
        self.WriteFile("preview/setup.html")
    else:
      self.write("Setup from localhost only.")

  def post(self, path):
    """Handles POST request for set-up."""
    if "cmd" not in self.request.arguments:
      self.servePortablePage(path)
      return

    cmd = self.request.arguments["cmd"][0]
    self.set_header("Content-Type", "text/html")
    print "cmd (post) : \"%s\"" % cmd
    if cmd == "quit":
      tornado.ioloop.IOLoop.instance().stop()
    elif cmd == "local_only":
      if self.verifyConfirmationId(self.request.arguments):
        tornado.web.globe_.config_.accept_all_requests_ = False
    elif cmd == "accept_all_requests":
      if self.verifyConfirmationId(self.request.arguments):
        tornado.web.globe_.config_.accept_all_requests_ = True
    else:
      self.write("Unknown cmd.")
