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

#
#
# Starts the server for Mac. If a globe is specified,
# the server is started with that globe.

"""Starts the server in the Mac OS."""

import os
import sys
import time
import urllib
import portable_config


def IsServerRunning(port):
  """Returns whether server is already running."""
  try:
    fp = urllib.urlopen("http://localhost:%s/ping" % port)
    fp.close()

  #Ok, if there isn't a server running.
  except:
    return False

  return True


def StopServer(port):
  """Stops server already running on the config port."""
  try:
    fp = urllib.urlopen("http://localhost:%s/setup?cmd=quit" % port)
    fp.close()

  except:
    pass


def StartServer(globe=""):
  """Starts server on the config port."""
  globe = globe.replace("'", "\\'")
  cmd = ("../../../portable_server.app/Contents/MacOS/portable_server '%s' &" %
         globe)
  print "Running %s" % cmd
  os.system(cmd)


def main(argv):
  port = portable_config.PortableConfig().Port()
  if IsServerRunning(port):
    StopServer(port)

  if len(argv) > 1:
    StartServer(argv[1])
  else:
    StartServer()

  # Give the server a chance to get started.
  time.sleep(2)

  cmd = "open http://localhost:%s" % port
  print "Running %s" % cmd
  os.system(cmd)
  print "Done."

if __name__ == "__main__":
  main(sys.argv)
