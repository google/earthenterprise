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

package FusionServerUtils;

use strict;
use warnings;
use FusionUtils;
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
    @EXPORT      = qw(&append_fusion_logs
                      &append_virtual_servers
                      &append_fusion_error_log
                      &append_published_dbs
                      &check_virtual_servers
                      &check_remote_servers
                      &check_maxpostsize
                      &check_tomcat
                      );
    %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],

    # your exported package globals go here,
    # as well as any optionally exported functions
    @EXPORT_OK   = qw();
}
our @EXPORT_OK;

# TODO determine if the Publisher httpd is authenticating or not.
sub is_authenticating() {
  return 1;
}

sub read_pipe_with_authentication($) {
  my $command = shift @_;
  # Some Fusion admin commands such as geserveradmin may require authentication.
  # for now, ask every time...if it's needed the script will pause and the user can
  # authenticate with the virtual server.
  print(STDERR "If geecheck script pauses here for more than a few seconds,\n" .
        "then enter the Fusion Server UserName: ");
  my $result = readpipe $command;
  print(STDERR "\n");
  return $result;
}

my %virtual_servers = ();
sub load_virtual_servers() {
  return if (keys(%virtual_servers) > 0);
  my $input = read_pipe_with_authentication("/opt/google/bin/geserveradmin --listvss");
  my @lines = split("\n", $input);

  foreach(@lines) {
    my $line = $_;
    my @parts = split("[,.] ", $line);
    if (@parts == 4) {
      $virtual_servers{$parts[1]}{"type"} = $parts[2];
      $virtual_servers{$parts[1]}{"url"} = $parts[3];
    }
  }
}

# Check the hostname, nslookup and splash screen for the specified server.
sub check_basic_server_info($$$) {
  my $server_domain = shift @_;
  my $port = shift @_;
  my $prefix = shift @_;
  my @results = ();
  # Do an NSLookup on the virtual server
  push @results, SystemUtils::check_hostname_validity("hostname", $server_domain, $prefix);
  push @results, SystemUtils::check_nslookup($server_domain, $prefix);
  # Check if server serves a splash screen.
  $server_domain .= ":$port" if ($port ne "");
  push @results, check_ge_splash_screen($server_domain, $prefix);
  return @results;
}

# Check that the virtual servers are reachable and log their configured urls.
sub check_virtual_servers() {
  load_virtual_servers();

  my $prefix = "  "; # padding for results readability.

  my @results = ("Config: ------------------------------------\nVIRTUAL SERVERS");
  for my $server (keys %virtual_servers) {
    my $url = $virtual_servers{$server}{"url"};
    if ($virtual_servers{$server}{"type"} eq "ge") {
      push @results, check_virtual_ge_server($server, $url, $prefix);
    } else {
      push @results, check_virtual_map_server($server, $url, $prefix);
    }

    # Do an NSLookup on the virtual server
    my ($server_domain, $port) = SystemUtils::server_domain_from_url($url);
    push @results, check_basic_server_info($server_domain, $port, $prefix . "  ");
  }
  # Check if localhost serves a splash screen.
  push @results, check_ge_splash_screen("localhost", "");
  push @results, "Config: ------------------------------------";
  return @results;
}

# Check that the dbRoot is accessible.
sub check_virtual_ge_server($$$) {
  # To test that a database is published and HTTPS mode is working.
  my $name = shift @_;
  my $url = shift @_;
  my $prefix = shift @_;

  my $option = "";
  if ($url =~ /^https/) {
    $option = "--no-check-certificate";
  }
  # Unfortunately, gedumpdbroot is not accessible at customer sites
  my $tempfile = "/tmp/geecheck-temp.dat";

  `rm $tempfile` if (-e$tempfile); # Make sure our test is fresh.
  # wget with 1 try and no stderr.
  `wget -t 1 -O $tempfile -o /dev/null $option $url/dbRoot.v5`;
  my @results = ();
  if (-e$tempfile) {
    push @results, "Config: $prefix" . "DBROOT [$name]: $url/dbRoot.v5 is accessible";
  } else {
    push @results, "Error: attempt to get $url/dbRoot.v5 failed";
    push @results, "Config: $prefix" . "DBROOT $name : $url/dbRoot.v5 is NOT accessible";
  }
  `rm $tempfile` if (-e$tempfile); # Cleanup our test.
  return @results;
}

