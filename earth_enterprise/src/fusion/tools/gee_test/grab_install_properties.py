#!/usr/bin/python
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

"""Copies property file during IA install before it is deleted.

Copy this tool into installer directory and run while doing an install.
Generated file can then be used for silent installs with the IA installer.
"""

import os
import shutil
import sys

INSTALL_PROPERTIES_FILE = ".installer/installer.properties"


def Usage(argv):
  print "usage: %s <path_to_store_properties_file>" % argv[0]
  print "E.g. %s /tmp/serverupgradeinstall.properties" % argv[0]
  print
  print "Put this program in the same directory as the installers. "
  print "Then run it in the background before doing an install that "
  print "you want to save as an install prototype for future "
  print "silent installs."
  print "E.g. sudo ./InstallGEServer.sh serverupgradeinstall.properties"
  exit(1)


def main(argv):
  print "Looking for properties file ..."
  if len(argv) != 2:
    Usage(argv)

  output_path = argv[1]
  if os.path.exists(output_path):
    print "** Error **: %s already exists." % output_path
    exit(1)

  install_properties_found = False
  # Loop until properties file is found.
  while not install_properties_found:
    # Loop while properties file exists.
    while os.path.exists(INSTALL_PROPERTIES_FILE):
      shutil.copyfile(INSTALL_PROPERTIES_FILE, output_path)
      # Only indicate one copy to output, but keep copying to be safe in
      # case more is added to the file.
      if not install_properties_found:
        print "Copied %s to %s" % (INSTALL_PROPERTIES_FILE, output_path)
      install_properties_found = True

  print "Done."

if __name__ == "__main__":
  main(sys.argv)
