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

"""Batch SQL manager.

Class for managing batch sql command processing.
"""

import logging
import psycopg2  # No lint.

import exceptions

ERR_BATCH_SQL_MANAGER_CLOSED = ("Server-side Internal Error"
                                " - BatchSqlManager has already been closed.")


class BatchSqlManager(object):
  """BatchSqlManager is a utility class to manage batch SQL command processing.

  It allows us to add commands and control the number of commands processed in
  the batch at one time.
  The group of SQL statements being handed to the database as one functional
  group in one transaction instead of commiting each one individually.

  This class basically works as follows
  batcher = BatchSqlManager(db_connection, 1000);
  batcher.AddModify(sql_command_string, parameters) or
  batcher.AddModifySet(sql_command_string, sequence_of_parameters)
  batcher.ExecuteRemaining();
  batcher.Close();

  when 1000 commands have been added, they will be batch executed immediately
  and the "ExecuteRemaining()" call will execute any remaining commands,
  and "Close()" will clean up the connection and sql resources.
  """

  def __init__(self, db_connection, batch_count, logger):
    """Creates a BatchSqlManager for a given database connection object.

    Allow to specify the number of commands which will be batch processed in one
    transaction.

    Args:
      db_connection: the database connection.
      batch_count: the max number of commands processed at any one time.
      logger: a logger object to log errors.
    """
    self.db_connection = db_connection
    self.batch_count = batch_count
    self.logger = logger
    self.cursor = None
    self.count = 0

  def __del__(self):
    self.Close()

  def AddModify(self, command, parameters=None):
    """Adds a Sql modify statement to the batcher.

    After every "batch_count" statements, the current batch will be executed.

    Args:
      command: sql modify command string.
      parameters: parameters that will be bound to variables in the SQL
      statement string.

    Raises:
      exceptions.BatchSqlManagerException: db_connection has been closed.
      psycopg2.Error/Warning
    """
    if not self.db_connection:
      raise exceptions.BatchSqlManagerException(
          ERR_BATCH_SQL_MANAGER_CLOSED)

    try:
      # Create the cursor object if necessary.
      if not self.cursor:
        self.cursor = self.db_connection.Cursor()
        self.count = 0

      self.logger.debug("AddModify...")
      self.logger.debug(self.cursor.mogrify(command, parameters))
      self.cursor.execute(command, parameters)
      self.logger.debug("ok")
      self.count += 1
      if self.count >= self.batch_count:
        self._InternalExecute()

    except (psycopg2.Warning, psycopg2.Error) as e:
      logging.debug(
          "Error while executing Sql command: %s, params: %s\n"
          "Error: %s", command, parameters, e)
      self.db_connection.Rollback()
      self.cursor.close()
      self.cursor = None
      raise

  def AddModifySet(self, command, parameters):
    """Adds set of SQL modify statements to the batcher.

    It creates and executes set of SQL statements based on specified command
    and list of sequence of parameters.
    e.g.
    command = "INSERT INTO db_files_table (db_id, file_id) VALUES(%s, %s)"
    parameters = [(5, 100), (5, 101), (5, 102)]
    result statements:
    INSERT INTO db_files_table (db_id, file_id) VALUES(5, 100)
    INSERT INTO db_files_table (db_id, file_id) VALUES(5, 101)
    INSERT INTO db_files_table (db_id, file_id) VALUES(5, 102)

    After every "batch_count" SQL modify commands, the current batch will be
    executed.

    Args:
      command: sql modify command.
      parameters: list of sequence of parameters. Every sequence of parameters
      will be bound to variables in SQL statement string.

    Raises:
      exceptions.BatchSqlManagerException: db_connection has been closed.
      psycopg2.Error/Warning
    """
    if not self.db_connection:
      raise exceptions.BatchSqlManagerException(
          ERR_BATCH_SQL_MANAGER_CLOSED)

    assert parameters
    try:
      # Create the cursor object if necessary.
      if not self.cursor:
        self.cursor = self.db_connection.Cursor()
        self.count = 0

      self.logger.debug("AddModifySet...")
      self.logger.debug("parameters list length: %d", len(parameters))

      i = 0
      parameters_len = len(parameters)
      while i < parameters_len:
        num_elements_todo = parameters_len - i
        rest_count = self.batch_count - self.count
        if num_elements_todo >= rest_count:
          self.logger.debug("executemany(): command: %s, parameters: %s",
                            command,
                            ", ".join(map(str, parameters[i:i + rest_count])))
          self.cursor.executemany(command, parameters[i:i + rest_count])
          i += rest_count
          self._InternalExecute()
        else:
          self.logger.debug("executemany(): command: %s, parameters: %s",
                            command,
                            ", ".join(map(str, parameters[i:])))
          self.cursor.executemany(command, parameters[i:])
          self.count += num_elements_todo
          break
    except (psycopg2.Warning, psycopg2.Error) as e:
      logging.debug("Error while executing set of Sql commands.\n"
                    "Error: %s", e)
      self.db_connection.Rollback()
      self.cursor.close()
      self.cursor = None
      raise

  def ExecuteRemaining(self):
    """Executes the remaining queries that have been added.

    Raises:
      exceptions.BatchSqlManagerException: db_connection has been closed.
      psycopg2.Error/Warning
    """
    if not self.db_connection:
      raise exceptions.BatchSqlManagerException(
          ERR_BATCH_SQL_MANAGER_CLOSED)

    self.logger.debug("ExecuteRemaining...")
    self._InternalExecute()
    # Release the cursor instance
    if self.cursor:
      self.cursor.close()
      self.cursor = None
    self.logger.debug("ExecuteRemaining done.")

  def Close(self):
    """Executes the batch query and release the references to the cursor object.

    This should be called at the very end of adding all queries to the
    BatchSqlManager (to make sure any remaining queries are executed.

    Raises:
      psycopg2.Error/Warning
    """
    if not self.db_connection:
      if self.count != 0:
        raise exceptions.BatchSqlManagerException(
            "Server-side Internal Error"
            " - db connection has been closed,"
            " while not all Sql statements are committed.")
      return  # already closed.

    self.logger.debug("Close...")
    self.ExecuteRemaining()
    self.logger.debug("Close done.")
    self.db_connection = None
    self.logger = None

  def _InternalExecute(self):
    """Executes the batched statements if any.

    Raises:
      psycopg2.Error/Warning
    """
    if (not self.cursor) or (self.count == 0):
      return

    try:
      self.logger.debug("InternalExecute...")
      self.db_connection.Commit()
      # Start the count over.
      self.count = 0
      self.logger.debug("InternalExecute done.")
    except (psycopg2.Warning, psycopg2.Error) as e:
      logging.debug("Error while executing SQL commit.\nError: %s", e)
      self.db_connection.Rollback()
      self.cursor.close()
      self.cursor = None
      raise
