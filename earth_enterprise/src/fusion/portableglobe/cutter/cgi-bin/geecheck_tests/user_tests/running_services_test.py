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


class TestRunningServices(unittest.TestCase):

  def setUp(self):
    self.processes = common.GetProcessesDetails()

  def AssertServiceIsRunning(self, service):
    exists = False
    pid = 'Undefined'
    for pid in self.processes:
      if self.processes[pid] and service in self.processes[pid]['cmdline']:
        exists = True
        break
    error_msg = 'Service: %s is NOT running' % service
    self.assertTrue(exists, error_msg)
    print 'Service %s is running with pid %s.' % (service, pid)

  @unittest.skipUnless(common.IsFusionInstalled(), 'Fusion is not installed')
  def testRunningServiceGesystemmanager(self):
    self.AssertServiceIsRunning('gesystemmanager')

  @unittest.skipUnless(common.IsFusionInstalled(), 'Fusion is not installed')
  def testRunningServiceGeresourceprovider(self):
    self.AssertServiceIsRunning('geresourceprovider')

  @unittest.skipUnless(common.IsGeeServerInstalled(), 'GEE Server is not installed')
  def testRunningServiceGehttpd(self):
    self.AssertServiceIsRunning('gehttpd')

  @unittest.skipUnless(common.IsGeeServerInstalled(), 'GEE Server is not installed')
  def testRunningServicepostgres(self):
    self.AssertServiceIsRunning('/opt/google/bin/postgres')


if __name__ == '__main__':
  unittest.main()
