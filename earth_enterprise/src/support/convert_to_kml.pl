#!/usr/bin/perl
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
# This script converts a geo-coded image to regionated KML without having to
# import it to Fusion (and cluttering Fusion asset manager). It does the
# following steps:
# gerasterimport -> to convert image to kip
# gemaskgen      -> to create a automask (tiff) from the kip
# gerasterimport --alphamask -> to create kmp from the above tiff
# geraster2kml   -> to create regionated kml from kip and kmp
#
# The script also optionally allows to use more than one cpu. In that case the
# image is divided into num_cpu horizontal strips. And follows the above process# for each strip. Finally it creates a combined kml using referenced to these
# strips in a top level NetworkLink. To avoid artifacts in border the strips
# overlap by (5 + feather_pixel) pixels.

use strict;
use File::Temp qw/ tempfile tempdir /;
use File::Spec;
use Getopt::Long;

my $num_cpu = 1;
my $feather_pixel = 8;  # For default value we chose smallest 2's even power
# which meets our purpose to get the raster of interest, no feather generates
# black overlays in boundary and high value of feather loses data in boundary.
my $hole_size = 0;
my $tolerance = 2;

sub print_help();
sub print_help() {
  print "\n\n  Converts a geo-coded image to a kml directory\n";
  print "  Usage: $0 \n" .
        "        --imagery geocoded_image_file\n" .
        "        --output output_kml_dir\n" .
        "        [--num_cpu cpus_to_use($num_cpu)]\n" .
        "        [--feather feather_pixel($feather_pixel)]\n" .
        "        [--holesize hole_size_pixel($hole_size)]\n" .
        "        [--tolerance fill_tolerance($tolerance)]\n\n";
        "        [-- any_optional_argument_for_gemaskgen]\n\n";
  print "Example: $0 --imagery usgsLanSat.jp2 --output usgsLanSat.jp2.kml " .
        "--num_cpu 4 -- --band 2 --fillvalues \\'0:5\\' \n";
}

# Create a temporary directory with given prefix and given suffix.
# Return the full name of the created directory.
sub create_temp_dir($$);
sub create_temp_dir($$) {
  my ($prefix, $suffix) = @_;
  my $tmp_dir = tempdir($prefix . "XXXX", DIR => File::Spec->tmpdir());
  my $ret_tmp_dir = $tmp_dir . $suffix;
  if ($suffix) {
    rename $tmp_dir, $ret_tmp_dir;
  }
  return $ret_tmp_dir;
}

# Create a sub directory with given base_dir and given relative_dir.
# Return the full name of the directory
sub create_sub_dir($$);
sub create_sub_dir($$) {
  my ($base_dir, $relative_dir) = @_;
  my $dir_name = File::Spec->catfile($base_dir, $relative_dir);
  mkdir $dir_name;
  return $dir_name;
}

# Given a image file, find information about it when we reproject it into the
# given projection. Use the tmp_dir for any temporary file.
# Return width_pxl_orig, height_pxl_orig, x_min_orig,$y_max_orig,
#        degrees_per_pixel_orig
sub get_image_info_in_projection($$$);
sub get_image_info_in_projection($$$) {
  my ($input_img_file, $projection, $tmp_dir) = @_;
  my $tmp_vrt_file = File::Spec->catfile($tmp_dir, "raster.vrt");
  `gdalwarp -of VRT -t_srs $projection $input_img_file $tmp_vrt_file`;
  open(INPUT, "gdalinfo $tmp_vrt_file |");
  my $width_pxl_orig;
  my $height_pxl_orig;
  my $x_min_orig;
  my $y_max_orig;
  my $degrees_per_pixel_orig;
  my $found = 0;
  while (my $line = <INPUT>) {
    if ($line =~ /^Size is ([^,]+), ([^)]+)/) {
      $width_pxl_orig = "$1";
      $height_pxl_orig = "$2";
      $found = $found + 2;
    }

    if ($line =~ /^Origin = \(([^,]+),([^)]+)/) {
      $x_min_orig = "$1";
      $y_max_orig = "$2";
      $found = $found + 2;
    }
    if ($line =~ /^Pixel Size = \(([^,]+),([^)]+)/) {
      $degrees_per_pixel_orig = "$1";
      $found = $found + 1;
      last;
    }
  }
  close(INPUT);
  ($found eq 5) or
      die "Cannot get enough geographic information about $input_img_file";
  return ($width_pxl_orig, $height_pxl_orig, $x_min_orig, $y_max_orig,
      $degrees_per_pixel_orig);
}


