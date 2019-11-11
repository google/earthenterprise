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

"""Module for implementing the Coordinate search."""

import os
from string import Template
import sys
from search.common import exceptions
from search.common import geconstants
from search.common import utils
from search.plugin import coordinate_transform


class CoordinateSearch(object):
  """Class for performing the Coordinate search.

  Coordinate search supports the following formats:
  1. Decimal Degrees (DD)
  2. Degrees Minutes Seconds (DMS)
  3. Degrees Decimal Minutes (DDM)
  4. Military Grid Reference System (MGRS)
  5. Universal Transverse Mercator (UTM)

  Coordinate search transforms coordinates from DMS, DDM, UTM, MGRS formats to
  DD, validates the coordinates and sends the response back to the client.
  Depending on the client type, KML or JSONP formats are supported.
  """

  NUM_OF_COORDS_IN_LAT_LNG_FORMAT = 2
  NUM_OF_COORDS_IN_MGRS_FORMAT = 1

  def __init__(self):
    """Inits CoordinateSearch.

    Initializes the logger "ge_search".
    Initializes templates for kml, placemark templates for KML/JSONP outputs.
    """

    self.utils = utils.SearchUtils()
    self._transform = coordinate_transform.CoordinateTransform()

    configs = self.utils.GetConfigs(
        os.path.join(geconstants.SEARCH_CONFIGS_DIR, "CoordinateSearch.conf"))

    self._jsonp_call = self.utils.jsonp_functioncall

    self._geom = """
             <name>%s</name>
             <styleUrl>%s</styleUrl>
             <Point>
               <coordinates>%s,%s</coordinates>
             </Point>\
    """
    self._json_geom = """
       {
         "Point": {
                "coordinates": "%s,%s"
          }
       }
    """
    self._kml = """

    <kml xmlns="http://www.opengis.net/kml/2.2"
         xmlns:gx="http://www.google.com/kml/ext/2.2"
         xmlns:kml="http://www.opengis.net/kml/2.2"
         xmlns:atom="http://www.w3.org/2005/Atom">
       <Folder>
         <name>Coordinate Search Results</name>
         <open>1</open>
         <Style id="placemark_label">\
            ${style}
         </Style>\
         ${placemark}
        </Folder>
    </kml>

    """
    self._kml_template = Template(self._kml)

    self._placemark_template = self.utils.placemark_template

    self._json_template = self.utils.json_template
    self._json_placemark_template = self.utils.json_placemark_template
    style_template = self.utils.style_template
    self.coordinates_in_lat_lng_format_ = ["DD", "DMS", "DDM"]

    self.logger = self.utils.logger

    self._style = style_template.substitute(
        balloonBgColor=configs.get("balloonstyle.bgcolor"),
        balloonTextColor=configs.get("balloonstyle.textcolor"),
        balloonText=configs.get("balloonstyle.text"),
        iconStyleScale=configs.get("iconstyle.scale"),
        iconStyleHref=configs.get("iconstyle.href"),
        lineStyleColor=configs.get("linestyle.color"),
        lineStyleWidth=configs.get("linestyle.width"),
        polyStyleColor=configs.get("polystyle.color"),
        polyStyleColorMode=configs.get("polystyle.colormode"),
        polyStyleFill=configs.get("polystyle.fill"),
        polyStyleOutline=configs.get("polystyle.outline"),
        listStyleHref=configs.get("iconstyle.href"))

  def HandleSearchRequest(self, environ):
    """Fetches the search tokens from form and performs the coordinate search.

    Args:
     environ: A list of environment variables as supplied by the
      WSGI interface to the coordinate search application interface.
    Returns:
     search_results: A KML/JSONP formatted string which contains search results.
    Raises:
     BadQueryException: if the search query is invalid.
    """
    search_results = ""

    # Fetch all the attributes provided by the user.
    parameters = self.utils.GetParameters(environ)
    response_type = self.utils.GetResponseType(environ)

    # Retrieve the function call back name for JSONP response.
    self.f_callback = self.utils.GetCallback(parameters)

    original_query = self.utils.GetValue(parameters, "q")

    if not original_query:
      msg = "Empty search query received."
      self.logger.error(msg)
      raise exceptions.BadQueryException(msg)

    search_status, search_results = self.DoSearch(original_query, response_type)

    if not search_status:
      folder_name = "Search returned no results."
      search_results = self.utils.NoSearchResults(
          folder_name, self._style, response_type, self.f_callback)

    return (search_results, response_type)

  def DoSearch(self, search_query, response_type):
    """Performs the coordinate search.

    Args:
     search_query: A string containing the search coordinates as
      entered by the user.
     response_type: Response type can be KML or JSONP, depending on the client.
    Returns:
     search_results: A KML/JSONP formatted string which contains search results.
    Raises:
     BadQueryException: if the search query is invalid.
    """
    coordinate_type = ""
    search_results = ""

    input_coordinates = []
    decimal_degrees_coordinates = []

    search_tokens = self.utils.SearchTokensFromString(search_query)
    self.logger.debug("coordinates: %s", ",".join(search_tokens))
    input_coordinates = self._transform.GetInputCoordinates(
        ",".join(search_tokens))
    number_of_coordinates = len(input_coordinates)

    if number_of_coordinates == 0:
      msg = "Incomplete search query %s submitted" % search_query
      self.logger.error(msg)
      raise exceptions.BadQueryException(msg)

    coordinate_type = self._transform.GetInputType(input_coordinates)

    self.logger.debug("Coordinate type is %s.", coordinate_type)

    if coordinate_type in self.coordinates_in_lat_lng_format_:
      reqd_num_of_coordinates = CoordinateSearch.NUM_OF_COORDS_IN_LAT_LNG_FORMAT
    else:
      reqd_num_of_coordinates = CoordinateSearch.NUM_OF_COORDS_IN_MGRS_FORMAT

    if number_of_coordinates > reqd_num_of_coordinates:
      self.logger.warning(
          "extra search parameters ignored: %s", ",".join(
              input_coordinates[reqd_num_of_coordinates:]))
      input_coordinates = input_coordinates[:reqd_num_of_coordinates]
    elif number_of_coordinates < reqd_num_of_coordinates:
      msg = "Incomplete search query %s submitted" % search_query
      self.logger.error(msg)
      raise exceptions.BadQueryException(msg)

    decimal_degrees_coordinates = self._transform.TransformToDecimalDegrees(
        coordinate_type, input_coordinates)

    search_results = self.ConstructResponse(
        response_type, decimal_degrees_coordinates)

    search_status = True if search_results else False

    return search_status, search_results

  def ConstructKMLResponse(self, latitude, longitude):
    """Prepares KML response.

    KML response has the below format:
      <kml>
       <Folder>
       <name/>
       <StyleURL>
             ---
       </StyleURL>
       <Point>
              <coordinates/>
       </Point>
       </Folder>
      </kml>

    Args:
     latitude: latitude in Decimal Degress format.
     longitude: longitude in Decimal Degress format.
    Returns:
     kml_response: KML formatted response.
    """

    placemark = ""
    kml_response = ""

    name = "%s, %s" % (latitude, longitude)
    style_url = "#placemark_label"
    geom = self._geom % (name, style_url, str(longitude), str(latitude))
    placemark = self._placemark_template.substitute(geom=geom)
    kml_response = self._kml_template.substitute(
        style=self._style, placemark=placemark)

    self.logger.info("KML response successfully formatted")

    return kml_response

  def ConstructJSONPResponse(self, latitude, longitude):
    """Prepares JSONP response.

      {
               "Folder": {
                 "name": "X,Y",
                 "Style": {
                     "IconStyle": {"scale": "1" },
                   "LineStyle": {
                       "color": "7fffff00",
                       "width": "5" },
                   "PolyStyle": {
                       "color": "7f66ffff",
                       "fill": "1",
                       "outline": "1" } },
                 "Placemark": {
                    "Point": {
                      "coordinates": "X,Y" } }
                 }
       }
    Args:
     latitude: latitude in Decimal Degress format.
     longitude: longitude in Decimal Degress format.
    Returns:
     jsonp_response: JSONP formatted response.
    """
    placemark = ""
    json_response = ""
    jsonp_response = ""

    folder_name = "%s, %s" % (latitude, longitude)
    json_geom = self._json_geom % (latitude, longitude)

    placemark = self._json_placemark_template.substitute(
        geom=json_geom)
    json_response = self._json_template.substitute(
        foldername=folder_name, json_placemark=placemark)

    # Escape single quotes from json_response.
    json_response = json_response.replace("'", "\\'")

    jsonp_response = self._jsonp_call % (self.f_callback, json_response)

    self.logger.info("JSONP response successfully formatted")

    return jsonp_response

  def ConstructResponse(self, response_type, decimal_degrees_coordinates):
    """Construct the response based on response_type.

    Args:
     response_type: Response type can be KML or JSONP, depending on the client.
     decimal_degrees_coordinates: List of coordinates in DD(Decimal Degrees)
      format.
    Returns:
     search_results: A KML/JSONP formatted string which contains search results.
    """
    search_results = ""

    assert response_type in self.utils.output_formats, (
        self.logger.error("Invalid response type %s", response_type))

    if response_type == "KML":
      search_results = self.ConstructKMLResponse(
          decimal_degrees_coordinates[0], decimal_degrees_coordinates[1])
    elif response_type == "JSONP":
      search_results = self.ConstructJSONPResponse(
          decimal_degrees_coordinates[0], decimal_degrees_coordinates[1])

    return search_results


def main(coords, response_type):
  gepobj = CoordinateSearch()
  gepobj.DoSearch(coords, response_type)


if __name__ == "__main__":
  main(sys.argv[1], sys.argv[2])
