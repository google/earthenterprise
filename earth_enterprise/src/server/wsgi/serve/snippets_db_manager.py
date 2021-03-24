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


"""The snippets_db_manager module.

Classes for working with gesnippets database.
"""

import logging

from common import postgres_manager
from common import postgres_properties
from serve import constants
from serve import http_io

logger = logging.getLogger("ge_snippets")


class SnippetsDbManager(object):
  """Class for managing gesnippets database.

  Snippet set table name: "snippet_set_table", fields: "name", "content".
  """

  def __init__(self):
    """Inits snippets DB manager."""
    # Note: added as a good practice since we use SnippetsDbManager as a base
    # class.
    super(SnippetsDbManager, self).__init__()

    # Init database connection.
    self._snippets_db_name = "geendsnippet"
    self._db_user = "geuser"
    postgres_prop = postgres_properties.PostgresProperties()
    self._port = postgres_prop.GetPortNumber()
    self._host = postgres_prop.GetHost()
    self._pass = postgres_prop.GetPassword()

    # Create DB connection to gesnippets database.
    self._snippets_db_connection = postgres_manager.PostgresConnection(
        self._snippets_db_name, self._db_user, self._host, self._port, self._pass, logger)

  def _DbQuery(self, query_string, parameters=None):
    """Handles DB query request to gesnippets database.

    Args:
      query_string: SQL query statement.
      parameters: sequence of parameters to populate into placeholders.
    Returns:
      Results as list of tuples (rows of fields).
    Raises:
      psycopg2.Error/Warning in case of error.
    """
    return self._snippets_db_connection.Query(query_string, parameters)

  def _DbModify(self, command_string, parameters=None):
    """Handles DB modify request to gesnippets database.

    Args:
      command_string: SQL UPDATE/INSERT/DELETE command string.
      parameters: sequence of parameters to populate into placeholders.
    Returns:
      Number of rows that sql command affected.
    Raises:
      psycopg2.Error/Warning in case of error.
    """
    return self._snippets_db_connection.Modify(command_string, parameters)

  def HandlePingRequest(self, request, response):
    """Handles ping server request.

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
          "Cannot ping geendsnippet database.")
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_FAILURE)

  def GetSnippetSetDetails(self, snippets_set_name):
    """Gets snippets set details (content) by name.

    Args:
      snippets_set_name: snippets set name.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      Snippets set content from gesnippets database or None if snippets set
      is not found.
    """
    # Get snippet set content from snippet_set_table.
    query_string = "SELECT content FROM snippet_set_table WHERE name = %s"

    result = self._DbQuery(query_string, (snippets_set_name,))

    if not result:
      return None

    assert isinstance(result, list) and len(result) == 1
    return result[0]

  def _SnippetSetExists(self, snippets_set_name):
    """Checks if specific snippets set exists.

    Args:
      snippets_set_name: snippets set name.
    Returns:
      Whether specific snippets set exists.
    """
    query_string = (
        "SELECT EXISTS (SELECT TRUE FROM snippet_set_table WHERE name = %s)")
    result = self._DbQuery(query_string, (snippets_set_name,))
    if not result:
      return False

    assert isinstance(result[0], bool)
    return result[0]


def main():
  pass


if __name__ == "__main__":
  main()
