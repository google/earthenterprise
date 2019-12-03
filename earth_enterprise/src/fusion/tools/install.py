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
# Installs files from a tarball into Google Fusion and/or
# Earth Builder. Uses filelist.txt to determine files
# to installed and their destinations. The format is one
# file per line:
#  path_to_source absolute_path_to_dest replace_or_new permission owner
#
# E.g.
#  google/bin/myfile /opt/google/bin/myfile replace 755 root:root

import os
import shutil
import sys

FILELIST = "filelist.txt"
# Types of file installs
REPLACE = "replace"
NEW = "new"


def Install(file_list, is_dry_run, is_reverse):
  """Install files specified by the given filelist.

  If it is a dry run, only show the copies that would
  have been done without actually doing them.

  The reverse option lets you gather the files to install from
  a working system using the same filelist.txt.

  Args:
    file_list: Iterable list of file replacement information.
    is_dry_run: Whether we should print but not execute file copies.
    is_reverse: Whether we should move from destination to source.
  """
  for line in file_list:
    line = line.strip()
    if not line or line[0] == "#":
      pass
    else:
      if is_reverse:
        (destination, source, install_type,
         permission, owner) = line.split(" ")
      else:
        (source, destination, install_type,
         permission, owner) = line.split(" ")
      if install_type.lower() == NEW and os.path.exists(destination):
        print "Warning: %s already exists." % destination

      if install_type.lower() == REPLACE and not os.path.exists(destination):
        print "Warning: %s does not exist." % destination

      if is_dry_run:
        print "cp %s %s" % (source, destination)
        print "chmod %s %s" % (permission, destination)
        print "chown %s %s" % (owner, destination)
      else:
        if install_type == REPLACE:
          os.system("chmod +w %s" % destination)
        shutil.copyfile(source, destination)
        os.system("chmod %s %s" % (permission, destination))
        os.system("chown %s %s" % (owner, destination))


def Usage():
  print "Usage: %s [--dryrun]" % sys.argv[0]
  sys.exit(0)


def main():
  if len(sys.argv) > 3:
    print "Error: Too many arguments."
    Usage()

  is_dry_run = False
  is_reverse = False
  if len(sys.argv) == 2:
    if sys.argv[1] == "--dryrun":
      is_dry_run = True
    elif sys.argv[1] == "--reverse":
      is_reverse = True
    else:
      print "Error: Unknown argument %s." % sys.argv[1]
      Usage()

  try:
    fp = open(FILELIST)
  except IOError:
    print "Error: Unable to open filelist.txt."
    print "Please run in the extracted directory."
    return

  Install(fp, is_dry_run, is_reverse)
  fp.close()

if __name__ == "__main__":
  main()
