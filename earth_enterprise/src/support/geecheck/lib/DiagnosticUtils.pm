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
# Utilities to run generic diagnostic checks, compile the results
# and output the results.
# RunDiagnostics takes a version number and list of functions of the form:
#     @result = f(version)
#       where @result is an array of strings.
#         These strings must be prepended by:
#           "Error: " for error messages, or
#           "Config: " for configuration messages.
#
# OutputDiagnostics will make a nice printout of such results,
# showing errors followed by configuration.
package DiagnosticUtils;

use strict;
use warnings;

# module constructor
BEGIN {
    use Exporter   ();
    our ($VERSION, @ISA, @EXPORT, @EXPORT_OK, %EXPORT_TAGS);

    # set the version for version checking
    $VERSION     = 1.00;
    @ISA         = qw(Exporter);
    @EXPORT      = qw(&RunDiagnostics &OutputDiagnostics);
    %EXPORT_TAGS = ( );     # eg: TAG => [ qw!name1 name2! ],

    # your exported package globals go here,
    # as well as any optionally exported functions
    @EXPORT_OK   = qw();
}
our @EXPORT_OK;

# Runs a set of input checks and returns the result as
# an array of messages. Each message is prepended by
# "Error: " or "Config: " depending on the nature of the message.
sub RunDiagnostics($$@)
{
  # Run through all checks and tally up the messages.
  my $version = shift @_;
  my $uses_packages = shift @_;
  my @checks = @_;
  my @messages = ();
  foreach(@checks) {
    my @these_messages = &$_($version, $uses_packages);
    foreach(@these_messages) {
      # for debug
      # print(STDERR $_ . "\n");
    }
    push @messages, @these_messages if (@these_messages > 0);
  }
  return @messages;
}

# Take a list of diagnostic messages and print them to stdout.
# Print the Errors first, followed by the Config messages.
sub OutputDiagnostics(@) {
  my @messages = @_;
  # First count the results (there should be a more succinct way of doing this)..
  my @problem_messages = grep(/^Error:/, @messages);
  my @config_messages = grep(/^Config:/, @messages);
  # This nasty code strips the Config/Error prefixes off
  s/^Error: // for(@problem_messages);
  s/^Config: // for(@config_messages);

  my $problem_count = @problem_messages;
  my $config_count = @config_messages;

  # Print the problem messages first
  print "\n\n";
  print "------------------------------------------------------------------------------------\n";
  if ($problem_count > 0) {
    print "$problem_count errors found in the installation.\n";
    my $i = 1;
    print "------------------------------------------------------------------------------------\n";
    foreach(@problem_messages) {
      print $i . ". " . $_ . "\n";
      $i++;
    }
    print "------------------------------------------------------------------------------------\n";
  } else {
     print "Installation passes all checks.";
  }
  print "\n\n";

  # Print the configuration messages.
  print "------------------------------------------------------------------------------------\n";
  if ($config_count > 0) {
    print "Configuration Settings:\n";
    print "------------------------------------------------------------------------------------\n";
    print (join("\n", @config_messages));
  } else {
     print "No Configuration Settings found.";
  }
  print "\n------------------------------------------------------------------------------------\n";
  print "\n\n";
}


END { }       # module clean-up code here (global destructor)

## YOUR CODE GOES HERE

1;  # don't forget to return a true value from the file


