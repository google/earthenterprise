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
# Utilities to check the presence of installed packages from
# a master list (stored in a CSV) file.
#
# check_package_versions
# is the main call that runs through the CSV file and checks
# the installation status of each package.

package PackageUtils;

use strict;
use warnings;
use FileUtils;

# module constructor
BEGIN {
    use Exporter   ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

    # set the version for version checking
    $VERSION     = 1.00;
    @ISA         = qw(Exporter);
    @EXPORT      = qw(&check_package_versions
                      &get_package_version);
    %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],

    # your exported package globals go here,
    # as well as any optionally exported functions
    @EXPORT_OK   = qw();
}
our @EXPORT_OK;

# Return 1 if the current platform supports RPMs,
# 0 if it supports DEBs (ubuntu only).
my $uses_rpm = -1; # Cached value of UsesRpm result.
sub UsesRpm()
{
  # Cache the value of UsesRpm (it's called a lot).
  return $uses_rpm if ($uses_rpm >= 0);
  $uses_rpm = !FileUtils::file_contains('/etc/lsb-release', "Ubuntu");
  return $uses_rpm;
}

# RPM Specific code to detect RPM package versions.
# Return a list of:
# (version_string, is_installed)
sub get_rpm_package_info ($) {
  my $package_name = shift @_;

  open(RPM, "rpm -q --queryformat \"%{NAME}:%{VERSION}-%{RELEASE}:%{ARCH}\\n\" $package_name 2>/dev/null |");
  my $rpm_query = <RPM>;
  close(RPM);
  if (defined $rpm_query) {
    chomp($rpm_query);
    if (($rpm_query eq '') || ($rpm_query =~ /not.installed/)) {
      $rpm_query = undef;
    }
  }

  my $version_string = "0.0.0";
  my $release = 0;
  my $is_installed = 0;
  my $arch = 0;
  if (defined $rpm_query) {
    ($package_name, $version_string, $arch) = split(/:/, $rpm_query);
    $is_installed = 1;
  }

  return ($version_string, $is_installed);
}

# DEB Specific code to detect RPM package versions.
# Return a list of:
# (version_string, is_installed)
sub get_deb_package_info ($) {
  my $package_name = lc(shift @_); # DEB packages are all lowercase.
  my $version_string = "0.0.0";
  my $is_installed = 0;

  # Set number of columns for dpkg -l, so it won't truncate the names of packages.
  #  Increase if longer package names get added"""
  $ENV{"COLUMNS"} = "200";

  my @install_info = readpipe "dpkg -l " . $package_name;
  foreach(@install_info) {
    if (!/No packages found/) {
      if (/$package_name/) {
        # Look for "ii" to indicate installation on dpkg results.
        $is_installed = 1 if (/^ii/);
        my @parts = split(/[ ]+/, $_);
        $version_string = $parts[2] if @parts >= 3;
      }
    }
  }
  return ($version_string, $is_installed);
}

# Generic utilities for version comparison.
# # Return a list of version numbers from a given version string:
# Given '4.1.2.3-0' return (4, 1, 2, 3, 0).
sub version_string_to_list ($) {
   my $version_string = shift @_;
   my @delimiters = ("-");

   foreach(@delimiters) {
     $version_string =~ s/$_/\./;
   }

   $version_string =~ s/[a-zA-Z]+//;
   # Split the string at the "."
   my @version_list = split(/\./, $version_string);


   # Return a list of ints.
   return map(int, @version_list);
}

# Compare two integer lists returning:
# -1 if a < b, 0 if a==b and 1 if a > b
sub int_list_compare(@@) {
  my ($version_a, $version_b) = @_; # Get the address of the two input arrays.

  # Identify the max index of the two version lists.
  my $max_index = @$version_a > @$version_b ? @$version_a : @$version_b;

  # Iterate through the versions to compare the two, can stop at the
  # first difference.
  for(my $i = 0; $i < $max_index; $i++) {
    # Need to extend the shorter list (if any) with 0's.
    my $a_value = 0;
    $a_value = $$version_a[$i] if $i < @$version_a;
    my $b_value = 0;
    $b_value = $$version_b[$i] if $i < @$version_b;

    return -1 if $a_value < $b_value;
    return 1 if $a_value > $b_value;
  }
  return 0
}

# Generic check of version numbers of the form 4.1.2.3-0.9.
# This assumes no alphabetical characters in the version string.
sub is_min_or_greater_version($$) {
  my $version_actual = shift @_;
  my $version_min = shift @_;

  # Check for empty version string;
  return 0 if $version_actual eq "";

  # Convert the version strings to version lists for easy comparison
  my @version_actual_list = version_string_to_list($version_actual);
  my @version_min_list = version_string_to_list($version_min);

  # We return true if the actual version is greater than or equal to the required version.
  return int_list_compare(\@version_actual_list, \@version_min_list) >= 0;
}

# Get the package info: ($version_string, $is_installed) for the specified package.
# This hides whether we're using RPM or DEB packages.
sub get_package_version($) {
  my $package_name = shift @_;
  return get_rpm_package_info($package_name) if UsesRpm();
  return get_deb_package_info($package_name);
}

# Return 1 if the specified package is installed at a
# version greater than or equal to the version_min
sub is_package_version_sufficient($$) {
   my $package_name = shift @_;
   my $version_min = shift @_;
   my ($version_actual, $is_installed) = get_package_version($package_name);

   return 0 if ($is_installed == 0);

   return is_min_or_greater_version($version_actual, $version_min);
}

# Check if the specified package is not installed at a
# version greater than or equal to the min_version.
# Return a problem description string if it's not found"""
sub check_package_version($$$) {
  my $package_group = shift @_;
  my $package_name = shift @_;
  my $version_min = shift @_;

  if (!is_package_version_sufficient($package_name, $version_min)) {
    my ($package_version, $package_installed) = get_package_version($package_name);
    if ($package_installed == 0) {
      return "Error: Package not installed: [$package_name] needed for $package_group: " .
      "required version=$version_min";
    } else {
      return "Error: Wrong version of package [$package_name] needed for $package_group: " .
      "required version=$version_min, actual version=$package_version";
    }
  }
  return "";
}

# Check that the appropriate package versions are installed by processing the
# expected values from a master CSV file.
sub check_package_versions($$) {
  my $fusion_version = shift @_;
  my $uses_packages = shift @_;
  return () if ($uses_packages == 0); # Skip if not using packages.

  # Simple diagnostic of which package system.
  my @result = ("Config: package system=" . (PackageUtils::UsesRpm() ? "RPM" : "DEB"));

  # Process the CSV master list of required packages and versions.
  # Open the file for reading.
  my $filename = "fusion_packages_" . $fusion_version . ".csv";
  my @csv_array = FileUtils::parse_csv_file($filename);

  # Check for an empty return array.
  return @result if @csv_array == 0;

  # Run through the list of CSV results and check each package's version.
  foreach(@csv_array) {
    my @line = $_;
    my ($package_group, $package_name, $version) = ($line[0][0], $line[0][1], $line[0][2]);
    my @error_messages = check_package_version($package_group, $package_name, $version);
    push @result, @error_messages if (@error_messages > 0);
  }
  return @result;
}


END { }       # module clean-up code here (global destructor)

1;  # don't forget to return a true value from the file

