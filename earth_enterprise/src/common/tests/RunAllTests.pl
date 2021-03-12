#!/usr/bin/perl -w-
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


use FindBin qw($Bin $Script);
use File::Basename;
use Cwd 'abs_path';
use strict;
use warnings;

#if we get a -o argument, the subsequent argument will be used as an OS label for the xml
#if there is no subsequent argument, $OSLABEL will remain blank. Note this doesn't
#validate the content of the label itself.  This enables jenkins to classify tests by OS
#platform in the test output, which looks much nicer and is easier to navigate.
my $OSLABEL = "";

#set to 1 to include stdout output for non-error tests
my $VERBOSE = 0;

# List tests that should be skipped here:
my %DISABLE_TESTS;

for my $name ('parse-raster-project-xml-no-content_unittest') {
  $DISABLE_TESTS{$name} = 1;
}

sub usage() {
  print "USAGE:  RunAllTests.pl [OPTIONAL ARGUMENTS]\n";
  print "\tOPTIONAL ARGUMENTS: \n";
  print "\t\t-o OSLABEL\n";
  print "\t\t-v verbose test output for non-failure cases\n";
  print "\t\t-h display this message\n";
  exit(0);
}

my $argc = scalar(@ARGV);
if($argc > 0) {
  foreach my $i (0 .. ($argc - 1)) {
    if($ARGV[$i] eq "-o") {
      $OSLABEL = $ARGV[$i + 1].".";
    }
    elsif($ARGV[$i] eq "-v") {
      $VERBOSE = 1;
    }
    elsif($ARGV[$i] eq "-h") {
      usage();
    }
  }
}

# This script should be run from <build_dir>/bin/tests
my $scriptDir = dirname(abs_path($0));
if ($scriptDir !~ /[a-zA-Z0-9\._-]*\/bin\/tests$/ or not -e "$scriptDir/../../gee_version.txt") {
    print "This script should be run from <build_dir>/bin/tests.\n";
    print "<build_dir> should contain gee_version.txt.\n";
    exit;
}
chdir($scriptDir);

# open an output file, cannot have "test" (lowercase) in its name
my $logfile = "$scriptDir/Output.xml";
open(my $fh, '>', $logfile) or die $!;

# Add path to build binary directory for searching executables (ogr2ogr,
# gdal_rasterize,...) used in tests.
$ENV{'PATH'} = join ":", "..", $ENV{'PATH'};

# Find binary names that contain 'test' in their name, but don't end in
# '_resourcetest':
my @tests =
  grep { $_ !~ /_resourcetest$/ && $_ !~ /.yaml$/ && $_ ne './generate_test_asset_xmls.py' }
  glob("./*test*");

my $longest = 0;
foreach my $test (@tests) {
    my $basename = basename($test);
    next if $basename eq $Script;
    if (length($basename) > $longest) {
        $longest = length($basename);
    }
}

#xml headers
print $fh "<?xml version=\"1.0\" ?>\n";
print $fh "<testsuites>\n";

$| = 1;
my $result = 0;

#for each file with the word "test" in its name in the current directory
foreach my $test (@tests) {
    my $basename = basename($test);
    next if $basename eq $Script;
    print "Running $basename ... ", ' 'x($longest-length($basename));

    if ($DISABLE_TESTS{$basename}) {
      print "DISABLED\n";
      next;
    }

    #set up xml tags for this file
    my $testsuite = "\t<testsuite ";
    my $stdout_data = "\t\t\t<system-out>\n";
    my $stderr_data = "\t\t\t<system-err>\n";
    my $test_count = 0;
    my $failures = 0;
    my $errors = 0;
    my $test_output = "";
    my @testcases;
    my $prev = "";

    if (open(OUTPUT, "LD_LIBRARY_PATH=\"../../lib:../../gehttpd/lib\" $test 2>&1 |")) {
        while (<OUTPUT>) {
            my $line = $_;

            #save test output for jenkins's benefit
            $test_output .= $line;

            #there are a lot of these from some of the test files and we don't care about them
            if ($line =~ /^Fusion Notice:/) {
              next;
            }

            #Are these important or are these warnings normal for the types of tests being run? TODO
            if ($line =~ /^Fusion Warning:/) {
              $errors++;
            }

            #is this a test case with an individual result?
            if ($line =~ /OK|succeeded|FAILED/i && $line !~ /[0-9]+ of [0-9]+/) {
              $test_count++;
              my $ms = 0.0;

              #count failures
              if($line =~ /FAILED/) {
                $failures++;
              }

              #capture the test name from successes, which come in 2 different formats
              $line =~ s/^\[.+\]\s+//;
              if ($line =~ /^(\[.+\]\w+)?(\w+)(\s+succeeded)$/) {
                $line = $2;
              }
              elsif ($line =~ /(.+)\s+\((\d+)\s+ms\).*$/) {
                $line = $1;
                $ms = $2;
              }

              #find class name if possible.. If not, use the filename
              my $classname = $basename;
              my $testname = $line;
              if($line =~ /(\w+)\.(\w+)/) {
                $classname = $1;
                $testname = $2;
              }
              next if($prev && "$classname.$testname" eq $prev);
              $prev = "$classname.$testname";

              #convert milliseconds to seconds for junit
              $ms *= 0.001;

              #add the xml to the list of testcases for this file
              chomp($testname);
              push(@testcases, "\t\t<testcase classname=\"$OSLABEL$classname\" name=\"$testname\" time=\"$ms\">\n");
            }

        }
        if (close(OUTPUT)) {
            $testsuite .= "errors=\"$errors\" failures=\"$failures\" ";
            print "SUCCEEDED\n";
        } else {
            $testsuite .= "errors=\"$errors\" failures=\"$failures\" ";
            $result = 1;
            print "FAILED\n";
            print "----------\n";
            print $test_output;
            print "----------\n";
        }
    } else {
        $stderr_data .= "FAILED to launch\n";
        $testsuite .= "errors=\"1\" failures=\"1\"";
        $failures = 1;
        print "FAILED to launch\n";
    }

    #if we didn't count any subtests, this should not be 0
    if($test_count == 0) {
      $test_count = 1;
    }

    ### print xml for the test file we just ran
    $testsuite .= " name=\"$basename\" tests=\"$test_count\">\n";

    #if there was a failure, save the output in the stderr-data xml block
    if($failures > 0) {
      $stderr_data .= $test_output;
      $stdout_data .= "\n\t\t\t</system-out>\n";
      $stderr_data .= "\n\t\t\t</system-err>\n";
    }
    else {
      if($VERBOSE == 1) {
        $stdout_data .= $test_output;
        $stdout_data .= "\n\t\t\t</system-out>\n";
      }
      else {
        $stdout_data = "";
      }
      $stderr_data = "";
    }

    print $fh $testsuite;

    #print xml for each testcase inside this testsuite
    foreach my $testcase (@testcases) {
      print $fh $testcase;
      print $fh $stdout_data;
      print $fh $stderr_data;
      print $fh "\t\t</testcase>\n";
    }
    print $fh "\t</testsuite>\n";
}

#close the top level xml tag
print $fh "</testsuites>\n";
close($fh);

exit $result;
