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

class PostgresDatabase(object):
  def __init__(self, database_name, separator="|-|"):
    self.separator_ = separator
    self.kml_placemark_ = KmlPlacemark()
    self.json_placemark_ = JsonPlacemark()
    self.database_ = database_name
    self.ClearSearchTables()

  def Query(self, query):
    """Submits the query to the database and returns tuples.
    Args:
      query SQL SELECT statement.
      db The database being queried.
    Returns:
      Results as list of lists (rows of fields).
    """
    command = (("psql -c \"%s\" -P format=unaligned -P tuples_only"
               " -P fieldsep=\"%s\" %s")
               % (query, self.separator_, self.database_))
    out = os.popen(command)
    results = []
    for line in out.readlines():
      row = line.strip().split(self.separator_)
      if len(row) == 1:
        results.append(row[0])
      else:
        results.append(row)
    out.close()
    return results

  def TableColumns(self, table):
    """Returns list of column names for the given table in the given db."""
    query = ("SELECT column_name FROM INFORMATION_SCHEMA.columns "
             "WHERE table_name='%s'") % table
    return self.Query(query)

  def ClearSearchTables(self):
    """Clear search tables so new ones can be loaded."""
    self.search_tables_ = []

  def LoadSearchTable(self, table_name, content):
    """Load data for search into a database table."""
    self.search_tables_.append(table_name)
    first_line = True
    for line in content.split("\n"):
      if first_line:
        first_line = False
        # self.write("<p>Columns: ")
        # Do not use strip here, otherwise lose leading tabs
        # indicating empty strings in initial fields.
        column_names = line[:-1].split("\t")
        query = "DROP TABLE %s;" % table_name
        self.Query(query)
        query = "CREATE TABLE %s (%s);" % (
            table_name, " VARCHAR,".join(column_names) + " VARCHAR")
        self.Query(query)
      else:
        # Do not use strip here, otherwise lose leading tabs
        # indicating empty strings in initial fields.
        if line:
          row_values = line[:-1].split("\t")
          query = "INSERT INTO %s VALUES (%s);" % (
              table_name, "'" + "','".join(row_values) + "'")
          self.Query(query)

  def SearchTable(self, table_name, search_term, placemark_):
    """Search given table for given term and return results as placemarks."""
    column_names = self.TableColumns(table_name)

    # Search everything but the last two columns (lat/lon).
    query = "SELECT * FROM %s WHERE " % table_name
    where_clause = " OR ".join(
        ["lower(%s) LIKE '%%%s%%'" %  (column_name, search_term)
         for column_name in column_names[:-2]])
    query += where_clause

    content = ""
    results = self.Query(query)
    for row in results:
      summary = ""
      idx = 0
      for column_name in column_names:
        summary += "<br><b>%s:</b> %s" % (column_name, row[idx])
        idx += 1
      content += placemark_.GetPlacemark(row, summary)

    return content

  def Search(self, search_term, start_template,
             end_template, placemark_):
    """Search all tables and create results using templates."""
    content = start_template % search_term
    for table_name in self.search_tables_:
      content += self.SearchTable(table_name, search_term, placemark_)

    content += end_template
    return content

  def KmlSearch(self, search_term):
    """Search all tables and return resuls as kml."""
    return self.Search(search_term, kml_start_template,
                       kml_end_template, self.kml_placemark_)

  def JsonSearch(self, search_term, cb):
    """Search all tables and return resuls as json."""
    return self.Search(search_term, json_start_template % cb,
                       json_end_template, self.json_placemark_)
