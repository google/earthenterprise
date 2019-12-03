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


"""Stream publisher helper module.

Helper classes for handling stream publisher requests.
"""

import datetime
import itertools
import json
import logging
import math
import os
import socket
import sys

from common import batch_sql_manager
from common import exceptions
from common import utils
from serve import basic_types
from serve import constants
from serve import http_io
from serve import serve_utils
from serve import stream_manager

# TODO:
# 1. in the code, os.path.join() is not used for building path to database files
# because db_path begins with '/'. The db_path is an assetroot's path.
# os.path.join("/gevol/published_dbs/stream_space/<HOST>", "/usr/local...")
# returns "/usr/local/.."

# Get logger
logger = logging.getLogger("ge_stream_publisher")


class StreamPushManager(stream_manager.StreamManager):
  """Class for handling stream data managing requests.

  Stream data managing requests: AddDb, PushDb, DeleteDb, GarbageCollect.
  """

  def __init__(self):
    """Inits StreamPushManager."""
    super(StreamPushManager, self).__init__()
    self._allow_symlinks = "N"
    self._ReadPublishRootConfig()

  def _ReadPublishRootConfig(self):
    self._allow_symlinks = "N"
    config_path = os.path.join(os.path.dirname(self.server_prefix), ".config")
    try:
      # Older publish roots may not have the .config file so we default to "N".
      if os.path.exists(config_path):
        sub_groups = utils.MatchPattern(
            config_path,
            "^AllowSymLinks:\\s*([YyNn])\\s*$")
        if sub_groups:
          self._allow_symlinks = sub_groups[0]
        else:
          raise exceptions.StreamPushServeException(
              "Invalid Publish root config.")
    finally:
      logger.info("AllowSymlinks: " + self._allow_symlinks)

  def HandleCleanupRequest(self, request, response):
    """Handles cleanup request.

    Unregisters all portable globes that do not exist in file system.
    Args:
      request: request object.
      response: response object.
    Returns:
      in response object list of portables that were unregistered/unpublished in
      case of success, or list of portables that are registered but do not exist
      in file system in case of failure.
    """
    cmd = request.GetParameter(constants.CMD)
    assert cmd and (cmd == constants.CMD_CLEANUP)

    # Get information about registered portables.
    query_string = "SELECT db_name FROM db_table WHERE host_name = ''"

    results = self.DbQuery(query_string)

    # Flag for whether globes directory is mounted and at least one portable
    # globe exists. If not, don't remove Portables from postgres db.
    is_globes_mounted = (
        os.path.exists(stream_manager.StreamManager.CUTTER_GLOBES_PATH) and
        serve_utils.ExistsPortableInDir(
            stream_manager.StreamManager.CUTTER_GLOBES_PATH))

    if not is_globes_mounted:
      logger.warning(
          "HandleCleanupRequest: No portable files in directory %s."
          " Volume may not be mounted.",
          stream_manager.StreamManager.CUTTER_GLOBES_PATH)
      logger.warning("Cleanup has not been run.")
      registered_portables = [
          {"name": db_name,
           "path": "{0}{1}".format(
               stream_manager.StreamManager.CUTTER_GLOBES_PATH,
               db_name)} for db_name in results]
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_DATA, json.dumps(registered_portables))
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_MESSAGE,
          "No portable files in directory {0}."
          " Volume may not be mounted.".format(
              stream_manager.StreamManager.CUTTER_GLOBES_PATH))
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_FAILURE)
      return

    unregistered_portables = []
    for db_name in results:
      # Get database type, path.
      (db_path, db_type) = serve_utils.IdentifyPublishedDb(db_name)
      assert serve_utils.IsPortable(db_type)

      publish_db_path = "{0}{1}".format(
          stream_manager.StreamManager.CUTTER_GLOBES_PATH, db_path)

      if not os.path.exists(publish_db_path):
        # Check if non-existing portable is registered.
        client_host_name = ""
        db_id = self.QueryDbId(client_host_name, db_path)
        if db_id != 0:
          # Unpublish portable.
          self._UnpublishPortable(db_id)
          # Unregister portable.
          self._UnregisterPortable(db_id)
          unregistered_portables.append(
              {"name": db_name, "path": publish_db_path})

    logger.info("Push info cleanup is complete.")

    http_io.ResponseWriter.AddBodyElement(
        response, constants.HDR_DATA, json.dumps(unregistered_portables))
    http_io.ResponseWriter.AddBodyElement(
        response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)

  def HandleQueryRequest(self, request, response):
    """Handles query requests.

    Args:
      request: request object.
      response: response object.

    Raises:
      StreamPushServeException.
    """
    query_cmd = request.GetParameter(constants.QUERY_CMD)
    if not query_cmd:
      raise exceptions.StreamPushServeException(
          "Internal Error - Missing Query Command.")

    # List all DBs registered on server.
    if query_cmd == constants.QUERY_CMD_LIST_DBS:
      query_string = "SELECT host_name, db_name, db_pretty_name FROM db_table"
      results = self.DbQuery(query_string)
      for line in results:
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_HOST_NAME, line[0])
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_DB_NAME, line[1])
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_DB_PRETTY_NAME, line[2])

      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    # List all published DBs.
    elif query_cmd == constants.QUERY_CMD_PUBLISHED_DBS:
      query_string = """
          SELECT target_table.target_path, virtual_host_table.virtual_host_name,
               db_table.db_name, db_table.db_pretty_name, db_table.host_name
          FROM db_table, virtual_host_table, target_db_table, target_table
          WHERE db_table.db_id = target_db_table.db_id AND
              target_table.target_id = target_db_table.target_id AND
              virtual_host_table.virtual_host_id = target_db_table.virtual_host_id
          """
      results = self.DbQuery(query_string)
      for line in results:
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_TARGET_PATH, line[0])
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_VS_NAME, line[1])
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_DB_NAME, line[2])
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_DB_PRETTY_NAME, line[3])
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_HOST_NAME, line[4])

      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    # Get DB details.
    elif query_cmd == constants.QUERY_CMD_DB_DETAILS:
      db_name = request.GetParameter(constants.DB_NAME)
      if not db_name:
        raise exceptions.StreamPushServeException(
            "HandleQueryRequest: missing db name")
      client_host_name = request.GetClientHostName()
      if not client_host_name:
        raise exceptions.StreamPushServeException(
            "HandleQueryRequest: missing Hostname.")

      query_string = (
          "SELECT file_path FROM files_table WHERE file_id IN ("
          "SELECT file_id FROM db_files_table WHERE db_id IN ("
          "SELECT db_id FROM db_table WHERE "
          "host_name = %s AND db_name = %s))")
      results = self.DbQuery(query_string, (client_host_name, db_name))
      for line in results:
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_FILE_NAME, line)

      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    # Get server prefix - path to published_dbs directory.
    elif query_cmd == constants.QUERY_CMD_SERVER_PREFIX:
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_SERVER_PREFIX, self.server_prefix)
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    # Get client hostname.
    elif query_cmd == constants.QUERY_CMD_HOST_ROOT:
      client_host_name = request.GetClientHostName()
      if not client_host_name:
        raise exceptions.StreamPushServeException(
            "HandleQueryRequest: missing Hostname.")

      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_HOST_ROOT, client_host_name)
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    # Get server hostname.
    elif query_cmd == constants.QUERY_CMD_SERVER_HOST:
      host_name = socket.gethostname()
      full_host_name = socket.getfqdn()
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_SERVER_HOST, host_name)
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_SERVER_HOST_FULL, full_host_name)
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    # Get allow symbolic link parameter.
    elif query_cmd == constants.QUERY_CMD_ALLOW_SYM_LINKS:
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_ALLOW_SYM_LINKS, self._allow_symlinks)
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    else:
      raise exceptions.StreamPushServeException(
          "Internal Error - Invalid Query Command: %s." % query_cmd)

  def HandleAddDbRequest(self, request, response):
    """Handles add database request.

    Args:
      request: request object.
      response: response object.

    Raises:
      StreamPushServeException.
    """
    logger.debug("HandleAddDbRequest..")
    db_name = request.GetParameter(constants.DB_NAME)
    db_pretty_name = request.GetParameter(constants.DB_PRETTY_NAME)
    if not db_name:
      raise exceptions.StreamPushServeException(
          "Internal Error - HandleAddDbRequest: missing db name")

    if not db_pretty_name:
      # if db_pretty_name is not specified, set it equal to db_name.
      # Note: db_pretty_name is a database alias to make it easy to remember.
      # For the fusion databases, we set it equal to the asset root database
      # path, by default.
      db_pretty_name = db_name

    file_paths = request.GetMultiPartParameter(constants.FILE_PATH)
    file_sizes = request.GetMultiPartParameter(constants.FILE_SIZE)

    if (not file_sizes) or (not file_paths):
      raise exceptions.StreamPushServeException(
          "Internal Error - HandleAddDbRequest: missing file paths/sizes")
    if len(file_paths) != len(file_sizes):
      raise exceptions.StreamPushServeException(
          "Internal Error - HandleAddDbRequest:"
          " file_paths/file_sizes mismatch: %d/%d" % (len(file_paths),
                                                      len(file_sizes)))

    db_timestamp = request.GetParameter(constants.DB_TIMESTAMP)
    db_size = request.GetParameter(constants.DB_SIZE)

    # Full ISO 8601 datetime format: '%Y-%m-%dT%H:%M:%S.%f-TZ'.
    # Check if database timestamp begins with a valid date format.
    if db_timestamp:
      try:
        datetime.datetime.strptime(db_timestamp[:10],
                                   serve_utils.DATE_ISO_FORMAT)
      except ValueError, e:
        logger.warning(
            "Database/Portable timestamp format is not valid. Error: %s", e)
        db_timestamp = None

    # Collecting database properties and packing into db_flags variable.
    db_use_google_basemap = request.GetParameter(
        constants.DB_USE_GOOGLE_BASEMAP)
    logger.debug("HandleAddDbRequest: db_use_google_basemap: %s",
                 db_use_google_basemap)
    db_flags = 0
    if db_use_google_basemap and int(db_use_google_basemap) == 1:
      db_flags = basic_types.DbFlags.USE_GOOGLE_BASEMAP

    (db_path, db_type) = serve_utils.IdentifyPublishedDb(db_name)
    if  serve_utils.IsFusionDb(db_type):
      client_host_name = request.GetClientHostName()
      if not client_host_name:
        raise exceptions.StreamPushServeException(
            "HandleAddDbRequest: missing Hostname.")
    elif serve_utils.IsPortable(db_type):
      # Note: The client host name is not used for portable globes, so we just
      # make it an empty string.
      client_host_name = ""
      # Get timestamp and size from portable info.
      db_info = basic_types.DbInfo()
      db_info.name = db_name
      db_info.type = db_type
      serve_utils.GlxDetails(db_info)
      db_timestamp = db_info.timestamp
      db_size = db_info.size
    else:
      raise exceptions.StreamPushServeException(
          "HandleAddDbRequest: Unsupported DB type %s.", db_type)

    # Check if database already exists.
    # The assumption is if it already exists, all the files and db-to-files
    # links have already been published to the database.
    db_id, curr_db_flags = self.QueryDbIdAndFlags(client_host_name, db_path)
    logger.debug("HandleAddDbRequest: db_id: %d, curr_db_flags: %d",
                 db_id, curr_db_flags)

    if db_id > 0:
      # It's OK if the database already exists. Don't throw an exception.
      # Note: db_table.db_flags field was added in GEE 5.0.1, so for databases
      # pushed with GEE Fusion 5.0.0 and less we need to update db_flags in
      # postgres.
      if db_flags and (not curr_db_flags):
        # Update db_table.db_flags for existing database.
        query_string = ("UPDATE db_table SET db_flags = %s WHERE db_id = %s")
        self.DbModify(query_string, (db_flags, db_id))

      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
      return

    # We will now add the following entries into the db:
    # 1) the db_table entry for the database
    # 2) the files_table entries for each file path
    # 3) the db_files_table entries linking each file to this database
    # If anything fails in here, we want to remove the db_table entry (and any
    # db_files_table entries) so that publish will start from this point on
    # the next try.
    try:
      # Add the db entry.
      query_string = (
          "INSERT INTO db_table (host_name, db_name, db_pretty_name,"
          " db_timestamp, db_size, db_flags) "
          "VALUES(%s, %s, %s, %s, %s, %s)")
      self.DbModify(query_string,
                    (client_host_name, db_name, db_pretty_name,
                     db_timestamp, db_size, db_flags))

      # Record the db_id now, if it succeeded, and the following inserts fail,
      # we can at least back some of the inserts out so we can try again later.
      db_id = self.QueryDbId(client_host_name, db_name)
      if db_id == 0:
        raise exceptions.StreamPushServeException(
            "HandleAddDbRequest: unable to add database: hostname:"
            " %s, database name: %s" % (client_host_name, db_name))

      # Check which files already exist in the files_table.
      file_path_exists_flags = self._CheckFileExistence(client_host_name,
                                                        file_paths)
      assert file_path_exists_flags

      # Add files entries.
      # Batch the SQL commands in groups of 1000 speeds things up noticeably.
      # fewer gets slightly worse, more is not noticeable.
      group_size = 1000
      batcher = batch_sql_manager.BatchSqlManager(
          self.stream_db, group_size, logger)
      query_string = ("INSERT INTO files_table "
                      "(host_name, file_path, file_size, file_status) "
                      "VALUES(%s, %s, %s, 0)")

      for i in range(len(file_paths)):
        # Add only the newer files to avoid throwing SQLException.
        if not file_path_exists_flags[i]:
          batcher.AddModify(
              query_string,
              [client_host_name, file_paths[i], file_sizes[i]])

      batcher.ExecuteRemaining()

      # Finally update the db_files_table by first querying the file_ids from
      # the files_table.
      file_ids = self._QueryFileIds(client_host_name, file_paths)
      assert file_ids

      query_string = ("INSERT INTO db_files_table (db_id, file_id) "
                      "VALUES(%s, %s)")

      parameters = [(db_id, file_id) for file_id in file_ids]

      batcher.AddModifySet(query_string, parameters)

      batcher.ExecuteRemaining()
      batcher.Close()  # Need to close to release SQL resources.

      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    except Exception as e:
      # Record the internal exception to pass out via the final exception.
      error_message = ("HandleAddDbRequest:"
                       " Error adding files to publisher database\n") + repr(e)

      # Clean up any remnants of this db_id, so that a subsequent attempt to
      # publish will start over at this step.
      # Note: this assumes that our connection is still valid.
      try:
        if db_id > 0:
          # Note: this will leave behind some entries in files_table, but this
          # will not hurt anything.
          query_string = "DELETE FROM db_files_table WHERE db_id = %s"
          self.DbModify(query_string, [db_id])
          query_string = "DELETE FROM db_table WHERE db_id = %s"
          self.DbModify(query_string, [db_id])
      except exceptions.Error:
        error_message += "\nAttempt to cleanup the database failed"

      raise exceptions.StreamPushServeException(error_message)

  def HandleDeleteDbRequest(self, request, response):
    """Handles delete database requests.

    Args:
      request: request object.
      response: response object.
    Raises:
      OSError, psycopg2.Warning/Error, StreamPushServeException
    """
    logger.debug("HandleDeleteDbRequest...")
    db_name = request.GetParameter(constants.DB_NAME)

    if not db_name:
      raise exceptions.StreamPushServeException(
          "HandleDeleteDbRequest: Missing database name.")

    (db_path, db_type) = serve_utils.IdentifyPublishedDb(db_name)
    if serve_utils.IsFusionDb(db_type):
      client_host_name = request.GetClientHostName()
      if not client_host_name:
        raise exceptions.StreamPushServeException(
            "HandleDeleteDbRequest: missing Hostname.")
    elif serve_utils.IsPortable(db_type):
      # Note: The client host name is not used for portable globes, so we just
      # make it an empty string.
      client_host_name = ""
    else:
      raise exceptions.StreamPushServeException(
          "HandleDeleteDbRequest: Unsupported DB type %s.", db_type)

    # Check if the database exists.
    db_id = self.QueryDbId(client_host_name, db_path)
    if db_id == 0:
      raise exceptions.StreamPushServeException(
          "HandleDeleteDbRequest: Could not find database: "
          "Fusion host: {0} Database name: {1}.".format(
              client_host_name, db_path))

    # Check if the database is currently published.
    if self._QueryDbPublished(db_id):
      raise exceptions.StreamPushServeException(
          "HandleDeleteDbRequest: Database is currently published."
          " Please unpublish it first.")

    # Delete the entries in db_table.
    query_string = "DELETE FROM db_table WHERE db_id = %s"
    self.DbModify(query_string, (db_id,))

    if serve_utils.IsFusionDb(db_type):
      # Delete the entries in db_files_table. The entries in the
      # files_table and the actual files are untouched. Those will be garbage
      # collected at the request of the user.
      query_string = "DELETE FROM db_files_table WHERE db_id = %s"
      self.DbModify(query_string, [db_id])
    elif serve_utils.IsPortable(db_type):
      query_string = ("SELECT file_id FROM db_files_table WHERE db_id = %s")
      rs_db_files = self.DbQuery(query_string, (db_id,))

      if rs_db_files:
        assert len(rs_db_files) == 1
        file_id = rs_db_files[0]

        # Delete the entries in db_files_table.
        delete_db_files_table_cmd = (
            "DELETE FROM db_files_table WHERE db_id = %s")
        self.DbModify(delete_db_files_table_cmd, (db_id,))

        # Get file_path from files_table and delete file.
        query_string = ("SELECT file_path FROM files_table"
                        " WHERE file_id = %s")
        rs_files = self.DbQuery(query_string, (file_id,))
        if rs_files:
          assert len(rs_files) == 1
          assert db_path == rs_files[0]
          if os.path.exists(rs_files[0]):
            os.unlink(rs_files[0])

        # Delete the file entry from files_table.
        delete_files_table_cmd = "DELETE FROM files_table WHERE file_id = %s"
        self.DbModify(delete_files_table_cmd, (file_id,))
    else:
      raise exceptions.StreamPushServeException(
          "Unsupported DB type %s.", db_type)

    http_io.ResponseWriter.AddBodyElement(
        response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)

  def HandleUnregisterPortableRequest(self, request, response):
    """Handles unregister portable globe requests.

    Cleans up table's entries related to specified portable globe.
    Args:
      request: request object.
      response: response object.
    Raises:
      OSError, psycopg2.Warning/Error, StreamPushServeException
    """
    logger.debug("HandleUnregisterPortableRequest...")
    db_name = request.GetParameter(constants.DB_NAME)

    if not db_name:
      raise exceptions.StreamPushServeException(
          "HandleUnregisterPortableRequest: Missing database name.")

    (db_path, db_type) = serve_utils.IdentifyPublishedDb(db_name)

    if not serve_utils.IsPortable(db_type):
      raise exceptions.StreamPushServeException(
          "HandleUnregisterPortableRequest: Unsupported DB type %s.", db_type)

    # Note: The client host name is not used for portable globes, so we just
    # make it an empty string.
    client_host_name = ""
    # Check if the database exists.
    db_id = self.QueryDbId(client_host_name, db_path)
    if db_id == 0:
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
      return

    # Check if the database is currently published.
    if self._QueryDbPublished(db_id):
      raise exceptions.StreamPushServeException(
          "HandleUnregisterPortableRequest: Database is currently published."
          " Please unpublish it first.")

    self._UnregisterPortable(db_id)
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
      StreamPushServeException in case of invalid database.
    """
    logger.debug("HandleSyncRequest..")
    db_name = request.GetParameter(constants.DB_NAME)
    if not db_name:
      raise exceptions.StreamPushServeException(
          "HandleSyncRequest: missing db name.")

    (db_path, db_type) = serve_utils.IdentifyPublishedDb(db_name)
    if not serve_utils.IsFusionDb(db_type):
      raise exceptions.StreamPushServeException(
          "HandleSyncRequest: Unsupported DB type %s.", db_type)

    client_host_name = request.GetClientHostName()
    if not client_host_name:
      raise exceptions.StreamPushServeException(
          "HandleSyncRequest: missing Hostname.")

    db_id = self.QueryDbId(client_host_name, db_path)
    if db_id == 0:
      raise exceptions.StreamPushServeException(
          "HandleSyncRequest: Database %s is not registered on server." %
          db_name)

    transfer_file_paths = self.SynchronizeDb(db_id, db_type, client_host_name)
    if not transfer_file_paths:
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    else:
      for transfer_file_path in transfer_file_paths:
        http_io.ResponseWriter.AddBodyElement(
            response, constants.HDR_FILE_NAME, transfer_file_path)

      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_UPLOAD_NEEDED)

  def HandleLocalTransferRequest(self, request, response):
    """Handles Local Transferring request.

    Args:
      request: request object.
      response: response object.
    Raises:
      StreamPushServeException
    """
    logger.debug("HandleLocalTransferRequest...")
    src_path = request.GetParameter(constants.FILE_PATH)
    dest_path = request.GetParameter(constants.DEST_FILE_PATH)

    if (not src_path) or (not dest_path):
      raise exceptions.StreamPushServeException(
          "HandleLocalTransferRequest: Missing src/dest paths.")

    src_path = os.path.normpath(src_path)
    dest_path = os.path.normpath(dest_path)
    logger.debug("HandleLocalTransferRequest: %s to %s", src_path, dest_path)

    force_copy = request.IsForceCopy()
    prefer_copy = request.IsPreferCopy()
    if serve_utils.LocalTransfer(src_path, dest_path,
                                 force_copy, prefer_copy, self._allow_symlinks):
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_CODE, constants.STATUS_SUCCESS)
    else:
      raise exceptions.StreamPushServeException("Local transfer failed.")

  def HandleGarbageCollectRequest(self, request, response):
    """Handles Garbage Collect request.

    Removes all unnecessary files and file references from the publish root
    volume and database.
    Args:
      request: request object.
      response: response object.
    Raises:
      psycopg2.Error/Warning
    """
    logger.debug("HandleGarbageCollectRequest..")
    assert request.GetParameter(constants.CMD) == constants.CMD_GARBAGE_COLLECT
    parent_dirs_set = set()
    delete_count = 0
    delete_size = 0

    # The "NOT IN" sql operator is painfully slow for large collections of
    # files.
    # Instead we do 2 queries of all the file_id(s) in files_table and
    # db_files_table
    # and walk through the list removing files_table entries that don't occur
    # in db_files_table and vice versa.
    query = ("SELECT file_id, host_name, file_path, file_size"
             " FROM files_table ORDER BY file_id")
    rs_files = self.DbQuery(query)
    query = ("SELECT file_id FROM db_files_table"
             " GROUP BY file_id ORDER BY file_id")
    rs_db_files = self.DbQuery(query)

    db_file_id = sys.maxint
    if rs_db_files:
      db_file_iter = iter(rs_db_files)
      db_file_id = int(db_file_iter.next())

    # Use a BatchSqlManager to batch up many individual SQL commands into
    # one postgres invocation.
    batcher = batch_sql_manager.BatchSqlManager(
        self.stream_db, 1000, logger)
    top_level_dir_prefix = self.server_prefix + "/"
    delete_files_table_cmd = "DELETE FROM files_table WHERE file_id = %s"
    delete_db_files_table_cmd = (
        "DELETE FROM db_files_table WHERE file_id = %s")

    # We have 2 sorted lists of file_id's in the query results.
    # db_files_table is the subset of files_table
    # We will walk the two lists removing any files_table entries that don't
    # appear in db_files_table.
    for rs_file in rs_files:
      file_id = int(rs_file[0])
      # Check the boundary case...this could happen and would basically mean
      # pain for whomever this happens to.
      if file_id == sys.maxint:
        logger.error("HandleGarbageCollectRequest has encountered a file_id "
                     "equal to the max int value. "
                     "The database has run out of valid file id's.")
        raise exceptions.StreamPushServeException(
            "The publisher database has run out of valid file id's."
            " This published database must likely be recreated from scratch.")

      if file_id < db_file_id:
        # Delete this file:
        # the files_table entry does not exist in the db_files_table

        # Check if the file exists and if so delete it.
        top_level_dir = top_level_dir_prefix + rs_file[1]
        server_file_path = os.path.normpath(top_level_dir + rs_file[2])
        if os.path.exists(server_file_path):
          # Keep a set of parent dirs so that we can delete them later if
          # empty.
          parent_dir = os.path.dirname(server_file_path)
          while parent_dir != top_level_dir:
            parent_dirs_set.add(parent_dir)
            parent_dir = os.path.dirname(parent_dir)

          # Now we can delete the file from the file system. And, whether
          # it is successful, we delete the file entry from files_table.
          try:
            os.remove(server_file_path)
            delete_count += 1
            delete_size += rs_file[3]
            # Delete the file entry from files_table.
            batcher.AddModify(delete_files_table_cmd, [file_id])
          except OSError:
            logger.error(
                "HandleGarbageCollectRequest: Could not delete file %s.",
                server_file_path)
        else:
          # File does not exist - just delete the file entry from files_table.
          batcher.AddModify(delete_files_table_cmd, [file_id])
      elif file_id == db_file_id:
        # This file is ok, skip to the next db_file_id
        try:
          db_file_id = int(db_file_iter.next())
        except StopIteration:
          db_file_id = sys.maxint
      else:
        # Note: db_files_table's file_id(s) should be a subset of
        # files_table's file_id(s), if we encounter a db_files_table entry
        # which has no files_table entry, we will delete the db_files_table
        # entry. This really shouldn't happen, but who knows if a user pokes
        # around the psql database and deletes something they shouldn't.
        while db_file_id < file_id:
          batcher.AddModify(delete_db_files_table_cmd, [db_file_id])
          try:
            db_file_id = int(db_file_iter.next())
          except StopIteration:
            db_file_id = sys.maxint
    # Close up the sql objects and execute the remaining batch commands.
    batcher.ExecuteRemaining()
    batcher.Close()

    # Finally delete all the dirs if they are empty. We walk the list in the
    # reverse order coz we want to delete the child dirs before we try to
    # delete the parent dirs.
    parent_dirs_list = sorted(parent_dirs_set)
    for parent_dir in reversed(parent_dirs_list):
      try:
        os.rmdir(parent_dir)
        logger.debug("HandleGarbageCollectRequest: remove dir: %s", parent_dir)
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

  def _UnregisterPortable(self, db_id):
    """Unregisters portable globe.

    Clean up push/registration info in postgres database for specified db_id.
    Args:
      db_id: index in gestream.db_table (db_table.db_id).
    """
    # Delete the entries in db_table.
    query_string = "DELETE FROM db_table WHERE db_id = %s"
    self.DbModify(query_string, (db_id,))

    query_string = ("SELECT file_id FROM db_files_table WHERE db_id = %s")
    rs_db_files = self.DbQuery(query_string, (db_id,))

    if rs_db_files:
      assert len(rs_db_files) == 1
      file_id = rs_db_files[0]

      # Delete the entries in db_files_table.
      delete_db_files_table_cmd = (
          "DELETE FROM db_files_table WHERE db_id = %s")
      self.DbModify(delete_db_files_table_cmd, (db_id,))

      # Delete the file entry from files_table.
      delete_files_table_cmd = "DELETE FROM files_table WHERE file_id = %s"
      self.DbModify(delete_files_table_cmd, (file_id,))

  def _UnpublishPortable(self, db_id):
    """Un-publishes portable globe.

    Cleans up publish info in postgres database for specified db_id.
    Args:
      db_id: index in gestream.db_table (db_table.db_id).
    """
    # Check if the database is currently published.
    if self._QueryDbPublished(db_id):
      # Delete the entry in target_db_table.
      query_string = "DELETE FROM target_db_table WHERE db_id = %s"
      self.DbModify(query_string, (db_id,))

  def _QueryDbPublished(self, db_id):
    """Queries whether database is published.

    Args:
      db_id: database ID.
    Returns:
      whether database with specified db_id is published.
    """
    published = False
    query_string = "SELECT target_id FROM target_db_table WHERE db_id = %s"
    result = self.DbQuery(query_string, (db_id,))
    if result:
      published = True
    return published

  def _CheckFileExistence(self, client_host_name, file_paths):
    """Checks for each file's presence in the files_table.

    Args:
      client_host_name: client host name.
      file_paths: list of file paths.
    Returns:
      return a boolean array with an entry of true for the corresponding
      file_paths entries that exist in the files_table or None in case
      the file_paths is empty list.
    """
    logger.debug("CheckFileExistence..")
    if not file_paths:
      return None

    result_array = [False] * len(file_paths)

    # The "IN" call in SQL does not perform well enough to use a single
    # SQL statement here, but
    # in this case N/10 SQL calls using 10 IN is faster than N SQL calls.
    # Note: at 1000, the "IN" operation is unusable and 100 is roughly
    # equivalent to N single SQL queries.
    group_size = 10
    cur_idx = 0
    remaining = len(file_paths)
    groups = int(math.ceil(float(len(file_paths)) / float(group_size)))
    # Loop through the groups of group_size- paths and check their existence.
    for unused_i in xrange(groups):
      count = min(group_size, remaining)
      file_paths_temp = file_paths[cur_idx : cur_idx + count]
      assert file_paths_temp

      # Make a single call to get a Set of file paths.
      # The result only contains entries for those files found.
      result_set = self._CheckFileExistenceBatch(
          client_host_name, file_paths_temp)

      if result_set:
        # Special case to save lookups.
        if len(result_set) == count:
          result_array[cur_idx : cur_idx + count] = itertools.repeat(
              True, count)
        else:
          for j, file_path_j in enumerate(file_paths_temp):
            if file_path_j in result_set:
              result_array[cur_idx + j] = True

      cur_idx += count
      remaining -= count

    return result_array

  def _CheckFileExistenceBatch(self, client_host_name, file_paths):
    """Checks the file existence in a single SQL statement.

    Be sure to keep the number of file paths small as SQL performance
    degrades quickly with the number of arguments to the IN operator.

    Args:
      client_host_name: client host name.
      file_paths: the list of files to check.
    Returns:
      a set of file paths found or None in case the file_paths is empty list.
    """
    logger.debug("_CheckFileExistenceBatch..")
    if not file_paths:
      return None

    # Compile a comma separated list of filenames for the SQL query
    file_list_query = "%s" + ", %s" * (len(file_paths) - 1)

    # Create the SQL query.
    query_string = ("SELECT file_path FROM files_table "
                    "WHERE host_name = %s"
                    " AND file_path IN (") + file_list_query + ")"
    parameters = [client_host_name] + file_paths
    result_set = set(self.DbQuery(query_string, parameters))
    return result_set

  def _QueryFileIds(self, client_host_name, file_paths):
    """Gets file's IDs by file path from files_table.

    Args:
      client_host_name: client host name.
      file_paths: a list of file paths.
    Returns:
      an array with an ID entry for the corresponding file_paths entries in
      the files_table or None in case the file_paths is empty list.
    Raises:
      StreamPushServeException in case the requested file path is not registered
      in the files_table.
    """
    if not file_paths:
      return None

    not_valid_id = 0
    fileid_list = [not_valid_id] * len(file_paths)

    # The "IN" call in SQL does not perform well enough to use a single
    # SQL statement here, but
    # in this case N/10 SQL calls using 10 IN is faster than N SQL calls.
    # Note: at 1000, the "IN" operation is unusable and 100 is roughly
    # equivalent to N single SQL queries.
    group_size = 10
    cur_idx = 0
    remaining = len(file_paths)
    groups = int(math.ceil(float(remaining) / float(group_size)))
    # Loop through the groups of group_size- paths and check their existence.
    for unused_i in xrange(groups):
      count = min(group_size, remaining)
      file_paths_temp = file_paths[cur_idx : cur_idx + count]
      assert file_paths_temp

      # Make a query to get a list of (file_path, file_id) tuples by
      # (hostname, file_path).
      # All files should have been registered in postgres .
      path_id_pairs = self._QueryFileIdsBatch(client_host_name, file_paths_temp)
      if (not path_id_pairs) or (len(path_id_pairs) != len(file_paths_temp)):
        raise exceptions.StreamPushServeException(
            "The publisher (gestream) database is not consistent.\n"
            "Not all Fusion database files are registered in files_table.")

      # Convert the result to a map<file_path, file_id>.
      path2id_map = dict(path_id_pairs)

      for j, file_path_j in enumerate(file_paths_temp):
        assert file_path_j in path2id_map
        fileid_list[cur_idx + j] = path2id_map[file_path_j]

      cur_idx += count
      remaining -= count

    return fileid_list

  def _QueryFileIdsBatch(self, client_host_name, file_paths):
    """Gets file's IDs for a specified array of file paths from files_table.

    Be sure to keep the number of file paths small as SQL performance
    degrades quickly with the number of arguments to the IN operator.

    Args:
      client_host_name: client host name.
      file_paths: the list of files.
    Returns:
      a list of (file path, file ID) pairs for all file paths found or None
      in case the file_paths is empty list.
    """
    if not file_paths:
      return None

    # Compile a comma separated list of filenames for the SQL query.
    file_list_query = "%s" + ", %s" * (len(file_paths) - 1)
    # Create the SQL query.
    query_string = (
        "SELECT file_path, file_id FROM files_table "
        "WHERE host_name = %s AND file_path IN (" + file_list_query + ")")

    parameters = [client_host_name]
    parameters.extend(file_paths)
    path_id_pairs = self.DbQuery(query_string, parameters)

    return path_id_pairs
