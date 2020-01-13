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

"""Projection objects, with operations and meta-information."""

import logging
import math

import geom
import utils

# Get logger
logger = logging.getLogger("wms_maps")

WGS84_EQUATORIAL_RADIUS = 6378137.0

_DEGREES_TO_RADIANS = math.pi / 180.0
_RADIANS_TO_DEGREES = 180.0 / math.pi


def DegToRad(deg):
  return deg * _DEGREES_TO_RADIANS


def RadToDeg(rad):
  return rad * _RADIANS_TO_DEGREES


class Mercator(object):
  """Spherical Mercator, that is."""
  # 900913 is an earlier but still-used alias of 3857.
  EPSG_NAMES = ["EPSG:3857", "EPSG:900913"]

  # drops coords down to -100 - 100 range, for easier debugging
  # EARTH_RADIUS = 100.0 / math.pi
  EARTH_RADIUS = WGS84_EQUATORIAL_RADIUS

  # Some places list 85.0, one this 85.0511... value, which makes the
  # Y extent the same as the X; a nice square.
  # Strangely, MapServer sees our value but produces 85.084.  Don't
  # know where that comes from.
  MAX_LATITUDE = 85.051128779806589
  MAX_LONGITUDE = 180.0

  def __init__(self):
    self.radius = Mercator.EARTH_RADIUS
    self.name = "mercator"

  def MercX(self, lon):
    log_x = self.radius * DegToRad(lon)
    return log_x

  def MercY(self, lat):
    if Mercator.MAX_LATITUDE < lat:
      lat = Mercator.MAX_LATITUDE
    elif lat < -Mercator.MAX_LATITUDE:
      lat = -Mercator.MAX_LATITUDE
    # No limit on longitude - it wraps around just fine.

    try:
      log_y = self.radius * math.log(math.tan(
          math.pi / 4.0 + DegToRad(lat) / 2.0))
      return log_y
    except ValueError:
      logger.error("Mercator ValueError")
    except ZeroDivisionError:
      logger.error("Mercator ZeroDivisionError")

  def _Lat(self, merc_y):
    unit_y = merc_y / self.radius
    rad_y = 2.0 * math.atan(math.exp(unit_y)) - math.pi / 2.0
    lat = RadToDeg(rad_y)
    return lat

  def _Lon(self, merc_x):
    lon = RadToDeg(merc_x / self.radius)
    return lon

  def LogXYFromlonLat(self, lonlat):
    """Projects the lon, lat point, in degrees, to logical.

    Args:
        lonlat: Pair of lon, lat to be projected.
    Returns:
        Logical point projected from lon, lat.
    """
    utils.Assert(isinstance(lonlat, geom.Pair))
    return geom.Pair(self.MercX(lonlat.x), self.MercY(lonlat.y))

  def LonLatFromLogXY(self, log_xy):
    """Inverse projection from the Mercator map coordinates.

    Args:
        log_xy: Pair- point of logical space to be reverse-projected back into
            a lon,lat.
    Returns:
        A Pair of the resulting lon, lat.
    """
    utils.Assert(isinstance(log_xy, geom.Pair))
    return geom.Pair(self._Lon(log_xy.x), self._Lat(log_xy.y))

  def _LogOuterBounds(self):
    """Outer bounds of this projection. As near as practical to the whole globe.

    Returns:
        The logical, map space over which the projection is defined.
    """
    xy0 = geom.Pair(self.MercX(-Mercator.MAX_LONGITUDE),
                    self.MercY(-Mercator.MAX_LATITUDE))
    xy1 = geom.Pair(self.MercX(Mercator.MAX_LONGITUDE),
                    self.MercY(Mercator.MAX_LATITUDE))

    return geom.Rect(xy0.x, xy0.y, xy1.x, xy1.y)

  def InternalLogOuterBounds(self):
    return self._LogOuterBounds()

  def AdvertizedLogOuterBounds(self):
    return self._LogOuterBounds()

  def LogTotalHeight(self):
    outer = self.logOuterBounds()
    return outer.y1 - outer.y0

  def LogTotalWidth(self):
    outer = self.logOuterBounds()
    return outer.x1 - outer.x0


