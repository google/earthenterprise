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
use POSIX;
use File::Basename;


sub usage() {
    die "usage: invmosaic <sourceimg>\n";
}

sub min($$) {
    return ($_[0] < $_[1]) ? $_[0] : $_[1];
}


my $numTilesX = 10;
my $overlap = 50;

my $source = shift;
usage() unless -r $source;

my ($srcbase, $srcdir, $srcsuffix) = fileparse($source, qr{\..*});

my $width = 0;
my $height = 0;

open(GDALINFO, "gdalinfo $source | ");
while (<GDALINFO>) {
    next unless /Size is (\d+), (\d+)/;
    $width = $1;
    $height = $2;
}
close(GDALINFO);

die "Unable to determine image size\n" if (($width == 0) || ($height == 0));

my $blockWidth  = POSIX::ceil(($width / $numTilesX)) + $overlap;
my $blockHeight = $blockWidth;
my $numTilesY   = POSIX::ceil($height / ($blockHeight - $overlap));



print "Source size   = ${width}x${height}\n";
print "New Tile size = ${blockWidth}x${blockHeight}\n";
print "Num Tiles     = ${numTilesX}x${numTilesY}\n";

my $y;
my $x;

for ($y = 0; $y < $numTilesY; ++$y) {
    my $yoff = $y * ($blockHeight - $overlap);
    for ($x = 0; $x < $numTilesX; ++$x) {
	my $xoff = $x * ($blockWidth - $overlap);
	my $sizex = min($blockWidth,  $width - $xoff);
	my $sizey = min($blockHeight, $height - $yoff);
	my $cmd = sprintf "gdal_translate -srcwin $xoff $yoff $sizex $sizey $source $srcdir$srcbase-p%02d,%02d.tif", $y, $x;
	print "$cmd\n";
	(system($cmd) == 0) || die "Error running gdal_translate\n";
    }
}
