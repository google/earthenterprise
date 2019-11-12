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

"""Tile calculations.
"""

import math

import geom
import utils
import xform


# From the tiles pyramid spreadsheet:
#   https://docs.google.com/a/google.com/spreadsheet/ccc?
#     key=0Ajr8MzyreERNcFBPWkx6TXFSa1hudW9IUFFIWlB0WFE&hl=en#gid=0
_MAX_ZOOM = 23


def CalcZoomLevel(log_extent, total_log_extent, pixel_extent):
  """Calculates zoom level.

  We want a zoom level that has enough detail to match the user's request
  (ie we do our best not to give stretched-out pixels). A bigger zoom ==
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
  utils.logger.debug('fraction %s' % str(fraction_of_total))
  total_pixel_extent_needed = pixel_extent / fraction_of_total
  utils.logger.debug('pixel extent needed %s' % str(total_pixel_extent_needed))

  # We want to round the zoom level up; ie, have /at least/ 1 tile pixel
  # per requested pixel.
  # 256 x 2^zoom >= totalpixelextent => zoom = log_2(totalpixelextent / 256)
  x_zoom = int(math.log(math.ceil(total_pixel_extent_needed.x / 256.0), 2))
  y_zoom = int(math.log(math.ceil(total_pixel_extent_needed.y / 256.0), 2))

  utils.logger.debug('zoom:x,y %s, %s' % (str(x_zoom), str(y_zoom)))
  zoom = max(x_zoom, y_zoom)
  if _MAX_ZOOM < zoom:
    utils.logger.warning('WHOA; wild zoom (%s - should be max 23), '
                         'alert Google. Limiting to 23.' % str(zoom))
    zoom = _MAX_ZOOM

  return zoom


def _DumpCenterInfo(total_tilepixel_length, user_width, user_height):
  # debugging - should be in the center of the tiles
  c_tilex = total_tilepixel_length / 2
  c_tiley = total_tilepixel_length / 2
  c_tilew = user_width / 2
  c_tileh = user_height / 2
  centered_tilepixel_rect = geom.Rect(c_tilex - c_tilew, c_tiley - c_tileh,
                                      c_tilex + c_tilew, c_tiley + c_tileh)
  utils.logger.debug('centered pixels would be: ~ %s' %
                     str(centered_tilepixel_rect))

  centered_rect = centered_tilepixel_rect / 256
  utils.logger.debug('centered tile rect would be: ~ %s' % str(centered_rect))
  centered_rect.x1 += 1
  centered_rect.y1 += 1
  utils.logger.debug('high-exclusive centered tile rect would be: ~ %s' %
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
  total = 256 * (2 ** zoom)
  utils.logger.debug('total pixels:%s' % str(total))
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
  utils.Assert(isinstance(bbox, geom.Rect), 'bbox not Rect')

  total_log_rect = proj.internalLogOuterBounds()

  total_tilepixel_length = TotalTilepixelExtent(zoom_level)
  # Is exclusive
  total_tilepixel_rect = geom.Rect(
      0, 0, total_tilepixel_length, total_tilepixel_length)
  # flip log because the corners have to correspond, and tiles' 0,0 is
  # upper left, log's -max, -max is lower left.
  mapping = xform.WindowViewportMapping(
      total_log_rect,
      geom.Rect.fromLlAndExtent(
          total_tilepixel_rect.xy0,
          # We leave this exclusive, here, because that acts more like
          # an 'ideal', real-valued rectangle, as our logical rect is
          # pretty much 'ideal' at 4e7 pixels. It matters with
          # more-zoomed-out, -> smaller tilepixel spaces.
          total_tilepixel_rect.Extent())
      .flippedVertically()
      )
  utils.logger.debug('bbox: %s' % str(bbox))
  tilepixel_rect = mapping.LogRectToPhys(bbox).asInts()

  # TODO: eliminate double flipping.
  if tilepixel_rect.isFlippedVertically():
    tilepixel_rect = tilepixel_rect.flippedVertically()

  utils.logger.debug('tilepixel_rect: %s' % str(tilepixel_rect))

  # A pixel will land somewhere within /some/ tile, and that's
  # the tile we want.
  rect_of_tiles = tilepixel_rect / 256

  utils.logger.debug('rect_of_tiles %s' % str(rect_of_tiles))
  # Tiles and pixels are all 'graphics space', ie origin is upper-left.
  # Make the rects exclusive on the lower right, for looping convenience.
  rect_of_tiles.x1 += 1
  rect_of_tiles.y1 += 1

  utils.logger.debug('high-exclusive rect_of_tiles %s' % str(rect_of_tiles))

  return tilepixel_rect, rect_of_tiles
