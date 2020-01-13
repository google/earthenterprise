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

"""Module for implementing the Custom POI search."""

import json
import logging
import logging.config
from string import Template
import urllib2
from xml.etree.cElementTree import SubElement, tostring
import defusedxml.cElementTree as ET
from search.common import exceptions
from search.common import utils

ET.SubElement = SubElement
ET.tostring = tostring

# Get the logger for logging purposes.
logging.config.fileConfig("/opt/google/gehttpd/conf/ge_logging.conf",
                          disable_existing_loggers=False)


class CustomPOISearch(object):
  """Class for performing the Custom POI search.

  Custom POI Search is a nearby search that demonstrates
  how to construct and query an external database based on URL
  search string, extract geometries from the result, associate
  various styles with them and return the response back to the client.

  In this module, an implementation has been provided to
  access the Google Places database, format the results and
  send the response in XML or JSON as per the requirement.

  Valid Inputs are:
  location=lat,lng
  radius = 10(in miles)
  key = <server key required to access the Google Places database>.

  More information on getting the server keys is documented
  at the below link under section 'Authentication'.

  https://developers.google.com/places/documentation/
  """

  def __init__(self):
    """Inits Custom POI Search.

    Initializes the logger "ge_search".
    Initializes templates for kml, json, placemark templates
    for the KML/JSON output.
    """
    # For the sake of simplicity, we presently look for
    # "name", "geometry", "icon", "vicinity" tags only in the
    # response and ignore the others.
    self._reqd_tags = ["name", "geometry", "icon", "vicinity"]

    self._f_name = "Custom POI Search."
    self._no_results = "No search results found."

    self._kml = """<kml xmlns="http://www.opengis.net/kml/2.2"
         xmlns:gx="http://www.google.com/kml/ext/2.2"
         xmlns:kml="http://www.opengis.net/kml/2.2"
         xmlns:atom="http://www.w3.org/2005/Atom">
    <Folder>
         <name>${foldername}</name>
         <open>1</open>
         <Style id="placemark_label">\
               ${style}\
         </Style>\
         ${placemark}
    </Folder>
    </kml>
    """

    self._iconstyle = """
            <IconStyle>
                 <scale>1</scale>
            </IconStyle>\
    """
    self._linestyle = """
            <LineStyle>
                  <color>7fffff00</color>
                  <width>5</width>
            </LineStyle>\
    """
    self._polystyle = """
            <PolyStyle>
                  <color>7f66ffff</color>
                  <colorMode>normal</colorMode>
                  <fill>1</fill>
                  <outline>1</outline>
            </PolyStyle>
    """
    self._style = (
        "%s %s %s"
        % (self._iconstyle, self._linestyle, self._polystyle))

    self._json = """ {\
        "Folder": {\
            "name": "${name}",\
            "Placemark":${placemark}\
            }\
      }\
    """
    self._json_template = Template(self._json)
    self._kml_template = Template(self._kml)

    # URL to access Google Places database.
    # output type(xml or json), location, radius and server key are
    # mandatory parameters required to perform the search.
    self._baseurl = "https://maps.googleapis.com/maps/api/place/nearbysearch"
    self._places_api_url = self._baseurl + "/%s?location=%s&radius=%s&key=%s"

    self.logger = logging.getLogger("ge_search")
    self._content_type = "Content-type, %s"

    self.utils = utils.SearchUtils()

  def FormatKMLResponse(self, xml_data):
    """Prepares KML/XML response.

    KML response has the below format:
      <kml>
       <Folder>
       <name/>
       <StyleURL>
             ---
       </StyleURL>
       <Placemark>
       <Point>
              <coordinates/>
       </Point>
       </Placemark>
       </Folder>
      </kml>

    Args:
     xml_data: Query results from the Google Places database.
    Returns:
     kml_response: KML formatted response.
    """

    kml_response = ""

    xmlstr, total_results = self._RetrievePlacemarks(xml_data)

    if not xmlstr:
      self.logger.debug(self._no_results)
      return self._kml_template.substitute(
          foldername=self._no_results, style="", placemark="")

    folder_name = ("Grouped results:<![CDATA[<br/>]]> %s(%s)"
                   % (self._f_name, total_results))
    kml_response = self._kml_template.substitute(
        foldername=folder_name,
        style=self._style,
        placemark=xmlstr)

    self.logger.info("KML response successfully formatted.")
    return kml_response

  def _RetrievePlacemarks(self, xml_data):
    """Retrieve placemarks from xml data.

    Args:
      xml_data: Query results from the Google Places database.
    Returns:
      xmlstr: XML with placemarks.
      total_placemarks: Total no. of placemarks.
    """

    xmlstr = ""
    total_results = 0
    # Perform XML parsing using cElementTree.
    root = ET.parse(xml_data).getroot()

    for element in root:
      if element.tag == "result":
        # Rename "result" tags as "Placemark" as per KML(XML) requirements.
        element.tag = "Placemark"

        for subelement in element[:]:
          # For the sake of simplicity, we presently look for
          # "name", "geometry", "icon", "vicinity" tags only in the
          # response and ignore the others.
          if subelement.tag not in self._reqd_tags:
            element.remove(subelement)
            continue

          if subelement.tag == "geometry":
            # Extract latitude and longitude coordinates.
            lat = subelement.find("location").find("lat").text
            lng = subelement.find("location").find("lng").text

            # Add "Point" and "coordinates" tags to element.
            point = ET.SubElement(element, "Point")
            coords = ET.SubElement(point, "coordinates")
            coords.text = "%s, %s" %(lng, lat)
            element.remove(subelement)

          # Rename "vicinity" and "icon" tags to
          # "snippet" and "description" as per naming convention
          # being followed in existing Search Services.
          elif subelement.tag == "vicinity":
            subelement.tag = "snippet"
          elif subelement.tag == "icon":
            subelement.tag = "description"

        xmlstr += ET.tostring(element, method="xml")
        total_results += 1

    return (xmlstr, total_results)

  def FormatJSONResponse(self, fp):
    """Formats JSON response.

      {
        "Folder": {
           "name": "Latitude X Longitude Y",
           "Placemark": {
              "Point": {
                 "coordinates": "X,Y" }
           }
        }
       }
    Args:
     fp: File pointer to results from the Google Places database.
    Returns:
     json_response: JSON formatted response.
    """
    # Prepare a python dictionary from json string.
    results = json.load(fp)["results"]

    if not results:
      self.logger.debug(self._no_results)
      return self._json_template.substitute(name=self._no_results,
                                            placemark="\"\"")

    for result in results:
      # For the sake of simplicity, we presently look for
      # "name", "geometry", "icon", "vicinity" tags only in the
      # results and ignore the others.
      for key in result.keys():
        if key not in self._reqd_tags:
          result.pop(key)
          continue
        if key == "geometry":
          # Add "Point" and "coordinates" tags to result.
          result["type"] = "Point"
          # First longitude and then latitude, to be in consistent
          # with the existing search services.
          result["coordinates"] = [result[key]["location"]["lng"],
                                   result[key]["location"]["lat"]]
          result.pop(key)

      # Rename "vicinity" and "icon" tags to
      # "description" and "snippet" as per naming convention
      # being followed in existing search services.
      if result.has_key("vicinity"):
        result["description"] = result.pop("vicinity")
      if result.has_key("icon"):
        result["snippet"] = result.pop("icon")

    json_response = self._json_template.substitute(
        name=self._f_name, placemark=json.dumps(results))
    self.logger.info("JSON response successfully formatted.")
    return json_response


  def GetOutputType(self, response_type):
    """Provide the output type for the Google places search."""
    if response_type == "KML":
      return "xml"
    return "json"


  def HandleSearchRequest(self, environ):
    """Retrieves search tokens from form and performs the custom POI search.

    Args:
     environ: A list of environment variables as supplied by the
      WSGI interface to the custom POI search application interface.
    Returns:
     A XML/JSON formatted string with search results.
    Raises:
     BadQueryException: When all the mandatory parameters are not provided
     to the Google Places database.
    """
    # Retrieve "response_type", "location", "radius", "key" and
    # "callback" parameters from the form.
    parameters = self.utils.GetParameters(environ)
    response_type = self.utils.GetResponseType(environ)
    location = self.utils.GetValue(parameters, "location")
    radius = self.utils.GetValue(parameters, "radius")
    server_key = self.utils.GetValue(parameters, "key")

    # Default "callback" value is "handleSearchResults".
    f_callback = self.utils.GetCallback(parameters)

    if location and server_key and radius:
      output_type = self.GetOutputType(response_type)

      # Create Google Places database URL here.
      url = self._places_api_url % (output_type, location, radius, server_key)
      self.logger.debug("Google Places database URL is %s.", url)

      url_fetch = urllib2.urlopen(url)

      if response_type == "KML":
        return self.FormatKMLResponse(url_fetch)
      elif response_type == "JSONP":
        # Prepare a "jsonp" message as per the requirements of
        # GEE search services for maps.
        return "%s(%s);" % (f_callback, self.FormatJSONResponse(url_fetch))
    else:
      error = ("location, radius or key information not "
               "received by the server.")
      raise exceptions.BadQueryException(error)


def main():
  cpoiobj = CustomPOISearch()

  baseurl = "https://maps.googleapis.com/maps/api/place/nearbysearch/"
  url = baseurl + "%s?location=%s&radius=%s&key=%s"

  # Information on getting the server keys is documented
  # at the below link under section 'Authentication'.
  #
  # https://developers.google.com/places/documentation/
  server_key = "<your_key_here>"

  url_fetch = urllib2.urlopen(url %("xml", "-33.8670522,151.1957362",
                                    "10", server_key))
  cpoiobj.FormatXMLResponse(url_fetch)

if __name__ == "__main__":
  main()
