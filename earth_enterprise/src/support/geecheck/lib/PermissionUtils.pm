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
# Utilities to check file permissions from
# a master list (stored in a CSV) file.

# The main call for this is
#    check_permissions
# which reads the CSV call and then calls
#    check_permissions_for_path
# check_permissions_for_path also calls to recursive function
#    check_owners_for_path which
# will go down the directory structure as deeply as specified by the CSV entry and
# check for ownership matching the parent directory.

package PermissionUtils;

use strict;
use warnings;

use File::stat;
use FindBin;
use Fcntl ':mode';

# module constructor
BEGIN {
    use Exporter   ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

    # set the version for version checking
    $VERSION     = 1.00;
    @ISA         = qw(Exporter);
    @EXPORT      = qw(&check_permissions
                      &check_permissions_for_path
                      &is_readable
                      &is_writable
                      &is_executable);
    %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],

    # your exported package globals go here,
    # as well as any optionally exported functions
    @EXPORT_OK   = qw();
}
our @EXPORT_OK;

my %version_map = ("3_0_1" => "3_0_0");
# This is an associative array keyed with the paths in permissions csv file
my %paths_in_csv;

# Utilities (not exported)

# Get the file info for the specified path:
# return ($user, $group, $mode);
sub get_file_info($) {
  my $path = shift @_;
  # Stat the file to get the permissions.
  my $stat = stat($path);
  # Check for missing file.
  if (!(defined $stat) || $stat == 0) {
    return ("", "", 0);
  }

  # Get the human readable user,  group and mode.
  my @info = getpwuid($stat->uid);
  my $user = $info[0];
  @info = getgrgid($stat->gid);
  my $group = $info[0];
  return ($user, $group, $stat->mode);
}

# Need a forward declaration for this recursive call.
sub check_owners_for_path($$$$);

# Check the owners of subfolders and files.
sub check_owners_for_path($$$$) {
  my $path = shift @_;
  my $owner = shift @_;
  my $group = shift @_;
  my $recurse_level = shift @_;
  my @result = ();

  # Stat the file to get the permissions.
  my ($user_actual, $group_actual, $mode_raw) = get_file_info($path);
  return @result if ($user_actual eq ""); # Ignore unstatable files

  # Check that they all match the expected values.
  if ($user_actual ne $owner || $group_actual ne $group) {
    push @result, "Error: Invalid ownership: [$path]: actual: [$user_actual, $group_actual], expected: [$owner, $group]";
    return @result;
  }

  # Check subfolders and files for correct ownership.
  if (S_ISDIR($mode_raw) && $recurse_level > 0) {
    $path .= "/" if !($path =~ m/\/$/);
    # Recursively get all the subfolders and verify the owner, group of its files.
    my $dirhandle;
    if (opendir($dirhandle, $path)) {
      foreach(readdir($dirhandle)) {
        if (!/^\.\./ && !/^\./) {
          my $child = "$path$_";
          # If a path exists in permissions csv file, then we don't need to
          # check that here as a subdirectory of some other path
          # (with possibly of different permissions).
          if (!exists($paths_in_csv{"$child"})) {
            my @messages = check_owners_for_path($child, $owner, $group, $recurse_level - 1);
            push @result, @messages if (@messages > 0);
          }
        }
      }
      closedir $dirhandle;
    }
  }

  return @result;
}
my %username_map;
# Check the permissions, owner and group on a specified path.
# If the path is a directory, check all files underneath it for the same owner/group.
sub check_permissions_for_path($$$$$) {
  my $path = shift @_;
  my $owner = shift @_;
  my $group = shift @_;
  my $mode = shift @_;
  my $recurse_level = shift @_;
  my @result = ();

  # Handle custom user names remapping from the standard names.
  load_username_map();
  $owner = $username_map{$owner};
  $group = $username_map{$group};

  # Stat the file to get the permissions.
  my ($user_actual, $group_actual, $mode_raw) = get_file_info($path);
  if ($user_actual eq "") {
    push @result, "Error: Missing file/directory: $path";
    return @result;
  }
  my $mode_actual = sprintf("%5o", $mode_raw);

  # Check that they all match the expected values.
  if ($user_actual ne $owner || $group_actual ne $group || $mode_actual ne $mode) {
    push @result, "Error: Invalid permissions [$path]: actual: [$user_actual, $group_actual, $mode_actual], expected: [$owner, $group, $mode]";
    return @result;
  }

  # Check subfolders and files for correct ownership.
  if (S_ISDIR($mode_raw) && $recurse_level > 0) {
    # Recursively get all the subfolders and verify the owner, group of its files.
    my @messages = check_owners_for_path($path, $owner, $group, $recurse_level);
    push @result, @messages if (@messages > 0);
  }

  return @result;
}

