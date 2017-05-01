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

"""A sample unit test for geecheck.

   Users can add their own customized tests to geecheck.  This sample
   test serves as a blueprint for new tests to be created.

   Tests should follow the naming convention *_test.py.
"""

fusion_is_installed = False

def helperFunction():
  """A helper function created to return a value to the test."""
  value = 10 > 0

  return value


class TestExample(unittest.TestCase):

    # The name of this method and its docstring are important.  They will
    # appear in the test results as the name and description of this test.
    # Method name should begin with "test", eg testNameOfTest().
    def testSampleInformation(self):
      """A sample test on which to base custom tests."""
      value = helperFunction()
      self.assertTrue(value)

    # To skip tests you must use a decorator. You can use one of skip(reason),
    # skipIf(condition, reason) and SkipUnless(condition, reason)
    #
    # see https://docs.python.org/2/library/unittest.html#skipping-tests-and-expected-failures
    # for more information.
    @unittest.skipUnless(fusion_is_installed, "Fusion is not installed")
    def testSampleWithCondition(self):
      """A sample test with condition on which to base custom tests."""
      self.assertTrue(False)


if __name__ == '__main__':
    unittest.main()
