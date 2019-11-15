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

"""Search data publishing manager module.

Classes for handling search data publishing requests.
"""

import logging

from common import exceptions
from serve import basic_types
from serve import constants
from serve import http_io
# TODO: move to serve
from serve.push.search.core import search_manager


# Create logger.
logger = logging.getLogger("ge_search_publisher")


class SearchPublishManager(search_manager.SearchManager):
  """Class for handling search data managing requests."""

  SEARCH_DEF_NAME = "search_def_name"
  SEARCH_DEF_CONTENT = "search_def_content"

  SYSTEM_SEARCH_DEF_LIST = ["POISearch", "GeocodingFederated", "Coordinate",
                            "Places", "ExampleSearch", "SearchGoogle"]

  def __init__(self):
    """Inits SearchPublishManager."""
    super(SearchPublishManager, self).__init__()

  def HandleQueryRequest(self, request, response):
    """Handles query requests.

    Args:
      request: request object.
      response: response object
    Raises:
      SearchPublishServeException, psycopg2.Error/Warning.
    """
    query_cmd = request.GetParameter(constants.QUERY_CMD)
    if not query_cmd:
      raise exceptions.SearchPublishServeException(
          "Internal Error - Missing Query Command.")

    # List all Search Definitions registered on the server.
    if query_cmd == constants.QUERY_CMD_LIST_SEARCH_DEFS:
      query_string = (
          "SELECT search_def_name, search_def_content FROM search_def_table")

      results = self._DbQuery(query_string)

      search_def_list = [
          self.__BuildSearchDefObj(
              name, basic_types.SearchDefContent.Deserialize(content))
          for name, content in results]

      http_io.ResponseWriter.AddJsonBody(
          response, constants.STATUS_SUCCESS, search_def_list)
    # Preview details for a specific search definition.
    elif query_cmd == constants.QUERY_CMD_SEARCH_DEF_DETAILS:
      search_def_name = request.GetParameter(constants.SEARCH_DEF_NAME)
      if not search_def_name:
        raise exceptions.SearchPublishServeException(
            "HandleQueryRequest: missing search definition name.")

      search_def_content = self.GetSearchDefDetails(search_def_name)
      if not search_def_content:
        raise exceptions.SearchPublishServeException(
            "The search definition %s does not exist." % search_def_name)

      # De-serialize the SearchDef content to set default values for
      # properties that may not be present in postgres.
      search_def_obj = self.__BuildSearchDefObj(
          search_def_name,
          basic_types.SearchDefContent.Deserialize(search_def_content))

      logger.debug("SearchDef name: %s", search_def_name)
      logger.debug("SearchDef content: %s",
                   search_def_obj["search_def_content"].DumpJson())

      http_io.ResponseWriter.AddJsonBody(
          response, constants.STATUS_SUCCESS, search_def_obj)
    else:
      raise exceptions.SearchPublishServeException(
          "Internal Error - Invalid Query Command: %s." % query_cmd)

  def HandleAddSearchDefRequest(self, request, response):
    """Handles add search definition request.

    Args:
      request: request object.
      response: response object.

    Raises:
      SearchPublishServeException, psycopg2.Warning/Error.
    """
    logger.debug("HandleAddSearchDefRequest...")

    search_def_name = request.GetIdentifier(constants.SEARCH_DEF_NAME)
    if not search_def_name:
      raise exceptions.SearchPublishServeException(
          "HandleAddSearchDefRequest: missing or invalid name of search"
          " definition.")

    search_def_content = request.GetParameter(constants.SEARCH_DEF)
    if not search_def_content:
      raise exceptions.SearchPublishServeException(
          "HandleAddSearchDefRequest: missing search definition data.")

    try:
      # The content of SearchDef will be sanitized when de-serializing.
      search_def_content_obj = basic_types.SearchDefContent.Deserialize(
          search_def_content)
    except Exception as e:
      raise exceptions.SearchPublishServeException(
          "HandleAddSearchDefRequest:"
          " couldn't parse search definition data. Error: %s", str(e))

    # Update search definition content serializing from object.
    search_def_content = search_def_content_obj.DumpJson()

    # Check if search definition already exists.
    search_def_id = self._QuerySearchDefId(search_def_name)
    logger.debug("HandleAddSearchDefRequest: search_def_id: %d", search_def_id)

    if search_def_id > 0:
      # It's ok if search definition already exists. It means we edit existing
      # search definition.
      logger.debug("Update search definition: %s", search_def_name)

      if search_def_name in SearchPublishManager.SYSTEM_SEARCH_DEF_LIST:
        raise exceptions.SearchPublishServeException(
            "HandleAddSearchDefRequest:"
            " system default search definition can't be edited.")

      query_string = (
          "UPDATE search_def_table SET search_def_content = %s"
          " WHERE search_def_id = %s")
      self._DbModify(query_string, (search_def_content, search_def_id))
    else:
      logger.debug("Add search definition: %s", search_def_name)
      query_string = (
          "INSERT INTO search_def_table (search_def_name, search_def_content)"
          " VALUES(%s, %s)")
      self._DbModify(query_string, (search_def_name, search_def_content))

    search_def_obj = self.__BuildSearchDefObj(
        search_def_name, search_def_content)

    http_io.ResponseWriter.AddJsonBody(
        response, constants.STATUS_SUCCESS, search_def_obj)

  def HandleDeleteSearchDefRequest(self, request, response):
    """Handles delete search definition request.

    Args:
      request: request object.
      response: response object.

    Raises:
      SearchPublishServeException, psycopg2.Warning/Error.
    """
    logger.debug("HandleDeleteSearchDefRequest...")
    search_def_name = request.GetParameter(constants.SEARCH_DEF_NAME)
    if not search_def_name:
      raise exceptions.SearchPublishServeException(
          "HandleDeleteSearchDefRequest: missing search definition name.")

    if search_def_name in SearchPublishManager.SYSTEM_SEARCH_DEF_LIST:
      raise exceptions.SearchPublishServeException(
          "HandleDeleteSearchDefRequest:"
          " system default search definition can't be deleted.")

    # Check if the search definition exists. If it doesn't then just return
    # success.
    search_def_id = self._QuerySearchDefId(search_def_name)
    if search_def_id == 0:
      http_io.ResponseWriter.AddJsonBody(
          response, constants.STATUS_SUCCESS, None)
      return

    # Delete the search definiton from search_def_table.
    query_string = "DELETE FROM search_def_table WHERE search_def_name = %s"
    self._DbModify(query_string, (search_def_name,))

    http_io.ResponseWriter.AddJsonBody(
        response, constants.STATUS_SUCCESS, None)

  def __BuildSearchDefObj(self, name, content):
    """Builds SearchDef object for response.

    Args:
      name: name of Search Definition.
      content: SearchDefContent obj or string.
    Returns:
      SearchDef object for response.
    """
    return ({SearchPublishManager.SEARCH_DEF_NAME: name,
             SearchPublishManager.SEARCH_DEF_CONTENT: content})


def main():
  pass


if __name__ == "__main__":
  main()
