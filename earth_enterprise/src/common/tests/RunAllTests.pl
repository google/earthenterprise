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

# Add path to build binary directory for searching executables (ogr2ogr,
# gdal_rasterize,...) used in tests.
$ENV{'PATH'} = join ":", "$Bin/..", $ENV{'PATH'};

my @tests = glob("$Bin/*");

my $longest = 0;
foreach my $test (@tests) {
    my $basename = basename($test);
    next if $basename eq $Script;
    if (length($basename) > $longest) {
        $longest = length($basename);
    }
}

$| = 1;
my $result = 0;
foreach my $test (@tests) {
    my $basename = basename($test);
    next if $basename eq $Script;
    print "Running $basename ... ", ' 'x($longest-length($basename));
    if (open(OUTPUT, "$test 2>&1 |")) {
        my @output;
        while (<OUTPUT>) {
            push @output, $_;
        }
        if (close(OUTPUT)) {
            print "SUCCEEDED\n";
        } else {
            print "FAILED\n";
            print "----------\n";
            print @output;
            print "----------\n";
            $result = 1;
        }
    } else {
        print "FAILED to launch\n";
    }
}
exit $result;
