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

"""Classes for creating and populating PostgreSQL table with search data."""

import logging

from common import exceptions
import psycopg2
from serve.push.search.util import search_schema_parser

# Create logger
logger = logging.getLogger("ge_search_publisher")


class SearchSchemaTableUtil(object):
  """Creates and populates table for POI search in gepoi database.

  It passes the callback-object DB-updater to SearchSchemaParser that generates
  SQL statement for each record of POI-file and passes it to Execute method of
  DB updater.
  """

  def __init__(self, poi_db_connection):
    """Inits SearchSchemaTableUtil.

    Args:
      poi_db_connection: gepoi database coneection.
    """
    self._poi_db_connection = poi_db_connection

  def CreatePoiTable(self, poi_file, table_name, file_prefix=None):
    """Creates and populates POI table in gepoi database.

    Creates POI table with specified table name and populates with data from
    specified POI file.
    Args:
      poi_file: a path to a poifile.
      table_name: the table name to use for this poifile in database.
    Returns:
      tuple that contains:
      numFields: total number of fields the SchemaParser extracts from
      the .poi file.
      query_string: SQL query string that should be used to query the .poi table
      created.
      balloon_style: balloon style that should be used for POI balloon.
    Raises:
      SearchSchemaTableUtilException, SearchSchemaParserException,
      psycopg2.Error/Warning.
    """
    # Instantiate DB updater and parser.
    db_updater = SearchSchemaTableUtil.DatabaseUpdater(self._poi_db_connection,
                                                       table_name)
    parser = search_schema_parser.SearchSchemaParser(db_updater)

    # Need to drop the table in the case that it already exists.
    db_updater.DropTable()

    # Parse POI search file.
    try:
      if file_prefix is None:
        logger.info("file prefix is None")
      else:
        logger.info("file prefix is: '%s'", file_prefix)
      (num_fields, query_string, balloon_style) = parser.Parse(
          poi_file, table_name, file_prefix)
    except (psycopg2.Error, psycopg2.Warning) as e:
      logger.error("Error when parsing/ingesting poifile %s: %s", poi_file, e)
      db_updater.Rollback()
      raise exceptions.SearchSchemaTableUtilException(
          "New POI table %s neither successfully created nor populated."
          " All transactions have been rollbacked." % table_name)
    except Exception as e:
      logger.error("Error when parsing/ingesting poifile %s: %s", poi_file, e)
      db_updater.Rollback()
      logger.error("Search file %s unsuccessfully parsed."
                   " All transactions have been rollbacked.", poi_file)
      raise

    # For debugging, print out our results.
    logger.debug("Total number of extracted fields: %s", num_fields)
    logger.debug("SQL Search Statement is: %s", query_string)
    logger.debug("Balloon Style is: %s", balloon_style)

    logger.info("New table %s successfully created and populated.",
                table_name)
    return (num_fields, query_string, balloon_style)

  def DropTable(self, table_name):
    """Drops specified table.

    Args:
      table_name: name of the table to drop.
    Raises:
      psycopg2.Warning/Error.
    """
    command = "DROP TABLE %s" % table_name
    self._poi_db_connection.Modify(command)

  class DatabaseUpdater(object):
    """Inner class to assist with Database interaction.

    This object is passed to the Parser that is invoked on the POI file.
    While parsing the POI file, for each record Parser generates SQL statement
    and passes it to the Execute method of this inner class for execution.
    """

    def __init__(self, poi_db_connection, table_name):
      """Initializes database updater.

      Args:
        poi_db_connection: the connection object used to interact with the
        gepoi database.
        table_name: name of gepoi table for creating and populating.
      """
      self._poi_db_connection = poi_db_connection
      self._table_name = table_name

    def DropTable(self):
      """Convenient function used to drop the table being managed.

      Raises:
        psycopg2.Error, psycopg2.Warning.
      """
      logger.debug("Drop table: %s", self._table_name)
      try:
        command = "DROP TABLE %s" % self._table_name
        self._poi_db_connection.Modify(command)
        self._poi_db_connection.ExecuteOutOfTransaction("VACUUM ANALYZE")
      except psycopg2.ProgrammingError as e:
        # Just report warning in case of programming error (e.g. table doesn't
        # exist).
        logger.warning("CreatePoiTable - drop table: %s", e)

    def Rollback(self):
      """Attempts to return to initial state before parsing.

      Rollbacks unsubmitted inserts and drops gepoi table.
      Used if the Parser fails or any other major exception occurs.
      """
      logger.debug("Rollback table: %s", self._table_name)
      try:
        # Need to remove any unsubmitted inserts that are associated with this
        # poi_db_connection and drop POI table.
        self._poi_db_connection.Rollback()
        self.DropTable()
      except (psycopg2.Error, psycopg2.Warning) as e:
        # Just report rollback error.
        logger.error("CreatePoiTable - rollback: %s", e)

    def Execute(self, command, params=None):
      """Convenient method to execute the supplied SQL statement.

      A single SQL statement is expected.
      Args:
        command: SQL statement to execute.
        params: sequence of parameters to populate into command's placeholders.
      Raises:
        psycopg2.Waring/Error exceptions.
      """
      logger.debug("command: %s, params: %s", command, params)
      self._poi_db_connection.Modify(command, params)

    def CreatePoiTableIndexes(self, indices):
      """CreatePoiTableIndexes, create indices, multithreaded.

      Args:
        indices: list of index creation sql statements.
      """
      logger.info("Creating indices in the background.")
      # Runs commands in threads but will block until all are done.
      self._poi_db_connection.ExecuteThreaded(indices)

    def CopyFrom(self, file_handle):
      """CopyFrom, import records from a file into a table.

      Args:
        file_handle: open for read file handle
      """
      logger.info("Copying records from file into database table.")
      self._poi_db_connection.CopyFromFile(file_handle, self._table_name)


def main():
  pass


if __name__ == "__main__":
  main()
