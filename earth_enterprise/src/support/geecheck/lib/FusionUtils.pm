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
# Utilities to check various Google Earth Fusion configuration settings.

package FusionUtils;

use strict;
use warnings;
use PackageUtils;
use PermissionUtils;
use SystemUtils;

# module constructor
BEGIN {
    use Exporter   ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

    # set the version for version checking
    $VERSION     = 1.00;
    @ISA         = qw(Exporter);
    @EXPORT      = qw(&asset_root
                      &check_fusion_config
                      &check_publish_root
                      &check_fusion_volumes
                      &check_running_services
                      &check_fusion_version
                      &check_published_root
                      &check_postgres
                      &append_fusion_files);
    %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],

    # your exported package globals go here,
    # as well as any optionally exported functions
    @EXPORT_OK   = qw();
}
our @EXPORT_OK;

# Define defaults for these various configuration settings.
# These will be checked by the various check utilities.
#

# Services that should be running on a proper install of Fusion.

my @running_services = ("gesystemmanager",
                        "geresourceprovider",
                        "gehttpd",
                        "/opt/google/bin/postgres");
my %running_services_pids = ("gesystemmanager" => "/var/opt/google/run/gesystemmanager.pid",
                             "geresourceprovider" => "/var/opt/google/run/geresourceprovider.pid");
my $gehttpd_service = "/opt/google/gehttpd/bin/gehttpd";

my $systemrc_filename = "/opt/google/etc/systemrc";
# A Hash map of fusion volume info (parse once, use multiple).
my %fusion_volume_info = ();

# Utility functions

# Pull the Fusion config elements out of the systemrc file.
sub get_systemrc_element($) {
  my $element = shift @_;
  my ($value, $error) = FileUtils::get_xml_element($systemrc_filename, $element);
  return $value if ($error eq "");
  return $error;
}

# Get the element from the systemrc file, and return a Config line for it.
sub get_systemrc_element_config($$) {
  my $element = shift @_;
  my $suffix = shift @_;
  my ($value, $error) = FileUtils::get_xml_element($systemrc_filename, $element);
  return "Config:   $element=$value" if ($error eq "");
  return $error . $suffix;
}

# Pull the Fusion asset root out of the systemrc file.
sub asset_root() {
  return get_systemrc_element("assetroot");
}

##########################################################
# The following "check_*" commands use the convention:
# Report a list of results of configuration items and error
#   items of the following form:
#     Config: configuration message
#     Error: specific error message
##########################################################


# Print the config line for the current running version of Fusion.
sub check_fusion_version($$) {
  my $version = shift @_;
  my $uses_packages = shift @_;
  my @result = ();
  if ($uses_packages == 1) {
     my @packages = ("GoogleEarthFusionPro", "GoogleEarthFusionGoogle", "GoogleEarthFusionLT");
     foreach(@packages) {
       my $package = $_;
       my ($version, $is_installed) = PackageUtils::get_package_version($package);
       if ($is_installed != 0) {
         push @result, "Config: Fusion Version=$package : $version";
         return @result;
       }
     }
     # Could not find a version, add an error.
     push @result, "Config: Fusion Version=version not found";
     push @result, "Error: Did not detect GoogleEarthFusionPro, GoogleEarthFusionLT, or GoogleEarthFusionGoogle package";
  } else {
     my $version_file = "/etc/opt/google/fusion_version";
     if (-e $version_file) {
        my $version = `cat $version_file`;
        chomp $version;
        push @result, "Config: Fusion Version=$version";
     } else {
        push @result, "Config: Fusion Version=version not found";
        push @result, "Error: Fusion version file not found: $version_file";
     }
     $version_file = "/etc/opt/google/fusion_server_version";
     if (-e $version_file) {
        my $version = `cat $version_file`;
        chomp $version;
        push @result, "Config: Fusion Server Version=$version";
     } else {
        push @result, "Config: Fusion Server Version=version not found";
        push @result, "Error: Fusion Server version file not found: $version_file";
     }
  }

  return @result;
}

