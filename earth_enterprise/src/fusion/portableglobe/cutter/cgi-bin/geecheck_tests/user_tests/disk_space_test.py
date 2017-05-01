#!/usr/bin/env python
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


import os
import unittest
import xml.etree.ElementTree as ET


def getDiskInfo():
  """Returns disk usage represented as percent of total available."""
  tree = ET.parse('/etc/opt/google/systemrc')
  root = tree.getroot()

  sys_rc = {}
  for child in root:
    sys_rc[child.tag] = child.text

  asset_root = sys_rc["assetroot"];
  mount_point = getMountPoint(asset_root)

  available_space, size = getFsFreespace(mount_point)
  percentage_used = (size - available_space) * 100 / size

  return percentage_used


def getMountPoint(pathname):
  """Get the mount point of the filesystem containing pathname."""
  pathname = os.path.normcase(os.path.realpath(pathname))
  parent_device = path_device = os.stat(pathname).st_dev
  while parent_device == path_device:
    mount_point = pathname
    pathname = os.path.dirname(pathname)
    if pathname == mount_point:
      break
    return mount_point


def getFsFreespace(pathname):
  """Get the free space of the filesystem containing pathname."""
  statvfs = os.statvfs(pathname)
  # Size of filesystem in bytes
  size = statvfs.f_frsize * statvfs.f_blocks
  # Number of free bytes that ordinary users are allowed to use.
  avail = statvfs.f_frsize * statvfs.f_bavail

  return avail, size


class TestDiskSpace(unittest.TestCase):

    def testAdequateDiskSpace(self):
      """Check that the remaining disk space is at least 20%."""
      self.assertLessEqual(20, getDiskInfo())

if __name__ == '__main__':
    unittest.main()
