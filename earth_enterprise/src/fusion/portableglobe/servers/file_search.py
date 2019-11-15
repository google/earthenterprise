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


import os
import re

# Beginning template for search results.
kml_start_template = """<?xml version="1.0" encoding="latin1"?>
<kml xmlns="http://earth.google.com/kml/2.1">
    <Folder>
        <name><![CDATA[Grouped search results : %s]]></name>
        <open>true</open>
"""

# Template for each placemark of search results.
kml_placemark_template = """
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
kml_end_template = """   </Folder>
</kml>"""


# Beginning template for search results.
json_start_template = """%s({displayKeys: "none", success: true, responses: [
{
datastoreName: "POI Search Results", searchTerm: "%%s", success: true,  data:
["""

# Template for each placemark of search results.
json_placemark_template = """
{lon: "%s", lat: "%s", name: "%s", snippet: "%s", description: "%s"}
"""

# Ending template for search results.
json_end_template = """]
}
]
}
);"""

# lat/lon regular expression
latlon_regex = re.compile(r"^(-?\d+\.?\d*),\s*(-?\d+\.?\d*)$")


class KmlPlacemark(object):
  """Class for returning search results as kml."""

  def GetPlacemark(self, row, results):
    """Write kml for next placemark."""
    if len(row) < 3:
      return ""
    elif len(row) == 3:
      return (kml_placemark_template % (row[0], "", "",
                                        row[0], "", results, row[1],
                                        row[2]))
    else:
      return (kml_placemark_template % (row[0], row[1], "",
                                        row[0], row[1], results, row[-2],
                                        row[-1]))


class JsonPlacemark(object):
  """Class for returning search results as json."""

  def GetPlacemark(self, row, results):
    """Write json for next placemark."""
    if len(row) < 3:
      return ""
    elif len(row) == 3:
      return (json_placemark_template % (row[-2], row[-1],
                                         row[0], "", results))
    else:
      return (json_placemark_template % (row[-2], row[-1],
                                         row[0], row[1], results))

class TableData(object):
  """Class for holding table data."""
  def __init__(self):
    self.columns_ = []
    self.rows_ = []

class FileDatabase(object):
  def __init__(self):
    self.kml_placemark_ = KmlPlacemark()
    self.json_placemark_ = JsonPlacemark()
    self.ClearSearchTables()

  def ClearSearchTables(self):
    """Clear search tables so new ones can be loaded."""
    self.search_tables_ = []
    self.search_data_ = {}

  def LoadSearchTable(self, table_name, content):
    """Load data for search into a database table."""
    self.search_tables_.append(table_name)
    self.search_data_[table_name] = TableData()

    first_line = True
    for line in content.split("\n"):
      if first_line:
        first_line = False
        # Do not use strip here, otherwise lose leading tabs
        # indicating empty strings in initial fields.
        self.search_data_[table_name].columns_ = line[:-1].split("\t")
      else:
        # Do not use strip here, otherwise lose leading tabs
        # indicating empty strings in initial fields.
        if line:
          self.search_data_[table_name].rows_.append(line[:-1].split("\t"))

  def Match(self, search_pattern, row):
    """Match pattern with table data, aligned from beginning."""
    for term in row:
      if search_pattern.match(term):
        return True

    return False

  def Find(self, search_pattern, row):
    """Find pattern anywhere in table data."""
    for term in row:
      if search_pattern.search(term):
        return True
    return False

  def SearchTable(self, table_name, search_term, placemark_,
                  delimiter, num_found, search_func):
    """Search given table for given term and return results as placemarks."""
    table_data = self.search_data_[table_name]
    content = ""
    search_pattern = re.compile(search_term, re.IGNORECASE)
    for row in table_data.rows_:
      if search_func(search_pattern, row[:-2]):
        summary = ""
        idx = 0
        for column_name in table_data.columns_:
          if idx < len(row):
            summary += "<br><b>%s:</b> %s" % (column_name, row[idx])
          else:
            print "Bad search row. Too few columns."
            print row
          idx += 1
        if num_found:
          content += delimiter
        num_found += 1
        content += placemark_.GetPlacemark(row, summary)

    return (num_found, content)

  def Search(self, search_term, start_template,
             end_template, placemark_, delimiter=""):
    """Search all tables and create results using templates."""
    content = start_template % search_term
    num_found = 0
    for func in (self.Match, self.Find):
      for table_name in self.search_tables_:
        (num_found, data) = self.SearchTable(
            table_name, search_term, placemark_, delimiter, num_found, func)
        content += data
      if num_found > 0:
        break

    content += end_template
    return content

  def Location(self, lat, lon, start_template,
               end_template, placemark):
    """Return location for a lat/lon request."""
    content = start_template
    row = ["%s, %s" % (lat, lon), lon, lat]
    content += placemark.GetPlacemark(row, "")
    content += end_template
    return content

  def KmlSearch(self, search_term):
    """Search all tables and return resuls as kml."""
    m = latlon_regex.match(search_term)
    if m:
      return self.Location(m.group(1), m.group(2),
                           kml_start_template % search_term,
                           kml_end_template, self.kml_placemark_)

    return self.Search(search_term, kml_start_template,
                       kml_end_template, self.kml_placemark_)

  def JsonSearch(self, search_term, cb):
    """Search all tables and return resuls as json."""
    m = latlon_regex.match(search_term)
    if m:
      print "lat/lon"
      return self.Location(m.group(1), m.group(2),
                           (json_start_template % cb) % search_term,
                           json_end_template, self.json_placemark_)

    return self.Search(search_term, json_start_template % cb,
                       json_end_template, self.json_placemark_, ",")
