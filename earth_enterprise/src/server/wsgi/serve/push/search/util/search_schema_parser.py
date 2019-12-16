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


"""Search schema parser.

Classes for parsing POI-files and generating: SQL statements for creating and
populating search tables, SQL statement for querying.
"""
import binascii
import getopt
import logging
import logging.config
from struct import pack
import sys
from tempfile import SpooledTemporaryFile as TempFile
import time
import defusedxml.cElementTree as ET
from xml.etree.cElementTree import ParseError
import os

from common import exceptions

ET.ParseError = ParseError

# Memory buffer size for temp file, before it's written
_K_SPOOL_SIZE = 1024 * 1024 * 256

# Create logger.
logger = logging.getLogger("ge_search_publisher")


# Create point data as a string that psql knows how to import.
def _PointFromCoordinates(lon, lat):
  packed_data = pack("<dd", float(lon), float(lat))
  return "0101000020E6100000%s" % binascii.hexlify(packed_data)


class MockDatabaseUpdater(object):
  """Mock database updater."""

  def Execute(self, statement, params=None):
    logger.info(statement)
    if params:
      logger.info("params: %s", ", ".join(params))

  def CreatePoiTableIndexes(self, indices):
    logger.info("Mock Creating indices in the background.")
    for index in indices:
      logger.info("Index: %s", index)

  def CopyFrom(self, unused_):
    logger.info("Mock Copying from file into database table.")


