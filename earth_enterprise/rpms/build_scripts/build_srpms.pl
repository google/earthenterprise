#!/usr/bin/perl -w
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
#
# build_srpms.pl builds and installs all 3rd party *-ge.rpm packages from .spec files
# These are necessary for building Google Earth Enterprise Fusion.
# At the end, we have
# packages/RPMS, packages/SRPMS

# deprecated - we do not use svn anymore
require 5; # just to be safe
use strict;

use bootstrap_utils;

my $package_path = "./packages";

# Determine the architecture of the current machine.
my $architecture = bootstrap_utils::get_architecture();

# List the packages in the order in which they need to be installed.
# The 3rd element of each array is 1 if the devel version of the package is
# required, otherwise 0.
my @packages = (
  ['openssl-ge', '0.9.8l-0', $architecture, 1],
  ['openldap-ge', '2.4.11-0', $architecture, 1],
  ['expat-ge', '2.0.1-0', $architecture, 1],
  ['qt-ge', '3.3.6-8', $architecture, 1],
  ['xerces-c-ge', '2.7.0-3', $architecture, 1],
  ['libcurl-ge', '7.19.3-0', $architecture, 1],
  ['libtiff-ge', '3.8.2-2', $architecture, 1],
  ['libgeotiff-ge', '1.2.3-4', $architecture, 1],
  ['ogdi-ge', '3.1.5-6', $architecture, 1],
  ['geos-ge', '2.2.3-0', $architecture, 1],
  ['gdal-ge', '1.6.1-0', $architecture, 1],
  ['libjs-ge', '2.0.0.20-0', $architecture, 1],
  ['libsgl-ge', '0.8-6', $architecture, 1],
  ['postgresql-ge', '8.1.8-4', $architecture, 1],
  ['postgis-ge', '1.1.7-1', $architecture, 0],
  ['apache-ge', '2.2.14-0', $architecture, 1],
  ['tomcat-ge', '6.0.20-1', $architecture, 0],
);

print "Building the 3rd Party RPMS for Google Earth Fusion Developer Machines\n";
print "Building and Installing " . @packages . " packages for architecture " . $architecture . "\n";

# Check that SPEC FILES are there.
bootstrap_utils::check_spec_files($package_path, @packages);

if (is_ubuntu() == 1) {
   print "Skipping install on Ubuntu systems\n";
   exit;
}

# Must uninstall in reverse order...for dependency reasons.
bootstrap_utils::rpm_uninstall_packages(@packages);


bootstrap_utils::rpm_build_and_install_packages($package_path, @packages);

# Print the final instructions for building the packages.
my $home = $ENV{"HOME"};

my $text = <<EOF;
\# You are ready to download the source and build Fusion:
\# Run the following command lines to download and kick it off.
cd $home/tree
svn co 'svn://hostname/r/legacy/gefusion/trunk/' -q
cd ../rpm
export PATH=/opt/google/bin:\$PATH
make all
EOF

print "$text\n";



