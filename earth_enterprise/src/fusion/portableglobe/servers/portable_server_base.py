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


"""Base class for the handlers used in the portable server."""

import os
import sys
import traceback

import tornado.web

import portable_globe


class BaseHandler(tornado.web.RequestHandler):
  """Historically, the base class for local and remote servers.

  Remote servers are no longer supported.
  """

  def WriteFile(self, path):
    """Return a simple file as content."""
    # If local override is on, return the local file if it exists.
    if (tornado.web.globe_.config_.LocalOverride() and
        os.path.isfile("./local/%s" % path)):
      print "Using local file:", path
      return self.WriteLocalFile(path)

    # Write the file from the package.
    try:
      self.write(tornado.web.globe_.ReadFile(path))
      return True

    except portable_globe.UnableToFindException as e:
      return False

  def WriteLocalFile(self, path):
    """Return a simple local file as content."""
    path = path.replace("..", "__")

    # Write the file from the package.
    try:
      fp = open("./local/%s" % path, "rb")
      self.write(fp.read())
      fp.close()
      return True

    except portable_globe.UnableToFindException as e:
      print e.message
      return False

  def ShowUri(self, host):
    """Show the uri that was requested."""
    # Comment out next line to increase performance.
    if tornado.web.globe_.config_.Debug():
      print "Host: %s Request: %s" % (host, self.request.uri)

  def IsLocalhost(self):
    """Checks if request is from localhost."""
    host = self.request.headers["Host"]
    try:
      (server, server_port) = host.split(":")
    except:
      server = host

    # Accept localhost requests
    if server == "localhost" or server == "127.0.0.1":
      return True

    return False


  def IsValidLocalRequest(self):
    """Make sure that the request looks good before processing.

    Returns:
      Whether request should be processed.
    """
    host = self.request.headers["Host"]
    try:
      (caller_host, _) = host.split(":")
    except:
      caller_host = host

    # Accept all localhost requests
    if caller_host == "localhost" or caller_host == "127.0.0.1":
      self.ShowUri(host)
      return True

    return False

  def IsBroadcasting(self):
    return (not tornado.web.globe_.config_.disable_broadcasting_ and
            tornado.web.globe_.config_.accept_all_requests_)

  def IsValidRequest(self):
    """Makes sure that the request looks valid.

    Returns:
      Whether request should be processed.
    """
    return self.IsBroadcasting() or self.IsValidLocalRequest()


class LocalDocsHandler(BaseHandler):
  """Class for returning the content of files directly from disk."""

  def get(self, path):
    """Handle GET request for some local file.

    For example it is used for setup pages.

    Args:
      path: Path to file to be returned.
    """
    if not self.IsValidRequest():
      raise tornado.web.HTTPError(404)

    if path[-3:].lower() == "gif":
      self.set_header("Content-Type", "image/gif")
    elif path[-3:].lower() == "png":
      self.set_header("Content-Type", "image/png")
    elif path[-3:].lower() == "css":
      self.set_header("Content-Type", "text/css")
    else:
      self.set_header("Content-Type", "text/html")

    self.WriteLocalFile(path)


class ExtHandler(BaseHandler):
  """Class for passing control to externally defined routines."""

  def get(self, path):
    """Handle GET request for some external request.

    Args:
      path: Path relative to the ext directory.
    """
    try:
      tornado.web.local_server_.ext_service_.ExtGetHandler(self, path)
    except:
      if self.get_argument("debug", "") or tornado.web.globe_.config_.Debug():
        e = sys.exc_info()
        self.set_header("Content-Type", "text/html")
        self.write("<pre>%s</pre>" % "".join(
            traceback.format_exception(e[0], e[1], e[2])
        ))
      else:
        self.set_header("Content-Type", "text/html")
        self.write("GET service failed or is undefined.")

  def post(self, path):
    """Handle POST request for some external request.

    Args:
      path: Path relative to the ext directory.
    """
    try:
      tornado.web.local_server_.ext_service_.ExtPostHandler(self, path)
    except:
      if self.get_argument("debug", "") or tornado.web.globe_.config_.Debug():
        e = sys.exc_info()
        self.set_header("Content-Type", "text/html")
        self.write("<pre>%s</pre>" % "".join(
            traceback.format_exception(e[0], e[1], e[2])
        ))
      else:
        self.set_header("Content-Type", "text/html")
        self.write("POST service failed or is undefined.")
