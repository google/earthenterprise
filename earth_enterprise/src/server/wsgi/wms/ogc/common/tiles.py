#!/usr/bin/env python2.7
#
# Copyright 2017 Google Inc.
# Copyright 2019-2020 Open GEE Contributors
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


"""Deals with fetching tiles and composing them for the final image."""

import logging
import os
import StringIO
import tempfile
import wms_connection

import geom
import images
import PIL.Image as Image
import tilecalcs

logger = logging.getLogger("wms_maps")
_TILE_PIXEL_SIZE = 256
_NO_DATA_PIXELS = (0, 0, 0)
_OPAQUE_ALPHA = (255,)
_TRANSPARENT_ALPHA = (0,)
_ALPHA_THRESHOLD = 128
ALL_WHITE_PIXELS = (255, 255, 255)

def StitchTiles(tiles_array, layer_properties, tilepixel_rect,
  rect_of_tiles, user_width, user_height):
  """Stitch an array of image tiles creating an image that fits within
    a rectangle and same size as passed width and height.

  Args:
      tiles_array: ImageArray of the tiles to stitch together.
      layer_properties: Object with details about the layer.
      tilepixel_rect: The desired tile pixel space rect of our image
        gotten from the server, at a given zoom level. Unscaled; still
        needs to be shrunk (most of the time) to fit the final image.
      rect_of_tiles: <tilepixel_rect> 'rounded out' to whole tiles.
        There's one tile address extra at both the right, and down,
        for cleaner loops.
      user_width: The user-requested width of the image.
      user_height: The user-requested height of the image.
  Returns:
     The output image of the tiles stitching, clipped and resized.
  """

  im_whole_tiles_extent = geom.Pair(rect_of_tiles.Width() * _TILE_PIXEL_SIZE,
                                    rect_of_tiles.Height() * _TILE_PIXEL_SIZE)

  # Process transparency for map.
  # Presently "image/png" is the only picture format
  # which supports transparency.
  # If the picture format is "image/jpeg", then send
  # the image as it is, without processing it
  # for any transparency requirements.

  set_pixel_to_bgcolor = (layer_properties.image_format == "image/png" and
                          layer_properties.is_transparent == "FALSE")

  bgcolor = ALL_WHITE_PIXELS
  if layer_properties.bgcolor:
      bgcolor = layer_properties.bgcolor

  alpha = _TRANSPARENT_ALPHA
  if set_pixel_to_bgcolor:
      alpha = _OPAQUE_ALPHA

  # Alpha channel is not required for jpeg formats.
  if layer_properties.image_format == "image/jpeg":
    mode = "RGB"
    color = bgcolor
  else:
    mode = "RGBA"
    color = bgcolor + alpha

  # If TRANSPARENT = TRUE, image format is "image/png",
  # then create a transparent image (alpha = 0) with a black background.

  # If TRANSPARENT = FALSE, image format is "image/png",
  # then create an opaque image (alpha = 255) with bgcolor background.

  # Will have unwanted margin but we'll crop it off later.
  im_whole_tiles = Image.new(mode, im_whole_tiles_extent.AsTuple(), color)

  logger.debug("Tiles rect (in tiles): %s %s",
               str(rect_of_tiles), str(rect_of_tiles.Extent()))
  logger.debug("im_whole_tiles pixel extent: %s",
               str(im_whole_tiles_extent))

  for row in range(rect_of_tiles.Height()):
    for column in range(rect_of_tiles.Width()):
      pos = (
          int(column * _TILE_PIXEL_SIZE),
          int(row * _TILE_PIXEL_SIZE),
          int((column + 1) * _TILE_PIXEL_SIZE),
          int((row + 1) * _TILE_PIXEL_SIZE)
          )
      im_tile = tiles_array.ImageAt(column, row)

      if set_pixel_to_bgcolor:
        im_tile = _SetTransPixelToBgcolor(im_tile, bgcolor)

      # It may be None.
      if im_tile:
        _PasteTile(im_whole_tiles, im_tile, pos)

  logger.debug("tilepixel_rect: %s", str(tilepixel_rect))

  # Relative to / within im_whole_tiles.
  # Round down to nearest 256.
  offset_within_tiled_image = geom.Pair(
      tilepixel_rect.x0 % _TILE_PIXEL_SIZE,
      tilepixel_rect.y0 % _TILE_PIXEL_SIZE
      )

  logger.debug("Offset within: %s", str(offset_within_tiled_image))

  within_tiled_image = geom.Rect.FromLowerLeftAndExtent(
      offset_within_tiled_image, tilepixel_rect.Extent())

  logger.debug("Cropping to: %s", str(within_tiled_image.AsTuple()))

  im_true = im_whole_tiles.crop(within_tiled_image.AsTuple())

  logger.debug("Stretching to requested: %s", str(
      (user_width, user_height)))

  # Stretch the final pixels to match the aspect ratio of WIDTH /
  # HEIGHT. This is per the spec; doing this lets the client compensate
  # for non-square pixels.
  im_user = im_true.resize((user_width, user_height), Image.ANTIALIAS)

  return im_user

