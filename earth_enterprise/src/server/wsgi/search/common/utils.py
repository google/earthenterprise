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

"""Utils Module for common utils used across all the search functionality."""

import cgi
import ConfigParser
import logging
import logging.config
import re
from string import Template
from xml.sax.saxutils import escape
from search.common import geconstants

# saxutils escape characters like "<", ">" and "&".
# Any other characters to be escaped should be added to the below
# dictionary.
_HTML_ESCAPE_TABLE = {
    '"': "&quot;",
    "'": "&apos;"
}


def HtmlEscape(msg):
  return escape(msg, _HTML_ESCAPE_TABLE)


class SearchUtils(object):
  """Class for adding common utils used for search implementation.
  """

  def __init__(self):
    """Inits SearchUtils.

    Initializes the logger "ge_search".
    Initializes templates for kml, placemark templates for KML/JSONP outputs.
    """

    self.kml = """<?xml version="1.0" encoding="UTF-8"?>
         <kml xmlns="http://www.opengis.net/kml/2.2"
         xmlns:gx="http://www.google.com/kml/ext/2.2"
         xmlns:kml="http://www.opengis.net/kml/2.2"
         xmlns:atom="http://www.w3.org/2005/Atom">
    <Folder>
         <name>${foldername}</name>
         <open>1</open>
         <Style id="placemark_label">\
               ${style}\
         </Style>\
         ${lookat}
         ${placemark}
    </Folder>
    </kml>

    """
    self.lookat = """
           <LookAt>
                 <longitude>%s</longitude>
                 <latitude>%s</latitude>
                 <altitude>%s</altitude>
                 <range>%s</range>
                 <tilt>0.0</tilt>
                 <heading>0.0</heading>
           </LookAt>
    """
    self.placemark = """
         <Placemark>\
            ${geom}
         </Placemark>\
    """
    self.balloonstyle = """
            <BalloonStyle>
                  <bgColor>${balloonBgColor}</bgColor>
                  <textColor>${balloonTextColor}</textColor>
                  <text>${balloonText}</text>
            </BalloonStyle>
    """
    self.iconstyle = """
            <IconStyle>
                 <scale>${iconStyleScale}</scale>
                 <Icon>
                    <href>${iconStyleHref}</href>
                 </Icon>
            </IconStyle>\
    """
    self.linestyle = """
            <LineStyle>
                  <color>${lineStyleColor}</color>
                  <width>${lineStyleWidth}</width>
            </LineStyle>\
    """
    self.polystyle = """
            <PolyStyle>
                  <color>${polyStyleColor}</color>
                  <colorMode>${polyStyleColorMode}</colorMode>
                  <fill>${polyStyleFill}</fill>
                  <outline>${polyStyleOutline}</outline>
            </PolyStyle>
    """

    self.liststyle = """
            <ListStyle>
                  <listItemType>check</listItemType>
                  <bgColor>ffffffff</bgColor>
                  <ItemIcon>
                     <state>open</state>
                     <href>${listStyleHref}</href>
                  </ItemIcon>
            </ListStyle>
    """

    self.style = (
        "%s %s %s %s %s"
        % (self.balloonstyle, self.iconstyle, self.linestyle,
           self.polystyle, self.liststyle))

    self.json_placemark = """\
            "Placemark":${geom}
    """
    self.json = """ {
        "Folder": {
            "name":"${foldername}",
            ${json_placemark}
            }
      }\
    """

    self.jsonp_functioncall = "%s(%s);"
    self.content_type = "Content-type, %s;charset=utf-8"

    self.kml_template = Template(self.kml)
    self.placemark_template = Template(self.placemark)
    self.style_template = Template(self.style)

    self.json_template = Template(self.json)
    self.json_placemark_template = Template(self.json_placemark)

    logging.config.fileConfig("/opt/google/gehttpd/conf/ge_logging.conf",
                              disable_existing_loggers=False)
    self.logger = logging.getLogger("ge_search")

    self.coordinate_regex = (
        r"<Point><coordinates>\s*([-\d.]+),\s*([-\d.]+)\s*,?([\d.]+)?"
        "</coordinates></Point>")

    # TODO:Calculate range based on the geometry size and area.
    self.lookat_range = {
        "POINT": 1000.0,
        "LINE_STRING": 3000000.0,
        "POLYGON": 3000000.0,
        "MULTIPOLYGON": 3000000.0,
        "ANY": 3000000.0
        }

    self.output_formats = ["KML", "JSONP"]

  def NoSearchResults(self, folder_name, style, response_type, callback=None):
    """Prepares KML/JSONP output in case of no search results.

    Args:
      folder_name: Name of the folder which is used in folder->name
        in the response.
      style: Style information for KML.
      response_type: Response type can be KML or JSONP, depending on the client.
      callback: Call back function for the JSONP response.
    Returns:
       search_results: An empty KML/JSONP file with no search results.
    """
    search_results = ""

    if response_type == "KML":
      search_results = self.kml_template.substitute(
          foldername=folder_name, style=style, lookat="", placemark="")
    elif response_type == "JSONP":
      search_results = self.json_template.substitute(
          foldername=folder_name, json_placemark="")
      search_results = self.jsonp_functioncall % (callback, search_results)
    else:
      self.logger.debug("%s response_type not supported", response_type)

    self.logger.info("search didn't return any results")

    return search_results

  def GetContentType(self, response_type):
    """Content Type based on the response type.

    Args:
     response_type: Whether response is KML or JSONP.
    Returns:
     Content-Type based on the response type.
    """
    assert response_type in ["KML", "JSONP"], self.logger.exception(
        "Invalid response type %s", response_type)

    if response_type == "KML":
      return self.content_type %("application/"
                                 "vnd.google-earth.kml+xml")
    elif response_type == "JSONP":
      return self.content_type %("application/javascript")

  def GetAccumFunc(self, response_type):
    """Accumulative function based on response type.

    Args:
     response_type: Whether response is KML or JSONP.
    Returns:
      Accumulation function for geometry types based on response_type.
    """
    if response_type == "KML":
      return "ST_AsKML"
    elif response_type == "JSONP":
      return "ST_AsGeoJSON"

  def GetResponseType(self, environ):
    """Get the response type KML or JSONP.

    Args:
     environ: A list of environment variables as supplied by the
      WSGI interface to the search application interface.
    Returns:
     response_type: Whether response is KML or JSONP.
    """

    # User-Agent header for client requests coming from GoogleEarthiClient(EC)
    # would be as below
    #
    # 'User-Agent: GoogleEarth/7.0.3.8542(X11;Linux(3.2.5.0);en;kml:2.2;
    #  client:EC;type:default)'.
    #
    # User-Agent header for client requests coming from  a browser would be as
    # below
    #
    # 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko)
    #  Chrome/27.0.1453.73 Safari/537.36'
    #
    # The above two User-Agent header values can be used to identify
    # the type of client
    # If the header value has the string "client:EC" then its from EC
    # otherwise a MAPS

    user_agent_header_value = environ["HTTP_USER_AGENT"]

    # Fetch all the attributes provided by the user.
    parameters = self.GetParameters(environ)

    # Fetch "output" parameter value from the URL.
    # Supplemental UI sends search requests with "output" as
    # query parameter.
    output_type = self.GetValue(parameters, "output")

    if output_type:
      if output_type.lower().find("kml") >= 0:
        return "KML"
      elif output_type.lower().find("jsonp") >= 0:
        return "JSONP"

    if user_agent_header_value.find("client:EC") > 0:
      return "KML"
    else:
      return "JSONP"

  def GetParameters(self, environ):
    """Fetch the query attributes from the HTTP request.

    Args:
      environ: A list which contains the WSGI environment information.
    Returns:
      All of the query parameters like "q","DbId"  etc.
    """
    parameters = {}
    form = cgi.FieldStorage(fp=environ["wsgi.input"], environ=environ)

    for key in form.keys():
      parameters[key.lower()] = form.getvalue(key)

    return parameters

  def GetValue(self, dictionary, key_no_case):
    """Gets a value from a case-insensitive key.

    Args:
      dictionary: dict of key -> value
      key_no_case: your case-insensitive key.

    Returns:
      Value of first case-insensitive match for key_no_case.

    """
    key = key_no_case.lower()

    if dictionary.has_key(key):
      return dictionary[key]

    return None

  def GetCallback(self, parameters):
    """Gets a call back name for the JSONP response.

    Args:
      parameters: All of the query parameters like "q","DbId"  etc.
    Returns:
      Call back name.

    """
    # Retrieve the function call back name from URI for JSONP response.
    f_callback = self.GetValue(parameters, "callback")

    # If function call back name is not available, then
    # default it to "handleSearchResults".
    if not f_callback:
      f_callback = "handleSearchResults"

    return f_callback

  def GetLookAtRange(self, geom_type):
    """Range for the <LookAt> tag.

    Args:
     geom_type: Geometry type.
    Returns:
      Range in meters.
    """

    if geom_type in self.lookat_range.keys():
      return self.lookat_range[geom_type]
    else:
      return self.lookat_range["ANY"]

  def GetCoordinates(self, geom):
    """Fetch coordinates from geometry.

    Args:
     geom: Geometry data.
    Returns:
      Longitude, latitude and altitude information.
    """

    # This method is executed for Geometry types of 'POINT' only.
    # For POINT types, there is always longitude, latitude and
    # altitude information.
    match = re.search(self.coordinate_regex, geom)

    if match:
      longitude = match.group(1)
      latitude = match.group(2)
      altitude = match.group(3)
    else:
      self.logger.error("Unable to retrieve coordinates from %s", geom)

    return longitude, latitude, altitude

  def GetLookAtInfo(self, geom):
    """Populate <LookAt> tag in KML.

    Args:
     geom: Geometry data.
    Returns:
      Populated <LookAt> tag in KML.
    """
    (longitude, latitude, altitude) = self.GetCoordinates(geom)

    # Presently for 'POINT' geometry types only.
    lookat_range = self.GetLookAtRange("POINT")

    lookat_info = self.lookat %(longitude, latitude, altitude, lookat_range)

    return lookat_info

  def GetCityView(self, search_query):
    """Get city view based on the search_query encoding(ascii or utf8).

    Args:
     search_query: Search input as entered by the user.
    Returns:
      Cities view.
    """
    try:
      search_query = search_query.encode("ascii")
      return "cities"
    except UnicodeDecodeError, unused_e:
      return "cities_utf8"

  def SearchTokensFromString(self, tokens_string, delimiter=","):
    """Extracts search tokens from the given string.

    Splits the given string into tuple of tokens. If tokens are quoted,
    then outer quotes are removed. White space before or after a token is
    ignored. Note that the delimiter cannot be included in a token,
    even with quoting. Quotes (single or double), however, can be included
    within a token without escaping.
    Note: allows not paired single/double quote at end of tokens string.

    Args:
      tokens_string: String to be split into search tokens.
      delimiter: Cannot be space or tab character.
    Returns:
      Tuple of search tokens.
    Raises:
      ValueError: if any token is partially quoted.
    """
    tokens = []
    state = "PRE-TOKEN"
    next_token = ""
    cur_delimiter = delimiter
    i = 0
    for ch in tokens_string:
      i += 1  # Keep location so we can treat end like delimiter.
      # Waiting to start next token.
      if state == "PRE-TOKEN":
        # skip white space.
        if ch in " \t":
          pass
        # Ignore blank token or delimiter after single/double quote.
        elif ch == cur_delimiter:
          # e.g. ', 23.47, -123.27', '"23.47", "-123.27"'
          pass
        elif ch == '"':
          state = "EXPECT_DOUBLE_QUOTES"
          cur_delimiter = ch
        elif ch == "'":
          state = "EXPECT_SINGLE_QUOTE"
          cur_delimiter = ch
        else:
          state = "IN_TOKEN"
          next_token += ch
      # Already in a token.
      else:
        if ch != cur_delimiter:
          next_token += ch
        # If delimiter or end of string, check the token and
        # add it if it is valid.
        if ch == cur_delimiter or len(tokens_string) == i:
          if state == "EXPECT_DOUBLE_QUOTES" or state == "EXPECT_SINGLE_QUOTE":
            assert ((state == "EXPECT_DOUBLE_QUOTES" and
                     cur_delimiter == '"') or
                    (state == "EXPECT_SINGLE_QUOTE" and cur_delimiter == "'"))
            if ch == cur_delimiter:
              # Ignore white space at end of token.
              tokens.append(next_token.strip())
              next_token = ""
              cur_delimiter = delimiter
              state = "PRE-TOKEN"
            else:
              logging.error(
                  "Expected search token to end in single/double quotes.")
              raise ValueError(
                  "Expected search token to end in single/double quotes.")
          else:
            assert cur_delimiter == delimiter
            # Ignore single/double quotes at end of tokens string (last token).
            # Note: we allow not paired single/double quote at end of string.
            # e.g. 23.47, -123.27".
            if len(tokens_string) == i:
              next_token = next_token.rstrip("'\"")
            # Ignore white space at end of token.
            next_token = next_token.strip()
            tokens.append(next_token)
            next_token = ""
            state = "PRE-TOKEN"
    return tuple(tokens)

  def _ParseConfigfile(self, config_parser):
    """Parses the config file and creates a python dictionary.

    The dictionary has config parameters and its values as key/value pairs.
    Args:
      config_parser: An instance of ConfigParser().
    Returns:
      A python dictionary.
    """
    tmp_dict = {}
    for section in config_parser.sections():
      for option in config_parser.options(section):
        tmp_dict[option] = config_parser.get(section, option).strip()
    return tmp_dict

  def GetConfigs(self, config_file):
    """Process config parameters.

    Args:
      config_file: File with parameters in "param = value" format.
    Returns:
      A dictionary with config parameters and values as key/value pairs.
    """
    # Get default values for the config parameters.
    defaults = geconstants.Constants().defaults

    # Create a ConfigParser() object and read the config file.
    config_parser = ConfigParser.ConfigParser()
    config_parser.read(config_file)

    # create a dict of config parameters and values.
    configs_from_file = self._ParseConfigfile(config_parser)

    # If configs_from_file has parameters without any value specified,
    # then pick it from the defaults.
    for param, value in configs_from_file.iteritems():
      if not value:
        configs_from_file[param] = (defaults[param] if defaults.has_key(param)
                                    else None)
    return configs_from_file


def main():
  utils = SearchUtils()
  utils.GetSearchQuery([])

  # Tests for SearchTokensFromString().
#  utils.SearchTokensFromString('"Paris, France"', delimiter=",")
#  utils.SearchTokensFromString('"Paris France"', delimiter=",")
#  utils.SearchTokensFromString("Paris, France", delimiter=",")
#  utils.SearchTokensFromString("Paris France", delimiter=",")
#  utils.SearchTokensFromString('"Paris", "France"', delimiter=",")
#  utils.SearchTokensFromString('Paris, France"', delimiter=",")
#  utils.SearchTokensFromString("Paris, France'", delimiter=",")


if __name__ == "__main__":
  main()
