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


"""Utilities for common search functionality."""

# Adapted from wsgi core search utilities.

from string import Template


class SearchUtils(object):
  """Class for adding common utils used for search implementation.
  """

  def __init__(self):
    """Inits SearchUtils."""

    self.kml = """

    <kml xmlns="http://www.opengis.net/kml/2.2"
         xmlns:gx="http://www.google.com/kml/ext/2.2"
         xmlns:kml="http://www.opengis.net/kml/2.2"
         xmlns:atom="http://www.w3.org/2005/Atom">
    <Folder>
         <name>${foldername}</name>
         <open>1</open>
         <Style id="placemark_label">\
               ${style}
         </Style>
         ${placemark}
    </Folder>
    </kml>

    """
    self.placemark_point = """
         <Placemark>
            <name>${name}</name>
            <styleUrl>${styleUrl}</styleUrl>
            <description>${description}</description>
            <Point><coordinates>${lng},${lat},0</coordinates></Point>
         </Placemark>\
    """
    self.placemark_polygon = """
         <Placemark>
            <name>${name}</name>
            <styleUrl>${styleUrl}</styleUrl>
            <description>${description}</description>
            <Polygon>
              <outerBoundaryIs>
                 <LinearRing>
                  <coordinates>${coordinates}</coordinates>
                 </LinearRing>
              </outerBoundaryIs>
            </Polygon>
         </Placemark>\
    """
    self.iconstyle = """
            <IconStyle>
                 <scale>1</scale>
            </IconStyle>\
    """
    self.linestyle = """
            <LineStyle>
                  <color>7fffff00</color>
                  <width>5</width>
            </LineStyle>\
    """
    self.polystyle = """
            <PolyStyle>
                  <color>7f66ffff</color>
                  <colorMode>normal</colorMode>
                  <fill>1</fill>
                  <outline>1</outline>
            </PolyStyle>\
    """
    self.style = (
        "%s %s %s"
        % (self.iconstyle, self.linestyle, self.polystyle))

    self.json_style = """ {
                "IconStyle": {
                    "scale": "1"
                },
                "LineStyle": {
                    "color": "7fffff00",
                    "width": "5"
                },
                "PolyStyle": {
                    "color": "7f66ffff",
                    "fill": "1",
                    "outline": "1"
                }
            }\
    """
    self.json_placemark_point = """
            {
              "name":"${name}",
              "description":"${description}",
              "styleUrl":"${styleUrl}",
              "type":"Point",
              "coordinates": [${lng},${lat},0]
            }
    """
    self.json_placemark_polygon = """
            {
              "name":"${name}",
              "description":"${description}",
              "styleUrl":"${styleUrl}",
              "type":"Polygon",
              "coordinates": [${coordinates}]
            }
    """
    self.json = """
      {
        "Folder": {
            "name":"${foldername}",
            "Style":${json_style},
            "Placemark": [${json_placemark}]
            }
      }
    """

    self.content_type = "Content-type: %s;charset=utf-8"

    # Use these templates to build kml.
    self.kml_template = Template(self.kml)
    self.placemark_point_template = Template(self.placemark_point)
    self.placemark_polygon_template = Template(self.placemark_polygon)

    # Use these templates to build json.
    self.json_template = Template(self.json)
    self.json_placemark_point_template = Template(self.json_placemark_point)
    self.json_placemark_polygon_template = Template(self.json_placemark_polygon)

  def NoSearchResults(self, folder_name, response_type):
    """Prepares KML/JSON output in case of no search results.

    Args:
      folder_name: Name of the folder which is used in folder->name
        in the response.
      response_type: Response type can be KML or JSON, depending on the client.
    Returns:
       search_results: An empty KML/JSON file with no search results.
    """
    search_results = ""

    if response_type == "kml":
      search_results = self.kml_template.substitute(
          foldername=folder_name, style=self.style, placemark="")
    elif response_type == "json":
      search_results = self.json_template.substitute(
          foldername=folder_name, json_style=self.json_style, json_placemark=""
          )

    return search_results

  def GetContentType(self, response_type):
    """Content Type based on the response type.

    Args:
     response_type: Whether response is KML or JSON.
    Returns:
     Content-Type based on the response type.
    """

    if response_type == "kml":
      return self.content_type %("application/"
                                 "vnd.google-earth.kml+xml")
    else:
      return self.content_type %("application/json")


def main():
  print "Search utilities instance:", SearchUtils()

if __name__ == "__main__":
  main()