# Create a common kml from the $num_cpu strips. The output KMLs are expected
# to be in directory output_kml_dir/1, output_kml_dir/2 ...
# output_kml_dir/num_cpu. The generated file will be output_kml_dir/index.kml.
sub MergeKmls($$);
sub MergeKmls($$) {
  my ($output_kml_dir, $num_cpu) = @_;
  my $file_name = "$output_kml_dir/index.kml";
  open(INDEX, ">$file_name") or die "Cannot open file '$file_name' ($!)";
  $file_name = "$output_kml_dir/1/index.kml";
  open(INPUT, "<$file_name") or die "Cannot open file '$file_name' ($!)";
  my $line;
  while ($line = <INPUT>) {
    if ($line =~ /^[\s]*\<NetworkLink\>/) {
      last;
    }
    print INDEX $line;
  }
  print INDEX "<Folder>\n";
  close INPUT;
  for (my $i = 1; $i <= $num_cpu; $i++) {
    $file_name = "$output_kml_dir/$i/index.kml";
    open(INPUT, "<$file_name") or die "Cannot open file '$file_name' ($!)";
    while ($line = <INPUT>) {
      if ($line =~ /^[\s]*\<NetworkLink\>/) {
        print INDEX $line;
        last;
      }
    }
    while ($line = <INPUT>) {
      if ($line =~ /(^[\s]*\<href\>)(.+)/) {
        print INDEX "$1" . "$i" . "/" . "$2" . "\n";
      } else {
        print INDEX $line;
      }
      if ($line =~ /^[\s]*<\/NetworkLink\>/) {
        last;
      }
    }
    close INPUT;
  }
  print INDEX "</Folder>\n";
  print INDEX "</kml>\n";
  close INDEX;
}

if ($ARGV[0] =~ /^-h|^-help|^--help|^--usage/i or $#ARGV < 0) {
  print_help;
  exit 0;
}

my ($input_img_file, $output_kml_dir);

unless (GetOptions(
    'i|imagery=s'  => \$input_img_file,
    'o|output=s'   => \$output_kml_dir,
    'n|num_cpu=i' => \$num_cpu,
    'f|feather=i' => \$feather_pixel,
    'h|holesize=i' => \$hole_size,
    't|tolerance=i' => \$tolerance
  )) {
  print_help;
  exit 1;
}

my $maskgen_options = "@ARGV";

# Set to 0 to see all the intermediate files; This is only for debugging and is
# not a user option.
my $unlink = 1;
# Not a user option. This is a constant, tweaked to a minimum value so that the
# boundary effects don't show up. The total overlap pixel is
# 4 + 1 + $feather_pixel
my $mask_guard_pixel = 4;
my $tmp_dir = create_temp_dir("convert_to_kml_", "");

