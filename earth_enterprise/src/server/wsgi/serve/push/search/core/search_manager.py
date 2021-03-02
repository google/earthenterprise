#!/usr/bin/env python2.7
#
# Copyright 2017 Google Inc.
# Copyright 2018 Open GEE Contributors
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

"""The search_manager module.

Classes for managing search (gesearch/gepoi) databases.
"""

import logging

from common import postgres_manager
from common import postgres_properties
from serve import constants
from serve import http_io
from serve.push.search.util import search_schema_table_util


# Create logger.
logger = logging.getLogger("ge_search_publisher")


class SearchManager(object):
  """Class for managing search databases."""

  def __init__(self):
    """Inits SearchManager."""
    # Note: added as a good practice since we use SearchManager as a base class.
    super(SearchManager, self).__init__()

    # Init database connections
    self._search_database = "gesearch"
    self._poi_database = "gepoi"
    self._db_user = "geuser"
    postgres_prop = postgres_properties.PostgresProperties()
    self._port = postgres_prop.GetPortNumber()
    self._host = postgres_prop.GetHost()
    self._pass = postgres_prop.GetPassword()

    # Create DB connection to gesearch database.
    self._search_db_connection = postgres_manager.PostgresConnection(
        self._search_database, self._db_user, self._host, self._port, self._pass, logger)
    # Create DB connection to gepoi database.
    self._poi_db_connection = postgres_manager.PostgresConnection(
        self._poi_database, self._db_user, self._host, self._port, self._pass, logger)
    # Create search schema table utility.
    self.table_utility = search_schema_table_util.SearchSchemaTableUtil(
        self._poi_db_connection)

  def HandlePingRequest(self, request, response):
    """Handles ping database request.

    Args:
      request: request object.
      response: response object
    Raises:
      psycopg2.Error/Warning.
    """
    cmd = request.GetParameter(constants.CMD)
    assert cmd == "Ping"
    # Fire off a pinq  query to make sure we have a valid db connection.
    query_string = "SELECT 'ping'"
    results = self._DbQuery(query_string)
    if results and results[0] == "ping":
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    else:
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_MESSAGE,
          "Cannot ping gesearch database.")
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_FAILURE)

  def QueryDbId(self, client_host_name, db_name):
    """Queries database ID by name.

    Args:
      client_host_name: Fusion client host name (part of database ID).
      db_name: database name.
    Returns:
      database ID.
    Raises:
      psycopg2.Error/Warning
    """
    db_id = 0
    query_string = ("SELECT db_id FROM db_table "
                    "WHERE host_name = %s AND db_name = %s")
    result = self._DbQuery(query_string, (client_host_name, db_name))
    if result:
      db_id = int(result[0])

    return db_id

  def QueryListDbs(self):
    query_string = "SELECT host_name, db_name FROM db_table"
    results = self._DbQuery(query_string)
    return results

  def _QuerySearchDefId(self, search_def_name):
    """Queries search definition ID by name.

    Args:
      search_def_name: search definition name.
    Returns:
      search definition ID or 0 if search definition is not found.
    Raises:
      psycopg2.Error/Warning
    """
    search_def_id = 0
    query_string = ("SELECT search_def_id FROM search_def_table"
                    " WHERE search_def_name = %s")
    result = self._DbQuery(query_string, (search_def_name,))
    if result:
      assert isinstance(result, list) and len(result) == 1
      search_def_id = result[0]

    return search_def_id

  def GetSearchDefDetails(self, search_def_name):
    """Gets search definition details by name.

    Args:
      search_def_name: search definitions name.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      Search definition content record from search_def_table or None if search
      definition is not found.
    """
     # Get search def info from search_def_table.
    query_string = ("SELECT search_def_content"
                    " FROM search_def_table WHERE search_def_name = %s")
    result = self._DbQuery(query_string, (search_def_name,))

    if not result:
      return None

    assert isinstance(result, list) and len(result) == 1
    return result[0]

  def _DbQuery(self, query_string, parameters=None):
    """Handles DB query request to gesearch database.

    Args:
      query_string: SQL query statement.
      parameters: sequence of parameters to populate into placeholders.
    Returns:
      Results as list of tuples (rows of fields).
    Raises:
      psycopg2.Error/Warning in case of error.
    """
    return self._search_db_connection.Query(query_string, parameters)

  def _DbModify(self, command_string, parameters=None):
    """Handles DB modify request to gesearch database.

    Args:
      command_string: SQL UPDATE/INSERT/DELETE command string.
      parameters: sequence of parameters to populate into placeholders.
    Returns:
      Number of rows that sql command affected.
    Raises:
      psycopg2.Error/Warning in case of error.
    """
    return self._search_db_connection.Modify(command_string, parameters)

  def _QueryPoiId(self, host_name, poi_file_path):
    """Queries POI ID for hostname:poi_file_path.

    Args:
      host_name: host name. it is a part of database identifier.
      poi_file_path: POI file path.
    Returns:
      POI ID for the pair hostname:poi_file_path.
    Raises:
      psycopg2.Error/Warning
    """
    poi_id = 0
    query_string = ("SELECT poi_id FROM poi_table "
                    "WHERE host_name = %s AND poi_file_path = %s")

    results = self._DbQuery(query_string, [host_name, poi_file_path])
    if results:
      poi_id = results[0]

    return poi_id


def main():
  pass


if __name__ == "__main__":
  main()
