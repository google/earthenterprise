#!/usr/bin/env python3.8
#
# Copyright 2021 the Open GEE Contributors
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

import sys
import unittest
from geecheck_tests import common


class TestPython(unittest.TestCase):

  def testPython3Version(self):
    '''Verify that OpenGEE Server is running Python 3.8'''
    self.assertEqual(sys.version_info[:2], (3, 8), msg="Incorrect Python version: Version %s found, but version 3.8.x is required." % sys.version)


if __name__ == '__main__':
  unittest.main()
