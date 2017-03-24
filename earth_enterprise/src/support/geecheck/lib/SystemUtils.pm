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
# Utilities to check various system settings as expected
# by Google Earth Fusion.

package SystemUtils;

use strict;
use warnings;
use Config;
use FileUtils;

# module constructor
BEGIN {
    use Exporter   ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

    # set the version for version checking
    $VERSION     = 1.00;
    @ISA         = qw(Exporter);
    @EXPORT      = qw(&check_umask
                      &is_localhost
                      &hostname
                      &hostname_short
                      &server_domain_from_url
                      &check_hostname_validity
                      &check_nslookup
                      &check_localhost
                      &append_cpu_info
                      &append_network_files
                      &append_environment_variables
                      &check_os_version
                      &check_hostname
                      &check_dnsdomainname
                      &check_uptime
                      &config_disk_info
                      &check_linux_distribution
                      &check_port_status
                      &check_firewall
                      &check_java
                      &check_memory
                      &check_selinuxstatus);
    %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],

    # your exported package globals go here,
    # as well as any optionally exported functions
    @EXPORT_OK   = qw();
}
our @EXPORT_OK;

# Define defaults for these various configuration settings.
# These will be checked by the various check utilities.
#
# This UMASK is needed for publishing databases.
my $umask = 22;

# Invalid hostnames for Fusion Server.
my @bad_hostnames = ("(none)",
                    "linux",
                    "localhost",
                    "dhcp",
                    "bootp");

# Utility check functions
my %volumes = (); # Cached map of volume information.

# Load the volumes info into a hash for later use.
# %Volumes will then contain a map from
# volume to ($file_system, $format, $size, $used, $available, $used_percent, $mount)
sub load_volumes() {
  # This is a cached global hash.
  return if (scalar keys %volumes > 0);

  # run "df -h" and pick out the volumes
  # Filesystem    Type    Size  Used Avail Use% Mounted on
  # /dev/sda1     ext3     37G   19G   17G  52% /
  # /varrun       tmpfs    1.9G  1.7M  1.9G   1% /var/run
  if (open(DF, "df -h -T|")) {
    # Pull out the volumes and stats.
    # Some entries will be spread across two lines.
    my @previous_parts = ();
    while(<DF>) {
      my $line = $_;
      chomp $line;
      if (!/^Filesystem/) {
        my @parts = split(/[ \t]+/, $line);
        if (@parts < 7 && @previous_parts == 0) {
          # We have a partial line...save the array to concat with the next line.
          @previous_parts = @parts;
        }
        else {
          if (@previous_parts > 0) {
            # Concat the two arrays and reset the previous array.
            shift @parts; # Strip off the first element after a newline.
            push @previous_parts, @parts;
            @parts = @previous_parts;
            @previous_parts = ();
          }
          my $volume = pop @parts; # Volume name is the last.
          $volumes{$volume} = \@parts;
        }
      }
    }
  }

  # For debug, it's usefult to print this out
  if (0) {
    while ( my ($key, $value) = each(%volumes) ) {
      print "$key => ";
      foreach(@$value) {
        print "$_ ";
      }
      print "\n";
    }
  }
  close(DF);
}

# Check disk volume for a file.
# This takes a directory and returns the following array of volume info:
# ($volume, $file_system, $size, $used, $available, $used_percent, $format)
sub get_disk_volume_info($) {
  my $directory = shift @_;
  load_volumes();
  # %Volumes contains a map from
  # volume to ($size, $used, $available, $used_percent, $mount, $format)

  # Run through the volumes and identify the one that corresponds to
  # our directory.  Find the longest volume that matches our directory.
  my $best_volume = "";
  my $volume;
  foreach $volume (keys %volumes) {
    if ($directory =~ /$volume/) {
      if (length($volume) > length($best_volume)) {
        $best_volume = $volume;
      }
    }
  }
  my @result = ($best_volume);
  my $volume_info = $volumes{$best_volume};
  foreach(@$volume_info) {
    push @result, $_;
  }
  return @result;
}

# Print the config line for the current running version of Fusion.
sub check_os_version() {
  my $version = $Config{osname} . ":" . $Config{osvers};
  my $uname = `uname -a`;
  chomp $uname;

  my @result = ();
  push @result, "Config: OS Version=$version";
  push @result, "Config: OS uname=$uname";
  return @result;
}

