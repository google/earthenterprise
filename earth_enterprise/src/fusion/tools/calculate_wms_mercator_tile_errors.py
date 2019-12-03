#!/usr/bin/env python2.7
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
# Wms-plate-carree-as-Mercator-tile error measurement tool.
#
# Tool for showing the pixel errors associated with serving plate
# carree tiles as Mercator tiles if the tile northern and southern
# boundaries match those of the Mercator tile that they are impersonating.
# Plate carree tiles with Mercator bounds are typically requested
# from wms for linear sampling of the plate carree map being served.
#
# In short, it demonstrates for latitudes near the equator at low
# LODs, and for latitudes further from the equator at higher LODs,
# the errors become very small (less than 1 pixel), which is probably
# better than errors within the imagery itself.
#
# Typically, we measure error at the center of the tile, which is not
# the exact position of maximum error, but close enough.
#
# E.g.
# LOD 4
#   Tile at 0.0 degrees
#     Error at 0.50: 2.41357 pixels
#   Tile at 21.9 degrees
#     Error at 0.50: 6.59861 pixels
#   Tile at 41.0 degrees
#     Error at 0.50: 9.41641 pixels
# LOD 6
#   Tile at 0.0 degrees
#     Error at 0.50: 0.15400 pixels
#   Tile at 21.9 degrees
#     Error at 0.50: 1.30346 pixels
#   Tile at 41.0 degrees
#     Error at 0.50: 2.14443 pixels
# LOD 8
#   Tile at 0.0 degrees
#     Error at 0.50: 0.00964 pixels
#   Tile at 21.9 degrees
#     Error at 0.50: 0.30174 pixels
#   Tile at 41.0 degrees
#     Error at 0.50: 0.52049 pixels
#   Tile at 82.7 degrees
#     Error at 0.50: 0.77914 pixels

import math

# Range of LODs to calculate errors for.
START_LOD = 4
END_LOD = 9

# Define measurement locations within an LOD.
# Steps begin at center of map and move up.
# Mirror results for southern hemisphere.
# E.g. at LOD 4, the x and y dimensions are 16 tiles,
# so 8 steps would touch each tile from the equator to the poles.
# At LOD 5, we would check every other tile. And so on.
NUM_LOD_STEPS = 8

# Define measurement locations within a tile.
# Position is defined from 0 to 1, meaning from
# y_pixel = 0 (southern border) to
# y_pixel = 255 (northern border).
NUM_TILE_STEPS = 4
TILE_START_POSITION = 0.0
TILE_STEP_SIZE = 1.0 / NUM_TILE_STEPS


def ToMercDegrees(y, num_tiles):
  """Calculate latitude of southern border of yth tile in degrees.

  LOD is log2(num_tiles)
  Args:
    y: (float) Position of tile in qt grid moving from south to north.
       Non-integer values give latitude within tile.
    num_tiles: (integer) Number of tiles in the qt grid.
  Returns:
    Latitude of southern border of tile in degrees.
  """
  # Calculate on standard Mercator scale that spans from -pi to pi.
  # There is no intrinsic reason for using these values, which correspond to
  # about -85 to 85 degrees, other than it matches (albeit somewhat
  # misleadingly) the longitudinal radian span, and it's the span Google
  # uses for its 2d maps.
  y_merc = 2.0 * math.pi * y / num_tiles - math.pi
  latitude_rad = (math.atan(math.exp(y_merc)) - math.pi / 4.0) * 2.0
  return latitude_rad / math.pi * 180.0


def ToMercPosition(lat_deg, num_tiles):
  """Calculate position of a given latitude on qt grid.

  LOD is log2(num_tiles)
  Args:
    lat_deg: (float) Latitude in degrees.
    num_tiles: (integer) Number of tiles in the qt grid.
  Returns:
    Floating point position of latitude in tiles relative to equator.
  """
  lat_rad = lat_deg / 180.0 * math.pi
  y_merc = math.log(math.tan(lat_rad / 2.0 + math.pi / 4.0))
  return num_tiles / 2.0 * (1 + y_merc / math.pi)


def ErrorInPixels(y, num_tiles, pos_flat_tile_0_to_1):
  """Calculate error in pixels at given position within tile.

  Position ranges from 0.0 to 1.0 (y_pixel = 0 to y_pixel = 255).
  Maximum error should be near the middle since it is 0 at the
  two borders.

  Args:
    y: (integer) Position of tile in qt grid moving from south to north.
    num_tiles: (integer) Number of tiles in the qt grid.
    pos_flat_tile_0_to_1: (float) Position of pixel in plate carree tile.
        Position is from 0.0 (southern border) to 1.0 (northern border).
  Returns:
    Error in pixel distance of given pixel.
  """
  # Top and bottom bounds in degrees (same for flat and Mercator)
  # since we are passing these to wms as the bounds.
  bottom_merc_tile_deg = ToMercDegrees(y + 0.0, num_tiles)
  top_merc_tile_deg = ToMercDegrees(y + 1.0, num_tiles)
  # Get fractional y position (in tiles).
  y_flat = y + pos_flat_tile_0_to_1
  # Use linear interpoation to find flag latitude for given point.
  flat_deg = ((top_merc_tile_deg - bottom_merc_tile_deg)
              * pos_flat_tile_0_to_1 + bottom_merc_tile_deg)
  # Get fractional y position of that latitude on Mercator map.
  true_y_merc = ToMercPosition(flat_deg, num_tiles)
  # Subtract difference and multiply by pixel height of tile to
  # get error in pixels.
  return (y_flat - true_y_merc) * 256


def main():
  for lod in xrange(START_LOD, END_LOD + 1):
    print "LOD", lod
    num_tiles = 1 << lod
    middle = num_tiles / 2
    lod_step = middle / NUM_LOD_STEPS
    for i in xrange(NUM_LOD_STEPS):
      y = middle + lod_step * i
      print "  Tile at %3.1f degrees" % ToMercDegrees(y, num_tiles)
      tile_position = TILE_START_POSITION
      for unused_ in xrange(NUM_TILE_STEPS):
        print "    Error at %3.2f: %6.5f pixels" % (
            tile_position, ErrorInPixels(y, num_tiles, tile_position))
        tile_position += TILE_STEP_SIZE

if __name__ == "__main__":
  main()
