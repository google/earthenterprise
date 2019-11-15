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


"""Contains 2D ImagesArray."""

import logging
import PIL.Image as Image
import utils

# Get logger
logger = logging.getLogger("wms_maps")

TILE_PIXEL_SIZE = 256


class ImagesArray(object):
  """2D array of images.

  Images are stored row-major (row, col), but the interface is col, row
  for consistency with the rest of the program.
  """

  def __init__(self, ncols, nrows):
    self.ncols = ncols
    self.nrows = nrows
    self.images = []
    self._PreallocateImages(self.ncols, self.nrows)

  def _PreallocateImages(self, ncols, nrows):
    """Makes a 2D row-major (y, x) array  of <None>s.

    Args:
        ncols: number of slots along the x axis.
        nrows: number of slots along the y axis.
    """
    # This version of the next line:
    #   self.images = [[None] * ncols] * nrows
    # causes the row lists to share references somehow.  You might
    # think that if that were true, that the columns would be tied in
    # the version that's used here, too, but it isn't so. It's
    # strange, but you can confirm the difference by trying the two
    # versions and looking at the results.
    self.images = [[None] * ncols for unused_entry in range(nrows)]

  def AddImage(self, col, row, image):
    """Add an image to this 2D array.

    Args:
        col: x [0..ncols)
        row: y [0..nrows)
        image: the 256x256 tile image to add.
    """
    if not isinstance(image, Image.Image):
      logger.error("Image parameter is not of type Image.Image")
    if not (0 <= col and col < self.ncols):
      logger.error("Column %d out of bounds: 0 - %d", col, self.ncols)
      utils.Assert(False)
    if not (0 <= row and row < self.nrows):
      logger.error("Row %d out of bounds: 0 - %d", row, self.nrows)
      utils.Assert(False)

    self.images[row][col] = image

  def CheckAll(self):
    """For debugging. Logs any image missing or the wrong size."""
    for row in range(self.nrows):
      for col in range(self.ncols):
        if self.images[row][col] is None:
          logger.error("Missing %d,%d from image array", col, row)
        elif self.images[row][col].size != (TILE_PIXEL_SIZE, TILE_PIXEL_SIZE):
          logger.debug("%d,%d is size: %s",
                       col, row, str(self.images[row][col].size))

  def ImageAt(self, col, row):
    """Accessor.

    Args:
        col: xth of 2D array of images to return.
        row: yth of 2D array of images to return.
    Returns:
        The image at col, row.
    """
    if not (0 <= col and col < self.ncols):
      logger.error("Bad column %d", col)
    if not (0 <= row and row < self.nrows):
      logger.error("Bad row %d", row)
    return self.images[row][col]


def main():
  im = ImagesArray(2, 3)
  im_tile = im.ImageAt(1, 2)
  print im_tile

if __name__ == "__main__":
  main()
