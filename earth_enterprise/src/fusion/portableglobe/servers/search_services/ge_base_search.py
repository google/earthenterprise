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


"""Base class for creating search services.

You can follow the example given in search_service_geplaces.py.
You need to provide:
  A) a function that registers the service.
  B) a search service object that inherits from this class with 3 methods:
    1) a method for getting kml search results.
    2) a method for getting json search results.
"""

import re

# lat/lng regular expression
LATLNG_REGEX = re.compile(r"^(-?\d+\.?\d*),\s*(-?\d+\.?\d*)$")


class GEBaseSearch(object):
  """Base class for creating search services."""

  def __init__(self):
    # Beginning template for search results.
    self.kml_start_template_ = """<?xml version="1.0" encoding="latin1"?>
<kml xmlns="http://earth.google.com/kml/2.1">
    <Folder>
        <name><![CDATA[Grouped search results: %s]]></name>
        <open>true</open>
"""

    # Template for each placemark of search results.
    self.kml_placemark_template_ = """
    <Placemark>
        <name><![CDATA[%s]]></name>
        <Snippet>%s</Snippet>
        <description><![CDATA[%s]]></description>
        <Style>
            <IconStyle>
                <scale>1.0</scale>
                <Icon>
                    <href>icons/location_pin.png</href>
                </Icon>
            </IconStyle>
            <LineStyle>
                <color>7FFFFF00</color>
                <width>5.0</width>
            </LineStyle>
            <PolyStyle>
                <color>7F66FFFF</color>
                <colorMode>normal</colorMode>
                <fill>true</fill>
                <outline>true</outline>
            </PolyStyle>
            <BalloonStyle>
                <bgColor>FFFFFFFF</bgColor>
                <textColor>FF000000</textColor>
                <text><![CDATA[
                  <table border="0" width="250" cellspacing="0"
                   cellpadding="0" align="center" bgcolor="#FFFFFF">
                    <tr><td><b><font size="+3">%s</font></b></td></tr>
                    <tr><td><font size="+1">%s</font></td></tr>
                    <tr><td><hr/></td></tr>
                    <tr><td><font size="+0">%s</font></td></tr>
                  </table>]]></text>
            </BalloonStyle>
        </Style>
        <Point>
            <coordinates>%s,%s</coordinates>
        </Point>
    </Placemark>
"""

    # Ending template for search results.
    self.kml_end_template_ = """   </Folder>
</kml>"""

    # Beginning template for search results.
    self.json_start_template_ = """
%s({displayKeys: "none", success: true, responses: [
{
datastoreName: "%s", searchTerm: "%s", success: true,  data:
["""

    # Template for each placemark of search results.
    self.json_placemark_template_ = """
{lon: "%s", lat: "%s", name: "%s", snippet: "%s", description: "%s"}
"""

    # Ending template for search results.
    self.json_end_template_ = """]
}
]
}
);"""

  def SetWorkingDirectory(self, working_directory):
    """Use the given base directory to find the geplaces database file."""
    # Set the location of our geplaces data.
    self.working_directory_ = working_directory

  def KmlStart(self, handler, search_term):
    """Writes initial header for kml results."""
    handler.write(self.kml_start_template_ % search_term)

  def KmlPlacemark(self, handler, name, snippet, description,
                   title, subtitle, info, longitude, latitude):
    """Writes a placemark for kml results."""
    handler.write(self.kml_placemark_template_ % (
        name, snippet, description, title, subtitle,
        info, longitude, latitude))

  def KmlEnd(self, handler):
    """Writes final footer for kml results."""
    handler.write(self.kml_end_template_)

  def JsonStart(self, handler, cb, datastoreName, search_term):
    """Writes initial header for json results."""
    handler.write(self.json_start_template_ % (cb, datastoreName, search_term))

  def JsonPlacemark(self, handler, name, snippet, description,
                    unused_title, unused_subtitle, unused_info,
                    longitude, latitude):
    """Writes a placemark for json results."""
    handler.write(self.json_placemark_template_ % (
        longitude, latitude, name, snippet, description))

  def JsonEnd(self, handler):
    """Writes final footer for json results."""
    handler.write(self.json_end_template_)

  def LatLngSearch(self, handler, placemark_fn, search_term):
    """Does a lat/lng search if search_term looks like a lat/lng.

    Since the geplaces file is stored in rough order by population, the
    results will come back with most populous locations first.

    Args:
      handler: Web server handler serving this request.
      placemark_fn: Function to call to render the next placemark.
      search_term: Search term (or phrase) to use in grep.
    Returns:
      Whether a lat/lng search was done.
    """
    # If not a lat/lng argument, abandon this search.
    m = LATLNG_REGEX.match(search_term)
    if not m:
      return False

    placemark_fn(handler, search_term, "", "", "", "",
                 "", m.group(2), m.group(1))
    return True

  # Methods to override with custom service.

  def KmlSearch(self, unused_handler):
    print "Need to override KmlSearch."

  def JsonSearch(self, unused_handler, unused_cb):
    print "Need to override JsonSearch."
