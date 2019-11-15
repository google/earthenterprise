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


"""Supports interface for looking at and reviewing previously cut a globes."""

import os
from common import portable_exceptions
import common.globe_directory
import common.portable_globe
import common.utils
from core import globe_info


WEB_URL_BASE = "/cutter/globes"
BASE_DIR = "../htdocs"
GLOBE_DIR = "%s%s" % (BASE_DIR, WEB_URL_BASE)
# Off by default. Set to desired file if needed.
LOG_FILE = "/tmp/gee_globe_info_log"

# Template for json used to return info about globe being served.
GLOBE_INFO_OBJ_TEMPLATE = """{
  "name":"%s",
  "timestamp":"%s",
  "size":"%s",
  "size_in_bytes":%s,
  "description":"%s",
  "path":"%s",
  "is_gee":%s,
  "is_2d":%s,
  "is_3d":%s,
  "has_polygon":%s,
  "is_mercator":%s,
  "is_being_served":%s
}"""

# Template for json used to return info about bad globe file.
BAD_FILE_INFO_OBJ_TEMPLATE = """{
  "name":"%s",
  "size":"%s",
  "size_in_bytes":%s,
  "path":"%s",
  "is_gee":%s,
  "message":"%s"
}"""

TABLE_ENTRY_TEMPLATE = ("<tr>"
                        "  <td width='100px' align='right'>"
                        "    <a href='%%s'>[$SHORT_NAME]</a></td>"
                        "  <td>[$SIZE]</td>"
                        "  <td><i>[$TIMESTAMP]</i></td>"
                        "  <td>[$DESCRIPTION]</td>"
                        "</tr>")


def main():
  logger = common.utils.Log(LOG_FILE, False)
  try:
    logger.Log(os.environ["REQUEST_URI"])
    params = os.environ["REQUEST_URI"].split("/")
    cmd = params[3]
    server_name = os.environ["SCRIPT_URI"]
    server_name = server_name[:server_name.find("/cgi-bin")]

    try:
      globe_name = params[4]
      request = params[5].split("?")[0]
      arguments = os.environ["REQUEST_URI"].split("?")[1]
    except IndexError:
      request = ""

    if cmd == "serve":
      globe = common.portable_globe.Globe("%s/%s" % (GLOBE_DIR, globe_name))
      if globe.IsGee():
        server = globe_info.Server(globe, logger)
        logger.Log("Request: %s" % request)
        if request == "flatfile":
          server.FlatFileHandler(arguments)
        elif request == "dbRoot.v5":
          server.DbRootHandler()
        elif request == "query":
          server.QueryHandler(arguments)
        else:
          raise portable_exceptions.PortableException(
              "Unknown request: %s" % request)
      else:
        raise portable_exceptions.PortableException(
            "Corrupted glx: %s." % globe_name)

    elif cmd == "dbroot_info":
      common.utils.WriteHeader("application/json")
      globe = common.portable_globe.Globe("%s/%s" % (GLOBE_DIR, globe_name))
      print common.utils.GetDbrootInfoJson(globe, globe_name)

    elif cmd == "list_globe_dir":
      common.utils.WriteHeader("text/html")
      globe_dir = common.globe_directory.GlobeDirectory(GLOBE_DIR, True)
      content = common.utils.GlobesToText(
          globe_dir.globes_, TABLE_ENTRY_TEMPLATE, "name")
      print ("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN"
             "\"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>")
      print "<html><body>"
      print "<b>List of globes in</b> <i>%s</i>:" % GLOBE_DIR
      print "<table cellspacing=10>"
      print content
      print "</table>"
      print "</body></html>"

    elif cmd == "info":
      globe = common.portable_globe.Globe("%s/%s" % (GLOBE_DIR, globe_name))
      if globe.IsGee():
        content = globe.ReadFile("earth/info.txt")
        common.utils.WriteHeader("text/html")
        print ("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN"
               "\"\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd'>")
        print "<html><body><h2>Globe info for %s</h2><pre>" % globe_name
        print content
        print "</pre></body></html>"
      else:
        raise portable_exceptions.PortableException(
            "Corrupted glx: %s." % globe_name)

    elif cmd == "help":
      common.utils.WriteHeader("text/plain")
      print params
      print os.environ
      print params
      print "/".join(params[5:])

    elif cmd == "polygon":
      globe_dir = common.globe_directory.GlobeDirectory(GLOBE_DIR, True)
      polygon = globe_dir.globes_[globe_name]["globe"].Polygon()
      common.utils.WriteHeader("text/plain")
      if polygon.find("<kml") >= 0:
        print polygon

    elif cmd == "preview":
      common.utils.WriteHeader("text/html")
      if globe_name[-4:] == ".glb":
        params = common.utils.GlobeNameReplaceParams(globe_name)
        params["[$URL]"] = "%s/glc/%s" % (server_name, globe_name)
        common.utils.OutputFile(
            "%s/cutter/preview_globe.html" % BASE_DIR, params)
      elif globe_name[-4:] == ".glc":
        params = common.utils.GlobeNameReplaceParams(globe_name)
        params["[$URL]"] = "%s/glc/%s/a" % (server_name, globe_name)
        common.utils.OutputFile(
            "%s/cutter/preview_glc.html" % BASE_DIR, params)
      else:
        common.utils.OutputFile(
            "%s/cutter/preview_map.html" % BASE_DIR,
            common.utils.GlobeNameReplaceParams(globe_name))

    elif cmd == "globes":
      globe_dir = common.globe_directory.GlobeDirectory(GLOBE_DIR, True)

      common.utils.WriteHeader("text/plain")

      globe_info_list = []
      for globe_key in globe_dir.globes_.iterkeys():
        globe = globe_dir.globes_[globe_key]
        if globe["is_gee"]:
          globe_info_list.append(GLOBE_INFO_OBJ_TEMPLATE % (
              globe["name"], globe["timestamp"], globe["size"],
              globe["size_in_bytes"], globe["description"], globe["path"],
              common.utils.JsBoolString(globe["is_gee"]),
              common.utils.JsBoolString(globe["is_2d"]),
              common.utils.JsBoolString(globe["is_3d"]),
              common.utils.JsBoolString(globe["has_polygon"]),
              common.utils.JsBoolString(globe["is_mercator"]),
              common.utils.JsBoolString(globe["is_being_served"])))
        else:
          globe_info_list.append(BAD_FILE_INFO_OBJ_TEMPLATE % (
              globe["name"], globe["size"],
              globe["size_in_bytes"], globe["path"],
              common.utils.JsBoolString(globe["is_gee"]),
              globe["message"]))

      print "["
      print ",\n".join(globe_info_list)
      print "]"

    else:
      common.utils.WriteHeader("text/plain")
      logger.Log("Unknown command: %s" % cmd)
      print "Unknown command:", cmd

  except Exception, e:
    common.utils.WriteHeader("text/html")
    print type(e), e


if __name__ == "__main__":
  main()
