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
# Run a sanity check on the current system to detect the
# correct installation of Google Earth Fusion
# Usage :
# geecheck [-version=3.2.0]
#   where version defaults to 3.2

require 5; # just to be safe

use strict;

# We want to share the version number as a global.
use vars qw( %hash @array);
package main;

# Standard packages
use FindBin;
use lib "$FindBin::Bin/lib";
BEGIN {
  our $ProductDir = "$FindBin::Bin/products"
}
use Getopt::Long;

# Custom packages
use FileUtils;
use FusionUtils;
use FusionServerUtils;
use DiagnosticUtils;
use PackageUtils;
use PermissionUtils;
use SystemUtils;

# Define usage
my $prog_name = $0;
my $usage = <<EOF;
$prog_name : checks the installation settings of Google Earth Fusion 3.0 and higher
 Errors detected will be highlighted and other diagnostics will be printed out
 that will be helpful for Google support in diagnosing Fusion issues.\n
Usage: $prog_name [-version=VERSION] [-server] [-date=DATE] [-log] [-env] [-cpu] [-all]
    -version=VERSION : specifies the version of Fusion to be checked
                        VERSION is a Fusion version number one of [3.X or 3.X.Y]]
                        and defaults to the latest version
    -log appends the tails of Fusion logs
    -cpu appends the cpuinfo file
    -network appends the /etc/hosts, /etc/resolv.conf, and /etc/nsswitch.conf
    -env appends the environment variables
    -date=DATE: where DATE is either YYYY-MM-DD or "latest"
                 to specify a date for a specific error log to append
    -all turns on all settings (show everything including the latest error log)
    -server : will list all published databases and virtual servers
    -internalinstaller
EOF

# Parse the command line arguments.
my $version = "5.3.9";
my $server = 0;
my $show_logs = 0;
my $show_cpu = 0;
my $show_env = 0;
my $show_all = 0;
my $show_usage = 0;
my $show_network = 0;
my $error_log_date = 0;
my $ret = Getopt::Long::GetOptions("version=s" => \$version,
                                   "log" => \$show_logs,
                                   "cpu" => \$show_cpu,
                                   "env" => \$show_env,
                                   "network" => \$show_network,
                                   "date=s" => \$error_log_date,
                                   "server" => \$server,
                                   "all" => \$show_all,
                                   "help" => \$show_usage);
if ($show_usage) {
  print $usage;
  exit;
}

# If show_all, turn on everything
if ($show_all) {
  $show_logs = $show_cpu = $show_env = $show_network = 1;
  $server = SystemUtils::hostname();
  $error_log_date = "latest" if (!$error_log_date);
}

# 3.0.3 is the last internal installer to use RPM/DEB package versions for installation.
my %version_table = ( "3" => ["1", 1],
                      "3.0" => ["1", 1],
                      "3.0.0" => ["1", 1],
                      "3.0.1" => ["1", 1],
                      "3.0.2" => ["1", 1],
                      "3.0.3" => ["1", 1],
                      "3.1" => ["1", 0],
                      "3.1.0" => ["1", 0],
                      "3.1.1" => ["1", 0],
                      "3.2" =>   ["1", 0],
                      "3.2.0" => ["1", 0],
                      "3.2.1" => ["1", 0],
                      "4.0.0" => ["1", 0],
                      "4.4.0" => ["1", 0],
                      "4.4.1" => ["1", 0],
                      "4.5.0" => ["1", 0],
                      "5.0.0" => ["1", 0],
                      "5.0.1" => ["1", 0],
                      "5.0.2" => ["1", 0],
                      "5.0.3" => ["1", 0],
                      "5.1.0" => ["1", 0],
                      "5.1.1" => ["1", 0],
                      "5.1.2" => ["1", 0],
                      "5.1.3" => ["1", 0],
                      "5.2.0" => ["1", 0],
                      "5.2.1" => ["1", 0],
                      "5.2.2" => ["1", 0],
                      "5.2.3" => ["1", 0],
                      "5.2.4" => ["1", 0],
                      "5.2.5" => ["1", 0],
                      "5.3.0" => ["1", 0],
                      "5.3.1" => ["1", 0],
                      "5.3.2" => ["1", 0],
                      "5.3.3" => ["1", 0],
                      "5.3.4" => ["1", 0],
                      "5.3.5" => ["1", 0],
                      "5.3.6" => ["1", 0],
                      "5.3.7" => ["1", 0],
                      "5.3.8" => ["1", 0],
                      "5.3.9" => ["1", 0],
                      );
my @known_versions = keys(%version_table);

my $version_info = $version_table{$version};
# Define global version for tests to use if necessary.
$main::version = $version;

if (@$version_info == 0) {
   my $version_input = $version;
   $version = $known_versions[$#known_versions];
   $version_info = $version_table{$version};
   print "Unrecognized Fusion version $version_input, defaulting to latest version $version.\n\n";
}
my $version_path = @$version_info[0];
my $uses_packages = @$version_info[1];

print "Checking Google Earth Fusion Version: $version [defaults read from " . $version . "]\n";

# Create a list of all checks we will run through.
my @checks = (\&SystemUtils::check_os_version,
              \&SystemUtils::check_linux_distribution,
              \&FusionUtils::check_fusion_version,
              \&SystemUtils::check_hostname,
              \&SystemUtils::check_localhost,
              \&FusionUtils::check_postgres,
              \&SystemUtils::check_dnsdomainname,
              \&FusionServerUtils::check_maxpostsize,
              \&FusionUtils::check_running_services,
              \&SystemUtils::check_memory,
              \&FusionUtils::check_fusion_config,
              \&SystemUtils::check_java,
              \&SystemUtils::check_umask,
              \&SystemUtils::check_uptime,
              \&SystemUtils::check_firewall,
              \&PackageUtils::check_package_versions,
              \&PermissionUtils::check_permissions,
              \&SystemUtils::check_port_status,
              \&FusionUtils::check_published_root,
              \&FusionUtils::check_fusion_volumes,
              \&FusionServerUtils::check_virtual_servers,
              \&FusionServerUtils::check_remote_servers,
              \&SystemUtils::check_selinuxstatus
              );

# Run the checks and output them to stdout.
my @results = DiagnosticUtils::RunDiagnostics($version_path, $uses_packages, @checks);
DiagnosticUtils::OutputDiagnostics(@results);

# Only show logs if requested.
FusionServerUtils::append_fusion_logs() if ($show_logs);

# Show a specific error log.
FusionServerUtils::append_fusion_error_log($error_log_date) if ($error_log_date);

# Show the published databases.
FusionServerUtils::append_published_dbs() if ($server);
FusionServerUtils::append_virtual_servers() if ($server);

# Show the CPU info.
SystemUtils::append_cpu_info() if ($show_cpu);

# Show the current environment variables.
SystemUtils::append_environment_variables() if ($show_env);

# Show some network related systems files.
SystemUtils::append_network_files() if ($show_network);

# Append the systemrc and volumes.xml
FusionUtils::append_fusion_files();