# Print the current published root path and check the permissions on it.
sub check_published_root() {
  my @result = ();

  my $search_space = FileUtils::get_nth_word("/opt/google/gehttpd/conf.d/search_space", 2);
  my $stream_space = FileUtils::get_nth_word("/opt/google/gehttpd/conf.d/stream_space", 2);

  push @result, "Config: ---------------------------------------\nPublish Root Info";
  # Get info about the disk the published db's reside on.
  push @result, SystemUtils::config_disk_info("published db root[stream]", $stream_space);
  push @result, SystemUtils::config_disk_info("published db root[search]", $search_space);

  # Also check the following file permissions:
  # /gevol/published_dbs/, root, root, 40755, 100
  # Do this check once for each of stream/search:
  push @result,
    PermissionUtils::check_permissions_for_path($search_space . "/..", "root", "root", "40755", 0);
  push @result,
    PermissionUtils::check_permissions_for_path($stream_space . "/..", "root", "root", "40755", 0);
  # /gevol/published_dbs/.config, root, root, 100644, 100
  # Do this check once for each of stream/search:
  push @result,
    PermissionUtils::check_permissions_for_path($search_space . "/../.config", "root", "root", "100644", 0);
  push @result,
    PermissionUtils::check_permissions_for_path($stream_space . "/../.config", "root", "root", "100644", 0);

  # Check the permissions of the directory and its contents (for search/stream).
  # /gevol/published_dbs/search_space/, geapacheuser, gegroup, 40775, 100
  push @result,
    PermissionUtils::check_permissions_for_path($search_space, "geapacheuser", "gegroup", "40775", 0);
  # /gevol/published_dbs/stream_space/, geapacheuser, gegroup, 40775, 100
  push @result,
    PermissionUtils::check_permissions_for_path($stream_space, "geapacheuser", "gegroup", "40775", 0);

  # Check that parent folder is readable by geapacheuser, gegroup
  if (!PermissionUtils::is_readable($search_space . "/../..", "geapacheuser", "gegroup")) {
    push @result, "Error: parent directory of $search_space needs to be readable by geapacheuser, gegroup";
  }
  if (!PermissionUtils::is_readable($stream_space . "/../..", "geapacheuser", "gegroup")) {
    push @result, "Error: parent directory of $stream_space needs to be readable by geapacheuser, gegroup";
  }
  return @result;
}

# Print the current asset src volumes and check the permissions on them.
sub check_asset_src_volume($$$) {
  my $name = shift @_;
  my $host = shift @_;
  my $asset_src = shift @_;

  my @result = ();
  # Check the hostname for validity/accessibility.
  push @result, SystemUtils::check_hostname_validity("src volume[$name] host", $host, "  ");
  # Get info about the disk the source volumes reside on.
  push @result, SystemUtils::config_disk_info("  disk info", $asset_src);

  # Also check the following file permissions:
  # /gevol/src/, root, root, 40755, 100
  push @result,
    PermissionUtils::check_permissions_for_path($asset_src, "root", "root", "40755", 100);

  # Check that parent folder is readable by gefusionuser, gegroup
  if (!PermissionUtils::is_readable($asset_src . "/..", "gefusionuser", "gegroup")) {
    push @result, "Error: parent directory of $asset_src needs to be readable by gefusionuser, gegroup";
  }

  return @result;
}

# Print the current asset root path and check the permissions on it.
sub check_asset_root($$$) {
  my $name = shift @_;
  my $host = shift @_;
  my $asset_root = shift @_;

  my @result = ();
  # Get the asset root version.
  my $version = `cat $asset_root/.config/.fusionversion`;
  chomp $version;
  push @result, "Config:   asset root version=$version";
  # Check the hostname for validity/accessibility.
  push @result, SystemUtils::check_hostname_validity("assetroot host", $host, "  ");


  # Get info about the disk the asset root resides on.
  push @result, FusionServerUtils::check_tomcat($host, "  assetroot host");

  # Get info about the disk the asset root resides on.
  push @result, SystemUtils::config_disk_info("  disk info", $asset_root);

  # Check file permissions on the asset root
  # /gevol/assets/, gefusionuser, gegroup, 40755, 100
  push @result,
    PermissionUtils::check_permissions_for_path($asset_root, "gefusionuser", "gegroup", "40755", 100);

  # Check that parent folder is readable by gefusionuser, gegroup
  if (!PermissionUtils::is_readable($asset_root . "/..", "gefusionuser", "gegroup")) {
    push @result, "Error: parent directory of $asset_root needs to be readable by gefusionuser, gegroup";
  }

  return @result;
}

