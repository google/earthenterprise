#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-
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

"""Module for implementing the Coordinate Transformation."""

import math
import re
import sys

import mgrs
from search.common import exceptions
from search.common import utils


class CoordinateTransform(object):
  """Class for performing the Coordinate Transformation.

  Coordinate transformation supports the following formats:
  1. Decimal Degrees (DD)
  2. Degrees Minutes Seconds (DMS)
  3. Degrees Decimal Minutes (DDM)
  4. Military Grid Reference System (MGRS)
  5. Universal Transverse Mercator (UTM)
  """

  # Below constants are used in validating the latitude/longitude pair
  LATITUDE_MIN_VALUE = -90
  LATITUDE_MAX_VALUE = 90
  LONGITUDE_MIN_VALUE = -180
  LONGITUDE_MAX_VALUE = 180

  MINUTES_UNITS = 60
  SECONDS_UNITS = 3600

  # Below constants are used in UTM transformation to DD

  # K0 is scale along central meridian of zone factor
  K0 = 0.9996

  # radius of earth measured in meters
  EARTH_RADIUS = 6378137.0

  # eccentricity of earth squared
  ECC_SQUARED = 0.00669438

  # adjustment factor for easting the coordinates
  EASTING_ADJ_FACTOR = 500000.0

  # adjustment factor for northing the coordinates
  NORTHING_ADJ_FACTOR = 10000000.0

  def __init__(self):
    """Inits CoordinateTransform.

    Initializes the logger "ge_search".
    """

    self._utils = utils.SearchUtils()
    self._logger = self._utils.logger

    self._dms_pattern = (
        r"\s*(\d+)\s*°\s*(\d+)\s*(?:ʹ|′|')\s*(\d+\.?\d*)\s*(?:ʺ|\"|″)"
        r"\s*([EWNS])\s*")
    self._ddm_pattern = (
        r"\s*(\d+)\s*°\s*(\d+\.?\d*)\s*(?:ʹ|′|')\s*([EWNS])\s*")
    self._dd_pattern = r"\s*(-?\d+\.?\d*)\s*(?:°)?\s*[EWNS]?"
    self._mgrs_pattern = r"\s*\d?\d[A-Z]([A-Z][A-Z])?(\d\d){0,5}\s*"
    self._utm_pattern = (
        r"(\d?\d)\s*([A-Z])\s*(\d+\.?\d*)\s*M\s*E\s*"
        r"\s*(\d+\.?\d*)\s*M\s*N\s*")

    # List of lists for storing the input pattern and types
    self._input_pattern_types = [
        [self._dms_pattern, "DMS"],
        [self._ddm_pattern, "DDM"],
        [self._utm_pattern, "UTM"],
        [self._mgrs_pattern, "MGRS"],
        [self._dd_pattern, "DD"]]

    self._valid_latitude_directions = ["S", "N"]
    self._valid_longitude_directions = ["E", "W"]
    self._dms_negative_directions = ["S", "W"]

    self._e1 = ((1 - math.sqrt(1- CoordinateTransform.ECC_SQUARED))/
                (1 + math.sqrt(1- CoordinateTransform.ECC_SQUARED)))
    self._e1_prime = (
        CoordinateTransform.ECC_SQUARED/(1 - CoordinateTransform.ECC_SQUARED))

  def TransformToDecimalDegrees(self, coordinate_type, coordinates):
    """Performs a coordinate transformation from DMS, DDM, MGRS, UTM to DD.

    Args:
     coordinate_type: Format of coordinates, can be either of DD, DMS, DDM,
      UTM, MGRS.
     coordinates: List of coordinates in coordinate_type format.
    Returns:
     decimal_degrees_coordinates: List of coordinates in DD(Decimal Degrees)
      format.
    Raises:
     BadQueryException: if the coordinates are invalid.
    """

    conversion_status = False
    decimal_degrees_coordinates = []

    if coordinate_type == "DMS":
      conversion_status, decimal_degrees_coordinates = self.TransformDMSToDD(
          coordinates)
    elif coordinate_type == "DDM":
      conversion_status, decimal_degrees_coordinates = self.TransformDDMToDD(
          coordinates)
    elif coordinate_type == "MGRS":
      conversion_status, decimal_degrees_coordinates = self.TransformMGRSToDD(
          coordinates)
    elif coordinate_type == "UTM":
      conversion_status, decimal_degrees_coordinates = self.TransformUTMToDD(
          coordinates)
    elif coordinate_type == "DD":
      conversion_status, decimal_degrees_coordinates = self.TransformDDToDD(
          coordinates)
    else:
      self._logger.error(
          "Invalid type %s for coordinates %s.", coordinate_type, coordinates)

    self._logger.debug(
        "Transformation of %s to DD format %s",
        coordinates, ("successful" if conversion_status else "Failed"))

    if (not conversion_status or len(decimal_degrees_coordinates) != 2 or
        not self.IsValidCoordinatePair(
            float(decimal_degrees_coordinates[0]),
            float(decimal_degrees_coordinates[1]))):
      msg = "Couldn't transform coordinates: %s of type: %s" % (
          coordinates, coordinate_type)
      self._logger.error(msg)
      raise exceptions.BadQueryException(msg)

    return decimal_degrees_coordinates

  def TransformDDToDD(self, coordinates):
    """Removes non-ASCII characters.

    Valid coordinates in DD format are as below:
      Latitude: -49(degrees)
      Longitude: 131.345(degrees)

      Latitude: 19.345
      Longitude: -89

    Invalid values are:
      Latitude: 95(degrees)
      Longitude: 183.345(degrees)

      Latitude: -93
      Longitude: -194

    Args:
     coordinates: List of coordinates in DD format.
    Returns:
     tuple containing
      conversion_status: Whether format conversion to decimal degrees succeeded.
      decimal_degrees_coordinates: List of coordinates in DD(Decimal Degrees)
       format, after removing any non-ASCII characters.
    """
    # if the coordinate format is DD, then ignore the Unicode
    # characters(like degrees) and return the Decimal Degrees
    conversion_status = True
    is_valid_input = False

    decimal_degrees_coordinates = []

    self._logger.debug(
        "Performing DD format change to DD on coordinates %s, %s.",
        coordinates[0], coordinates[1])

    for coordinate in coordinates:
      matched = re.match(self._dd_pattern, coordinate)
      if matched:
        try:
          (degrees,) = matched.groups()
          decimal_degrees_coordinates.append(float(degrees))
          is_valid_input = True
        except ValueError, e:
          is_valid_input = False
          self._logger.error(
              "DD coordinates %s not in proper format, %s.", coordinate, e)

      conversion_status = conversion_status and is_valid_input

    return (conversion_status, decimal_degrees_coordinates)

  def TransformDMSToDD(self, coordinates):
    """Performs a coordinate transformation from DMS format to DD.

    Valid coordinates in DMS format are as below:
      Latitude: 49(degrees)32(minutes)10.76(seconds) N
      Longitude: 131(degrees)9(minutes)22.34(seconds) E

      Latitude: 19(degrees)11(minutes)0.789(seconds) S
      Longitude: 89(degrees)59(minutes) 12.23456(seconds) W

    Invalid values are:
      Latitude: 74(degrees)22.876(minutes)25(seconds) N
      Longitude: 131(degrees)99(minutes)34.56(seconds) W

      Latitude: 19(degrees)11(minutes)23.789(seconds) W
      Longitude: 89(degrees)59(minutes)11.23(seconds) S

    Latitude's in Southern Hemisphere take negative values and those
    in Nothern, positive
    Longitude's in Western Hemisphere take negative values and those
    in Eastern, positive

    Args:
     coordinates: List of coordinates in DMS(Degrees, Minutes, Seconds) format.
    Returns:
     tuple containing
      conversion_status: Whether format conversion to decimal degrees succeeded.
      decimal_degrees_coordinates: List of coordinates in DD(Decimal Degrees)
       format.
    """

    conversion_status = False
    is_valid_latitude = False
    is_valid_longitude = False

    decimal_degrees_coordinates = []

    self._logger.debug(
        "Performing DMS to DD transformation on coordinates %s, %s.",
        coordinates[0], coordinates[1])

    for (iteration, coordinate) in enumerate(coordinates):
      matched = re.match(self._dms_pattern, coordinate)
      if matched:
        try:
          (degrees, minutes, seconds, direction) = matched.groups()

          decimal_degrees = (
              float(degrees) + (
                  (float(minutes) * CoordinateTransform.MINUTES_UNITS) +
                  float(seconds))/CoordinateTransform.SECONDS_UNITS)

          if direction in self._dms_negative_directions:
            decimal_degrees = -decimal_degrees

          if iteration == 0 and direction in self._valid_latitude_directions:
            is_valid_latitude = True
          if iteration == 1 and direction in self._valid_longitude_directions:
            is_valid_longitude = True

          decimal_degrees_coordinates.append(decimal_degrees)
        except ValueError, e:
          self._logger.error(
              "DMS coordinates %s not in proper format, %s.", coordinate, e)

    conversion_status = is_valid_latitude and is_valid_longitude

    return (conversion_status, decimal_degrees_coordinates)

  def TransformDDMToDD(self, coordinates):
    """Performs a coordinate transformation from DDM format to DD.

    Valid coordinates in DDM format are as below:
      Latitude: 49(degrees)32.876(minutes) N
      Longitude: 131(degrees)9.193(minutes) E
      Latitude: 19(degrees)11(minutes) S
      Longitude: 89(degrees)59.193(minutes) W

    Invalid values are:
      Latitude: 74(degrees)22.876(minutes) E
      Longitude: 131(degrees)99.193(minutes) N
      Latitude: 19(degrees)11(minutes)12.23(seconds) N
      Longitude: 89(degrees)59.193(minutes)12(seconds) W

    Latitude's in Southern Hemisphere take negative values and those
    in Nothern, positive
    Longitude's in Western Hemisphere take negative values and those
    in Eastern, positive

    Args:
     coordinates: List of coordinates in DDM(Degrees, Decimal Minutes) format.
    Returns:
     tuple containing
     conversion_status: Whether format conversion to decimal degrees succeeded.
     decimal_degrees_coordinates: List of coordinates in DD(Decimal Degrees)
      format.
    """

    conversion_status = False
    is_valid_latitude = False
    is_valid_longitude = False

    decimal_degrees_coordinates = []

    self._logger.debug(
        "Performing DDM to DD transformation on coordinates %s, %s.",
        coordinates[0], coordinates[1])

    for (iteration, coordinate) in enumerate(coordinates):
      matched = re.match(self._ddm_pattern, coordinate)
      if matched:
        try:
          (degrees, minutes, direction) = matched.groups()

          decimal_degrees = (
              float(degrees) +
              float(minutes)/CoordinateTransform.MINUTES_UNITS)

          if direction in self._dms_negative_directions:
            decimal_degrees = -decimal_degrees

          if iteration == 0 and direction in self._valid_latitude_directions:
            is_valid_latitude = True
          if iteration == 1 and direction in self._valid_longitude_directions:
            is_valid_longitude = True
          decimal_degrees_coordinates.append(decimal_degrees)
        except ValueError, e:
          self._logger.error(
              "DDM coordinates %s not in proper format, %s.", coordinate, e)

    conversion_status = is_valid_latitude and is_valid_longitude

    return (conversion_status, decimal_degrees_coordinates)

  def TransformMGRSToDD(self, coordinate):
    """Performs a coordinate transformation from MGRS format to DD.

    Valid coordinates in MGRS format are as below:
      19LCC3119933278
      49MEU7917581850
    Invalid values are:
      237CC3119933278
      72KCC3119933278

    MGRS stands for Military Grid Reference System.
    Each MGRS coordinate transforms into a latitude and longitude values.

    Args:
     coordinate: A list containing coordinates in MGRS format.
    Returns:
     tuple containing
      conversion_status: Whether format conversion to decimal degrees succeeded.
      decimal_degrees_coordinates: List of coordinates in DD(Decimal Degrees)
      format.
    """

    conversion_status = False

    decimal_degrees_coordinates = []

    self._logger.debug(
        "Performing MGRS to DD transformation on coordinates %s.", coordinate)

    try:
      mgrs_obj = mgrs.MGRS()
      (latitude, longitude) = mgrs_obj.toLatLon(str(coordinate[0]))
      conversion_status = True
    except Exception, e:
      self._logger.error("MGRS coordinates %s not in proper format, %s.",
                         coordinate[0], e)

    if conversion_status:
      decimal_degrees_coordinates.append(latitude)
      decimal_degrees_coordinates.append(longitude)

    return (conversion_status, decimal_degrees_coordinates)

  def TransformUTMToDD(self, coordinates):
    """Performs a coordinate transformation from UTM format to DD.

    UTM stands for Universal Transverse Mercator.
    Each UTM coordinate transforms into a latitude and longitude values
    Latitude bands in A, B, I, O, X and Y are not valid
    Zone range should be in 1 to 60
    Zone bands 32X, 34X, 36X are not used

    Valid coordinates in UTM format are as below:
      43 R 637072.95 mE 2825582.86 m N
      43 X 12345 m 345672 m
    Invalid values are:
      61 R 637072.95 mE 2825582.86 m N
      42 I 637072.95 mE 2825582.86 m N

    Args:
     coordinates: A list containing coordinates in UTM format.
    Returns:
     tuple containing
      conversion_status: Whether format conversion to decimal degrees succeeded.
      decimal_degrees_coordinates: List of coordinates in DD(Decimal Degrees)
      format.
    """
    conversion_status = True
    decimal_degrees_coordinates = []

    zone_number = 0
    zone_letter = ""
    easting = 0.0
    northing = 0.0

    self._logger.debug(
        "Performing UTM to DD transformation on coordinates %s.", coordinates)

    matched = re.match(self._utm_pattern, coordinates[0])
    if matched:
      try:
        (zone_number, zone_letter, easting, northing) = matched.groups()
      except Exception, e:
        conversion_status = False
        self._logger.error(
            "UTM coordinates %s not in proper format, %s.", coordinates, e)

    x = float(easting) - CoordinateTransform.EASTING_ADJ_FACTOR
    y = float(northing)

    zone_number = int(zone_number)
    longitude_origin_adjustment = ((zone_number * 6) - 183)

    if (
        (zone_letter in ["A", "B", "I", "O", "Y", "Z"]) or
        (zone_number not in xrange(1, 61)) or
        ((zone_letter in ["X"]) and (zone_number in [32, 34, 36]))
        ):
      conversion_status = False
      self._logger.error("Invalid grid Zone information entered %s.",
                         coordinates)

    if zone_letter < "N":
      y -= CoordinateTransform.NORTHING_ADJ_FACTOR

    # meridional_arc and mu are used in calculating n1, t1, c1, r1 and d1 values
    meridional_arc = float(y/CoordinateTransform.K0)
    mu = (meridional_arc /
          (CoordinateTransform.EARTH_RADIUS *
           (1 - (CoordinateTransform.ECC_SQUARED/4) -
            (3 * math.pow(CoordinateTransform.ECC_SQUARED, 2)/64) -
            (5 * math.pow(CoordinateTransform.ECC_SQUARED, 3)/256))))

    # fp is footprint_latitude
    h1 = 3 * self._e1/2
    h2 = 27 * math.pow(self._e1, 3)/32
    h3 = 21 * math.pow(self._e1, 2)/16
    h4 = 55 * math.pow(self._e1, 4)/32
    h5 = 151 * math.pow(self._e1, 3)/96
    h6 = 1097 * math.pow(self._e1, 4)/512

    x1 = ((h1 - h2) * math.sin(2 * mu))
    x2 = ((h3 - h4) * math.sin(4 * mu))
    x3 = (h5 * math.sin(6 * mu))
    x4 = (h6 * math.sin(8 * mu))
    fp = mu + x1 + x2 + x3 + x4

    n1 = (CoordinateTransform.EARTH_RADIUS/
          math.sqrt(1- (CoordinateTransform.ECC_SQUARED *
                        math.pow(math.sin(fp), 2))))
    t1 = math.pow(math.tan(fp), 2)
    c1 = self._e1_prime * math.pow(math.cos(fp), 2)
    r1 = (CoordinateTransform.EARTH_RADIUS * (
        (1 - CoordinateTransform.ECC_SQUARED)/math.pow(
            1- (CoordinateTransform.ECC_SQUARED *
                math.pow(math.sin(fp), 2)), 1.5)))
    d1 = x/(n1 * CoordinateTransform.K0)

    # q1, q2, q3 and q4 are used in calculating the latitude

    q1 = n1 * math.tan(fp)/r1
    q2 = (math.pow(d1, 2)/2)
    q3 = (5 + (3 * t1) + (10 * c1) - (4 * math.pow(c1, 2)) -
          (9 * self._e1_prime)) * math.pow(d1, 4)/24
    q4 = (61 + (90 * t1) + (298 * c1) + (45 * math.pow(t1, 2)) -
          (3 * math.pow(c1, 2)) - (252 * self._e1_prime)) * math.pow(d1, 6)/720

    # q5, q6 and q7 are used in calculating the longitude

    q5 = d1
    q6 = (1 + (2 * t1) + c1) * math.pow(d1, 3)/6
    q7 = (5 -(2 * c1) + (28 * t1)-(3 * math.pow(c1, 2)) + (8 * self._e1_prime) +
          (24 * math.pow(t1, 2))) * math.pow(d1, 5)/120

    latitude = ((fp - (q1 * (q2 - q3 + q4))) * 180.0/math.pi)
    longitude = longitude_origin_adjustment + (
        (q5 - q6 + q7)/math.cos(fp)) * 180.0/math.pi

    if conversion_status:
      decimal_degrees_coordinates.append(latitude)
      decimal_degrees_coordinates.append(longitude)

    return (conversion_status, decimal_degrees_coordinates)

  def IsValidCoordinatePair(self, latitude, longitude):
    """Validates Latitude/Longitude coordinate pair.

    Valid Latitude's are in range -90 to +90 both values inclusive
    Valid Longitude's are in range -180 to +180 both values inclusive

    Args:
     latitude: latitude in Decimal Degress format(float type).
     longitude: longitude in Decimal Degress format(float type).
    Returns:
     boolean: 'True' if latitude and longitude are valid else 'False'.
    """

    is_valid_latitude = False
    is_valid_longitude = False

    if (
        latitude >= CoordinateTransform.LATITUDE_MIN_VALUE and
        latitude <= CoordinateTransform.LATITUDE_MAX_VALUE
        ):
      is_valid_latitude = True
    else:
      self._logger.error(
          "%s coordinate out of range %s, %s.",
          latitude,
          CoordinateTransform.LATITUDE_MIN_VALUE,
          CoordinateTransform.LATITUDE_MAX_VALUE)

    if (
        longitude >= CoordinateTransform.LONGITUDE_MIN_VALUE and
        longitude <= CoordinateTransform.LONGITUDE_MAX_VALUE
        ):
      is_valid_longitude = True
    else:
      self._logger.error(
          "%s coordinate out of range %s, %s.",
          longitude,
          CoordinateTransform.LONGITUDE_MIN_VALUE,
          CoordinateTransform.LONGITUDE_MAX_VALUE)

    self._logger.debug(
        "Latitude %s validity status : %s, Longitude %s validity status: %s.",
        str(latitude), is_valid_latitude, str(longitude), is_valid_longitude
        )

    return is_valid_latitude and is_valid_longitude

  def GetInputType(self, coordinates):
    """Check the coordinate type. Valid types are DD, DMS, DDM, UTM, MGRS.

    Args:
     coordinates: List containing latitude and longitude pair (both should be
      in same format).
    Returns:
     coordinate_type: coordinate type i.e., DD, DMS, DDM, UTM or MGRS.
    Raises:
     BadQueryException: for invalid coordinate type.
    """
    coordinate_type = ""
    pattern_matched = []

    for pattern, input_type in self._input_pattern_types:
      pattern_obj = re.compile(pattern)
      pattern_matched = [pattern_obj.match(z) is not None for z in coordinates]

      if self.IsValidInputType(pattern_matched):
        coordinate_type = input_type
        break

    if not coordinate_type:
      msg = ("Invalid coordinate type. Supported formats are "
             "DD, DMS, DDM, UTM and MGRS.")
      self._logger.error(msg)
      raise exceptions.BadQueryException(msg)

    return coordinate_type

  def IsValidInputType(self, list_of_matches):
    """Check if all the inputs are of the same format.

    Args:
     list_of_matches: List containing the status(True/False) of each input type.
    Returns:
     boolean: True or False based on whether the input type is valid or not.
    """
    for entry in list_of_matches:
      if not entry:
        return False

    return True

  def GetInputCoordinates(self, search_query):
    """Extract the input coordinates from the search query.

    User can enter the inputs either in a comma or space
    seperated formats.

    Valid inputs are:
      latitude, longitude
       or
      latitude longitude

    Examples:

     38°57'33.8080" S, -095°15'55.7366" W
     38°57'33.8080" S  -095°15'55.7366" W
     38°  57'33.8080" S  -095°15'55.7366" W
     38°57'33.8080" S  -095°15'   55.7366" W
        38°57'   33.8080" S  -095°15'55.7366" W

    Args:
     search_query: Search query as entered by the user.
    Returns:
      input_coordinates: List of coordinates.
    """

    input_coordinates = []
    input_query = search_query.upper()

    if input_query.find(",") > 0:
      input_coordinates = [coordinate
                           for coordinate in input_query.split(",")
                           if coordinate.strip()]
    else:
      for pattern, unused_input_type in self._input_pattern_types:
        input_coordinates = []
        input_query = input_query.strip()
        pattern_obj = re.compile(pattern)
        while input_query:
          matched = pattern_obj.match(input_query)
          if not matched:
            input_coordinates = []
            break
          coordinate = input_query[:matched.end()].strip()
          input_query = input_query[matched.end():].strip()
          input_coordinates.append(coordinate)

        if input_coordinates:
          break

    return input_coordinates


def main(coords):
  obj = CoordinateTransform()
  obj.GetInputType(coords)


if __name__ == "__main__":
  main(sys.argv[1])
