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


"""Service for serving requests to /ext/... path.

The service defines three methods: SetWorkingDirectory,
ExtGetHandler, and ExtPostHandler.

SetWorkingDirectory sets the directory where this file is located and allows
the service to easily find external files that it may want to use to respond
to requests.

ExtGetHandler handles a GET request to the /ext/... pathway. Arguments for
the request are passed in via the handler object. The relative path
after "/ext/" is also passed in as a parameter.

ExtGetHandler handles a POST request to the /ext/... pathway. Arguments for
the request are passed in via the handler object. The relative path
after "/ext/" is also passed in as a parameter.
"""

import datetime


class ExtService(object):
  """Class for serving requests to /ext/... path."""

  def __init__(self):
    pass

  def SetWorkingDirectory(self, working_directory):
    """Use the given base directory to find any associated files."""
    # Set the location of our geplaces data.
    self.working_directory_ = working_directory
    print "Ext working directory is %s" % self.working_directory_

  def ExtGetHandler(self, handler, path):
    """Service a GET request to the /ext/... pathway."""
    handler.set_header("Content-Type", "text/html")
    handler.write("<p>Process GET request...")
    try:
      handler.write("<p>Hello %s!" % handler.request.arguments["name"][0])
    except KeyError:
      handler.write("<p>Hello unknown!")
    handler.write("<p>Path: %s" % path)

    handler.write("<p>Time: %s" % datetime.datetime.now().ctime())

  def ExtPostHandler(self, handler, path):
    """Service a POST request to the /ext/... pathway."""
    handler.set_header("Content-Type", "text/html")
    handler.write("<p>Process POST request...")
    try:
      handler.write("<p>Hello %s!" % handler.request.arguments["name"][0])
    except KeyError:
      handler.write("<p>Hello unknown!")
    handler.write("<p>Path: %s" % path)

    handler.write("<p>Time: %s" % datetime.datetime.now().ctime())
