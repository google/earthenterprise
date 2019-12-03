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


"""Classes for emulating a portable server."""

from common import portable_exceptions
import common.globe_directory
import common.portable_globe
import common.utils


class Server(object):
  """Class for emulating a portable server."""

  def __init__(self, globe, logger):
    self.globe_ = globe
    self.logger_ = logger

  def _FlatFileRequest(self, request):
    """Reads requested packet from glb file."""
    arguments = request.split("-")
    tag = arguments[0]
    qtnode = arguments[1]
    request_type = arguments[2][0]
    version = arguments[2][2:]

    if tag == "q2" and request_type == "q":
      # Read quadtree packet from globe package.
      content = self.globe_.ReadQtPacket(qtnode)
      common.utils.WriteHeader("application/octet-stream")

    elif (tag == "f1c" or tag == "f1") and request_type == "i":
      # Read imagery packet from globe package.
      content = self.globe_.ReadImageryPacket(qtnode)
      common.utils.WriteHeader("application/octet-stream")

    elif (tag == "f1c" or tag == "f1") and request_type == "t":
      # Read terrain packet from globe package.
      content = self.globe_.ReadTerrainPacket(qtnode)
      common.utils.WriteHeader("application/octet-stream")

    elif (tag == "f1c" or tag == "f1") and request_type == "d":
      # Read vector packet.
      channel = int(version.split(".")[0])
      # Read terrain packet from globe package.
      content = self.globe_.ReadVectorPacket(qtnode, channel)
      common.utils.WriteHeader("application/octet-stream")

    elif tag == "lf":
      # Read icon packet.
      icon_args = arguments[2].split("&")
      content = self.globe_.ReadFile(icon_args[0])
      common.utils.WriteHeader("image/png")

    else:
      raise portable_exceptions.PortableException(
          "unknown packet request: %s" % self.request.uri)

    print content

  def FlatFileHandler(self, request):
    """Handles a flat file request."""
    self._FlatFileRequest(request)

  def _QueryRequest(self, request):
    """Processes query request."""
    arguments = request.split("&")
    args = {}
    for argument in arguments:
      pair = argument.split("=")
      args[pair[0]] = pair[1]

    if args["request"] == "Json":
      common.utils.WriteHeader("text/plain")
      if self.globe_.Is2d():
        print self.globe_.ReadFile("maps/map.json")
      else:
        print self.globe_.ReadFile("earth/earth.json")

    elif args["request"] == "ImageryMaps":
      common.utils.WriteHeader("image/jpg")
      qtpath = common.utils.ConvertToQtNode(int(args["z"]), int(args["x"]),
                                            int(args["y"]))
      print self.globe_.ReadMapImageryPacket(qtpath, int(args["channel"]))

    elif args["request"] == "VectorMapsRaster":
      common.utils.WriteHeader("image/png")
      qtpath = common.utils.ConvertToQtNode(int(args["level"]),
                                            int(args["col"]), int(args["row"]))
      print self.globe_.ReadMapVectorPacket(qtpath, int(args["channel"]))

    else:
      pass

  def QueryHandler(self, request):
    """Handles query requests."""
    self._QueryRequest(request)

  def DbRootHandler(self):
    """Handles request for the dbroot."""

    # Read dbRoot from glb file.
    content = self.globe_.ReadDbRoot()
    common.utils.WriteHeader("application/octet-stream")
    print content


def main():
  pass

if __name__ == "__main__":
  main()