# Check all non-local servers from server associations manager.
my %server_associations = ();
sub load_server_associations() {
  return (1, "") if (keys(%server_associations) > 0);

  my $filename = FusionUtils::asset_root() . "/.userdata/serverAssociations.xml";

  if (!-e$filename) {
    return (0, "Error: No Server Associations found at $filename");
  }

  if (!open(FILE, $filename)) {
    return (0, "Error: No Server Associations found, unable to open $filename");
  }
  my $nickname;
  my $type;
  my $in_stream = 0;
  my $stream_url;
  my $stream_vs;
  my $search_url;
  my $search_vs;
  while(<FILE>) {
    my ($element, $value) = FileUtils::get_xml_element_from_line($_);
    if ($element eq "end:combination") {
      # Create the server association instance.
      $server_associations{$nickname}{"type"} = $type;
      $server_associations{$nickname}{"streamurl"} = $stream_url . "/" . $stream_vs;
      $server_associations{$nickname}{"searchurl"} = $search_url . "/" . $search_vs;
    }
    $nickname = $value if ($element eq "nickname");
    $type = ($value eq "TYPE_GE" ? "GE3D" : "2D MAP") if ($element eq "type");
    $in_stream = 1 if ($element eq "stream");
    $in_stream = 0 if ($element eq "end:stream");
    if ($in_stream == 1) {
      $stream_url = $value if ($element eq "url");
      $stream_vs = $value if ($element eq "vs");
    } else {
      # In Search
      $search_url = $value if ($element eq "url");
      $search_vs = $value if ($element eq "vs");
    }
  }
  close(FILE);
  return (1, "");
}

# Pull out remote servers from the server associations file.
# Check each REMOTE server depending on type.
sub check_remote_servers() {
  my ($status, $message) = load_server_associations();
  my @results = ();
  if ($status == 0) {
    push @results, $message;
    return @results;
  }

  my $prefix = "  "; # padding for results readability.
  # Pull out servers from the following XML format file.
  # Check each server depending on type.
  push @results, "Config: ------------------------------------\nServer Associations: ";
  for my $server (keys %server_associations) {
    my $type = $server_associations{$server}{"type"};
    my $url = $server_associations{$server}{"streamurl"};
    my $searchurl = $server_associations{$server}{"searchurl"};

    my ($server_domain, $port) = SystemUtils::server_domain_from_url($url);

    # Do an NSLookup on the virtual server
    if (FusionUtils::is_localhost($server_domain) == 0) {
      push @results, "Config:   REMOTE $type Server [$server]: $server_domain";
      push @results, "Config:      stream url: $url";
      push @results, "Config:      search url: $searchurl";
      if ($type eq "GE3D") {
        # Search
        push @results, check_virtual_ge_server($server . " stream:", $url, $prefix . "  ");
      } else {
        push @results, check_virtual_map_server($server . " stream:", $url, $prefix . "  ");
      }
      # TODO check searchurl : but for what info?

      push @results, check_basic_server_info($server_domain, $port, $prefix . "  ");
    } else {
      # Log basic data
      push @results, "Config:   LOCAL $type Server $server: $server_domain";
      push @results, "Config:      stream url: $url";
      push @results, "Config:      search url: $searchurl";
    }
  }
  push @results, "Config: ------------------------------------";

  return @results;
}

# Check the tile request URLs for maps databases (possible cause of blank tiles?).
# The tests for 3.2+ are different than 3.1.X and earlier.
sub check_virtual_map_server($$$) {
  # To test that a database is published and HTTPS mode is working.
  my $name = shift @_;
  my $url = shift @_;
  my $prefix = shift @_;
  if ($main::version ge "3.2") {
    check_virtual_map_server_3_2($name, $url, $prefix);
  } else {
    check_virtual_map_server_3_1($name, $url, $prefix);
  }
}

