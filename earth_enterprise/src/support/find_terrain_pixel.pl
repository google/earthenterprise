#!/usr/bin/perl
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

# Finds value of a terrain pixel from a terrain file with given longitude,
# latitude. For terrain file it supports all formats supported by gdal.

use File::Temp qw/ tempfile tempdir /;

if ($ARGV[0] =~ /^-h|^-help|^--help|^--usage/i or $#ARGV < 2) {
  print "\n\n  Usage: $0 terrain_file longitude latitude \n\n";
  print "Example: $0 S14W073.tif -72.96833332999999 -13.89\n";
  exit 0;
}

($terrain_file, $lon, $lat) = @ARGV;

# [1] Find pixel size of the terrain file using gdalinfo
open(INPUT, "gdalinfo $terrain_file |");
while ($line = <INPUT>) {
  if ($line =~ /^Pixel Size = \(([^,]+),([^)]+)/) {
    $width = "$1";
    $height = "$2";
    $height *= -1.0;
    last;
  }
}
close(INPUT);

# [2] Use gdalwarp to create a single pixel terrain file, centered at lat, lon
$half_width = $width / 2.0;
$half_height = $height / 2.0;
$lat_min = $lat - $half_width;
$lat_max = $lat + $half_width;
$lon_min = $lon - $half_height;
$lon_max = $lon + $half_height;

($fh, $tmp_file) = tempfile("single_pxl_XXXX", SUFFIX => ".tiff", UNLINK => 1);
`gdalwarp -r bilinear -ot Float32 -of GTiff -te $lon_min $lat_min $lon_max $lat_max $terrain_file $tmp_file`;

# [3] Use tiffinfo to get the value of this single pixel
open(INPUT, "tiffinfo -d $tmp_file 2>/dev/null |");
while (<INPUT>) {
  $line = "$_";
}
close(INPUT);
chomp($line);
$line =~ s/ //g;
# tiffinfo prints the value as hex bytes. This is the way to recreate the float.
$result = unpack "f", pack "H*", $line;
print "$result" . "\n";
