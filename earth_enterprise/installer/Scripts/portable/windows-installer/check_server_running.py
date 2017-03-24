#!/usr/bin/python2.4
#
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

"""This Script will prompt the user if portable server is running."""

import os
import sys
import urllib
import portable_config
import Tkinter
import tkMessageBox


def IsServerRunning(port):
  """Returns whether server is already running."""
  try:
    fp = urllib.urlopen('http://localhost:%s/ping'% port)
    fp.close()
  # sOk, if there isn't a server running.
  except:
    return False
  return True


def main():
  """Display Error Message, if the server is running."""
  port = portable_config.PortableConfig().Port()
  root = Tkinter.Tk()
  root.title('Uninstall portable server')
  root.withdraw()
  uninstaller_flag = os.getenv('GEE_PORTABLE_SILENT_UNINSTALLER_DELETE_DATA')
  unistall_msg = ('Portable Server is Running. \n'
                  'Please Stop the server for Uninstallation.')
  if IsServerRunning(port):
    if uninstaller_flag is None:
      tkMessageBox.showerror('Uninstallation Failed!', '%s'% unistall_msg)
    else:
      print 'Uninstallation Failed!\n%s '% unistall_msg
    sys.exit(1)

if __name__ == '__main__':
  main()
