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

# packageInfo.py
# Return the package information for the current version of
# Google Earth Enterprise Fusion.
import sys
import os

class PackageInfo(object):
    """PackageInfo is a simple container for package information, including
    name, version, architecture and whether it's a 'required-to-ship' SRPM or
    not.

    Attributes:
        name: name of the package
        version: version number of this version of the package
        basic_version: the basic version (i.e., "3.1" where the version is
            "3.1.beta-20080626"  for Google packages needed
            to identify the InstallAnywhere directory for this package
        arch: architecture of the package (i686, x86_64, noarch)
        ship_srpm: True if the package has a src.rpm package that must be
            shipped with Fusion.
    """
    name = ""
    version = ""
    basic_version = ""
    arch = ""
    ship_srpm = ""

    def __init__ (self, name, basic_version, version, arch, ship_srpm):
        """Constructor"""
        self.name = name
        self.basic_version = basic_version
        self.version = version
        self.arch = arch
        self.ship_srpm = ship_srpm


    def RpmName (self, dir = ""):
        """Return the name of the RPM package. i.e., apache-2.0.0.i686.rpm
        dir: if specified, specifies the packages/RPMS/ directory and
            packages/RPMS/arch/ will be prepended to the RPM package file name.
        """
        filename = self.name + "-" + self.version + "." + self.arch + ".rpm"
        if dir == "":
            return filename
        return dir + self.arch + "/" + filename

    def SrpmName (self, dir = ""):
        """Return the name of the SRPM package. i.e., apache-2.0.0.src.rpm
        dir: is prepended to the SRPM filename if specified,
            e.g., typically specify the packages/SRPMS/ directory.
        """
        filename = self.name + "-" + self.version + ".src.rpm"
        return dir + filename

    def SrpmRequired (self):
        """Return True if the srpm for this package must be shipped with Fusion.
        """
        return self.ship_srpm

    def InstallAnywhereDir(self, dir):
        """InstallAnywhere requires the unpacked contents of the package to be
        placed in its lib directory.
        The naming is generally: "lib/arch/packagename-version.arch"
        the one caveat is for the Google packages:
           GoogleEarthFusionTutorial-3.1.beta-20080627.noarch
        would be placed in:
           lib/noarch/GoogleEarthFusionTutorial-3.1.noarch
        i.e., we only use the shipping version number in the InstallAnywhere
        installer."""
        ia_version = self.version
        if self.basic_version:
            ia_version = self.basic_version

        return (dir + self.arch + "/" + self.name + "-" +
                ia_version + "." + self.arch)