# Check that the current umask is correct.
# Also checks the /etc/profile and /etc/login.defs for defaults too.
sub check_umask($$) {
  my $version = shift @_;
  my $umask_is_important = shift @_; # as of 3.1, umask should be unnecessary
  my $prefix = "";
  $prefix = "[note: as of 3.1 this is not considered necessary]" if ($umask_is_important == 0);
  my $umask_actual = umask;
  $umask_actual = int(sprintf("%04o", $umask_actual));

  my @result = ("Config: umask=$umask_actual");
  if ($umask != $umask_actual) {
    push @result, "Error: umask expected: $umask, was: $umask_actual $prefix";
  }

  # TODO check umask for root, gefusionuser, geapacheuser, gepguser, getomcatuser, and gegroup
  # Check  grep umask /etc/profile
  # umask 022
  $umask_actual = int(FileUtils::get_nth_word("grep '^umask[ ]\\+[0-9]\\+' /etc/profile|", 1));
  push @result, "Config: umask /etc/profile=$umask_actual";
  if ($umask != $umask_actual) {
    push @result, "Error: umask in /etc/profile expected: $umask, was: $umask_actual $prefix";
  }

  # Check grep UMASK /etc/login.defs
  # UMASK          022
  # This uses a nasty double grep, the obvious incantations didn't seem to work.
  my $command = "grep 'UMASK' /etc/login.defs | grep '[0-9]\$' |";
  $umask_actual = int(FileUtils::get_nth_word($command, 1));
  push @result, "Config: umask /etc/login.defs=$umask_actual";
  if ($umask != $umask_actual) {
    push @result, "Error: umask in /etc/login.defs expected: $umask, was: $umask_actual $prefix";
  }
  return @result;
}

# Check whether the standard firewall is running.
sub check_firewall() {
  my $suse_firewall = "/etc/init.d/SuSEfirewall2_setup";
  my $suse_firewall_init = "/etc/init.d/SuSEfirewall2_init";
  my $redhat_firewall = "/etc/init.d/iptables";
  my @result = ();
  if (-e$suse_firewall) {
     my $firewall_status = readpipe "$suse_firewall status";
     if ($firewall_status =~ /running/) {
       push @result, "Error: SuSE firewall is running. You may wish to stop the firewall or " .
         "at least check its settings if you are debugging connection problems.\n" .
         "  Use  $suse_firewall stop, and $suse_firewall_init stop, or check your settings";
       push @result, "Config: SuSE firewall= RUNNING (this may cause connection issues if not setup properly)";
     } else {
       push @result, "Config: SuSE firewall= not running (this is the desired situation)";
     }
  } elsif (-e$redhat_firewall) {
    my $firewall_status = readpipe "$redhat_firewall status";
    if ($firewall_status =~ /Firewall is stopped/) {
      push @result, "Config: RedHat firewall= not running (this is the desired situation)";
    } else {
      push @result, "Error: RedHat firewall is running. You may wish to stop the firewall or " .
         "at least check its settings if you are debugging connection problems.\n";
      push @result, "Config: RedHat firewall= RUNNING (this may cause connection issues if not setup properly)";
    }
  } else {
    push @result, "Error: No firewall recognized for this system.";
    push @result, "Config: firewall= not recognized";
  }
  return @result;
}


# Return a string to indicate if the binary is 32-bit or 64-bit.
sub binary_bit_architecture($) {
  my $filename = shift @_;
  my $info = `file -L $filename`;
  return '32-bit' if ($info =~ /32-bit/);
  return '64-bit' if ($info =~ /64-bit/);
  return 'unknown-bits';
}

# Test whether a domain is the localhost domain.
sub is_localhost($) {
  my $domain = shift @_;
  return 1 if ($domain eq "localhost");
  return 1 if ($domain eq hostname());
  return 1 if ($domain eq hostname_short());
  return 0;
}

# Return the hostname of this computer.
sub hostname() {
  my $hostname = readpipe "hostname -f";
  chomp $hostname;
  return $hostname;
}

# Return the shortened hostname of this computer.
sub hostname_short() {
  my $hostname = readpipe "hostname -s";
  chomp $hostname;
  return $hostname;
}

# Breaks a domain with port number across the ":".
# Returning (domain, port) where port is "" by default.
# returns the (xxx, yyy) from xxx:yyy
sub split_port_from_domain($) {
  my $hostname = shift @_;
  my @parts = split(":", $hostname);
  my $port = "";
  $port = $parts[1] if (@parts > 1);
  return ($parts[0], $port);
}

# Extract the domain from a URL.
# Checks for http/https, port numbers and trailing directories and strips them off.
# Returns the (domain, port) in a list.
sub server_domain_from_url($) {
  my $url = shift @_;
  $url =~ s/http:\/\///;
  $url =~ s/https:\/\///;

  my @parts = split("/",$url);
  my ($domain, $port) = split_port_from_domain($parts[0]);
  return ($domain, $port);
}

# Append the CPU info.
sub append_cpu_info() {
  print "------------------------------------------------------------------------------------\n";
  print "CPU Info\n";
  print "------------------------------------------------------------------------------------\n";
  print `cat /proc/cpuinfo`;
  print "------------------------------------------------------------------------------------\n";
}

