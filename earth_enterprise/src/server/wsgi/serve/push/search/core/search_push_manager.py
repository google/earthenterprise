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

"""Search data pushing manager module.

Classes for handling search data pushing requests.
"""

import logging
import os
import socket

import psycopg2  # No lint.

from common import exceptions
from common import utils
from serve import constants
from serve import http_io
from serve import serve_utils
from serve.push.search.core import search_manager


# TODO:
# 1. in the code os.path.join() is not used for building path to database files
# because we have host name in directory path.
# os.path.join("/gevol/published_dbs/stream_space/<HOST>", "/usr/local...")
# returns "/usr/local/..".
# It can be refactored when host name is demoted.

# TODO: remove VS managing?

# Create logger.
logger = logging.getLogger("ge_search_publisher")


class SearchPushManager(search_manager.SearchManager):
  """Class for handling search data pushing requests."""

  # TODO: decide whether these will be read from some config?
  # TODO: drop default search VS.
  DAV_CONF_PATH = "/opt/google/gehttpd/conf.d/search_space"
  DEFAULT_SEARCH_VS = "default_search"
  POI_TABLE_PREFIX = "GEPOI_"

  def __init__(self):
    """Inits SearchPushManager."""
    super(SearchPushManager, self).__init__()

    self.server_prefix = ""
    self.allow_symlinks = "N"
    self.__ReadDavConfig()
    self.__ReadPublishRootConfig()

  def HandleQueryRequest(self, request, response):
    """Handles query requests.

    Args:
      request: request object.
      response: response object
    Raises:
      SearchPushServeException, psycopg2.Error/Warning.
    """
    query_cmd = request.GetParameter(constants.QUERY_CMD)
    if not query_cmd:
      raise exceptions.SearchPushServeException(
          "Internal Error - Missing Query Command.")

    if query_cmd == constants.QUERY_CMD_LIST_DBS:
      # List all DBs registered on the server.
      query_string = "SELECT host_name, db_name, db_pretty_name FROM db_table"
      results = self._DbQuery(query_string)
      for line in results:
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_HOST_NAME, line[0])
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_DB_NAME, line[1])
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_DB_PRETTY_NAME, line[2])

      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    elif query_cmd == constants.QUERY_CMD_DB_DETAILS:
      # Preview details for a specific DB.
      db_name = request.GetParameter(constants.DB_NAME)
      if not db_name:
        raise exceptions.SearchPushServeException(
            "HandleQueryRequest: missing database name.")
      client_host_name = request.GetClientHostName()
      if not client_host_name:
        raise exceptions.SearchPushServeException(
            "HandleQueryRequest: missing Hostname.")
      # TODO: implement db details request handling:
      # maybe return list of poi tables.

      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    elif query_cmd == constants.QUERY_CMD_SERVER_PREFIX:
      # Get server prefix - path to search space directory.
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_SERVER_PREFIX, self.server_prefix)
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    elif query_cmd == constants.QUERY_CMD_HOST_ROOT:
      # Host root is the top-level directory where a particular fusion client
      # host will put its files. Currently this is the same as host name but
      # we might set it to whatever we want.
      client_host_name = request.GetClientHostName()
      if not client_host_name:
        raise exceptions.SearchPushServeException(
            "HandleQueryRequest: missing Hostname.")

      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_HOST_ROOT, client_host_name)
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    elif query_cmd == constants.QUERY_CMD_SERVER_HOST:
      # Get server host name.
      host_name = socket.gethostname()
      full_host_name = socket.getfqdn()
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_SERVER_HOST, host_name)
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_SERVER_HOST_FULL, full_host_name)
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    elif query_cmd == constants.QUERY_CMD_ALLOW_SYM_LINKS:
      # Get allow symbolic links status.
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_ALLOW_SYM_LINKS, self.allow_symlinks)
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    else:
      raise exceptions.SearchPushServeException(
          "Internal Error - Invalid Query Command: %s." % query_cmd)

  def HandleAddDbRequest(self, request, response):
    """Handles add database request.

    Args:
      request: request object.
      response: response object.

    Raises:
      SearchPushServeException, psycopg2.Warning/Error.
    """
    logger.debug("HandleAddDbRequest...")
    client_host_name = request.GetClientHostName()
    if not client_host_name:
      raise exceptions.SearchPushServeException(
          "HandleAddDbRequest: missing Hostname.")

    db_name = request.GetParameter(constants.DB_NAME)
    db_pretty_name = request.GetParameter(constants.DB_PRETTY_NAME)
    if not db_name:
      raise exceptions.SearchPushServeException(
          "HandleAddDbRequest: missing database name.")

    if not db_pretty_name:
      raise exceptions.SearchPushServeException(
          "HandleAddDbRequest: missing pretty database name.")

    poi_file_paths = request.GetMultiPartParameter(constants.FILE_PATH)
    poi_file_sizes = request.GetMultiPartParameter(constants.FILE_SIZE)

    if ((poi_file_paths and (not poi_file_sizes)) or
        ((not poi_file_paths) and poi_file_sizes)):
      raise exceptions.SearchPushServeException(
          "Internal Error - HandleAddDbRequest: missing file paths or sizes")

    if len(poi_file_paths) != len(poi_file_sizes):
      raise exceptions.SearchPushServeException(
          "Internal Error - HandleAddDbRequest:"
          " poi file_paths/file_sizes mismatch: %d/%d" %
          (len(poi_file_paths), len(poi_file_sizes)))

    logger.debug("poi_file_paths: %s", str(poi_file_paths))
    logger.debug("poi_file_sizes: %s", str(poi_file_sizes))

    # Check if database already exists.
    # The assumption is if it already exists, all the files and db-to-files
    # links have already been pushed to the database.
    db_id = self.QueryDbId(client_host_name, db_name)
    logger.debug("HandleAddDbRequest: db_id: %d", db_id)

    if db_id > 0:
      # It's ok if the database already exists. Don't throw an exception.
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
      return

    # We will now add the following entries into the db:
    # 1) the db_table entry for the database
    # 2) the poi_table entries for each poi  file path
    # 3) the db_poi_table entries linking each file to this database

    # TODO: If anything fails, we want to remove
    # the db_table entry and db_poi_table entries
    # so that publish will start from this point on the next try.

    # Add the db entry.
    query_string = (
        "INSERT INTO db_table (host_name, db_name, db_pretty_name) "
        "VALUES(%s, %s, %s)")

    self._DbModify(query_string,
                   (client_host_name, db_name, db_pretty_name))

    db_id = self.QueryDbId(client_host_name, db_name)
    if db_id == 0:
      raise exceptions.SearchPushServeException(
          "HandleAddDbRequest: unable to add database")

    insert_into_poi_table = (
        "INSERT INTO poi_table "
        "(query_str, style, num_fields, host_name, "
        "poi_file_path, poi_file_size, status) "
        "VALUES('empty query', 'empty_style', 0, %s, %s, %s, 0)")
    insert_into_db_poi_table = ("INSERT INTO db_poi_table (db_id, poi_id) "
                                "VALUES(%s, %s)")

    # Add entries for the poi_table and db_poi_table.
    for i in xrange(len(poi_file_paths)):
      # See if the search table entry already exists.
      poi_id = self._QueryPoiId(client_host_name, poi_file_paths[i])
      if poi_id == 0:
        # Add POI table entry.
        self._DbModify(
            insert_into_poi_table,
            (client_host_name, poi_file_paths[i], poi_file_sizes[i]))
        poi_id = self._QueryPoiId(client_host_name, poi_file_paths[i])
        logger.debug("inserted %s into poi_table with id of %d", poi_file_paths[i], poi_id)

      assert poi_id != 0
      # Add db_poi mapping table entry.
      self._DbModify(insert_into_db_poi_table, (db_id, poi_id))

    http_io.ResponseWriter.AddBodyElement(
        response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)

  def HandleDeleteDbRequest(self, request, response):
    """Handles delete database request.

    Args:
      request: request object.
      response: response object.

    Raises:
      SearchPushServeException, psycopg2.Warning/Error.
    """
    logger.debug("HandleDeleteDbRequest...")
    client_host_name = request.GetClientHostName()
    if not client_host_name:
      raise exceptions.SearchPushServeException(
          "HandleDeleteDbRequest: missing Hostname.")

    db_name = request.GetParameter(constants.DB_NAME)
    if not db_name:
      raise exceptions.SearchPushServeException(
          "HandleDeleteDbRequest: missing database name.")

    # Check if the database exists. If it doesn't then just return success.
    db_id = self.QueryDbId(client_host_name, db_name)
    if db_id == 0:
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
      return

    # Delete the db_poi mappings. The POI tables are not deleted.
    # These will be garbage collected at the request of the user.
    query_string = "DELETE FROM db_poi_table WHERE db_id = %s"
    self._DbModify(query_string, (db_id,))

    # Delete the entry in db_table.
    query_string = "DELETE FROM db_table WHERE db_id = %s"
    self._DbModify(query_string, (db_id,))

    http_io.ResponseWriter.AddBodyElement(
        response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)

  def HandleSyncRequest(self, request, response):
    """Handles database sync request.

    Args:
      request: request object.
      response: response object.
    Returns:
      in response object, the list of files that need to be transfered.
    Raises:
      SearchPushServeException in case of invalid database.
    """
    logger.debug("HandleSyncRequest...")
    client_host_name = request.GetClientHostName()
    if not client_host_name:
      raise exceptions.SearchPushServeException(
          "HandleSyncRequest: missing Hostname.")

    db_name = request.GetParameter(constants.DB_NAME)
    if not db_name:
      raise exceptions.SearchPushServeException(
          "HandleSyncRequest: missing database name.")

    # Difference between stream and search publishers. The search publisher
    # does not care if the database does not exist coz there may be
    # no POI files to sync.
    db_id = self.QueryDbId(client_host_name, db_name)
    if db_id == 0:
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
      return

    transfer_file_paths = self.__SynchronizeDb(db_id, client_host_name)
    if not transfer_file_paths:
      # All the DB files are pushed, return success.
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    else:
      # Put in response files that need to be transferred on server and return
      # the status 'upload needed'.
      for transfer_file_path in transfer_file_paths:
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_FILE_NAME, transfer_file_path)

      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_UPLOAD_NEEDED)

  def HandleGarbageCollectRequest(self, request, response):
    """Handles Garbage Collect request.

    Removes all unnecessary files and file references from the publish root
    volume and database.
    Args:
      request: request object.
      response: response object.
    Raises:
      psycopg2.Error/Warning.
    """
    logger.debug("HandleGarbageCollectRequest...")
    assert request.GetParameter(constants.CMD) == constants.CMD_GARBAGE_COLLECT

    query_string = ("SELECT host_name, poi_file_path, poi_file_size, poi_id"
                    " FROM poi_table WHERE poi_id NOT IN"
                    " (SELECT poi_id FROM db_poi_table)")
    results = self._DbQuery(query_string)

    delete_count = 0
    delete_size = 0
    file_count = 0
    parent_dirs_set = set()

    for row in results:
      file_count += 1
      top_level_dir = "%s/%s" % (self.server_prefix, row[0])
      server_file_path = os.path.normpath("%s%s" % (top_level_dir, row[1]))
      if os.path.exists(server_file_path):
        # Keep a list of parent_dirs so that we can delete them later if empty.
        parent_dir = os.path.dirname(server_file_path)
        while parent_dir != top_level_dir:
          parent_dirs_set.add(parent_dir)
          parent_dir = os.path.dirname(parent_dir)

        # Now we can delete the file.
        try:
          os.remove(server_file_path)
          delete_count += 1
          delete_size += row[2]
        except OSError:
          pass

        # Drop the POI table.
        try:
          self.table_utility.DropTable(
              "%s%d" % (SearchPushManager.POI_TABLE_PREFIX, row[3]))
        except psycopg2.ProgrammingError as e:
          # Just report warning in case of table does not exist.
          logger.warning("HandleGarbageCollectRequest: %s", e)

    if file_count > 0:
      query_string = ("DELETE FROM poi_table WHERE poi_id NOT IN"
                      " (SELECT poi_id FROM db_poi_table)")
      self._DbModify(query_string)

    # Finally delete all the dirs if they are empty. We walk the list in the
    # reverse order coz we want to delete the child dirs before we try to
    # delete the parent dirs.
    parent_dirs_list = sorted(parent_dirs_set)
    for parent_dir in reversed(parent_dirs_list):
      try:
        logger.debug("HandleGarbageCollectRequest: remove dir: %s", parent_dir)
        os.rmdir(parent_dir)
      except OSError:
        pass

    # Queue up the stats for the garbage collection.
    http_io.ResponseWriter.AddBodyElement(
        response, constants.HDR_DELETE_COUNT, delete_count)
    http_io.ResponseWriter.AddBodyElement(
        response, constants.HDR_DELETE_SIZE, delete_size)
    http_io.ResponseWriter.AddBodyElement(
        response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    logger.debug("HandleGarbageCollectRequest done.")

  def HandleLocalTransferRequest(self, request, response):
    """Handles Local Transferring request.

    Args:
      request: request object.
      response: response object.
    Raises:
      SearchPushServeException, psycopg2.Warning/Error.
    """
    logger.debug("HandleLocalTransferRequest...")
    src_path = request.GetParameter(constants.FILE_PATH)
    dest_path = request.GetParameter(constants.DEST_FILE_PATH)

    if (not src_path) or (not dest_path):
      raise exceptions.SearchPushServeException(
          "HandleLocalTransferRequest: Missing src/dest paths")

    src_path = os.path.normpath(src_path)
    dest_path = os.path.normpath(dest_path)
    logger.debug("HandleLocalTransferRequest: %s to %s", src_path, dest_path)

    force_copy = request.IsForceCopy()
    prefer_copy = request.IsPreferCopy()
    if serve_utils.LocalTransfer(src_path, dest_path,
                                 force_copy, prefer_copy, self.allow_symlinks):
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    else:
      raise exceptions.SearchPushServeException(
          "Local transfer FAILED: from %s to %s" %  (src_path, dest_path))

  def __ReadDavConfig(self):
    """Reads WebDAV config (server_prefix)."""
    sub_groups = utils.MatchPattern(SearchPushManager.DAV_CONF_PATH,
                                    "^Alias\\s+/search_space\\s+(.*)")
    if sub_groups:
      self.server_prefix = sub_groups[0]

    if not self.server_prefix:
      raise exceptions.SearchPushServeException(
          "Unable to read WebDAV Config.")
    logger.info("Server Prefix: " + self.server_prefix)

  def __ReadPublishRootConfig(self):
    self.allow_symlinks = "N"
    config_path = os.path.join(os.path.dirname(self.server_prefix), ".config")
    try:
      # Older publish roots may not have the .config file so we default to "N".
      if os.path.exists(config_path):
        sub_groups = utils.MatchPattern(
            config_path,
            "^AllowSymLinks:\\s*([YyNn])\\s*$")
        if sub_groups:
          self.allow_symlinks = sub_groups[0]
        else:
          raise exceptions.SearchPushServeException(
              "Invalid Publish root config.")
    finally:
      logger.info("AllowSymlinks: " + self.allow_symlinks)

  def __SynchronizeDb(self, db_id, client_host_name):
    """Synchronizes database files.

    Args:
      db_id: database ID.
      client_host_name: Fusion client host name - part of database ID.
      It is used here to build path to published DB files.
    Returns:
      list of files that need to be transfered.
    """
    logger.debug("__SynchronizeDb...")
    file_good = "1"
    transfer_file_paths = []
    transfer_file_sizes = []
    query_string = ("SELECT poi_file_path, poi_file_size FROM poi_table"
                    " WHERE status != %s"
                    " AND poi_id IN(SELECT poi_id FROM db_poi_table"
                    " WHERE db_id = %s)")

    results = self._DbQuery(query_string, (file_good, db_id))
    for row in results:
      transfer_file_paths.append(row[0])
      transfer_file_sizes.append(row[1])

    transfer_file_paths = self.__UpdatePoiStatus(
        transfer_file_paths, transfer_file_sizes, client_host_name)
    logger.debug("__SynchronizeDb done.")
    return transfer_file_paths

  def __UpdatePoiStatus(self, file_paths, file_sizes, client_host_name):
    """Updates the status for files in poi_table.

    Fixes the status for any files that are really pushed/published but
    have invalid status values in the database.
    The input vectors are modified to leave the list of files (paths and sizes)
    that need to be transfered still.

    Args:
      file_paths: the list of file paths to check.
      file_sizes: the list of file sizes.
      client_host_name: Fusion client host name.
    Returns:
      file_paths: the list of file paths for files that need to be transfered.
    """
    logger.debug("__UpdatePoiStatus...")
    file_prefix = self.__ServerFilePrefix(client_host_name)
    if file_prefix is None:
      logger.info("File prefix is None")
    else:
      logger.info("File prefix is '%s'", file_prefix)
    existing_files = []
    i = 0
    while i in range(len(file_paths)):
      server_file_path = os.path.normpath(file_prefix + file_paths[i])
      if (os.path.exists(server_file_path) and
          serve_utils.FileSizeMatched(server_file_path, file_sizes[i])):

        existing_files.append(file_paths[i])
        logger.debug("%s already exist", server_file_path)

        file_paths.pop(i)
        file_sizes.pop(i)
      else:
        i += 1
        logger.debug("%s does not exists", server_file_path)

    # Create POI tables for the files that are already uploaded.
    update_string = ("UPDATE poi_table SET status = 1, num_fields = %s,"
                     " query_str = %s, style = %s WHERE poi_id = %s")
    for existing_file in existing_files:
      poi_id = self._QueryPoiId(client_host_name, existing_file)
      server_file_path = os.path.normpath(file_prefix + existing_file)
      (num_fields, query_str, balloon_style) = self.__CreatePoiTable(
          server_file_path, self.__PoiTableName(poi_id), file_prefix)

      # Update the poi_table. Change the status to 1 and update query_strs.
      self._DbModify(update_string,
                     (num_fields, query_str, balloon_style, poi_id))

    logger.debug("__UpdatePoiStatus done.")
    return file_paths

  def __ServerFilePrefix(self, client_host_name):
    """Builds server file prefix.

    Example: /gevol/published_dbs/host_name
    Args:
      client_host_name: Fusion client host name (part of database ID).
    Returns:
      generated server file prefix.
    """
    return "%s/%s" % (self.server_prefix, client_host_name)

  def __PoiTableName(self, poi_id):
    """Generates POI table name.

    Args:
      poi_id: POI table ID.
    Returns:
      generated POI table name.
    """
    return "%s%d" % (SearchPushManager.POI_TABLE_PREFIX, poi_id)

  def __CreatePoiTable(self, poi_file_path, table_name, file_prefix=None):
    """Creates and populates POI table.

    Args:
      poi_file_path: a path to a POI file.
      table_name: the table name to use for this POI file.
    Returns:
      tuple that contains:
      numFields: total number of fields the SchemaParser extracts from
      the POI file.
      query_string: SQL query string that should be used to query
      the POI table created.
      balloon_style: balloon style that should be used for each POI balloon.
    Raises:
      SearchSchemaTableUtilException, psycopg2.Warning/Error.
    """
    if file_prefix is None:
      logger.info("File prefix is None")
    else:
      logger.info("File prefix is '%s'", file_prefix)
    return self.table_utility.CreatePoiTable(poi_file_path, table_name.lower(), file_prefix)


def main():
  pass


if __name__ == "__main__":
  main()