# Get basic fusion config items.
sub check_fusion_config() {
  my @result = ();
  push @result, "Config: Configuration values for Fusion:";
  my $maxjobs = get_systemrc_element("maxjobs");
  my $numcpus = `grep processor /proc/cpuinfo | wc -l`;
  chomp $numcpus;
  if ($maxjobs ne $numcpus) {
     push @result, "Error: maxjobs = $maxjobs in systemrc while there are $numcpus CPUs";
  }
  push @result, "Config:   maxjobs=$maxjobs [numcpus = $numcpus]";
  push @result, get_systemrc_element_config("locked", "");
  push @result, get_systemrc_element_config("fusionUsername", " [Note: this is optional]");
  push @result, get_systemrc_element_config("userGroupname", " [Note: this is optional]");
  return @result;
}

# Check the volumes including hostnames, permissions etc.
sub check_fusion_volumes() {
  my @result = ();
  my ($status, $message) = load_fusion_volumes();
  if ($status == 0) {
    push @result, $message;
    return @result;
  }

  push @result, "Config: ---------------------------------------\nFusion Volume Info";
  my $current_hostname = SystemUtils::hostname();
  my $current_shorthostname = SystemUtils::hostname_short();
  my @elements = ("netpath", "host", "localpath");
  for my $volume ( keys %fusion_volume_info ) {
    # Print basic info.
    my $title = "fusion source volume";
    $title = "fusion asset root" if ($volume eq "assetroot");
    push @result, "Config: $title" . "[$volume]";
    my $host = $fusion_volume_info{$volume}{"host"};
    my $localpath = $fusion_volume_info{$volume}{"localpath"};
    my $netpath = $fusion_volume_info{$volume}{"netpath"};
    foreach(@elements) {
      my $tag = $_;
      my $value = $fusion_volume_info{$volume}{$_};
      push @result, "Config:   $tag=$value";
    }
    if ($volume eq "assetroot") {
      push @result,
        check_asset_root($volume, $host, $localpath);
    } else {
      push @result,
        check_asset_src_volume($volume, $host, $localpath);
    }
    if ($localpath =~ /\/$/) {
      push @result, "Error: Fusion volumes should not end in \"/\", please edit volumes.xml to fix entry $volume : $localpath";
    }
    if ($current_hostname ne $host && $current_shorthostname ne $host) {
      push @result, SystemUtils::check_nslookup($host, " ");
      push @result, "Error: Possibly incorrect hostname in volume[$volume]: hostname[$host] different from local hostname[$current_hostname]";
    }
  }
  return @result;
}

sub load_fusion_volumes() {
  return (1,"") if (keys(%fusion_volume_info) > 0);

  my $asset_root = asset_root();
  my $volumes_filename = $asset_root . "/.config/volumes.xml";
  if (!-e$volumes_filename) {
    return (0, "Error: No Volumes found at $volumes_filename");
  }

  if (!open(FILE, $volumes_filename)) {
    return (0, "Error: Unable to open $volumes_filename");
  }

  # Read through the XML file and print the volume info.
  # We're doing crude parsing because it is the perl XML module is NOT standard.
  my $in_volume_defs = 0;
  my $end_element = "";
  my $in_element = 0;
  my $volume;
  my @elements = ("netpath", "host", "localpath");
  while(<FILE>) {
    # Look for   <item key="src"> or <src>
    #     <netpath>/gevol/src</netpath>
    #     <host>servername</host>
    #     <localpath>/gevol/src</localpath>
    my $line = $_;
    # Check for "volumedefs" begin and end element.
    if (/<volumedefs>/) {
      $in_volume_defs = 1;
    } elsif (/<\/volumedefs>/) {
      $in_volume_defs = 0;
    } elsif ($in_volume_defs) {
      # Within the volumedefs, look for the first subelement
      if (!$in_element) {
        if (/item key="(.*?)"/) {
          $volume = $1;
          $end_element = "item";
          $in_element = 1;
        } elsif (/<([a-zA-Z0-9-._]+)>/) {
          $volume = $1;
          $end_element = $1;
          $in_element = 1;
        }
      } else {
        # Within the <item key="element"> or <element>:

        # Check for the end of the element
        if (/<\/$end_element>/) {
          $in_element = 0;
        } else {
          # Look for the interior elements.
          foreach(@elements) {
            my $element = $_;
            if ($line =~ /<$element>/) {
              $line =~ m/<$element>(.*?)<\/$element>/;
              $fusion_volume_info{$volume}{$element} = $1;
            }
          }
        }
      }
    }
  }
  close(FILE);
  return (1, "");
}