# Append the environment variables.
sub append_environment_variables() {
  print "------------------------------------------------------------------------------------\n";
  print "Environment Variables\n";
  print "------------------------------------------------------------------------------------\n";
  print `printenv`;
  print "------------------------------------------------------------------------------------\n";
}

##########################################################
# The following "check_*" commands use the convention:
# Report a list of results of configuration items and error
#   items of the following form:
#     Config: configuration message
#     Error: specific error message
##########################################################

# Utility method to check a hostname for validity and return a config list
# including any possible problems.
sub check_hostname_validity($$$) {
  my $title = shift @_;
  my $hostname = shift @_;
  my $prefix = shift @_;
  my @result = ("Config: $prefix$title=$hostname");
  if ($hostname eq "") {
    push @result, "Error: Bad hostname: $title: empty hostname";
    return @result;
  }
  foreach(@bad_hostnames) {
    my $badhostname = $_;
    if ($hostname =~ /$badhostname/i) {
      push @result, "Error: Bad hostname: $title=$hostname is of the form $badhostname";
    }
  }
  push @result, check_dns($hostname, $prefix);

  return @result;
}

# Check for a valid hostnames.
# Using both "hostname" and "hostname -f".
sub check_hostname() {
  my $hostname = readpipe "hostname";
  chomp $hostname;
  # Official hostname is using -f
  my $hostname_full = readpipe "hostname -f";
  chomp $hostname_full;
  my @result = ();
  # hostname and hostname -f should return the same result
  if ($hostname ne $hostname_full) {
     push @result, "Error: hostname != hostname -f: This will break publishing: fix by using command: hostname $hostname_full";
  }
  push @result, check_hostname_validity("hostname -f", $hostname_full, "");

  return @result;
}

sub check_dnsdomainname() {
  my $result = readpipe "dnsdomainname";
  chomp $result;
  return ("Config: dnsdomainname=$result");
}

# Check the DNS value of a domain name.
# There should be no port number on the domain name.
sub check_dns($$) {
  my $hostname = shift @_;
  my $prefix = shift @_;

  my @result = ();
  if ($hostname eq "") {
    push @result, "Config: $prefix" . "DNS CHECK Problem: empty $hostname";
    push @result, "Error: DNS Problem: empty hostname";
    return @result;
  }

  # Parse result line from "host $hostname" command.

  my $ipaddress = FileUtils::get_nth_word("host $hostname|", 3);
  if ($ipaddress eq "found:") {
    push @result, "Config: $prefix" . "DNS CHECK Problem: host $hostname  not found";
    push @result, "Error: DNS Problem: host $hostname not found";
    return @result;
  }
  # get the hostname to make sure we're using the full name.
  $hostname = FileUtils::get_nth_word("host $hostname|", 0);

  # Check to see that it is reciprocal.
  my $hostname2 = FileUtils::get_nth_word("host $ipaddress|", 4);
  $hostname2 =~ s/\.$//;
  if ($hostname2 eq $hostname) {
    push @result, "Config: $prefix" . "DNS CHECK $hostname == host $ipaddress";

  } else {
    push @result, "Config: $prefix" . "DNS CHECK Problem: host $hostname != (host $ipaddress = $hostname2)";
    push @result, "Error: DNS Problem: host $hostname != (host $ipaddress = $hostname2)";
  }
  return @result;
}

# Do an nslookup on a given domain name.
# The $domain argument should not contain a port number.
# an optional 2nd argument is a prefix for padding the left side of the output.
sub check_nslookup($$) {
  my $domain = shift @_;
  my $prefix = shift @_;

  my $lookup = "nslookup $domain|";

  # Grep the nslookup results
  #nslookup cobraa
  #Server:         xxx.xx.x.x
  #Address:        xxx.xx.x.x#xx
  #
  #** server can't find cobraa: NXDOMAIN
  my @result = ();
  if (FileUtils::file_contains($lookup, "server can't find") == 1) {
    push @result, "Error: NSLookup $domain FAILED";
    push @result, "Config: $prefix" . "NSLookup $domain FAILED";
  } else {
    push @result, "Config: $prefix" . "NSLookup $domain succeeded";
  }
  return @result;
}

# Check the uptime on this machine.
sub check_uptime() {
  my $uptime = `uptime`;
  chomp $uptime;
  my @result = ("Config: uptime=$uptime");
  return @result;
}

