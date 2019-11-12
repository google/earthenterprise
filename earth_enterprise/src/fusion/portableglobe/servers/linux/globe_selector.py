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

# Starts the server for Windows. If a globe is specified,
# the server is started with that globe.

"""Starts the server for Linux."""

import os
import sys
import time
import urllib
import portable_config
import portable_server


def IsServerRunning(port):
  """Returns whether server is already running."""
  try:
    fp = urllib.urlopen("http://localhost:%s/ping" % port)
    fp.close()

  # sOk, if there isn't a server running.
  except:
    return False

  return True


def StopServer(port):
  """Stops server already running on the config port."""
  try:
    fp = urllib.urlopen("http://localhost:%s/setup?cmd=quit" % port)
    fp.close()

  except:
    print "Unable to stop server on port %s." % port

  print "Server stopped."


# Depends on sys.argv[1] being the globe name to start (if any)
def StartServer():
  """Starts server on the config port."""
  portable_server.main()


def main(argv):
  os.chdir(os.path.abspath(os.path.dirname(argv[0])))
  port = portable_config.PortableConfig().Port()
  # Note double clicking the start_server, will start the default globe in
  # config only when a server is not running already. Double clicking will have
  # no effect when already a server is running. To force start a server always
  # drag-and-drop a globe to start_server.
  if IsServerRunning(port):
    if len(argv) > 1:
      StopServer(port)
      StartServer()
  else:
    print "Server was not running."
    StartServer()

  # Give the server a chance to get started.
  time.sleep(1)

  cmd = "start local/home.url"
  print "Running %s" % cmd
  os.system(cmd)
  time.sleep(5)
  print "Done."

if __name__ == "__main__":
  main(sys.argv)