# Check that the expected services are running.
sub check_running_services() {

  # We're going to run through a list of services and
  # report on whether they are running or not.
  my @missing_services = ();
  my @result = ();

  foreach(@running_services) {
     my $service = $_;
     # check that the service is running
     my @ps_result = readpipe "ps -ef | grep $service";

     my $found = 0;
     foreach(@ps_result) {
       if (!/grep/) {
         $found = 1 if (/$service/);
       }
     }
     if (!$found) {
       push @missing_services, $service;
       # Check for a PID file for the missing service.
       # If it exists this is a problem.
       if (exists $running_services_pids{$service}) {
         my $pidfile = $running_services_pids{$service};
         if (-e$pidfile) {
           push @result, "Error: the $service is NOT RUNNING, but $pidfile exists. Delete this file restart gefusion.";
         }
       }
     }
     push @result, "Config: service[" . $service . "]=" . ($found == 0 ? "NOT RUNNING" : "running");
  }
  if (@missing_services > 0) {
     my $error_message = "Error: The following Fusion services are not running: ";
     $error_message .= join(", ", @missing_services);
     push @result, $error_message;
  }

  push @result, check_httpd_services();
  return @result;
}

# Check for existance of gehttpd services.
# One should be running as root and at least two others should be running.
sub check_httpd_services() {

  # Check the running services for gehttpd
  my $service = $gehttpd_service;
  my @ps_result = readpipe "ps -ef | grep httpd";
  my $root_count = 0;
  my $child_count = 0;
  my @result = ();
  foreach(@ps_result) {
    if (!/grep/) {
      if (/$service/) {
        if (/^root/) {
          $root_count++
        } else {
          $child_count++;
        }
      } elsif (/ httpd / || / httpd$/) {
        push @result, "Error: there appears to be another HTTPD server running on this system. Only gehttpd should be running.";
      }
    }
  }

  # Return the result messages.
  push @result, "Config: service[$service]=running $root_count root, $child_count child";

  if ($root_count != 1) {
    push @result, "Error: $root_count root gehttpd processes, we should have 1 and only 1.";
  }
  if ($child_count < 2) {
    push @result, "Error: $child_count child gehttpd processes, we should have at least 2.";
  }
  return @result;
}

# Check the postgres installation.
sub check_postgres() {
  my @max_stack_depth_list = readpipe "sudo -u gepguser /opt/google/bin/psql postgres -c \"show max_stack_depth\"";
  my $max_stack_depth = $max_stack_depth_list[2];

  chomp $max_stack_depth;
  my @result = ();
  if ($max_stack_depth eq "") {
     push @result, "Config: postgres max_stack_depth = not found (may need to run geecheck as root)";
     push @result, "Error: postgres max_stack_depth not found (you may need to run geecheck as root)";
  } else {
     push @result, "Config: postgres max_stack_depth = $max_stack_depth";
  }
  return @result;
}

# Append the tails of various fusion logs.
sub append_fusion_files() {
  my $asset_root = asset_root();
  my @files = ("/etc/opt/google/systemrc",
               $asset_root . "/.config/volumes.xml");

  foreach(@files) {
    print "------------------------------------------------------------------------------------\n";
    print "Fusion Config File [$_]\n";
    print "------------------------------------------------------------------------------------\n";
    print `cat $_`;
    print "------------------------------------------------------------------------------------\n";
  }
}

END { }       # module clean-up code here (global destructor)

1;  # don't forget to return a true value from the file

