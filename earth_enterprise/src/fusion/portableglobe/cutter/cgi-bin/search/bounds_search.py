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


"""Bounds search for a point at a given LOD (search service example)."""

# This code is an example of an external search service.
#
# The service takes a latitude/longitude point and a given LOD
# (level of detail) and returns the point and the boundary of
# the tile that contains it at that LOD.
#
# The search utils import provides a class containing templates that
# help format the results in either kml or json.
#
# Many search services will only return points and will return the
# same basic information whether the results are formatted in kml
# or json. This example is somewhat unusual in that the bounds of the
# containing tile are  different depending on whether it is kml (WGS84)
# or json (Web Mercator).

import cgi
import math
from common import search_utils


def ToMercDegrees(y, num_tiles):
  """Calculate latitude of southern border of yth tile in degrees.

  Args:
    y: (float) Position of tile in tile grid moving from south to north.
       Non-integer values give latitude within tile.
    num_tiles: (integer) Number of tiles in the tile grid.
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
  """Calculate position of a given latitude on tile grid.

  Args:
    lat_deg: (float) Latitude in degrees.
    num_tiles: (integer) Number of tiles in the tile grid.
  Returns:
    Floating point position of latitude in tiles relative to equator.
  """
  lat_rad = lat_deg / 180.0 * math.pi
  y_merc = math.log(math.tan(lat_rad / 2.0 + math.pi / 4.0))
  return num_tiles / 2.0 * (1 + y_merc / math.pi)


def LongitudinalBounds(lng, num_tiles):
  """Return west/east bounds of tile containing given longitude.

  Args:
    lng: (float) Longitude in degrees.
    num_tiles: (integer) Number of tiles in the tile grid.
  Returns:
    Tuple of (west, east) bounds.
  """
  # Normalize to between -180 and 180 degrees longitude.
  while lng < -180.0:
    lng += 360.0
  while lng >= 180.0:
    lng -= 360.0

  degrees_per_tile = 360.0 / num_tiles
  x = int((lng + 180.0) / degrees_per_tile)
  west = x * degrees_per_tile - 180.0
  return (west, west + degrees_per_tile)


def MercatorBounds(lat, lng, lod):
  """Return bounds of Mercator tile containing given lat/lng at given lod.

  Args:
    lat: (float) Latitude in degrees.
    lng: (float) Longitude in degrees.
    lod: (integer) Level of detail (zoom).
  Returns:
    Dictionary of north, west, south, east bounds.
  """

  num_tiles = 1 << lod

  (west, east) = LongitudinalBounds(lng, num_tiles)

  # Normalize to between -90 and 90 degrees latitude.
  while lat < -90.0:
    lat += 180.0
  while lat >= 90.0:
    lat -= 180.0

  y = int(ToMercPosition(lat, num_tiles))
  south = ToMercDegrees(y, num_tiles)
  north = ToMercDegrees(y + 1, num_tiles)

  return {
      "south": south,
      "north": north,
      "west": west,
      "east": east
      }


def FlatBounds(lat, lng, lod):
  """Return bounds of WGS84 tile containing given lat/lng at given lod.

  Args:
    lat: (float) Latitude in degrees.
    lng: (float) Longitude in degrees.
    lod: (integer) Level of detail (zoom).
  Returns:
    Dictionary of north, west, south, east bounds.
  """
  num_tiles = 1 << lod

  (west, east) = LongitudinalBounds(lng, num_tiles)

  # Normalize to between -90 and 90 degrees latitude.
  while lat < -90.0:
    lat += 180.0
  while lat >= 90.0:
    lat -= 180.0

  # Can reuse longitude calcuation after -90.0/90.0 normalization.
  (south, north) = LongitudinalBounds(lat, num_tiles)
  return {
      "south": south,
      "north": north,
      "west": west,
      "east": east
      }


def BoundsToRawJson(lat, lng, lod, bounds):
  """Convert bounds and original point to json object and outputs the json.

  Args:
    lat: (float) Latitude in degrees.
    lng: (float) Longitude in degrees.
    lod: (integer) Level of detail (zoom).
    bounds: (dictonary)  North, west, south, east bounds.
  Returns:
    raw json containing point and tile bounds.
  """
  utils = search_utils.SearchUtils()
  print "%s\n" % utils.GetContentType("json")

  coordinates = "[%f,%f,0]" % (bounds["west"], bounds["south"])
  coordinates += ",[%f,%f,0]" % (bounds["west"], bounds["north"])
  coordinates += ",[%f,%f,0]" % (bounds["east"], bounds["north"])
  coordinates += ",[%f,%f,0]" % (bounds["east"], bounds["south"])
  coordinates += ",[%f,%f,0]" % (bounds["west"], bounds["south"])

  placemark = utils.json_placemark_polygon_template.substitute(
      name="bounds",
      styleUrl="#placemark_label",
      description="Bounds of tile containing %5.3f,%5.3f at LOD %d." % (
          lat, lng, lod),
      coordinates=coordinates)
  search_placemarks = "%s" % placemark

  placemark = utils.json_placemark_point_template.substitute(
      name="point",
      styleUrl="#placemark_label",
      description="%5.3f,%5.3f" % (lat, lng),
      lat=lat,
      lng=lng)
  search_placemarks += ",%s" % placemark

  json_response = utils.json_template.substitute(
      foldername="Results",
      json_style=utils.json_style,
      json_placemark=search_placemarks)
  return json_response


def BoundsToJson(lat, lng, lod, bounds):
  """Convert bounds and original point to json object and outputs the json.

  Args:
    lat: (float) Latitude in degrees.
    lng: (float) Longitude in degrees.
    lod: (integer) Level of detail (zoom).
    bounds: (dictonary)  North, west, south, east bounds.
  """
  print BoundsToRawJson(lat, lng, lod, bounds)


def BoundsToJsonp(lat, lng, lod, bounds, function_name):
  """Convert bounds and original point to json object and outputs the json.

  Args:
    lat: (float) Latitude in degrees.
    lng: (float) Longitude in degrees.
    lod: (integer) Level of detail (zoom).
    bounds: (dictonary)  North, west, south, east bounds.
    function_name: (string) Name of function to call on json argument.
  """
  print "%s('%s');" % (function_name, BoundsToRawJson(lat, lng, lod, bounds))


def BoundsToKml(lat, lng, lod, bounds):
  """Convert bounds and original point to kml and outputs the kml.

  Args:
    lat: (float) Latitude in degrees.
    lng: (float) Longitude in degrees.
    lod: (integer) Level of detail (zoom).
    bounds: (dictonary)  North, west, south, east bounds.
  """
  utils = search_utils.SearchUtils()
  print "%s\n" % utils.GetContentType("kml")

  coordinates = "%f,%f,0" % (bounds["west"], bounds["south"])
  coordinates += " %f,%f,0" % (bounds["west"], bounds["north"])
  coordinates += " %f,%f,0" % (bounds["east"], bounds["north"])
  coordinates += " %f,%f,0" % (bounds["east"], bounds["south"])
  coordinates += " %f,%f,0" % (bounds["west"], bounds["south"])

  placemark = utils.placemark_polygon_template.substitute(
      name="bounds",
      styleUrl="#placemark_label",
      description="Bounds of tile containing %5.3f,%5.3f at LOD %d." % (
          lat, lng, lod),
      coordinates=coordinates)
  search_placemarks = "\n%s" % placemark

  placemark = utils.placemark_point_template.substitute(
      name="point",
      styleUrl="#placemark_label",
      description="%5.3f,%5.3f" % (lat, lng),
      lat=lat,
      lng=lng)
  search_placemarks += "\n%s" % placemark

  kml_response = utils.kml_template.substitute(
      foldername="Results",
      style=utils.style,
      placemark=search_placemarks)
  print kml_response


def DoBoundsSearch(lat, lng, lod, output, function_name=""):
  """Finds bounds in corresponding projection and returns in requested format.

  Args:
    lat: (float) Latitude in degrees.
    lng: (float) Longitude in degrees.
    lod: (integer) Level of detail (zoom).
    output: (string) "json" or "kml"
    function_name: (string) Name of function to call on json argument.
  """

  if output == "json":
    # For json request, assume 2d Mercator request.
    bounds = MercatorBounds(lat, lng, lod)
    BoundsToJson(lat, lng, lod, bounds)
  elif output == "kml":
    # For kml request, assume 3d flat request.
    bounds = FlatBounds(lat, lng, lod)
    BoundsToKml(lat, lng, lod, bounds)
  else:
    # Return jsonp (javascript with function call) by default.
    # For jsonp request, assume 2d Mercator request.
    bounds = MercatorBounds(lat, lng, lod)
    BoundsToJsonp(lat, lng, lod, bounds, function_name)


def Fail(message):
  """Fail with the given message."""
  print "Content-type: text/plain\n"
  print message
  raise Exception(message)


def main():
  form = cgi.FieldStorage()
  try:
    lat = float(form.getvalue("lat").strip())
  except ValueError:
    Fail("Bad latitude.")
    return
  except AttributeError:
    Fail("No latitude parameter.")
    return

  try:
    lng = float(form.getvalue("lng").strip())
  except ValueError:
    Fail("Bad longitude.")
    return
  except AttributeError:
    Fail("No longitude parameter.")
    return

  try:
    lod = int(form.getvalue("lod").strip())
  except ValueError:
    lod = 5  # Default LOD.
  except AttributeError:
    lod = 5  # Default LOD.

  function_name = form.getvalue("callback")
  if not function_name:
    function_name = " jsonSearchResults"

  try:
    output = form.getvalue("output").strip()
  except AttributeError:
    output = "kml"

  DoBoundsSearch(lat, lng, lod, output, function_name)

if __name__ == "__main__":
  main()
