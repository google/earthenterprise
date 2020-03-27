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


"""Tile calculations."""

import logging
import math

import geom
import utils
import xform

_MAX_ZOOM = 23
_TILE_PIXEL_SIZE = 256

logger = logging.getLogger("wms_maps")


def CalcZoomLevel(log_extent, total_log_extent, pixel_extent):
  """Calculates zoom level.

  We want a zoom level that has enough detail to match the user's request
  (i.e., we do our best not to give stretched-out pixels). A bigger zoom ==
  more pixels + detail. But, it would be wasteful to use a higher zoom
  level than necessary.

  Args:
      log_extent: map-space width, height.
      total_log_extent: total defined map-space extent (size of projection
        bounds, in its output coordinates).
      pixel_extent: desired width, height of the final pixel image.

  Returns:
       zoom level with at least as much detail as required.
  """
  utils.Assert(isinstance(log_extent, geom.Pair))
  utils.Assert(isinstance(total_log_extent, geom.Pair))
  utils.Assert(isinstance(pixel_extent, geom.Pair))

  # Simple, component-wise division.
  fraction_of_total = log_extent / total_log_extent
  total_pixel_extent_needed = pixel_extent / fraction_of_total
  logger.debug("Pixel extent needed %s", str(total_pixel_extent_needed))

  # We want to round the zoom level up; ie, have /at least/ 1 tile pixel
  # per requested pixel.
  # 256 x 2^zoom >= totalpixelextent => zoom = log_2(totalpixelextent / 256)
  x_zoom = int(math.log(math.ceil(
      total_pixel_extent_needed.x / float(_TILE_PIXEL_SIZE)), 2))
  y_zoom = int(math.log(math.ceil(
      total_pixel_extent_needed.y / float(_TILE_PIXEL_SIZE)), 2))

  logger.debug("Zoom:x,y %d, %d", x_zoom, y_zoom)
  zoom = max(x_zoom, y_zoom)
  if _MAX_ZOOM < zoom:
    logger.warning("WHOA; wild zoom (%d - should be max %d), "
                   "alert Google. Limiting to %d.",
                   zoom, _MAX_ZOOM, _MAX_ZOOM)
    zoom = _MAX_ZOOM

  return zoom


def _DumpCenterInfo(total_tilepixel_length, user_width, user_height):
  """Calculate the center info of the tile.

  Args:
    total_tilepixel_length: Total tile length.
    user_width: The user-requested width of the image.
    user_height: The user-requested height of the image.
  """
  # debugging - should be in the center of the tiles
  c_tilex = total_tilepixel_length / 2
  c_tiley = total_tilepixel_length / 2
  c_tilew = user_width / 2
  c_tileh = user_height / 2
  centered_tilepixel_rect = geom.Rect(c_tilex - c_tilew, c_tiley - c_tileh,
                                      c_tilex + c_tilew, c_tiley + c_tileh)
  logger.debug("Centered pixels would be: ~ %s",
               str(centered_tilepixel_rect))

  centered_rect = centered_tilepixel_rect / _TILE_PIXEL_SIZE
  logger.debug("Centered tile rect would be: ~ %s", str(centered_rect))
  centered_rect.x1 += 1
  centered_rect.y1 += 1
  logger.debug("High-exclusive centered tile rect would be: ~ %s",
               str(centered_rect))


def TotalTilepixelExtent(zoom):
  """Number of pixels on a side, at a given zoom.

  Args:
    zoom: zoom level.
  Returns:
    Whole tilepixel space extent for this zoom level.
    Height == width so, just one number is returned.
  """
  utils.Assert(geom.IsNumber(zoom))

  total = _TILE_PIXEL_SIZE * (2 ** zoom)
  logger.debug("Total pixels:%d", total)

  return total


def CalcTileRects(proj, bbox, zoom_level):
  """Tile calculations.

  Args:
      proj: the projection to use.
      bbox: is BBOX, ie desired rectangle in map, aka logical coordinates.
      zoom_level: The zoom level the tiles must come from. For the server.

  Returns:
      tilepixel_rect: The desired tile pixel space rect of our image
        gotten from the server, at a given zoom level. Unscaled; still
        needs to be shrunk (most of the time) to fit the final image.
      rect_of_tiles: <tilepixel_rect> 'rounded out' to whole tiles.
        There's one tile address extra at both the right, and down,
        for cleaner loops.
  """
  utils.Assert(isinstance(bbox, geom.Rect), "bbox not Rect")

  total_log_rect = proj.InternalLogOuterBounds()

  total_tilepixel_length = TotalTilepixelExtent(zoom_level)

  # Is exclusive
  total_tilepixel_rect = geom.Rect(
      0, 0, total_tilepixel_length, total_tilepixel_length)

  # flip log because the corners have to correspond, and tiles' 0,0 is
  # upper left, log's -max, -max is lower left.
  mapping = xform.WindowViewportMapping(
      total_log_rect,
      geom.Rect.FromLowerLeftAndExtent(
          total_tilepixel_rect.xy0,
          # We leave this exclusive, here, because that acts more like
          # an 'ideal', real-valued rectangle, as our logical rect is
          # pretty much 'ideal' at 4e7 pixels. It matters with
          # more-zoomed-out, -> smaller tilepixel spaces.
          total_tilepixel_rect.Extent())
      .FlippedVertically()
      )
  logger.debug("bbox: %s", str(bbox))

  tilepixel_rect = mapping.LogRectToPhys(bbox).AsInts()

  # TODO: eliminate double flipping!
  if tilepixel_rect.IsFlippedVertically():
    tilepixel_rect = tilepixel_rect.FlippedVertically()

  logger.debug("tilepixel_rect: %s", str(tilepixel_rect))

  # A pixel will land somewhere within /some/ tile, and that's
  # the tile we want.
  rect_of_tiles = tilepixel_rect / _TILE_PIXEL_SIZE

  logger.debug("rect_of_tiles %s", str(rect_of_tiles))

  # Tiles and pixels are all 'graphics space', ie origin is upper-left.
  # Make the rects exclusive on the lower right, for looping convenience.
  rect_of_tiles.x1 += 1
  rect_of_tiles.y1 += 1

  logger.debug("High exclusive rect_of_tiles %s", str(rect_of_tiles))

  return tilepixel_rect, rect_of_tiles


def main():
  total = TotalTilepixelExtent(15)
  print total

if __name__ == "__main__":
  main()