# Check that localhost is defined in /etc/hosts on this machine.
sub check_localhost() {
  my $lines = `grep localhost /etc/hosts`;
  my @result = ();
  my @parts = split(/\n/, $lines);
  my $found = 0;
  foreach(@parts) {
     my $line = $_;
     push @result, "Config: localhost from /etc/hosts = $line";
     if ($line =~ /127.0.0.1/) {
       $found = 1;
     }
  }
  if ($found == 0) {
    push @result, "Error: localhost with 127.0.0.1 not defined in /etc/hosts";
  }
  return @result;
}

# Check the memory available.
sub check_memory() {
  my $memory = readpipe "cat /proc/meminfo | grep MemTotal";
  chomp $memory;
  # Parse Line:
  # MemTotal:      3982324 kB
  my @parts = split(/[ ]+/, $memory);
  my @result = ();
  if (@parts <= 0) {
    push @result, "Error: Unable to find MemTotal in /proc/meminfo";
    return @result;
  }
  $memory = int($parts[1]);
  my $size = lc($parts[2]);
  $memory /= 1000 if ($size eq "kb"); # Want results in MB.

  push @result, "Config: physical memory=$memory MB";
  return @result;
}

# Check the linux distro.
sub check_linux_distribution() {
  my $info = "Config: Linux Distribution\n";
  my @result = ();
  if (!open(ETC_RELEASE, "cat /etc/*release |")) {
    push @result, "Error: error getting Linux Distribution from /etc/*release";
    return @result;
  }
  while(<ETC_RELEASE>) {
    $info .= "  " . $_;
  }
  close(ETC_RELEASE);
  push @result, "Config: $info";
  return @result;
}

# Check the disk for a given file.
sub config_disk_info($$) {
  my $title = shift @_;
  my $filename = shift @_;

  my @result = ();

  # set the columns large to handle large filenames
  $ENV{"COLUMNS"} = "300";

  # Get the disk volume information for this directory.
  # TODO: perhapsr add Logical Volume.
  my ($volume, $file_system, $format, $size, $used, $available, $used_percent) =
    get_disk_volume_info($filename);

  # Create a single "Config" line with all this info.
  my $info = <<EOF;
$title=$filename
    Partition=$file_system
    Mount Volume=$volume
    Format=$format
    Size=$size
    Available space=$available
    Used space=$used
EOF
  push @result, "Config: $info";
  return @result;
}

# Check the port status for various desired ports.
sub check_port_status() {
  my @ports = (80, 443, 13031, 13032, 13033);
  my $info = "";
  foreach(@ports) {
    my $port = $_;
    my $port_status = readpipe "netstat -ln | grep \":$port \"";
    chomp $port_status;
    if (!defined $port_status || $port_status eq "") {
      $info = $info . "  $port not open\n";
    } else {
      $info = $info . "  " . $port_status . "\n";
    }
  }
  my $ports_joined = join(", ", @ports);
  my @results = ("Config: netstat -ln for ports $ports_joined\n$info");
  return @results;
}

# Append some useful system files.
sub append_network_files() {
  my @files = ("/etc/hosts", "/etc/resolv.conf", "/etc/nsswitch.conf");
  foreach(@files) {
    print "------------------------------------------------------------------------------------\n";
    print "SYSTEM FILE [$_]\n";
    print "------------------------------------------------------------------------------------\n";
    print `cat $_`;
    print "------------------------------------------------------------------------------------\n";
  }
}

#Check to see if selinux is there and if it's running.
sub check_selinuxstatus() {
  my $which_results = readpipe "which sestatus";
  chomp $which_results;
  if ($which_results && -x "$which_results") {
    my $result = readpipe "$which_results | grep -i mode";
    chomp $result;
    return ("Config: selinuxstatus=$result");
  } else {
  return ("Info: sestatus not available.");
  }
}
# Check that Java is installed and targeted for same architecture as the server.
sub check_java() {
  my $gehttpd_info = `file /opt/google/gehttpd/bin/gehttpd`;
  my $fusion_bits = binary_bit_architecture('/opt/google/gehttpd/bin/gehttpd');
  my @results = ();
  my $java_line = `grep JAVA_HOME /opt/google/getomcat/bin/setenv.sh`;
  chomp $java_line;


  if ($java_line eq '') {
    push @results, "Error: Java not installed";
    push @results, "Config: Java Install = Error: not found";
    return @results;
  }
  my @parts = split(/=/, $java_line);
  my $java = $parts[1] . '/bin/java';

  my $java_bits = binary_bit_architecture($java);
  if ($fusion_bits eq $java_bits) {
    push @results, "Config: Java Install = $java and Fusion Server are both $fusion_bits";
  } else {
    push @results, "Config: Java Install = Error: $java is $java_bits and Fusion Server is $fusion_bits";
    push @results, "Error: Java Install Error: $java is $java_bits and Fusion Server is $fusion_bits";
  }
  return @results;
}

END { }       # module clean-up code here (global destructor)

1;  # don't forget to return a true value from the file