# Check the tile request URLs for maps databases (possible cause of blank tiles?).
sub check_virtual_map_server_3_1($$$) {
  # To test that a database is published and HTTPS mode is working.
  my $name = shift @_;
  my $url = shift @_;
  my $prefix = shift @_;
  my $option = "";
  if ($name =~ /^https/) {
    $option = "--no-check-certificate";
  }

  # wget http://myserver/query?request=LayerDefs
  my @results = ();
  # wget with 1 try and no stderr.
  if (!open(FILE, "wget -t 1 -O - -o /dev/null $option $url/query?request=LayerDefs|")) {
    push @results, "Error: unable to download $url/query?request=LayerDefs";
    return @results;
  }

  my $map_is_valid = 0;
  my $uses_google_layers = 0;
  my @results_inner = ();
  while(<FILE>) {
    # Strip trailing whitespace, tabs, quotes and newlines.
    s/^\s+//g;
    s/\t//g;
    s/\n//g;
    s/"//g;
    my $line = $_;
    $map_is_valid = 1 if ($line =~ /var stream_url/);

    if ($line =~ m/var stream_url = (.*);/) {
      push @results_inner, "Config: $prefix" . "    url=$1";
    } elsif ($line =~ m/&channel=([0-9]+)/) {
      push @results_inner, "Config: $prefix" . "    channel=$1";
    } elsif ($line =~ m/&version=([0-9]+?)/) {
      push @results_inner, "Config: $prefix" . "    version=$1";
    } elsif ($line =~ m/var include_google_layers = true/) {
      $uses_google_layers = 1;
    }
  }
  close(FILE);
  if ($map_is_valid == 1) {
    push @results, "Config: $prefix" . "MAP DB [$name]: $url/query?request=LayerDefs contacted successfully";
    push @results, @results_inner;
  } else {
    push @results, "Error: attempt to get $name : $url/query?request=LayerDefs failed";
  }

  # Check the maps html to determine if we are using localmaps or Geo Database.
  $map_is_valid = 0;
  # wget with 1 try and no stderr.
  if (!open(FILE, "wget -t 1 -O - -o /dev/null $option $url|")) {
    push @results, "Error: unable to download $url";
    return @results;
  }

  # Can check the html for the correct javascript files in the html.
  $url = $url . "/";
  my $google_script_prefix = "<script src=\"http://maps.google.com/maps";
  my $script_prefix = "<script src=\"";
  my $localmaps_script = "fusionmaps_local.js";
  my $in_comment = 0;
    while(<FILE>) {
    if (/<!--/) {
      $in_comment = 1;
    }
    if ($in_comment == 0) {
      if (/$google_script_prefix/) {
        $map_is_valid = 1;
        if ($uses_google_layers == 0) {
          push @results, "Error: MAPS DB references maps.google.com javascript even though it's not using Geo Database";
        }
        push @results, "Config: $prefix" . "  $name using maps.google.com javascript";
      } elsif (/$localmaps_script/ && /$script_prefix/) {
        $map_is_valid = 1;
        if ($uses_google_layers == 1) {
          push @results, "Error: MAPS DB references fusionmaps_local.js even though it's using Geo Database";
        }
        push @results, "Config: $prefix" . "  $name using fusionmaps_local.js";
      }
    }
    if (/-->/) {
      $in_comment = 0;
    }
  }
  close(FILE);
  if ($map_is_valid == 1) {
    push @results, "Config: $prefix" . "  MAP DB $name : $url contacted successfully";
  } else {
    push @results, "Error: Did not find either maps.google.com javascript or fusionmaps_local.js in $url";
  }
  return @results;
}

