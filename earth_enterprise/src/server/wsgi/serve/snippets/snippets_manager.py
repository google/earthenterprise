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

"""Snippets data manager module.

Classes for handling snippets managing requests.
"""

import json
import logging

from common import exceptions
from serve import basic_types
from serve import constants
from serve import http_io
from serve import snippets_db_manager
from serve.snippets.data import metainfo_by_fieldpath
from serve.snippets.util import tree_utils


# Create logger.
logger = logging.getLogger("ge_snippets")


class SnippetsManager(snippets_db_manager.SnippetsDbManager):
  """Class for handling snippets managing requests."""

  def __init__(self):
    """Inits SnippetsManager."""
    super(SnippetsManager, self).__init__()

  def HandleQueryRequest(self, request, response):
    """Handles query requests.

    Args:
      request: request object.
      response: response object
    Raises:
      SnippetsServeException, psycopg2.Error/Warning.
    """
    query_cmd = request.GetParameter(constants.QUERY_CMD)
    if not query_cmd:
      raise exceptions.SnippetsServeException(
          "Internal Error - Missing Query Command.")

    # List all snippets sets registered on the server.
    if query_cmd == constants.QUERY_CMD_LIST_SNIPPET_SETS:
      query_string = "SELECT name, content FROM snippet_set_table"
      results = self._DbQuery(query_string)
      snippets_set_list = [
          self.__BuildSnippetSetObj(row[0], row[1]) for row in results]

      http_io.ResponseWriter.AddJsonBody(
          response, constants.STATUS_SUCCESS, snippets_set_list)
    # Preview details for a specific snippets set.
    elif query_cmd == constants.QUERY_CMD_SNIPPET_SET_DETAILS:
      snippets_set_name = request.GetParameter(constants.SNIPPET_SET_NAME)
      if not snippets_set_name:
        raise exceptions.SnippetsServeException(
            "HandleQueryRequest: missing snippets set name.")

      end_snippet = self.GetSnippetSetDetails(snippets_set_name)
      if not end_snippet:
        raise exceptions.SnippetsServeException(
            "The snippet set %s does not exist." % snippets_set_name)

      logger.debug("Snippet set name: %s", snippets_set_name)
      logger.debug("End snippet: %s", end_snippet)
      snippets_set = self.__BuildSnippetSetObj(snippets_set_name, end_snippet)
      http_io.ResponseWriter.AddJsonBody(
          response, constants.STATUS_SUCCESS, snippets_set)
    # List paths of metafields available for editing.
    elif query_cmd == constants.QUERY_CMD_LIST_META_FIELD_PATHS:
      meta_fields_obj = json.loads(metainfo_by_fieldpath.META_INFO)
      field_paths_list = meta_fields_obj.keys()
      field_paths_json = json.dumps(
          field_paths_list, indent=2, separators=(",", ":"))
      logger.debug("Meta field paths: %s", field_paths_json)

      http_io.ResponseWriter.AddJsonBody(
          response, constants.STATUS_SUCCESS, field_paths_json)
    # List metafields available for editing.
    elif query_cmd == constants.QUERY_CMD_META_FIELDS_SET:
      meta_fields_obj = json.loads(metainfo_by_fieldpath.META_INFO)
      meta_fields_json = json.dumps(
          meta_fields_obj, indent=2, separators=(",", ":"))
      logger.debug("Meta fields: %s", meta_fields_json)
      http_io.ResponseWriter.AddJsonBody(
          response, constants.STATUS_SUCCESS, meta_fields_json)
    else:
      raise exceptions.SnippetsServeException(
          "Invalid Query Command: %s." % query_cmd)

  def HandleAddSnippetSetRequest(self, request, response):
    """Handles add snippets set request.

    Args:
      request: request object.
      response: response object.

    Raises:
      SnippetsServeException, psycopg2.Warning/Error.
    """
    logger.debug("HandleAddSnippetSetRequest...")

    snippets_set_name = request.GetIdentifier(constants.SNIPPET_SET_NAME)
    if not snippets_set_name:
      raise exceptions.SnippetsServeException(
          "HandleAddSnippetSetRequest: missing or invalid snippet set name.")

    snippets_set = request.GetParameter(constants.SNIPPET_SET)
    if not snippets_set:
      raise exceptions.SnippetsServeException(
          "HandleAddSnippetSetRequest: missing snippet set data.")

    try:
      # Dense snippets tree, since we accept snippets in client friendly form,
      # i.e. sparse tree json.
      dense_snippets = tree_utils.CompactTree(snippets_set, logger)
    except Exception as e:
      raise exceptions.SnippetsServeException(
          "Couldn't parse snippet set: %s. Error: %s" % (snippets_set_name, e))

    set_content = json.dumps(dense_snippets)

    if self._SnippetSetExists(snippets_set_name):
      # It's ok if snippets set already exists. It means we edit existing one.
      logger.debug("Update snippets set: %s", snippets_set_name)

      query_string = "UPDATE snippet_set_table SET content = %s WHERE name = %s"
      self._DbModify(query_string, (set_content, snippets_set_name))
    else:
      logger.debug("Add snippets set: %s", snippets_set_name)
      query_string = (
          "INSERT INTO snippet_set_table (name, content) VALUES(%s, %s)")
      self._DbModify(query_string, (snippets_set_name, set_content))

    snippets_set = basic_types.SnippetSet(snippets_set_name, dense_snippets)
    http_io.ResponseWriter.AddJsonBody(
        response, constants.STATUS_SUCCESS, snippets_set)

  def HandleDeleteSnippetSetRequest(self, request, response):
    """Handles delete snippets set request.

    Args:
      request: request object.
      response: response object.

    Raises:
      SnippetsServeException, psycopg2.Warning/Error.
    """
    logger.debug("HandleDeleteSnippetSetRequest...")
    snippets_set_name = request.GetParameter(constants.SNIPPET_SET_NAME)
    if not snippets_set_name:
      raise exceptions.SnippetsServeException(
          "HandleDeleteSnippetSetRequest: missing snippets set name.")

    # Check if the snippets set exists. If it doesn't then just return success.
    if not self._SnippetSetExists(snippets_set_name):
      http_io.ResponseWriter.AddJsonBody(
          response, constants.STATUS_SUCCESS, None)
      return

    # Delete the snippets set entry.
    query_string = "DELETE FROM snippet_set_table WHERE name = %s"
    self._DbModify(query_string, (snippets_set_name,))

    http_io.ResponseWriter.AddJsonBody(
        response, constants.STATUS_SUCCESS, None)

  def __BuildSnippetSetObj(self, name, content):
    """Builds SnippetSet object.

    Args:
      name: name of a snippets set.
      content: content of a snippets set in json-formatted string.
    Returns:
      SnippetSet object.
    """
    # Restore SnippetSet object from json-formatted string.
    assert isinstance(name, str)
    assert isinstance(content, str)
    return basic_types.SnippetSet(name, json.loads(content))


def main():
  pass


if __name__ == "__main__":
  main()
