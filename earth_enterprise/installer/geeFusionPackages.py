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

# geeFusionPackages.py
# Return the package information for the current version of
# Google Earth Enterprise Fusion, including name, versions and srpm info.
from packageInfo import PackageInfo

def GetNoArchPackageInfo ():
    """Return the list of package info records for the NOARCH packages.
    Each entry in the package info list contains a flag indicating whether it
    is a SRPM that needs to be shipped.

    returns: a list of PackageInfo objects describing the packages.
    """
    return [
        PackageInfo("ant-ge", "", "1.7.0-1",      "noarch", False),
        ];

def GetPackageInfo (arch):
    """Return the list of package info records for the given architecture.
    Each entry in the package info list contains a flag indicating whether it
    is a SRPM that needs to be shipped.

    arch: the architecture for the packages. one of {i686, x86_64}
    returns: a list of PackageInfo objects describing the packages.
    """
    return [
        PackageInfo("binutils-ge"   , "", "2.16.91.0.7-4", arch, False),
        PackageInfo("gcc-ge"        , "", "4.1.1-4",       arch, True), # SRPM
        PackageInfo("gcc-ge-runtime", "", "4.1.1-4",       arch, False),
        PackageInfo("gdb-ge"        , "", "6.5-3",         arch, False),
        PackageInfo("expat-ge"      , "", "2.0.1-0",       arch, True), # SRPM
        PackageInfo("qt-ge"         , "", "3.3.6-8",       arch, False),
        PackageInfo("xerces-c-ge"   , "", "3.0.1-0",       arch, True), # SRPM
        PackageInfo("libtiff-ge"    , "", "3.8.2-2",       arch, True), # SRPM
        PackageInfo("libgeotiff-ge" , "", "1.2.3-4",       arch, True), # SRPM
        PackageInfo("ogdi-ge"       , "", "3.1.5-6",       arch, True), # SRPM
        PackageInfo("gdal-ge"       , "", "1.6.1-0",       arch, True), # SRPM
        PackageInfo("libcurl-ge"    , "", "7.19.3-0",      arch, True), # SRPM
        PackageInfo("libjs-ge"      , "", "2.0.0.20-0",    arch, True), # SRPM
        PackageInfo("libsgl-ge"     , "", "0.8-6",         arch, False),
        PackageInfo("openldap-ge"   , "", "2.4.11-0",      arch, True), # SRPM
        PackageInfo("openssl-ge"    , "", "0.9.8l-0",      arch, True), # SRPM
        PackageInfo("apache-ge"     , "", "2.2.14-0",       arch, True), # SRPM
        # Apache-ge-devel is needed as part of the module developer option for
        # GEE Server installs (i.e., if the user wants to build additional
        # modules.
        PackageInfo("apache-ge-devel","", "2.2.14-0",       arch, False),
        PackageInfo("tomcat-ge"     , "", "6.0.20-1",      arch, True), # SRPM
        PackageInfo("jdk-ge"        , "", "1.6.0-1",       arch, False),
        PackageInfo("geos-ge"       , "", "2.2.3-0",       arch, True), # SRPM
        PackageInfo("postgresql-ge" , "", "8.1.8-4",       arch, True), # SRPM
        PackageInfo("postgis-ge"    , "", "1.1.7-1",       arch, True), # SRPM
     ];