class SearchSchemaParser(object):
  """Search schema parser.

  Parses POI-file and generates: a string containing the SQL statements required
  to generate and populate postgres tables, a string for querying.
  """
  EVENTS = ("start", "end")

  SEARCH_TABLE_SCHEMA_TAG = "SearchTableSchema"
  SEARCH_TABLE_VALUES_TAG = "SearchTableValues"
  BALLOON_STYLE_TAG = "BalloonStyle"
  FIELD_TAG = "field"
  R_TAG = "r"
  LAT_TAG = "lat"
  LON_TAG = "lon"
  SEARCH_FILE_NAME = "SearchDataFile"

  # Types that need to be UTF-encoded when writing to postgres database.
  ENCODE_TYPES = ["varchar", "character varying", "character", "text"]

  def __init__(self, db_updater):
    """Inits search schema parser.

    Args:
      db_updater: database updater.
    """
    self._db_updater = db_updater
    self._current_tag = None
    self._within_record = False
    self._within_style = False

  def Parse(self, search_file, table_name, file_prefix=None):
    """Parser entry point.

    Parses the given POI file to POI elements, based on POI elements builds
    SQL statements for creating and populating POI table in POI database and
    triggers the DB updater to implement these SQL statements.

    Args:
      search_file: string containing absolute path to .poi file to be parsed.
      table_name: string containing name to use for POI table creation.
    Returns:
      num_fields: number of fields in search schema.
      sql_search: string containing SQL statement to execute for POI query.
      balloon_style: string containing associated balloon style.

    Raises:
      exceptions.SearchSchemaParserException exception.
      psycopg2.Warning/Error exceptions.
    """
    self._table_name = table_name
    self._file_prefix = file_prefix
    logger.info("Ingesting POI file %s into parser...", search_file)
    if file_prefix is None:
      logger.info("File prefix is None")
    else:
      logger.info("File prefix is '%s'", file_prefix)
    self.__StartDocument()
    try:
      context = ET.iterparse(search_file, SearchSchemaParser.EVENTS)
    except ET.ParseError, e:
      row, column = e.position
      raise exceptions.SearchSchemaParserException(
          "Unable to parse POI file %s."
          " A parsing error on row %d column %d: %s" % (
              search_file, row, column, e))

    logger.info("Ingesting POI file %s into parser done.", search_file)
    logger.info("Parsing POI elements and inserting into POI database...")
    # File as temp buffer to store records, for COPY db command
    self.tmp_file = TempFile(max_size=_K_SPOOL_SIZE, suffix=table_name)
    num_elements = 0
    self._element_start = self.__StartElementHeader
    self._element_end = self.__EndElementHeader
    for event, elem in context:
      if event == "start":
        self.__StartElement(elem)
      elif event == "end":
        self.__EndElement(elem)
        num_elements += 1
        elem.clear()

    self.__EndDocument()
    logger.info("Total POI elements: %s.", num_elements)
    logger.info("Parsing POI elements and inserting into POI database done.")
    return (self._num_fields, self._sql_search, self._balloon_style)

  def __StartElement(self, elem):
    """Start element handler.

    Args:
      elem: current element.
    """
    self._current_tag = elem.tag
    self._element_start(elem)

  def __StartElementHeader(self, elem):
    """Start element handler for non-data tags.

    They can be header or appear at the end, or even alternate.

    Args:
      elem: current element.
    """
    if self._current_tag == SearchSchemaParser.BALLOON_STYLE_TAG:
      self._within_style = True
    elif self._current_tag == SearchSchemaParser.FIELD_TAG:
      # Whilst parsing individual field elements, we need to continually
      # update our sql.
      # Extract values.
      type_val = elem.get("type")
      name_val = elem.get("name")
      use_val = elem.get("use")
      # Collect fields' names which values need to be UTF-encoded when writing
      # to postgres database.
      if type_val.lower() in SearchSchemaParser.ENCODE_TYPES:
        self._encode_fields.append(name_val)
      # Support fields names with spaces.
      display_name_val = elem.get("displayname").lower()
      if " " in display_name_val:
        display_name_val = display_name_val.replace(" ", "_")
        logger.warning("Found space in field:'%s', changed to underscore",
                       str(display_name_val))

      # Build up SQL Select
      if use_val == "display" or use_val == "both":
        self._select_clause.append("\"%s\"" % display_name_val)

      if use_val == "search" or use_val == "both":
        self._where_clause.append("lower(\"%s\") " % display_name_val)
        self._index_columns.append(display_name_val)
        self._num_fields += 1

      # handle all the fields except lat/lon.
      if (name_val != SearchSchemaParser.LAT_TAG and
          name_val != SearchSchemaParser.LON_TAG):
        self._table_fields.append("%s %s" % (display_name_val, type_val))
    # First we must generate the create table syntax. To do so, we set a flag.
    elif self._current_tag == SearchSchemaParser.SEARCH_TABLE_SCHEMA_TAG:
      self._sql_insert = ("%sCREATE TABLE %s (" % (self._sql_insert,
                                                   self._table_name))
      # TODO: seems we do not retrieve geometry as binary, can be
      # refactored.
      self._sql_search += (
          "SELECT Encode(ST_AsBinary(the_geom, 'XDR'), 'base64') AS the_geom, ")
    elif self._current_tag == SearchSchemaParser.SEARCH_TABLE_VALUES_TAG:
      self._element_start = self.__StartElementData
      self._element_end = self.__EndElementData

  def __StartElementData(self, elem):
    """Start element handler for data tags.

    Args:
      elem: current element.
    """
    if self._current_tag == SearchSchemaParser.R_TAG:
      # We're on to dealing with records
      self._within_record = True
    elif self._current_tag == SearchSchemaParser.SEARCH_FILE_NAME:
      data_file_name = elem.text
      if self._file_prefix is not None:
        logger.info("Adding prefix to poi data file: '%s'", self._file_prefix)
        data_file_name_with_prefix = os.path.normpath(self._file_prefix + data_file_name)
        if not os.path.exists(data_file_name_with_prefix) and os.path.exists(data_file_name):
          # might be talking to an old fusion client so preserve
          # old behavior if the prefixed file does not exist
          # but the non-prefixed file does
          pass
        else:
          data_file_name = data_file_name_with_prefix
      else:
        logger.info("Missing file prefix!")
      data_file = open(data_file_name, "r")
      logger.info("Importing records from:'%s'", data_file_name)
      self._db_updater.CopyFrom(data_file)
      data_file.close()

  def __EndElement(self, elem):
    """End element handler.

    Args:
      elem: current element.
    """
    self._current_tag = elem.tag
    self._element_end(elem)

  def __EndElementHeader(self, elem):
    """End element handler for non-data tags.

    They can be header or appear at the end, or even alternate.

    Args:
      elem: current element.
    """
    # Handling parse balloon style
    if self._within_style:
      # Append style value.
      value = elem.text
      if value:
        style_val_chars = value.replace("\'", "\\'")
        self._balloon_style_values.append(style_val_chars)

    if self._current_tag == SearchSchemaParser.BALLOON_STYLE_TAG:
      self._within_style = False
      self._balloon_style += " ".join(self._balloon_style_values)
      self._balloon_style_values = []

    elif self._current_tag == SearchSchemaParser.SEARCH_TABLE_SCHEMA_TAG:
      # Continue to build sql search query.
      self._sql_search = ("%s%s FROM %s WHERE ( %sLIKE ? ) " % (
          self._sql_search,
          ", ".join(self._select_clause),
          self._table_name,
          "LIKE ? OR ".join(self._where_clause)))

      # Create gepoi table.
      self._sql_insert = "%s%s);\n" % (
          self._sql_insert, ", ".join(self._table_fields))
      self._db_updater.Execute(self._sql_insert)
      # And create our geometry columns.
      self._sql_insert = (
          "SELECT AddGeometryColumn('%s', 'the_geom', 4326, 'POINT', 2);\n" %
          self._table_name)
      self._db_updater.Execute(self._sql_insert)
      self._sql_insert = ""

  def __EndElementData(self, elem):
    """End element handler for data tags.

    Args:
      elem: current element.
    """
    if self._current_tag == SearchSchemaParser.R_TAG:
      self._within_record = False
      # Calculate point data structure as hex and write to file
      point_string = _PointFromCoordinates(self._cur_lon, self._cur_lat)
      self.tmp_file.write("%s\n" % point_string)
      return
    elif self._current_tag == SearchSchemaParser.SEARCH_TABLE_VALUES_TAG:
      self._element_start = self.__StartElementHeader
      self._element_end = self.__EndElementHeader
      return

    if self._within_record:
      # Buffer lon/lat fields, otherwise write data to file
      if self._current_tag == SearchSchemaParser.LAT_TAG:
        self._cur_lat = elem.text
      elif self._current_tag == SearchSchemaParser.LON_TAG:
        self._cur_lon = elem.text
      else:
        # Handle search field values (field#).
        value = elem.text
        if value:
          value = value.strip()
          if value and self._current_tag in self._encode_fields:
            value = elem.text.encode("utf8")
        self.tmp_file.write("%s\t" % (value if value else ""))

  def __StartDocument(self):
    """Start document handler.

    It is used to initialize variables before any actual parsing happens.
    """
    self._num_fields = 0
    self._table_fields = []
    self._index_columns = []

    self._sql_insert = ""
    self._sql_search = ""
    self._select_clause = []
    self._where_clause = []

    self._within_record = False
    self._within_style = False
    self._balloon_style = ""
    self._balloon_style_values = []
    self._cur_lat = ""
    self._cur_lon = ""

    # List of fields' names which values need to be UTF-encoded when writing to
    # postgres database.
    self._encode_fields = []

  def __EndDocument(self):
    """End document handler.

    Final actions after parsing the document.
    Raises:
      psycopg2.Error/Warning.
    """
    start = time.time()
    file_size = self.tmp_file.tell()
    if file_size > 0:
      logger.info("Importing records into database")
      self.tmp_file.seek(0)
      self._db_updater.CopyFrom(self.tmp_file)
      elapsed = time.time() - start
      print "File import time: ", elapsed

    self.tmp_file.close()  # deletes file

    # Finally, run some post insertion commands.
    # Optimize processing time by indexing in the background.
    indices = []
    name = "the_geom_%s_idx" % self._table_name
    index = (
        "CREATE INDEX %s ON %s USING GIST(the_geom);\n" %
        (name, self._table_name))
    indices.append(index)

    for column_name in self._index_columns:
      name = "%s_%s_idx" % (column_name, self._table_name)
      index = (
          "CREATE INDEX %s ON %s USING BTREE(lower(%s));\n" %
          (name, self._table_name, column_name))
      indices.append(index)

    self._db_updater.CreatePoiTableIndexes(indices)

    self._sql_search = self._sql_search.replace("\'", "\\'")
    self._balloon_style = self._balloon_style.replace("\'", "\\'")


