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


import os
import xml.etree.ElementTree as ET
from geecheck_tests import common

# Need to use unittest2 for Python 2.6.
try:
  import unittest2 as unittest
except ImportError:
  import unittest



def getDiskFreeSpace():
  """Returns free space on disk represented as percent of total available."""
  tree = ET.parse('/etc/opt/google/systemrc')
  root = tree.getroot()

  sys_rc = {}
  for child in root:
    sys_rc[child.tag] = child.text

  asset_root = sys_rc["assetroot"];
  mount_point = getMountPoint(asset_root)

  available_space, size = getFsInfo(mount_point)
  percentage_avail = 100 - ((size - available_space) * 100 / size)

  return percentage_avail


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


def getFsInfo(pathname):
  """Get the free space and total size of the filesystem containing pathname."""
  statvfs = os.statvfs(pathname)
  # Size of filesystem in bytes
  size = statvfs.f_frsize * statvfs.f_blocks
  # Number of free bytes that ordinary users are allowed to use.
  avail = statvfs.f_frsize * statvfs.f_bavail

  return avail, size


class TestDiskSpace(unittest.TestCase):

    @unittest.skipUnless(common.IsFusionInstalled(), 'Fusion is not installed')
    def testAdequateDiskSpace(self):
      """Check that the remaining disk space is at least 20%."""
      self.assertLessEqual(20, getDiskFreeSpace())

if __name__ == '__main__':
    unittest.main()