# Check the tile request URLs for maps databases (possible cause of blank tiles?).
sub check_virtual_map_server_3_2($$$) {
  # To test that a database is published and HTTPS mode is working.
  my $name = shift @_;
  my $url = shift @_;
  my $prefix = shift @_;
  my $option = "";
  if ($name =~ /^https/) {
    $option = "--no-check-certificate";
  }

  # wget http://myserver/query?request=Json
  my @results = ();
  # wget with 1 try and no stderr.
  if (!open(FILE, "wget -t 1 -O - -o /dev/null $option $url/query?request=Json|")) {
    push @results, "Error: unable to download $url/query?request=Json";
    return @results;
  }

  my $map_is_valid = 0;
  my $uses_google_layers = 0;
  my @results_inner = ();
  while(<FILE>) {
    # Strip trailing whitespace, tabs, quotes and newlines.
    s/^\s+//g;
    s/\t//g;
    s/\n//g;
    s/"//g;
    my $line = $_;
    $map_is_valid = 1 if ($line =~ /isAuthenticated/);

    if ($line =~ m/serverUrl : (.*)/) {
      push @results_inner, "Config: $prefix" . "    url=$1";
    } elsif ($line =~ m/projection : (.*)/) {
      push @results_inner, "Config: $prefix" . "    projection=$1";
    } elsif ($line =~ m/useGoogleLayers : true/) {
      $uses_google_layers = 1;
    }
  }
  close(FILE);
  if ($map_is_valid == 1) {
    push @results, "Config: $prefix" . "MAP DB [$name]: $url/query?request=Json contacted successfully";
    push @results, @results_inner;
  } else {
    push @results, "Error: attempt to get $name : $url/query?request=Json failed";
  }

  # Check the maps html to determine if we are using localmaps or Geo Database.
  $map_is_valid = 0;
  # wget with 1 try and no stderr.
  if (!open(FILE, "wget -t 1 -O - -o /dev/null $option $url|")) {
    push @results, "Error: unable to download $url";
    return @results;
  }

  # Can check the html for the correct javascript files in the html.
  $url = $url . "/";
  my $google_script_prefix = "src=\"http://maps.google.com/maps";
  my $script_prefix = "<script ";
  my $localmaps_script = "fusionmaps_local.js";
  my $in_comment = 0;
    while(<FILE>) {
    if (/<!--/) {
      $in_comment = 1;
    }
    if ($in_comment == 0) {
      if (/$google_script_prefix/) {
        $map_is_valid = 1;
        if ($uses_google_layers == 0) {
          push @results, "Error: MAPS DB references maps.google.com javascript even though it's not using Geo Database";
        }
        push @results, "Config: $prefix" . "  $name using maps.google.com javascript";
      } elsif (/$localmaps_script/ && /$script_prefix/) {
        $map_is_valid = 1;
        if ($uses_google_layers == 1) {
          push @results, "Error: MAPS DB references fusionmaps_local.js even though it's using Geo Database";
        }
        push @results, "Config: $prefix" . "  $name using fusionmaps_local.js";
      }
    }
    if (/-->/) {
      $in_comment = 0;
    }
  }
  close(FILE);
  if ($map_is_valid == 1) {
    push @results, "Config: $prefix" . "  MAP DB $name : $url contacted successfully";
  } else {
    push @results, "Error: Did not find either maps.google.com javascript or fusionmaps_local.js in $url";
  }
  return @results;
}

# Check Localhost for the GE Splash screen.
sub check_ge_splash_screen($$) {
  my $hostname = shift @_;
  my $prefix = shift @_;
  # grep for:
  # <img src="ge-server.png"><br><br>
  my $target = 'img src="ge-server.png';
  # wget with 1 try and no stderr.
  my $input = `wget -t 1 -o /dev/null -O - http://$hostname/ | grep '$target'`;
  chomp $input;
  my @result = ();
  if ($input =~ m/$target/) {
    push @result, "Config: $prefix" . "http://$hostname/ responds with the GE Splash Screen";
    return @result;
  }
  push @result, "Error: $prefix" . "http://$hostname/ DOES NOT RESPOND with the GE Splash Screen";
  push @result, "Config: $prefix" . "http://$hostname/ DOES NOT RESPOND with the GE Splash Screen";
  return @result;
}

# Check our tomcat server response.
sub check_tomcat($$) {
  my $hostname = shift @_;
  my $prefix = shift @_;
  my $url = "http://$hostname/StreamPublisher/StreamPublisherServlet?Cmd=Ping&Host=$hostname";
  my $input = `curl -s "$url" | grep "Gepublish-StatusCode:0"`;
  chomp $input;
  my @result = ();
  if ($input eq '') {
    push @result, "Error: invalid response from $url";
    push @result, "Config: $prefix" . ": invalid response from $url";
  }
  else {
    push @result, "Config: $prefix" . ": Valid response from $url";
  }
  return @result;
}

