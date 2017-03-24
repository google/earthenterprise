#!/usr/bin/perl -w
#
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
# install_build_tools.pl installs the packages necessary for building
# Google Earth Enterprise Fusion. This is necessary to bootstrap a development
# or build machine for Fusion.
#
# This script works on RPM and Debian based systems with the following
# installed software:
#   RPM-based systems: yum, rpm
#   Debian-based systems: alien, apt-get, dpkg.
#
# This begins by uninstalling them first, then reinstalling the packages.
#
# TODO: A longterm goal is to free ourselves of the RPM
# package requirements and have a hermetic build out of the perforce repository.
#
# Usage example (note there are no options for this script):
# ./install_build_tools.pl
#
# See Readme.txt or the GEEnterprise wiki for more context on how/when to use
# this script.
#
require 5.6.0;

#"use strict" is incompatible with gLucid Perl

use bootstrap_utils;  # Our utilities for installing/uninstalling packages.

my $PACKAGE_PATH = "../";

# Determine the architecture of the current machine.
my $architecture = bootstrap_utils::get_architecture();

# STANDARD_PACKAGES :
#   Each list contains the [RPM_package_name, Debian_package_name]
# Empty items indicate they are not necessary for the build.
my @STANDARD_PACKAGES = (
  ['gdbm-devel', 'libgdbm-dev'],
  ['libtool', 'libtool'],
  ['glibc', 'libc6'],
  ['glibc-devel', 'libc6-dev'],
  ['zlib-devel', 'zlib1g-dev'],
  ['libcap-devel', 'libcap-dev'],
  ['libpng', 'libpng12-0'],
  ['libpng-devel', 'libpng12-dev'],
  ['libjpeg-devel', 'libjpeg62-dev'],
  ['freeglut-devel', 'freeglut3-dev'],
  ['rpm-build', ''],  # We don't do RPM builds on Debian machines
  ['flex', 'flex'],  # needed for QT
  ['libmng-devel', 'libmng-dev'],  # needed by QT
  ['xorg-x11-devel', 'libx11-dev'],  # needed by QT
  ['python-devel', 'python-dev'],  # needed by PostgresQL
  ['bison', 'bison'],  # needed by PostGIS and binutils
  ['doxygen', 'doxygen'], # needed for running doxygen
);

# List the packages in the order in which they need to be installed.
# These packages are needed for the build process:
#   source control, compilation, rpmbuild, scons etc.
my @TOOL_PACKAGES = (
  ['binutils-ge', '2.16.91.0.7-4', $architecture],
  ['gcc-ge-runtime', '4.1.1-4', $architecture],
  ['gcc-ge', '4.1.1-4', $architecture],
  ['gdb-ge', '6.5-3', $architecture],
  ['jdk-ge', '1.6.0-1', $architecture],
  ['ant-ge', '1.7.0-1', 'noarch'],
);

print("Installing Google Earth Enterprise Fusion Build Tools\n");
print("Installing " . @TOOL_PACKAGES . " packages for architecture ");
print($architecture . "\n");

# Print the final instructions for building the packages after successful
# installation of the build tools.
sub print_success() {
  my $home = $ENV{"HOME"};
  my $text = <<EOF;
The build tools are now installed for building Google Earth Enterprise Fusion.
Next you should either build_srpms.pl or install_third_party_rpms.pl
EOF

  print("$text\n");
}
# Check that packages are there.
bootstrap_utils::check_packages($PACKAGE_PATH, @TOOL_PACKAGES);

if (bootstrap_utils::is_ubuntu()) {
  print("Installing on Goobuntu $architecture system using apt-get and alien:");
  print("\n");
  my @debian_conflicts = (
    'openssl-devel', # openssl-devel conflicts with openldap-ge
    'freetype-devel' # conflicts with freetype2-devel
  );

  # Uninstall any conflicting packages
  bootstrap_utils::debian_cache_uninstall_packages(@debian_conflicts);

  # Must uninstall in reverse order...for dependency reasons.
  bootstrap_utils::debian_uninstall_packages(@TOOL_PACKAGES);

  # Install the Cached Debian packages.
  bootstrap_utils::debian_cache_install_packages(@STANDARD_PACKAGES);

  # Install the RPM packages after converting to Debian.
  bootstrap_utils::debian_install_packages($PACKAGE_PATH, @TOOL_PACKAGES);
} else {  # SLES/RHEL system
  print("Installing on SLES/RHEL $architecture system using Yum and RPM\n");

  my @yum_conflicts = (
    'openssl-devel', # openssl-devel conflicts with openldap-ge
    'freetype-devel' # conflicts with freetype2-devel
  );

  # Uninstall any conflicting packages
  bootstrap_utils::yum_uninstall_packages(@yum_conflicts);

  # Must uninstall in reverse order...for dependency reasons.
  bootstrap_utils::rpm_uninstall_packages(@TOOL_PACKAGES);

  # Install the YUM packages.
  bootstrap_utils::yum_install_packages(@STANDARD_PACKAGES);

  # Install the RPM packages.
  bootstrap_utils::rpm_install_packages($PACKAGE_PATH, @TOOL_PACKAGES);
}

print_success();


