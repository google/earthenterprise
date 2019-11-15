#!/usr/bin/env python2.7
#
# Copyright 2019 OpenGEE Contributors
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

"""
   Abstraction script for retrieving system information so that other code and
   executables can have a platform-independent way of retreiving needed info.
   New and existing code should be made to pull information from here so that
   it is retrieved in a consistent manner with less code changes needed
   elsewhere.
"""

import sys
import os
import socket

def GetHostName(useFqdn=False):
  """Returns the host name, and with FQDN if specified."""
  thisHost = "localhost"
  
  if useFqdn:
    thisHost = socket.getfqdn()
    # If the server is configured correctly getfqdn() works fine, but it might
    # take what's in /etc/hosts first, so double check to see if
    # hostname is more appropriate to avoid using localhost or something else
  
    if thisHost == "localhost" or thisHost == "127.0.0.1" or thisHost == "127.0.1.1" or "." not in thisHost:
      thisHost = socket.gethostname()
    
  else:
    # hostname itself could be set to FQDN too, so it could be the same as above
    thisHost = socket.gethostname()

  return thisHost


def Usage(cmd):
  """Show proper usage for this command."""
  print "usage: %s [hostname [-f]]" % cmd
  exit(1)


def main(args):
  if len(args) < 2:
    Usage(args[0])

  if args[1] == "hostname":
    useFqdn = False
    if len(args) > 2:
      useFqdn = args[2] == "-f"
    print GetHostName(useFqdn)
  else:
    Usage(args[0])

if __name__ == "__main__":
  main(sys.argv)
