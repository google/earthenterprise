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

"""The stream_manager module.

Classes for working with stream data.
"""

import logging
import os

from common import batch_sql_manager
from common import exceptions
from common import postgres_manager
from common import postgres_properties
from common import utils
from serve import constants
from serve import http_io
from serve import serve_utils

logger = logging.getLogger("ge_stream_publisher")


class StreamManager(object):
  """Class for managing stream data.

  Common functionality used in Publish/StreamPush Managers.
  """
  # TODO: decide whether these will be read from some config?
  DAV_CONF_PATH = "/opt/google/gehttpd/conf.d/stream_space"

  def __init__(self):
    """Inits stream manager."""
    # Note: added as a good practice since we use StreamManager as a base class.
    super(StreamManager, self).__init__()
    self.server_prefix = ""
    self._ReadDavConfig()

    # Get server host name (FQDN) - get once to use everywhere.
    self.server_hostname = utils.GetServerHost()

    # Init PostgresManager.
    self._database = "gestream"
    self._db_user = "geuser"
    postgres_prop = postgres_properties.PostgresProperties()
    self._port = postgres_prop.GetPortNumber()
    self._host = postgres_prop.GetHost()
    self._pass = postgres_prop.GetPassword()
    self.stream_db = postgres_manager.PostgresConnection(
        self._database, self._db_user, self._host, self._port, self._pass, logger)

  def DbQuery(self, query_string, parameters=None):
    """Handles DB query request.

    Args:
      query_string: SQL query statement.
      parameters: sequence of parameters to populate into placeholders.
    Returns:
      Results as list of tuples (rows of fields).
    Raises:
      psycopg2.Error/Warning in case of error.
    """
    return self.stream_db.Query(query_string, parameters)

  def DbModify(self, command_string, parameters=None, returning=False):
    """Handles DB modify request.

    Args:
      command_string: SQL UPDATE/INSERT/DELETE command string.
      parameters: sequence of parameters to populate into placeholders.
      returning: if True, then return query result
        (expecting that it is one row), otherwise - number of rows modified.
    Returns:
      Number of rows or query result depending on returning argument.
    Raises:
      psycopg2.Error/Warning in case of error.
    """
    return self.stream_db.Modify(command_string, parameters, returning)

  def HandlePingRequest(self, request, response):
    """Handles ping server request.

    Args:
      request: request object.
      response: response object
    Raises:
      psycopg2.Error/Warning,
    """
    cmd = request.GetParameter(constants.CMD)
    assert cmd == "Ping"

    # Fire off a simple query to make sure we have a valid db connection.
    query_string = "SELECT 'ping'"
    results = self.DbQuery(query_string)
    if results and results[0] == "ping":
      http_io.ResponseWriter.AddBodyElement(response, constants.HDR_STATUS_CODE,
                                            constants.STATUS_SUCCESS)
    else:
      http_io.ResponseWriter.AddBodyElement(
          response, constants.HDR_STATUS_MESSAGE,
          "Cannot ping gestream database.")
      http_io.ResponseWriter.AddBodyElement(response, constants.HDR_STATUS_CODE,
                                            constants.STATUS_FAILURE)

  def GetDbPublishPathPrefix(self, db_type, fusion_hostname):
    """Gets publish(push) path prefix for Fusion/Portable database.

    Args:
      db_type: database type.
      fusion_hostname: Fusion hostname.
    Returns:
      database publish path prefix which is
        for Fusion database - server_prefix/fusion_hostname, e.g
           /gevol/published_dbs/stream_space/fusion_host_name
        for Portable database - empty string.
    """
    if serve_utils.IsFusionDb(db_type):
      return self.GetFusionDbPublishPathPrefix(fusion_hostname)
    elif serve_utils.IsPortable(db_type):
      return ""
    else:
      raise exceptions.StreamPushServeException(
          "HandleSyncRequest: Unsupported DB type %s.", db_type)

  def GetFusionDbPublishPathPrefix(self, fusion_hostname):
    """Gets publish(push) path prefix for Fusion database.

    Args:
      fusion_hostname: Fusion hostname.
    Returns:
      database publish path prefix which is 'server_prefix/fusion_hostname', e.g
           /gevol/published_dbs/stream_space/fusion_host_name
    """
    return os.path.join(self.server_prefix, fusion_hostname)

  def _ReadDavConfig(self):
    """Reads WebDAV config (server_prefix)."""
    sub_groups = utils.MatchPattern(StreamManager.DAV_CONF_PATH,
                                    "^Alias\\s+/stream_space\\s+(.*)")

    if sub_groups:
      self.server_prefix = sub_groups[0]

    if not self.server_prefix:
      raise exceptions.PublishServeException(
          "Unable to read WebDAV Config.")
    logger.info("Server Prefix: %s", self.server_prefix)

  def QueryDbId(self, client_host_name, db_name):
    """Queries db_table.db_id from gestream database.

    The pair client_host_name:db_name is unique identifier for Fusion database.
    Args:
      client_host_name: Fusion client host name.
      db_name: database name.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      db_table.db_id from gestream database.
      If corresponding Fusion database is not registered, it returns 0.
    """
    db_id = 0
    query_string = ("SELECT db_id FROM db_table "
                    "WHERE host_name = %s AND db_name = %s")
    result = self.DbQuery(query_string, (client_host_name, db_name))
    if result:
      assert isinstance(result, list) and len(result) == 1
      assert isinstance(result[0], int)
      db_id = result[0]
    return db_id

  def QueryDbIdAndFlags(self, client_host_name, db_name):
    """Queries db_table.db_id and db_table.db_flags from gestream database.

    The pair client_host_name:db_name is unique identifier for Fusion database.
    Args:
      client_host_name: Fusion client host name.
      db_name: database name.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      tuple (db_table.db_id, db_table.db_flags) from gestream database.
      If corresponding Fusion database is not registered, it returns
      (0, 0)-tuple.
    """
    db_id = 0
    db_flags = 0
    query_string = ("SELECT db_id, db_flags FROM db_table "
                    "WHERE host_name = %s AND db_name = %s")
    result = self.DbQuery(query_string, (client_host_name, db_name))
    if result:
      assert isinstance(result, list) and len(result) == 1
      assert isinstance(result[0], tuple)
      db_id, db_flags = result[0]
    return db_id, db_flags

  def QueryVh(self, vh_name):
    """Queries virtual host info by name.

    Args:
      vh_name: virtual host hame.
    Raises:
      psycopg2.Error/Warning,  PublishServeException.
      PublishServeException is raised in case there is no record
      with specified vh_name in virtual_host_table.
    Returns:
      virtual host info tuple (vh_url, vh_ssl).
    """
    query_string = (
        "SELECT virtual_host_url, virtual_host_ssl"
        " FROM virtual_host_table"
        " WHERE virtual_host_name = %s")
    results = self.DbQuery(query_string, (vh_name,))

    if not results:
      raise exceptions.PublishServeException(
          "Couldn't get Virtual Host info for %s.", vh_name)

    assert len(results) == 1
    return results[0]

  def QueryVhList(self):
    """Queries Virtual Host list.

    Returns:
      list of tuples (vh_name, vh_url, vh_ssl).
    """
    query_string = (
        "SELECT virtual_host_name, virtual_host_url, virtual_host_ssl"
        " FROM virtual_host_table")
    vh_list = self.DbQuery(query_string)
    return vh_list

  def SynchronizeDb(self, db_id, db_type, fusion_hostname):
    """Synchronizes database files.

    Args:
      db_id: database ID.
      db_type: database type.
      fusion_hostname: Fusion hostname.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      list of files that need to be transferred.
    """
    logger.debug("SynchronizeDb...")

    file_good = 1
    query_string = ("SELECT file_path, file_size FROM files_table, "
                    "db_files_table WHERE file_status != %s"
                    " AND files_table.file_id = db_files_table.file_id AND "
                    "db_files_table.db_id = %s")

    files_to_update = self.DbQuery(query_string, (file_good, db_id))
    files_to_transfer = self._UpdateFileStatus(
        files_to_update, db_type, fusion_hostname)

    logger.debug("SynchronizeDb done.")
    return files_to_transfer

  def _UpdateFileStatus(self, file_path_size_list, db_type, fusion_hostname):
    """Updates the status for files in files_table.

    Fixes the status for any files that are really pushed but have an invalid
    status value in postgres table.
    Returns the list of files that need to be transferred still.

    Args:
      file_path_size_list: the list of files to check (item is (path, size)
                           tuple)
      db_type: database type.
      fusion_hostname: Fusion hostname.
    Raises:
      psycopg2.Error/Warning.
    Returns:
      files_to_transfer: the list of files (paths) that need to be transfered.
    """
    logger.debug("_UpdateFileStatus..")
    db_path_prefix = self.GetDbPublishPathPrefix(db_type, fusion_hostname)
    logger.debug("db path prefix: %s", db_path_prefix)

    batcher = batch_sql_manager.BatchSqlManager(
        self.stream_db, 1000, logger)
    query_string = ("UPDATE files_table SET file_status = 1"
                    " WHERE host_name = %s AND file_path = %s")

    files_to_transfer = []
    for (file_path, file_size) in file_path_size_list:
      file_complete_path = os.path.normpath(db_path_prefix + file_path)
      if (os.path.exists(file_complete_path) and
          serve_utils.FileSizeMatched(file_complete_path, file_size)):
        batcher.AddModify(query_string, (fusion_hostname, file_path))
      else:
        files_to_transfer.append(file_path)

    batcher.ExecuteRemaining()
    batcher.Close()
    logger.debug("_UpdateFileStatus done.")
    return files_to_transfer


def main():
  pass


if __name__ == "__main__":
  main()