def main(argv):
  poifile = ""
  try:
    opts, unused_args = getopt.getopt(argv, "hi:", ["help", "poifile="])
    if not opts:
      print "search_schema_parser.py -i <input poifile>"
      sys.exit(1)
  except getopt.GetoptError:
    print "search_schema_parser.py -i <input poifile>"
    sys.exit(2)

  for opt, arg in opts:
    if opt == "-h":
      print "search_schema_parser.py -i <input poifile>"
      sys.exit()
    elif opt in ("-i", "--poifile"):
      poifile = arg

  print "Input poifile is ", poifile

  db_updater = MockDatabaseUpdater()
  parser = SearchSchemaParser(db_updater)

  start = time.time()
  # TODO(RAW): add prefix path to test this
  (num_fields, sql_search, balloon_style) = parser.Parse(poifile, "test_poi", None)
  elapsed = time.time() - start
  print "Elapsed time: ", elapsed
  logger.info("Num fields: %s", num_fields)
  logger.info("SQL Search: %s", sql_search)
  logger.info("Balloon style: %s", balloon_style)


if __name__ == "__main__":
  # Create logger.
  logger = logging.getLogger("search_schema_parser")
  logger.setLevel(logging.INFO)
  # Create console handler and set level to info.
  ch = logging.StreamHandler()
  ch.setLevel(logging.INFO)
  # Create formatter.
  formatter = logging.Formatter(
      "%(asctime)s - %(name)s - %(levelname)s - %(message)s")
  # Add formatter to ch.
  ch.setFormatter(formatter)
  # Add ch to logger
  logger.addHandler(ch)

  main(sys.argv[1:])