sub load_username_map() {
  return (1,"") if (keys(%username_map) > 0);
  $username_map{"root"} = "root";
  $username_map{"gefusionuser"} = "gefusionuser";
  $username_map{"gegroup"} = "gegroup";
  $username_map{"geapacheuser"} = "geapacheuser";
  $username_map{"getomcatuser"} = "getomcatuser";
  $username_map{"gepguser"} = "gepguser";

  my $name_file = '/etc/init.d/gevars.sh';
  if (-e $name_file) {
    my %name_tokens = ("gefusionuser" => "GEFUSIONUSER",
                     "gegroup" => "GEGROUP",
                     "geapacheuser" => "GEAPACHEUSER",
                     "getomcatuser" => "GETOMCATUSER",
                     "gepguser" => "GEPGUSER"
                     );
    my $user = 0;
    for $user (keys %name_tokens) {
       my $token = $name_tokens{$user};
       my $name = FileUtils::get_nth_word_from_grep($name_file, $token, "\=", 1);
       print "Error: missing $token in $name_file\n" if ($name eq '0');
       $name = $user if ($name eq '0');
       $username_map{$user} = $name;
    }
  }
}

# Check that the desired file permissions are correct.
# The permissions are read from a master file (depends on the version)
# that has a CSV format.
# Inputs: $version
sub check_permissions(@) {
  my @version_info = shift @_;
  my $version = $version_info[0];

  # Remap to the most appropriate version if necessary.
  $version = $version_map{$version} if (exists $version_map{$version});

  # Process the CSV master list of required files and permissionss.
  my $filename = $FindBin::Bin . "/fusion_permissions_" . $version . ".csv";
  my @csv_array = FileUtils::parse_csv_file($filename);
  my @result = ();

  # Check for an empty return array.
  return @result if @csv_array == 0;

  # Run through the list of CSV results and check each package's version.
  # format: path, owner, group, mode, recurse_level
  #   e.g., /opt/google/, root, root, 40755, 0
  foreach(@csv_array) {
    my @line = $_;
    my $path = $line[0][0];
    $path =~ s/\/$//g;
    $paths_in_csv{"$path"} = 1;
  }
  foreach(@csv_array) {
    my @line = $_;
    my ($path, $owner, $group, $mode, $recurse_level) = ($line[0][0], $line[0][1], $line[0][2], $line[0][3], $line[0][4]);

    # Check the permissions on the path.
    my @error_messages = check_permissions_for_path($path, $owner, $group, $mode, $recurse_level);
    push @result, @error_messages if (@error_messages > 0);
  }
  return @result;
}

# Check permission given a user or group.
# Each of these Takes (filename, user, group)
sub is_readable($$$) {
  my $filename = shift @_;
  my $user = shift @_;
  my $group = shift @_;

  my ($user_actual, $group_actual, $mode) = get_file_info($filename);
  return 0 if ($user_actual eq "");

  return 1 if ($mode & S_IRUSR && $user eq $user_actual);
  return 1 if ($mode & S_IRGRP && $group eq $group_actual);
  return 1 if ($mode & S_IROTH);
  return 0;
}

sub is_writable($$$) {
  my $filename = shift @_;
  my $user = shift @_;
  my $group = shift @_;

  my ($user_actual, $group_actual, $mode) = get_file_info($filename);
  return 0 if ($user_actual eq "");

  return 1 if ($mode & S_IWUSR && $user eq $user_actual);
  return 1 if ($mode & S_IWGRP && $group eq $group_actual);
  return 1 if ($mode & S_IWOTH);
  return 0;
}

sub is_executable($$$) {
  my $filename = shift @_;
  my $user = shift @_;
  my $group = shift @_;

  my ($user_actual, $group_actual, $mode) = get_file_info($filename);
  return 0 if ($user_actual eq "");

  return 1 if ($mode & S_IXUSR && $user eq $user_actual);
  return 1 if ($mode & S_IXGRP && $group eq $group_actual);
  return 1 if ($mode & S_IXOTH);
  return 0;
}

END { }       # module clean-up code here (global destructor)

## YOUR CODE GOES HERE

1;  # don't forget to return a true value from the file