if ($num_cpu eq 1) {
  my $tmp_kip_dir = create_sub_dir($tmp_dir, "raster.kip");
  my $tmp_kmp_dir = create_sub_dir($tmp_dir, "raster.kmp");
  my $tmp_mask_file = File::Spec->catfile($tmp_dir, "raster.tiff");

  my $cmd = "gerasterimport --imagery $input_img_file -o $tmp_kip_dir";
  $cmd .= ";\ngemaskgen -mask -feather $feather_pixel -holesize $hole_size " .
          "-tolerance $tolerance $maskgen_options $tmp_kip_dir $tmp_mask_file";
  $cmd .= ";\ngerasterimport --alphamask $tmp_mask_file --dataproduct " .
          "$tmp_kip_dir -o $tmp_kmp_dir";
  $cmd .= ";\nrm -rf $output_kml_dir";
  $cmd .= ";\ngeraster2kml --output $output_kml_dir --kip $tmp_kip_dir --kmp " .
          "$tmp_kmp_dir";
  print "$cmd\n";
  `$cmd`;
} else {
  my $projection = 'EPSG:4326';

  # [1] Find pixel size of the terrain file using gdalinfo
  my ($width_pxl_orig, $height_pxl_orig, $x_min_orig, $y_max_orig,
      $degrees_per_pixel_orig) = get_image_info_in_projection(
      $input_img_file, $projection, $tmp_dir);

  # [2] Find nearest but smaller size of Fusion pixel
  my $degrees_per_pixel = 360.00 / 256.0;
  while ($degrees_per_pixel > $degrees_per_pixel_orig) {
    $degrees_per_pixel /= 2.0;
  }

  my $degrees_per_half_pixel = $degrees_per_pixel / 2.0;
  my $x_min = $degrees_per_pixel * int(($x_min_orig - $degrees_per_half_pixel) /
              $degrees_per_pixel);  # floor
  my $y_max = $degrees_per_pixel * int(($y_max_orig + $degrees_per_half_pixel) /
              $degrees_per_pixel);  # ceil
  my $width_in_pixel = int(($width_pxl_orig * $degrees_per_pixel_orig +
      $degrees_per_half_pixel) / $degrees_per_pixel);  # ceil
  my $height_in_pixel = int(($height_pxl_orig * $degrees_per_pixel_orig +
      $degrees_per_half_pixel) / $degrees_per_pixel);  # ceil
  my $x_max = $x_min + $degrees_per_pixel * $width_in_pixel;
  my $y_min = $y_max - $degrees_per_pixel * $height_in_pixel;
  my $pixels_per_cpu = int($height_in_pixel / $num_cpu);
  my $y_max_i = $y_max;

  # Divide the original image into $num_cpu horizontal strips.
  # Process each strip in a separate process.
  for (my $i = 1; $i <= $num_cpu; $i++) {
    my ($y_min_i, $height_pixel_i, $y_min_exact_i);
    if ($i == $num_cpu) {
      $y_min_i = $y_min;
      $height_pixel_i = int(($y_max_i - $y_min_i) / $degrees_per_pixel + 0.5);
    } else {
      $y_min_exact_i = $y_max_i - $pixels_per_cpu * $degrees_per_pixel;
      $y_min_i = $y_min_exact_i - ($feather_pixel + $mask_guard_pixel) *
                 $degrees_per_pixel;
      $height_pixel_i = $pixels_per_cpu + ($feather_pixel + $mask_guard_pixel);
    }
    my $max_pixel_i = ($height_pixel_i > $width_in_pixel) ? $height_pixel_i
                                                          : $width_in_pixel;
    unless (my $child_pid = fork()) {
      $tmp_dir = create_sub_dir($tmp_dir, $i);
      $output_kml_dir = create_sub_dir($output_kml_dir, $i);
      my $tmp_kip_dir = create_sub_dir($tmp_dir, "raster.kip");
      my $tmp_kmp_dir = create_sub_dir($tmp_dir, "raster.kmp");
      my $tmp_mask_file = File::Spec->catfile($tmp_dir, "raster.tiff");

      my $boundary_arg = "";
      if ($i != $num_cpu) {
        # need to remove the feathering on lower boundary
        $boundary_arg = sprintf("--south_boundary %.20f ", $y_min_exact_i);
      }
      if ($i != 1) {
        # need to remove the feathering on upper boundary
        my $y_max_exact_i = $y_max_i - ($feather_pixel + $mask_guard_pixel) *
                            $degrees_per_pixel;
        $boundary_arg .= sprintf("--north_boundary %.20f ", $y_max_exact_i);
      }

      my $cmd = sprintf("gerasterimport --imagery $input_img_file " .
                        "--north_boundary %.20f --south_boundary %.20f -o " .
                        "$tmp_kip_dir", $y_max_i, $y_min_i);
      $cmd .= ";\ngemaskgen --mask --maxsizeinmem $max_pixel_i --maxsize " .
              "$max_pixel_i -feather $feather_pixel -holesize $hole_size " .
              "-tolerance $tolerance $maskgen_options $tmp_kip_dir " .
              "$tmp_mask_file";
      $cmd .= ";\ngerasterimport --alphamask $tmp_mask_file --dataproduct " .
              "$tmp_kip_dir -o $tmp_kmp_dir";
      $cmd .= ";\nrm -rf $output_kml_dir";
      $cmd .= (";\ngeraster2kml $boundary_arg --output $output_kml_dir --kip " .
               "$tmp_kip_dir --kmp $tmp_kmp_dir");
      if ($unlink) {
        $cmd .= ";\nrm -rf $tmp_dir";
      }
      print "$cmd\n";
      `$cmd`;
      exit 0;
    }
    $y_max_i = $y_min_exact_i + ($feather_pixel + $mask_guard_pixel) *
        $degrees_per_pixel;
  }

  # Reap all children
  while ((my $pid = wait()) != -1) {
  }
  MergeKmls($output_kml_dir, $num_cpu);
}

if ($unlink) {
  my $cmd = "rm -rf $tmp_dir";
  print "$cmd\n";
  `$cmd`;
}

exit(0);
