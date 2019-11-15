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

# Need to use unittest2 for Python 2.6.
try:
  import unittest2 as unittest
except ImportError:
  import unittest

from geecheck_tests import common


class TestOS(unittest.TestCase):

  def testSupportability(self):
    """Check OS distribution and release is supported."""
    platform = common.GetOSPlatform()
    distro = common.GetOSDistro()
    release = common.GetOSRelease()
    architecture = common.GetLinuxArchitecture()

    error_msg = 'A Linux platform is required. Current platform: %s' % platform
    self.assertEqual(platform, 'linux', msg=error_msg)

    error_msg = ('This is a %s architecture. A 64-bit architecture is required.'
                 % architecture)
    self.assertEqual(architecture, '64bit', msg=error_msg)

    error_msg = ('The Linux distribution "%s" is not supported. These are the '
                 'supported distributions: %s.'
                 % (distro, ', '.join(common.SUPPORTED_OS_LIST.keys())))
    self.assertIn(distro, common.SUPPORTED_OS_LIST, msg=error_msg)

    if not(release >= common.SUPPORTED_OS_LIST[distro]['min_release'] and
           release <= common.SUPPORTED_OS_LIST[distro]['max_release']):
      AssertionError('%s version %s  is not supported. Only %s versions %s to '
                     '%s are supported.' %
                     (distro,
                      release,
                      distro,
                      common.SUPPORTED_OS_LIST[distro]['min_release'],
                      common.SUPPORTED_OS_LIST[distro]['max_release']))


if __name__ == '__main__':
  unittest.main()