class Flat(object):
  """Flat projection, ie from -180 - 179, -90 - +90.

  Note: if you request the 'whole' space, -180 - 180, this will wrap
  around on the right, back to the left, causing the right edge tiles
  to be fetched again and wrapped to the left.  Though it's not what
  people expect, it's not visible, just wasteful, and some thought
  will convince you that's what /should/ happen (-180 == 180),
  probably, so it's not a bug.  Using a right limit of 179.0 indeed
  does not cause this refetching + wrapping of tiles.

  For whatever reason, this isn't what happens with the Mercator
  projection; that just grows to the left and right indefinitely.
  TODO: get an explanation.
  """

  EPSG_NAMES = ["EPSG:4326"]

  MAX_LATITUDE = 90.0
  MAX_LONGITUDE = 180.0

  def __init__(self):
    self.name = "flat"

  def FlatX(self, lon):
    return lon

  def FlatY(self, lat):
    return lat

  def _Lat(self, log_y):
    return log_y

  def _Lon(self, flat_x):
    return flat_x

  # not that it matters but, candidate for pushing into a baseclass.
  def LogXYFromlonLat(self, lonlat):
    """Projects the lon, lat point, in degrees, to logical.

    Args:
        lonlat: Pair of lon, lat to be projected.
    Returns:
        Logical point projected from lon, lat.
    """
    utils.Assert(isinstance(lonlat, geom.Pair))

    return geom.Pair(self.FlatX(lonlat.x), self.FlatY(lonlat.y))

  def LonLatFromLogXY(self, log_xy):
    """Inverse projection; log_x/y are scaled to planet_radius.

    Args:
        log_xy: logical point to be 'inverted' (ie find lon, lat).
    Returns:
        Pair of lon, lat.
    """
    utils.Assert(isinstance(log_xy, geom.Pair))

    return geom.Pair(self._Lon(log_xy.x), self._Lat(log_xy.y))

  def InternalLogOuterBounds(self):
    """Outer mapping bounds of this projection that our transform sees.

    GE Map server 4.4 does not cover its entire tilespace with map
    data, instead it preserves the image aspect ratio (there's twice
    as much longitude space as latitude). The top and bottom 1/4 of
    the tilespace is empty / black.  We correct for this by treating
    the tilespace as representing 360 x 360 instead of 360 x 180.

    Returns:
        Rect of this projection's outer mapping / logical bounds,
        possibly adapted to quirks of the underlying pixel space (as
        here with 4.4's flat mapping).
    """

    xy0 = geom.Pair(self.FlatX(-Flat.MAX_LONGITUDE),
                    self.FlatY(2 * -Flat.MAX_LATITUDE))
    xy1 = geom.Pair(self.FlatX(Flat.MAX_LONGITUDE),
                    self.FlatY(2 * Flat.MAX_LATITUDE))

    return geom.Rect(xy0.x, xy0.y, xy1.x, xy1.y)

  def AdvertizedLogOuterBounds(self):
    """Outer mapping bounds of this projection, that the user sees.

    Returns:
        A Rect of the official, by-the-book logical space of the projection.
    """
    xy0 = geom.Pair(self.FlatX(-Flat.MAX_LONGITUDE),
                    self.FlatY(-Flat.MAX_LATITUDE))
    xy1 = geom.Pair(self.FlatX(Flat.MAX_LONGITUDE),
                    self.FlatY(Flat.MAX_LATITUDE))

    return geom.Rect(xy0.x, xy0.y, xy1.x, xy1.y)

  def LogTotalHeight(self):
    outer = self.logOuterBounds()
    return outer.y1 - outer.y0

  def LogTotalWidth(self):
    outer = self.logOuterBounds()
    return outer.x1 - outer.x0


class Identity(object):
  """An identity, tile bitmap pass-through non-transformation.

  This is to help us be sure of our own math, and might be useful to
  debug / sanity check configurations or other software that uses this
  PyWms.

  WMS 1.3.0 has a CRS:1 transform which this can be advertized as.
  WMS 1.x probably has something equivalent.

  Currently it's just a marker of intent.
  """
  pass


def PrintBBOX(left_lon, lower_lat, right_lon, upper_lat, msg):
  proj = Mercator()
  lrx = proj.MercX(left_lon)
  lry = proj.MercY(lower_lat)
  urx = proj.MercX(right_lon)
  ury = proj.MercY(upper_lat)

  print msg
  print "%f,%f,%f,%f" % (lrx, lry, urx, ury)


def main():
  # Wisconsin, Illinois, and Michigan, for comparison with:
  #   http://demo.mapserver.org/tutorial/example1-6.html
  PrintBBOX(-97.5, 41.619775, -82.122902, 49.38562, "upper midwest")

  # SF Bay area
  PrintBBOX(-123.0, 37.0, -122.0, 38.5, "SF bay area")

if __name__ == "__main__":
  main()
