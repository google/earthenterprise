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

use strict;
use Getopt::Long;
use FindBin;

my %bbox;
my $imgfile;
my $help = 0;
my $pixeledge;

sub usage
{
    die "usage: $FindBin::Script --pixeledge|--pixelcenter --north <lat> --west <long> --south <lat> --east <long> <imgfile>\n";
}

sub LongitudeArg($$) {
    my ($name, $val) = @_;
    die "Invalid longitude '$val' specified for $name\n"
	if ($val < -180.0 || $val > 180.0);
    $bbox{$name} = $val;
}

sub LatitudeArg($$) {
    my ($name, $val) = @_;
    die "Invalid longitude '$val' specified for $name\n"
	if ($val < -90.0 || $val > 90.0);
    $bbox{$name} = $val;
}

GetOptions("north=f"     => \&LatitudeArg,
	   "south=f"     => \&LatitudeArg,
	   "east=f"      => \&LongitudeArg,
	   "west=f"      => \&LongitudeArg,
	   "pixeledge"   => sub { $pixeledge = 1 },
	   "pixelcenter" => sub { $pixeledge = 0 },
           "help|?"      => \$help,
           ) || usage();
usage() if $help;
usage() unless defined($pixeledge);
$imgfile = shift;
usage() unless defined($imgfile);
die "Unable to read $imgfile\n" unless -r $imgfile;
my $imgroot = $imgfile;
$imgroot =~ s/\.[^\.]+$//;

usage() unless (exists($bbox{north}) && exists($bbox{south}) &&
		exists($bbox{east}) && exists($bbox{west}));

# pull them out of hash for syntactical convenience
my $north = $bbox{north};
my $south = $bbox{south};
my $east  = $bbox{east};
my $west  = $bbox{west};

print "north = $north\n";
print "south = $south\n";
print "east  = $east\n";
print "west  = $west\n";

die "north ($north) <= south ($south). Invalid region.\n" if ($north<=$south);
die "east ($east) == west ($west). Invalid region.\n" if ($east==$west);

if ($east < $west) {
    warn "east ($east) < west ($west). Assuming region crosses dateline.\n";
    $east += 360.0;
}

# get the image size from gdalinfo
my $width;
my $height;
open(GDAL, "gdalinfo -nomd -nogcp $imgfile |");
while (<GDAL>) {
    if (/^Size is\s+(\d+),\s+(\d+)/) {
	$width = $1;
	$height = $2;
	last;
    }
}
close(GDAL);
die "Unable to determine width & height from image file.\n"
    unless (defined($width) && defined($height));

my $pixelWidth;
my $pixelHeight;

# calculate the pixel width/height
if ($pixeledge) {
    $pixelWidth = ($east - $west) / $width;
    $pixelHeight = ($north - $south) / $height;
} else {
    $pixelWidth = ($east - $west) / ($width-1);
    $pixelHeight = ($north - $south) / ($height-1);
}
print "pixelWidth = $pixelWidth\n";
print "pixelHeight = $pixelHeight\n";

# write .wld file
my $wldfile = $imgroot . '.wld';
open(WLD, "> $wldfile") || die "Unable to write to $wldfile: $!\n";
printf WLD "%.10f\n", $pixelWidth;
printf WLD "0.0000000000\n";
printf WLD "0.0000000000\n";
printf WLD "%.10f\n", -$pixelHeight;
printf WLD "%.10f\n", $west + (0.5 * $pixelWidth);
printf WLD "%.10f\n", $north - (0.5 * $pixelHeight);
close(WLD);
