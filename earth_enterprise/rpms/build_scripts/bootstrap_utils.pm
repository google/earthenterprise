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
# Some basic RPM and YUM utilities to bootstrap Google Earth Enterprise Fusion
# build environments.
# Note: Fusion is typically built on RPM distributions, but Goobuntu developer
# machines are also supported using alien, apt-get and dpkg.

package bootstrap_utils;

require 5.6.0;

#"use strict" is incompatible with gLucid Perl

use warnings;

# module constructor
BEGIN {
  use Exporter   ();
  our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

  # set the version for version checking
  $VERSION     = 1.00;
  @ISA         = qw(Exporter);
  @EXPORT      = qw(&get_architecture
                    &is_ubuntu
                    &package_filename
                    &rpm_clean_packages
                    &check_packages
                    &check_spec_files
                    &yum_install_packages
                    &yum_uninstall_packages
                    &rpm_uninstall_packages
                    &rpm_install_packages
                    &debian_install_packages
                    &dpkg_cache_uninstall_packages
                    &dpkg_cache_install_packages
                    );

  %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],

  # your exported package globals go here,
  # as well as any optionally exported functions
  @EXPORT_OK   = qw();
}
our @EXPORT_OK;

# Is this Goobuntu...don't do installs on Debian.
sub is_ubuntu() {
  my $ubuntu = `grep Ubuntu /etc/lsb-release 2> /dev/null`;
  chomp($ubuntu);
  return 1 if ($ubuntu =~ /Ubuntu/);
  return 0;
}

# Determine the architecture of the current machine.
# Either 'i686' for 32 bit machines, or 'x86_64' for 64 bit machines.
sub get_architecture() {
  my $architecture = 'i686';
  my $bits = `getconf LONG_BIT`;
  chomp($bits);
  $architecture = 'x86_64' if ($bits =~ /64/);
  return $architecture;
}

# Package file name.
sub package_filename($$$) {
  my ($arch, $name, $version) = @_;
  my $package = "$arch/$name-$version.$arch.rpm";
  return $package;
}

# Check that packages are there.
sub check_packages($@) {
  my ($path, @packages) = @_;
  my $missing = '';
  foreach my $package_info (@packages) {
    my ($name, $version, $arch) = @{$package_info};
    my $filename = "$path/" . package_filename($arch, $name, $version);
    $missing = "$missing$filename\n" if !-e $filename;
  }

  if ($missing ne '') {
    print("Error: Missing the following packages:\n" . $missing . "\n");
    exit;
  }
  print("Verified all packages in place.\n");
}
# Check that packages are there.
sub rpm_clean_packages($@) {
  my ($path, @packages) = @_;
  foreach my $package_info (@packages) {
    my ($name, $version, $arch) = @{$package_info};
    my $filename = "$path/" . package_filename($arch, $name, $version);
    if ( -e $filename ) {
      print("Removing $filename\n");
      unlink $filename || die("Unable to remove $filename")
    }
  }

  print("all rpms have been removed.\n");
}


# Check that packages are there.
sub check_spec_files($@) {
  my ($path, @packages) = @_;
  my $missing = '';
  foreach my $package_info (@packages) {
    my ($name) = @{$package_info};
    my $filename = "$path/SPECS/$name.spec";
    $missing = "$missing$filename\n" if (!(-e $filename));
  }

  if ($missing ne '') {
    print("Error: Missing the following spec files:\n" . $missing . "\n");
    exit;
  }
  print("Verified all packages in place.\n");
}


# YUM install the specified RPM package.
sub yum_install($) {
  my $package = shift;
  print "Installing yum $package\n";
  check_command("yum install $package -y");
}

# YUM uninstall the specified RPM package.
sub yum_uninstall($) {
  my $package = shift;
  print "Removing yum $package\n";
  check_command("yum remove $package -y");
}

# Install the specified YUM packages.
sub yum_install_packages(@) {
 my @yum_packages = @_;
 # Install in the forward order.
 foreach my $package_info (@yum_packages) {
   my ($name) = @{$package_info};
   yum_install($name);
 }
}

# Unnstall the specified YUM packages.
sub yum_uninstall_packages(@) {
  my @yum_packages = @_;

  foreach my $name (@yum_packages) {
    yum_uninstall($name);
  }
}

# Uninstall the specified RPM package.
sub rpm_uninstall($) {
  my ($package) = @_;
  print("Uninstalling $package\n");
  # Check if it's installed first, otherwise
  my $command = "rpm -q $package";
  open(OUT, "$command |") || die("Unable to execute: $command\n");
  while(<OUT>) {
    if (/is not installed/) {
      print("  not installed\n");
      close(OUT);
      return;  # No need to uninstall.
    }
  }
  close(OUT);
  check_command("rpm -e $package");
}