def ProduceImage(layer_properties, user_log_rect, user_width, user_height):
  """High-level production of the image.

  Args:
      layer_properties: Object with details about the layer.
      user_log_rect: The user-requested projected, ie map coordinates,
        not lat/lon, limits of the desired region. Ie BBOX, pretty much.
      user_width: The user-requested width of the image.
      user_height: The user-requested height of the image.

  Returns:
      The image to be presented to the user.
  """
  proj = layer_properties.projection

  zoom_level = tilecalcs.CalcZoomLevel(user_log_rect.Extent(),
                                       proj.InternalLogOuterBounds().Extent(),
                                       geom.Pair(user_width, user_height))

  tilepixel_rect, rect_of_tiles = tilecalcs.CalcTileRects(
      proj, user_log_rect, zoom_level)

  logger.info("Done tile calcs")

  tiles_array = _FetchTiles(rect_of_tiles, zoom_level, layer_properties)

  im_user = StitchTiles(tiles_array, layer_properties, tilepixel_rect,
    rect_of_tiles, user_width, user_height)

  return im_user

def _FetchTiles(rect_of_tiles, zoom_level, layer_properties):
  """Fetches all the tiles for a given image.

  Args:
      rect_of_tiles: is ul - lr (lr is exclusive!) addresses of tiles at a given
        zoom_level.
      zoom_level: self-explanatory.
      layer_properties: Object with details about the layer.

  Returns:
      ImageArray of the tiles.

  FWIW fetching isn't a big part of the total time for our WMS (~0.3s),
  so we don't bother with threads.
  For 8 tiles, unthreaded was faster - ~0.017s vs ~0.029s;
    http://localhost/wms?LAYERS=1002&
    SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&STYLES=&FORMAT=image%2Fjpeg&
    CRS=EPSG:3857&BBOX=-30000000.0,-30000000.0,30000000.0,30000000.0&
    WIDTH=400&HEIGHT=400
  For 16, threaded is faster:  0.108545780182s vs 0.0410861968994s
    http://localhost/wms?LAYERS=1002&SERVICE=WMS&
    VERSION=1.3.0&REQUEST=GetMap&STYLES=&FORMAT=image%2Fjpg&CRS=EPSG:3857&
    BBOX=-400000.0,-400000.0,400000.0,400000.0&WIDTH=800&HEIGHT=800
  """
  # <world_extent_in_tiles> is the total tiles vertically and
  # horizontally, so that we know to either wrap around east-west, or
  # fill in with black tiles (per the WMS spec) for north-south
  # out-of-bounds requests.

  logger.info("Fetching tiles")
  logger.debug("rect_of_tiles: %s", rect_of_tiles)

  world_extent_in_tiles = 2 ** zoom_level
  tiles_array = images.ImagesArray(
      rect_of_tiles.Width(), rect_of_tiles.Height())

  logger.debug("World extent in tiles: %s", str(world_extent_in_tiles))

  base_url = layer_properties.GetMapBaseUrl()

  # We don't fetch 'black', empty tiles. They are always whole tiles.
  for abs_tile_row in range(rect_of_tiles.y0, rect_of_tiles.y1):
    # rel_tile_row is the row it will appear in, in the tile image.
    # Ie, [0 - n-1]; top-down.
    # Tile pixel space and tiles are both 'graphics space', ie 0,0 is
    # upper-left corner, down to lower-right.

    rel_tile_row = abs_tile_row - rect_of_tiles.y0
    logger.debug("Row - abs: %d; rel:%d", abs_tile_row, rel_tile_row)

    if rel_tile_row < 0:
      logger.error("Tile row %d - must never be < 0", rel_tile_row)

    for abs_tile_col in range(rect_of_tiles.x0, rect_of_tiles.x1):
      rel_tile_col = (abs_tile_col - rect_of_tiles.x0)

      if abs_tile_row < 0 or world_extent_in_tiles <= abs_tile_row:
        logger.debug("[%d] %d, %d is black",
                     world_extent_in_tiles,
                     abs_tile_col,
                     abs_tile_row)

        # Python Imaging Library (PIL)'s pixels are black by default;
        # we just don't set them.
        tiles_array.AddImage(rel_tile_col, rel_tile_row, None)
      else:
        # If more than 360-worth, could wrap back onto an
        # already-written part of the image array. Though if it did
        # it'd write the same thing so, it's only inefficient, not an
        # error.
        world_wrapped_tile_col = abs_tile_col % world_extent_in_tiles
        tile_args = layer_properties.GetTileArgs(world_wrapped_tile_col,
                                                 abs_tile_row, zoom_level)
        tile_url = base_url + tile_args
        im_tile = _FetchMapTile(tile_url)
        if im_tile:
          if im_tile.size == (1, 1):
            # 1x1 tiles come (probably mostly) from vector layers. They
            # mean that the whole 256x256 tile should be filled with the
            # color & opacity of this pixel.
            im_tile = im_tile.resize((_TILE_PIXEL_SIZE, _TILE_PIXEL_SIZE))
          tiles_array.AddImage(rel_tile_col, rel_tile_row, im_tile)

  return tiles_array


