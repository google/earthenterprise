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

# A set of utilities for determining whether DEB packages are installed at the
# desired version.
import sys
import os
import re

# Generic Utilities
def FileContains(filename, regex_pattern):
   """Generic utility for testing whether a pattern is found in a file."""
   try:
      f = file(filename, "r")

   except :
      return False
   for line in f:   ## iterates over the whole file
      if regex_pattern.search(line):
         return True
   f.close()
   return False


def AssertPackageVersion(package_name, min_version):
   """Exit if the specified package is not installed at a
   version greater than or equal to the min_version"""
   if IsPackageVersionSufficient(package_name, min_version) == False:
       print("BUILD ERROR: %s is not at required version %s" % (package_name, min_version))
       sys.exit(1)


def IsMinOrGreaterVersion(version_info, min_version):
    """Generic check of version numbers of the form 4.1.2.3-0.9.
    This assumes no alphabetical characters in the version string."""
    if len(version_info) <= 1:
        return False
    current_version = version_info[0]
    is_installed = version_info[1]
    if is_installed == False:
        return False

    current_version_list = VersionStringToList(current_version)
    min_version_list = VersionStringToList(min_version)
    return IntListCompare(current_version_list, min_version_list) >= 0

def IntListCompare (version_a, version_b):
    """Compare two integer lists returning:
    -1 if a < b, 0 if a==b and 1 if a > b"""
    max_index = max(len(version_a), len(version_b))
    for i in xrange(max_index):
        current_value = 0
        if i < len(version_a): current_value = version_a[i]
        min_value = 0
        if i < len(version_b): min_value = version_b[i]
        if current_value < min_value:
            return -1
        elif current_value > min_value:
            return 1
    return 0

def VersionStringToList (version_string, delimiters = ".-"):
   """Return a list of version numbers from a given version string
   Given '4.1.2.3-0' return [4, 1, 2, 3, 0]"""

   for i in xrange(len(delimiters)):
     version_string = version_string.replace(delimiters[i], ".")
   version_string_list = version_string.split(".")
   return [int(n) for n in version_string_list]

# System specific utilities

def UsesRPM():
   """Simply checks for Ubuntu in lsb-release, otherwise UsesRPM is true"""
   if IsSuSEDistro():
        return True
   return FileContains('/etc/lsb-release', re.compile(r"Ubuntu")) == False

def IsSuSEDistro():
   return (os.path.exists('/etc/SuSE-release') or
           os.path.exists('/etc/UnitedLinux-release'))

def IsSLES8():
   return os.path.exists('/etc/UnitedLinux-release')

def IsPackageVersionSufficient(package_name, min_version):
   """Return true if the specified package is installed at a
   version greater than or equal to the min_version"""
   if UsesRPM():
      return IsMinOrGreaterVersion(GetRPMPackageInfo(package_name), min_version)
   else:
      return IsMinOrGreaterVersion(GetDEBPackageInfo(package_name), min_version)

def GetRPMPackageInfo (package_name):
   """Return a list of:
   [version string, bool isInstalled"""
   pattern = re.compile(package_name)

   f = os.popen("rpm -q " + package_name);
   for line in f:
     if line.startswith(package_name):
       start = len(package_name) + 1 # increase by 1 to catch the extra "-"
       version_number = line[start:]
       f.close();
       return [version_number, True]
   f.close();
   return ["0", False]

def GetDEBPackageInfo (package_name):
   """Return a list of:
   [version string, bool isInstalled"""
   pattern = re.compile(package_name)
   """Set number of columns for dpkg -l, so it won't truncate the names of
   packages.  Increase if longer package names get added"""
   os.environ["COLUMNS"]="100"
   f = os.popen("dpkg -l " + package_name);
   for line in f:
     if pattern.search(line):
       items_list = line.split(" ")
       items_list = [item for item in items_list if item != '']
       if (package_name != items_list[1]):
          continue
       f.close();
       return [items_list[2], items_list[0] == "ii"]
   f.close();
   return ["0", False]