# Check for cause of some publisher problems where maxPostSize default causes
# large publishes to fail.
sub check_maxpostsize() {
  my $filename = "/opt/google/getomcat/conf/server.xml";
  my @results = ();

   if (!open(FILE, $filename)) {
     push @results, "Error: unable to read $filename";
     return @results;
   }

   # Can check the html for the correct javascript files in the html.
   my $connector = "<Connector port=\"8009\"";
   my $end_connector = "/>";
   my $maxpostsize = "maxPostSize=\"([0-9]+)\"";
   my $in_connector = 0;
   my $maxpostsize_found = 0;
   my $size = 0;
   while(<FILE>) {
     if (/$connector/) {
       $in_connector = 1;
     }
     if ($in_connector == 1) {
       if (/$maxpostsize/) {
         $size = $1;
         $maxpostsize_found = 1;
       }
     }
     if (/$end_connector/) {
       $in_connector = 0;
     }
   }
   close(FILE);
   if ($maxpostsize_found == 1) {
     push @results, "Config: Tomcat maxPostSize=$size";
     if ($size != 0) {
       push @results, "Error: Tomcat configuration issue: consider setting maxPostSize=\"0\" for Connector port 8009 in $filename, currently it is set to $size";
     }
   } else {
     push @results, "Config: Tomcat maxPostSize=default setting";
     push @results, "Error: Tomcat configuration issue: consider setting maxPostSize=\"0\" for Connector port 8009 in $filename";
   }
   return @results;

}
# Append the list of Fusion published databases.
sub append_published_dbs() {
  my $input = read_pipe_with_authentication("/opt/google/bin/geserveradmin --publisheddbs");
  print "------------------------------------------------------------------------------------\n";
  print "Published Databases\n";
  print "------------------------------------------------------------------------------------\n";
  print $input;
  print "------------------------------------------------------------------------------------\n";
}

# Append the list of Fusion published databases.
sub append_virtual_servers() {
  my $input = read_pipe_with_authentication("/opt/google/bin/geserveradmin --listvss");
  print "------------------------------------------------------------------------------------\n";
  print "Virtual Servers\n";
  print "------------------------------------------------------------------------------------\n";
  print $input;
  print "------------------------------------------------------------------------------------\n";
}

# Append the tails of various fusion logs.
sub append_fusion_logs() {
  my @log_files = ("/opt/google/gehttpd/logs/access_log",
                   "/opt/google/gehttpd/logs/error_log",
                   "/opt/google/getomcat/logs/ge_stream_publisher.out",
                   "/opt/google/getomcat/logs/ge_search_publisher.out",
                   "/var/opt/google/log/gesystemmanager.log",
                   "/var/opt/google/log/geresourceprovider.log");
  # Publisher logs are optional, but they may be in .fusion or tmp.
  my @publisher_logdirs = ($ENV{"HOME"} . "/.fusion", $ENV{"TMPDIR"}, "/tmp");
  foreach(@publisher_logdirs) {
    if (defined $_) {
      my $files = `ls -1 $_/gepublishdatabase*.log`;
      my @parts = split("\n", $files);
      foreach(@parts) {
         my $log = $_;
         push @log_files, $log if (-e$log);
      }
    }
  }

  my $length = 20;
  foreach(@log_files) {
    print "------------------------------------------------------------------------------------\n";
    print "LOG TAIL [$_]\n";
    print "------------------------------------------------------------------------------------\n";
    print `tail -$length $_`;
    print "------------------------------------------------------------------------------------\n";
  }
}

# Append the tails of a specific error logs for a specified date where
# date is of the form YYYY-MM-DD or "latest" to indicate the last error.
sub append_fusion_error_log($) {
  my $date = shift @_;
  my $log_file = "/opt/google/getomcat/logs/localhost.$date.log";
  if ($date eq "latest") {
    # Pick out the last file from the directory listing.
    my @files = `ls -1 /opt/google/getomcat/logs/localhost.*.log`;
    return if (@files == 0);
    $log_file = $files[$#files];
    chomp $log_file;
  }

  print "------------------------------------------------------------------------------------\n";
  print "ERROR LOG [$log_file]\n";
  print "------------------------------------------------------------------------------------\n";
  print `cat $log_file`;
  print "------------------------------------------------------------------------------------\n";
}


END { }       # module clean-up code here (global destructor)

1;  # don't forget to return a true value from the file