def _SetTransPixelToBgcolor(tile, bgcolor):
  """Set the transparent pixels to bgcolor.

  Args:
     tile: Tile as sent from the server.
     bgcolor: BGCOLOR parameter as sent by the GIS client's.
     Default is 0xFFFFFF(white).
  Returns:
     The source tile with transparent pixels fill w/ BGCOLOR and made opaque.
  """
  logger.debug("Processing the transparency for tile")

  if not tile:
    # Server returned a 404 Error.
    # _FetchmapTile() would return tile = None, in such cases.
    return tile

  # Palette or (1, 1) size tiles have mode "P".
  if tile.getbands() == ("P",):
    # convert the P mode to RGBA mode
    rgba_tile = Image.new("RGBA", tile.size)
    rgba_tile.paste(tile)

    # transparent tiles need not be processed, as the grid image is
    # already filled with bgcolor and made opaque.

    # Non-transparent tiles should be returned as it is to be inserted
    # into the grid image.

    tile = (tile if rgba_tile.getpixel((0, 0))[-1] != _TRANSPARENT_ALPHA[0]
            else None)
    return tile

  # RGB mode for PNG tiles.
  # Return the tile as is.
  # This tile will have opaque alpha in the grid image.
  if tile.getbands() == ("R", "G", "B"):
    return tile

  pixdata = tile.load()
  for row in xrange(tile.size[0]):
    for col in xrange(tile.size[1]):
      # If pixel alpha < threshold, make it opaque and fill it with bgcolor.
      if pixdata[row, col][3] <= _ALPHA_THRESHOLD:
        pixdata[row, col] = bgcolor + _OPAQUE_ALPHA
      else:
        (red, green, blue, alpha) = pixdata[row, col]
        if alpha < _OPAQUE_ALPHA[0]:
          pixdata[row, col] = (red, green, blue) + _OPAQUE_ALPHA

  return tile


def _FetchMapTile(url):
  """Fetches and returns a tile, given an url.

  Args:
      url: the exact url of the tile to fetch.

  Returns:
      The tile bitmap.
  """
  try:
    f = StringIO.StringIO(wms_connection.HandleConnection(url))
    im_tile = Image.open(f)
    im_tile.load()
  except IOError, e:
    im_tile = None
    logger.error("Failed to fetch tile:%s", e)

  return im_tile


def _SaveImage(image, fname, image_spec):
  """For debugging; saves the named tile to /tmp.

  Args:
      image: the PIL tile image.
      fname: the name to give the image.
      image_spec: all details about type, extension etc.
  """
  try:
    _, t_path = tempfile.mkstemp(suffix="." + image_spec.file_extension,
                                 prefix=fname + "-")
    # image.info is necessary to get transparency.
    image.save(t_path, image_spec.pil_format, **image.info)
    os.chmod(t_path, 777)
  except IOError, e:
    logger.error("Failed to save:%s", str(e.args[0]))
    raise


def _PasteTile(im_dest, im_src, box):
  """Copy the image.

  Args:
      im_dest: Destination of the image to be copied.
      im_src: Source image to be copied.
      box: the dimentions of the image.
  """
  try:
    im_dest.paste(im_src, box)
  except ValueError, e:
    logger.error("Failed to paste:%s", str(e.args[0]))
    logger.debug("Size %s vs %s", str(im_src.size), str(im_dest.size))
    logger.debug("Mode %s vs %s", str(im_src.mode), str(im_dest.mode))
    raise


def main():
  map_url = ("http://localhost/ca_maps/query?request=ImageryMaps&"
             "channel=1002&version=1&x=1&y=0&z=1")
  im = _FetchMapTile(map_url)
  print im

if __name__ == "__main__":
  main()
