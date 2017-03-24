#-*- Python -*-
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

import sys
import os
import re
from packageUtils import IsPackageVersionSufficient
from packageUtils import UsesRPM
from packageUtils import FileContains
from packageUtils import GetDEBPackageInfo

failure_list = []
# test FileContains

test_file = '/etc/profile'
if FileContains('/IDontExist', re.compile(r"a")):
  failure_list.append("FileContains Failed: returned true for non-existing file")
if FileContains(test_file, re.compile(r"PROFILE")) == False:
  failure_list.append("FileContains Failed: did not find PROFILE in /etc/hostname")
if FileContains(test_file, re.compile(r"not anything here")):
    failure_list.append("FileContains Failed: found garbage search string in /etc/hostname")

# test UsesRPM
print("Basic checks for Ubuntu vs RPM\nMake sure these coincide with your current system.\n\n")

uses_rpm = "does not use RPM"
if UsesRPM():
  uses_rpm = "uses RPM"
print("This machine %s" % uses_rpm)

# test GetDEBPackageInfo for non-RPM systems
if UsesRPM() == False:
   package_name = "gdal-ge"
   package_results = GetDEBPackageInfo (package_name)
   if len(package_results) != 2 | package_results[1] == False:
      failure_list.append("%s not installed: GetDEBPackageInfo returns %s" %
         (package_name, package_results))

# test Package check
valid_test_packages = [['apache-ge-devel', '2.2.2'],
                  ['apache-ge-devel', '2.2.2.1'],
                  ['jdk-ge', '1.6.0-1'],
                  ['jdk-ge', '1.6.0-0']];
invalid_test_packages = [['apache-ge-devel9', '2.2.2'],
                  ['apache-ge-devel', '10.2.2.1'],
                  ['j9dk-ge', '1.6.0-1'],
                  ['jdk-ge', '1.99.0-0']];

for package_list in valid_test_packages:
    if IsPackageVersionSufficient(package_list[0], package_list[1]) == False:
        failure_list.append("Failed test that should pass: %s" % (package_list))


print("Test is now looking for invalid packages (error messages expected until tests are complete).\n\n")

for package_list in invalid_test_packages:
    if IsPackageVersionSufficient(package_list[0], package_list[1]):
        failure_list.append("Passed test that should fail: %s" % (package_list))
print("\n\nTests complete.\n\n")

if len(failure_list) > 0:
   print("\n\n%s TEST FAILURES" % len(failure_list))
   for s in failure_list:
      print(s)
else:
   print("\n\nSUCCESS: All tests succeeded!")
