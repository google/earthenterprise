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


class TestVersion(unittest.TestCase):

  @unittest.skipUnless(common.IsFusionInstalled(), 'Fusion is not installed')
  def testFusionVersion(self):
    """Check if Fusion release is the latest available."""

    latest_version = common.GetLatestVersion()
    # Extract the main version information.
    fusion_version = common.GetFusionVersion()
    fusion_version = ".".join(fusion_version.split('.')[0:3])

    error_msg = ('Running Fusion version %s. Upgrade to version %s.' %
                 (fusion_version, latest_version))
    self.assertEqual(fusion_version, latest_version, msg=error_msg)

    print ('Currently running the latest version of Fusion (%s).' %
           fusion_version)

  @unittest.skipUnless(common.IsGeeServerInstalled(),
                       'GEE Server is not installed')
  def testGeeServerVersion(self):
    """Check if GEE Server release is the latest available."""

    latest_version = common.GetLatestVersion()
    # Extract the main version information.
    gee_server_version = common.GetGeeServerVersion()
    gee_server_version = ".".join(gee_server_version.split('.')[0:3])

    error_msg = ('Running GEE Server version %s. Upgrade to (%s).' %
                 (gee_server_version, latest_version))
    self.assertEqual(gee_server_version, latest_version, msg=error_msg)

    print ('Currently running the latest version of GEE Server (%s).' %
           gee_server_version)

  @unittest.skipUnless(common.IsFusionInstalled(), 'Fusion is not installed')
  @unittest.skipUnless(common.IsGeeServerInstalled(),
                       'GEE Server is not installed')
  def testFusionVersionsMatch(self):
    """Check Fusion and server versions are aligned."""
    fusion_version = common.GetFusionVersion()
    gee_server_version = common.GetGeeServerVersion()

    error_msg = ('Fusion and GEE Server versions DO NOT match. '
                 'Currently running Fusion v. %s and GEE Server v. %s.' %
                 (fusion_version, gee_server_version))
    self.assertEqual(fusion_version, gee_server_version, msg=error_msg)

    print 'Fusion and GEE Server versions match. Current version is %s.' % (
        fusion_version)


if __name__ == '__main__':
  unittest.main()
