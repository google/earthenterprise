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


"""The standard window / viewport mapping used in graphics."""

import logging

import geom
import utils

# Get logger
logger = logging.getLogger("wms_maps")


class WindowViewportMapping(object):
  """Well-known graphics window <-> viewport transformation.

  Many explanations on the web, one is:
    http://www.siggraph.org/education/materials/HyperGraph/viewing/
    view2d/pwint.htm
  Window (here 'logical') is the projection space, viewport
  ('physical') is tilepixel space.
  """

  def __init__(self, log_rect, phys_rect):
    """Initialization.

    The corners of <log_rect> and <phys_rect> have to correspond.

    Args:
        log_rect: the logical rectangle, the 'window'.
        phys_rect: the physical rectangle, the 'viewport' or 'device'.
    """
    logger.debug("Initializing xform: log: %s phys: %s",
                 str(log_rect), str(phys_rect))
    self.log_rect = log_rect
    self.phys_rect = phys_rect

    # These are not necessarily positive, is why we don't use width or height.
    self._w_log = self.log_rect.SignedWidth()
    self._h_log = self.log_rect.SignedHeight()
    self._w_phys = self.phys_rect.SignedWidth()
    self._h_phys = self.phys_rect.SignedHeight()

    self._logical_to_physical_x_scale = (
        phys_rect.SignedWidth() / log_rect.SignedWidth())
    self._logical_to_physical_y_scale = (
        phys_rect.SignedHeight() / log_rect.SignedHeight())

    self._physical_x_offset = (
        self._logical_to_physical_x_scale *
        -self.log_rect.x0 + self.phys_rect.x0)
    self._physical_y_offset = (
        self._logical_to_physical_y_scale *
        -self.log_rect.y0 + self.phys_rect.y0)

  def LogPtToPhys(self, log_pt):
    """Find tilepixel space point, of <log_pt>.

    Args:
        log_pt: window point.

    Returns:
        Corresponding viewport, for us tilepixel space, point.
    """
    utils.Assert(isinstance(log_pt, geom.Pair), "logpt is not a geom.Pair")

    phys_x = (log_pt.x * self._logical_to_physical_x_scale +
              self._physical_x_offset)
    phys_y = (log_pt.y * self._logical_to_physical_y_scale +
              self._physical_y_offset)

    return geom.Pair(phys_x, phys_y)

  def LogRectToPhys(self, log_rect):
    """Transforms a window rectangle -> viewport.

    Args:
        log_rect: window rect.
    Returns:
        viewport rect.
    """
    physxy0 = self.LogPtToPhys(log_rect.xy0)
    physxy1 = self.LogPtToPhys(log_rect.xy1)

    return geom.Rect.FromLowerLeftAndUpperRight(physxy0, physxy1)

  def PhysPtToLog(self, phys_pt):
    """Transforms 'back', from a pixel to the map-projected space.

    Args:
      phys_pt: Physical point.
    Returns:
      viewport rect.
    """
    log_x = ((phys_pt.x - self.phys_rect.x0) / self._w_phys * self._w_log +
             self.log_rect.x0)
    log_y = ((phys_pt.y - self.phys_rect.y0) / self._h_phys * self._h_log +
             self.log_rect.y0)

    return geom.Pair(log_x, log_y)


def main():
  phy = WindowViewportMapping((-3, 3), (-10, 10))
  print phy.PhysPtToLog((-5, 5))

if __name__ == "__main__":
  main()
