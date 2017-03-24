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
# install_third_party_rpms.pl installs all RPM packages necessary for building
# Google Earth Enterprise Fusion.
#
# Example Usage:
# To uninstall the packages (useful if trying to rebuild from scratch) use:
#    ./install_third_party_rpms.pl --uninstall
#
# To do a clean (which deletes the rpms so they can be rebuilt) use:
#    ./install_third_party_rpms.pl --clean
#
# To do a clean uninstall use:
#    ./install_third_party_rpms.pl --cleanuninstall
#
# See Readme.txt or the GEEnterprise wiki for more context on how/when to use
# this script.
#
require 5.6.0;
use strict;

use bootstrap_utils;

my $package_path = "../";

my $uninstall = 0;
$uninstall = 1 if @ARGV > 0 && $ARGV[0] =~ /uninstall/;
my $clean = 0;
$clean = 1 if @ARGV > 0 && $ARGV[0] =~ /clean/;


# Determine the architecture of the current machine.
my $architecture = bootstrap_utils::get_architecture();

# List the packages in the order in which they need to be installed.
my @PACKAGES = (
  ['openssl-ge', '0.9.8l-0', $architecture],
  ['openssl-ge-devel', '0.9.8l-0', $architecture],
  ['openldap-ge', '2.4.11-0', $architecture],
  ['openldap-ge-devel', '2.4.11-0', $architecture],
  ['expat-ge', '2.0.1-0', $architecture],
  ['expat-ge-devel', '2.0.1-0', $architecture],
  ['qt-ge', '3.3.6-8', $architecture],
  ['qt-ge-devel', '3.3.6-8', $architecture],
  ['xerces-c-ge', '3.0.1-0', $architecture],
  ['xerces-c-ge-devel', '3.0.1-0', $architecture],
  ['libcurl-ge', '7.19.3-0', $architecture],
  ['libcurl-ge-devel', '7.19.3-0', $architecture],
  ['libtiff-ge', '3.8.2-2', $architecture],
  ['libtiff-ge-devel', '3.8.2-2', $architecture],
  ['proj-ge', '4.5.0-0', $architecture],
  ['proj-ge-devel', '4.5.0-0', $architecture],
  ['libgeotiff-ge', '1.2.3-4', $architecture],
  ['libgeotiff-ge-devel', '1.2.3-4', $architecture],
  ['ogdi-ge', '3.1.5-6', $architecture],
  ['ogdi-ge-devel', '3.1.5-6', $architecture],
  ['geos-ge', '2.2.3-0', $architecture],
  ['geos-ge-devel', '2.2.3-0', $architecture],
  ['gdal-ge', '1.6.1-0', $architecture],
  ['gdal-ge-devel', '1.6.1-0', $architecture],
  ['libjs-ge', '2.0.0.20-0', $architecture],
  ['libjs-ge-devel', '2.0.0.20-0', $architecture],
  ['libsgl-ge', '0.8-6', $architecture],
  ['libsgl-ge-devel', '0.8-6', $architecture],
  ['postgresql-ge', '8.1.8-4', $architecture],
  ['postgresql-ge-devel', '8.1.8-4', $architecture],
  ['postgis-ge', '1.1.7-1', $architecture],
  ['apache-ge', '2.2.14-0', $architecture],
  ['apache-ge-devel', '2.2.14-0', $architecture],
  ['tomcat-ge', '6.0.20-1', $architecture],
);

if ($uninstall || $clean) {
  print("Uninstalling and cleaning ") if $uninstall && $clean;
  print("Uninstalling ") if $uninstall && !$clean;
  print("Cleaning ") if !$uninstall && $clean;
  print(@PACKAGES . " packages for architecture " . $architecture . "\n");
} else {
  print("Installing the 3rd Party Google Earth Enterprise Developer packages ");
  print("needed for Fusion Builds:\n");
  print("Installing " . @PACKAGES . " packages for architecture ");
  print($architecture . "\n");
}

# Print the final instructions for building the packages.
sub print_success() {
  my $home = $ENV{"HOME"};

  my $text = <<EOF;
\# The 3rd party developer RPMS are now installed so you may build Fusion.
cd ../rpm
export PATH=/opt/google/bin:\$PATH
make all
EOF

  print("$text\n");
}

# Check that packages are there.
bootstrap_utils::check_packages($package_path, @PACKAGES);

if (bootstrap_utils::is_ubuntu()) {
  print("Installing on Goobuntu $architecture system using alien\n");
  # Must uninstall in reverse order...for dependency reasons.
  bootstrap_utils::debian_uninstall_packages(@PACKAGES);

  # No debian packages are there to clean, they are only temporarily created
  # by alien and then removed after install.
  # It's only useful to clean on RPM build machines.
  exit if $uninstall || $clean;

  # Install the Debian packages.
  bootstrap_utils::debian_install_packages($package_path, @PACKAGES);
} else {  # SLES/RHEL system
  print("Installing on SLES/RHEL $architecture system using RPM\n");

  # Must uninstall in reverse order...for dependency reasons.
  bootstrap_utils::rpm_uninstall_packages(@PACKAGES);

  bootstrap_utils::rpm_clean_packages($package_path, @PACKAGES) if $clean;
  exit if $uninstall || $clean;

  # Install the RPM packages.
  bootstrap_utils::rpm_install_packages($package_path, @PACKAGES);
}

print_success();


