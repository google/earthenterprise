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


"""Checks for updates of the Earth plugin loader javascript.

If an update is available, it tries to load it into the local Earth
Server (/opt/google/gehttpd/...).
"""

import os
import smtplib
import stat
import sys
import time

# Earth plugin loader can be used from :
# googleclient/geo/earth_enterprise/src/javascript/earth_plugin_loader.js
google_earth_loader = (
    "/earthstablelocation/"
    "loader.js")

linux_loader = ("/opt/google/gehttpd/htdocs/js/earth_plugin_loader.js")

client_linux_loader = ("javascript/earth_plugin_loader.js")

client_portable_loader = (
    "fusion/portableglobe/servers/local/js/earth_plugin_loader.js")


class LoaderChecker(object):
  """Get copies of new loader if one is available."""

  def __init__(self, client_address):
    self.client_address_ = client_address

  def Log(self, msg):
    """Log message."""
    print msg

  def ExecuteCmd(self, os_cmd):
    """Execute os command and log results."""
    try:
      print "Executing %s" % os_cmd
      os.system(os_cmd)
    except Exception, e:
      self.Log("FAILED: %s" % e.__str__())
      raise IOError("Unable to execute: %s" % os_cmd)

  def SendMessage(self, body, email_address):
    """Mail message about loader update status."""
    subject = "Portable Earth Plugin Loader Update"
    msg = "From: %s\nSubject: %s\n\n%s" % (email_address,
                                           subject, body)
    smtp = smtplib.SMTP("localhost")
    smtp.sendmail(email_address, [email_address], msg)
    print "Mail sent."

  def LastModified(self, file_name):
    """Get last-modified time of file."""
    if not os.path.isfile(file_name):
      return "missing"

    return time.strftime(
        "%Y-%m-%d %H:%M:%S\n",
        time.localtime(os.stat(file_name)[stat.ST_MTIME]))

  def FileNeedsUpdate(self, file_name):
    """Check if file has been modified."""
    if not os.path.isfile(file_name):
      return True

    fp = os.popen("diff %s %s" % (google_earth_loader, file_name))
    same = fp.read().strip() == ""
    fp.close()
    return not same

  def Update(self, file_name):
    """Get current version of the loader for this file."""
    cmd = "cp %s %s" % (google_earth_loader, file_name)
    self.ExecuteCmd(cmd)
    cmd = "chmod a+r %s" % file_name
    self.ExecuteCmd(cmd)

  def UpdateForCheckIn(self, file_name):
    """G4 edit file and get current version of the loader for this file."""
    cwd = os.getcwd()
    os.chdir(self.client_address_)
    cmd = "g4 edit %s" % file_name
    self.ExecuteCmd(cmd)
    cmd = "cp %s %s" % (google_earth_loader, file_name)
    self.ExecuteCmd(cmd)
    cmd = "chmod a+r %s" % file_name
    self.ExecuteCmd(cmd)
    cwd = os.chdir(cwd)


def main(argv):
  num_args = len(argv) - 1
  if num_args > 2 or (num_args >= 1 and argv[1] == "help"):
    print """
Usage: update_loader.py [path_to_client] [email_addrss]

If no path to client is given, the current directory
is assumed to be the client path. If an email address is
given, then a messsage will be sent if there are any
updates.

Examples:
  ./fusion/portableglobe/tools/update_loader.py
  ./update_loader.py /usr/local/google/gee2/googleclient/geo/earth_enterprise/src username@email.com
"""
    return

  email_address = ""
  if num_args == 0:
    client_address = os.getcwd()
  else:
    client_address = argv[1]
    if num_args == 2:
      email_address = argv[2]

  print "Using client path: %s" % client_address

  print "Checking Earth plugin loader dates ..."
  loader = LoaderChecker(client_address)
  msg = ""

  if loader.FileNeedsUpdate(linux_loader):
    print "Updating %s ..." % linux_loader
    loader.Update(linux_loader)
    msg += "%s %s\n" % (linux_loader, loader.LastModified(linux_loader))
    msg += "Updated %s\n" % linux_loader
  else:
    print "No need to update %s." % linux_loader

  if loader.FileNeedsUpdate("%s/%s" % (client_address, client_linux_loader)):
    print "Updating %s ..." % client_linux_loader
    loader.UpdateForCheckIn(client_linux_loader)
    msg += "%s %s\n" % (client_linux_loader,
                        loader.LastModified(client_linux_loader))
    msg += "Updated %s\n" % client_linux_loader
    msg += "Need to check in the new loader.\n"
  else:
    print "No need to update %s." % client_linux_loader

  if loader.FileNeedsUpdate("%s/%s" % (client_address,
                                       client_portable_loader)):
    print "Updating %s ..." % client_portable_loader
    loader.UpdateForCheckIn(client_portable_loader)
    msg += "%s %s\n" % (client_portable_loader,
                        loader.LastModified(client_portable_loader))
    msg += "Updated %s\n" % client_portable_loader
    msg += "Need to check in the new loader.\n"
  else:
    print "No need to update %s." % client_portable_loader

  if msg and email_address:
    loader.SendMessage(msg, email_address)


if __name__ == "__main__":
  main(sys.argv)