# Check for Command Errors
# Exits with a warning for any detected errors.
# This assumes the command will print to stderr the text "Error:*" or "error:*".
sub check_command($) {
  my ($command) = @_;
  open(COMMAND_OUT, "$command 2>&1 |") || die("Unable to execute: $command\n");
  my $failure = 0;
  while(<COMMAND_OUT>) {
    print($_);
    $failure = 1 if (/^error:/i);
  }
  close(COMMAND_OUT);
  my $result = $? >> 8;  # Check return value of $command, die if not 0.
  if($result)
  {
    print "Command error result of $result: $command\n";
  }
  # BP (08/31/2010): Gets "incorrect format: unknown tag" error
  if($failure)
  {
    print "Failure: $command\n";
  }
}

# Install the specified RPM package.
sub rpm_install($$$$) {
  my ($path, $arch, $name, $version) = @_;
  my $package = "$path/" . package_filename($arch, $name, $version);
  print("Installing $package\n");
  check_command("rpm -i $package");
}

# Install the specified RPM package.
sub rpm_build($$) {
  my ($name, $arch) = @_;
  print("Building spec file: $name\n");
  # Build the binary rpm package (in RPMS)
  check_command("rpmbuild -bb $name");

  # Build the src rpm package in (SRPMS)
  print("Building src.rpm: $name\n");
  check_command("rpmbuild -bs $name");
}

# Uninstall the specified packages
sub rpm_uninstall_packages(@) {
  my @packages = @_;
  # Must uninstall in reverse order...for dependency reasons.
  my @rpackages = reverse(@packages);
  foreach my $package_info (@rpackages) {
    my ($name) = @{$package_info};
    rpm_uninstall($name);
  }
}


# Install the specified RPM packages.
sub rpm_install_packages($@) {
  my ($path, @packages) = @_;
  # Install in the forward order.
  foreach my $package_info (@packages) {
    my ($name, $version, $arch) = @{$package_info};
    rpm_install($path, $arch, $name, $version);
  }
}

# Build and Install the specified RPM packages from spec files.
sub rpm_build_and_install_packages($@) {
  my ($path, @packages) = @_;
  # Install in the forward order.
  foreach my $package_info (@packages) {
    my ($name, $version, $arch, $has_devel) = @{$package_info};
    my $spec_filename = "$path/SPECS/$name.spec";
    rpm_build($spec_filename, $arch);
    rpm_install($path . '/RPMS', $arch, $name, $version);
    rpm_install($path . '/RPMS', $arch, "$name-devel", $version) if ($has_devel);
  }
}

# -----------------------------------------------------------
# Ubuntu dpkg and alien commands for Goobuntu platform
#
# Unnstall the specified cached Debian packages.
sub debian_cache_uninstall_packages(@) {
  my @debian_packages = @_;

  foreach my $name (@debian_packages) {
    debian_uninstall($name);
  }
}

# DPKG uninstall the specified Debian package.
sub debian_uninstall($) {
  my ($package) = @_;
  print("Removing Debian $package\n");
  check_command("dpkg --remove --force-all $package");
}

# DPKG direct install from cache the specified Debian package.
sub debian_cache_install($) {
  my ($package) = @_;
  print("Installing Debian Cache $package\n");
  check_command("apt-get -f install $package");
}

# DPKG install from cache the specified Debian package.
sub debian_install($) {
  my ($debian_package) = @_;
  print("Installing Debian $debian_package\n");
  check_command("dpkg --install $debian_package");
}

# Install the specified Debian cache packages.
sub debian_cache_install_packages(@) {
  my @debian_packages = @_;

  # Install in the forward order.
  foreach my $package_info (@debian_packages) {
    my $name = $package_info->[1];
    debian_cache_install($name) if ($name ne '');
  }
}

# Install the specified RPM package.
sub debian_from_alien_install($$$$) {
  my ($path, $arch, $name, $version) = @_;
  my $rpm_package = "$path/" . package_filename($arch, $name, $version);
  print("Installing $rpm_package via alien\n");
  check_command("alien --scripts -i $rpm_package");
}

# Uninstall the specified packages
sub debian_uninstall_packages(@) {
  my @packages = @_;
  # Must uninstall in reverse order...for dependency reasons.
  my @rpackages = reverse(@packages);
  foreach my $package_info (@rpackages) {
    my ($name) = @{$package_info};
    debian_uninstall($name);
  }
}


# Install the specified RPM packages.
sub debian_install_packages($@) {
  my ($path, @packages) = @_;
  # Install in the forward order.
  foreach my $package_info (@packages) {
    my ($name, $version, $arch) = @{$package_info};
    debian_from_alien_install($path, $arch, $name, $version);
  }
}


END { }       # module clean-up code here (global destructor)

1;  # don't forget to return a true value from the file
