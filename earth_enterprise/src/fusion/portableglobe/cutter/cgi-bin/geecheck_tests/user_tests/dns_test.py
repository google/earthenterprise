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


class TestDns(unittest.TestCase):

    def testDns(self):
      """Test DNS to ensure hostname is correct for IP Address. """
      self.hostname, self.ip, self.host_check_list = common.GetHostInfo()
      self.assertTrue(self.hostname in self.host_check_list)

if __name__ == '__main__':
    unittest.main()
