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

"""Postgres connection.

Classes for interacting with PostgreSQL database.
"""

from contextlib import contextmanager
import threading

import psycopg2
from psycopg2.pool import ThreadedConnectionPool


def _ModifyThreaded(semaphore, connection_pool, sql, logger):
  with semaphore:
    semaphore.acquire()
    name = threading.currentThread().getName()
    logger.info("Thread name: %s", name)
    connection = connection_pool.getconn()
    cursor = connection.cursor()
    try:
      logger.info("Thread executing: %s", sql)
      cursor.execute(sql)
    except psycopg2.ProgrammingError as err:
      logger.error("Execute Error: %s", err)
    connection.commit()
    connection_pool.putconn(connection)
    semaphore.release()


class PostgresConnection(object):
  """Class for interacting with PostgreSQL database.

  It encapsulates psycopg2.connection object.
  This class basically works as follows:
  stream_db_con = PostgresConnection(dbname, username, host, port, pass, logger)
  stream_db_con.Query(query, params)
  stream_db_con.Modify(query, params)
  stream_db_con.Close()

  But also it has API Cursor(), Commit() that can be used to get
  psycopg2.cursor object to work with.
  stream_db_con = PostgresConnection(dbname, username, host, port, pass, logger)
  cursor = stream_db_conn.Cursor()
  cursor.execute(query, params)
  stream_db_con.Rollback()/stream_db_con.Commit()
  cursor.close()
  stream_db_con.Close()
  """
  MIN_THREADS = 2
  MAX_THREADS = 4

  def __init__(self, dbname, username, host, port, password, logger):
    """Inits PostgresConnection.

    Args:
      dbname: The database being queried.
      username: User name.
      host: Hostname of the machine on which server is running.
      port: Port on which server is listenning for connection.
      password: Password for remote geuser role
      logger: logger instance
    """
    self._database = dbname
    self._username = username
    self._host = host
    self._port = port
    self._pass = password
    self._logger = logger
    self._connection = None
    self._connection_pool = None
    self._Connect()

  def __del__(self):
    self.Close()

  def Close(self):
    """Closes communication with database.
    """
    if self._connection:
      self._connection.rollback()
      self._connection.close()
      self._connection = None
      self._connection_pool = None

  def Query(self, query, parameters=None):
    """Submits the query to the database and returns tuples.

    Note: variables placeholder must always be %s in query.
    Warning: NEVER use Python string concatenation (+) or string parameters
    interpolation (%) to pass variables to a SQL query string.
    Psycopg automatically convert Python objects to and from SQL literals:
    using this feature your code will be more robust and reliable.
    The correct way to pass variables in a SQL command is using the second
    argument of the cursor.execute() method.
    e.g.
    SELECT vs_url FROM vs_table WHERE vs_name = 'default_ge';
    query = "SELECT vs_url FROM vs_table WHERE vs_name = %s"
    parameters = ["default_ge"]

    Args:
      query: SQL SELECT statement.
      parameters: sequence of parameters to populate into placeholders.
    Returns:
      Results as list of tuples (rows of fields).
    Raises:
      psycopg2.Error/Warning in case of error.
    """
    self._logger.debug("Query..")
    assert self._connection
    results = []

    with self._Cursor() as cursor:
      self._logger.debug(cursor.mogrify(query, parameters))
      try:
        cursor.execute(query, parameters)
      except psycopg2.ProgrammingError as exc:
        print (exc.message)
        self._logger.error(exc.message)
        # conn.rollback()
      except psycopg2.InterfaceError as exc:
        print (exc.message)
        # conn = psycopg2.connect(...)
        # cursor = conn.cursor()

      self._logger.debug("ok: number of rows: %s", cursor.rowcount)
      for row in cursor:
        if len(row) == 1:
          results.append(row[0])
        else:
          results.append(row)

    return results

  def Modify(self, command, parameters=None, returning=False):
    """Submits the INSERT/UPDATE/DELETE sql command to the database.

    Note: variables placeholder must always be %s in SQL command string.
    Warning: NEVER use Python string concatenation (+) or string parameters
    interpolation (%) to pass variables to a SQL command string.
    Psycopg automatically convert Python objects to and from SQL literals:
    using this feature your code will be more robust and reliable.
    The correct way to pass variables in a SQL command is using the second
    argument of the cursor.execute() method.
    e.g.
    INSERT INTO db_table (host_name, db_name, db_pretty_name)
           VALUES('localhost', 'sf_road', 'pretty_sf_road');
    query = ("INSERT INTO db_table (host_name, db_name, db_pretty_name) "
             "VALUES(%s, %s, %s)")
    parameters = ["localhost", "sf_road", "pretty_sf_road"]

    returning: if True, then return query result
      (expecting that it is one row), otherwise - number of rows modified.

    DELETE FROM db_table WHERE db_id='20';
    query = "DELETE FROM db_table WHERE db_id=%s"
    parameters = [20]

    Args:
      command: SQL UPDATE/INSERT/DELETE command string.
      parameters: sequence of parameters to populate into placeholders.
      returning: if True, then return query result
        (expecting that it is one row), otherwise - number of rows modified.
    Returns:
      query result (expecting that it is one row).
    Raises:
      psycopg2.Error/Warning in case of error.
    """
    self._logger.debug("Modify..")
    assert self._connection
    ret_val = 0

    with self._Cursor() as cursor:
      self._logger.debug(cursor.mogrify(command, parameters))
      cursor.execute(command, parameters)
      if not returning:
        ret_val = cursor.rowcount
        self._logger.debug("Affected rows: %s" % ret_val)
      else:
        ret_val = cursor.fetchone()
        self._logger.debug("Query result: %s" % ret_val)

    return ret_val

  def Cursor(self):
    """Opens and returns a cursor to perform PostgreSQL command.

    Returns:
      psycopg2.cursor instance.
    """
    assert self._connection
    return self._connection.cursor()

  def Rollback(self):
    """Roll back any pending transaction.

    Raises:
      psycopg2.Error/Warning in case of error.
    """
    assert self._connection
    self._logger.debug("Rollback..")
    self._connection.rollback()
    self._logger.debug("ok")

  def Commit(self):
    """Commits pending transaction to the database.

    Raises:
      psycopg2.Error/Warning in case of error.
    """
    assert self._connection
    self._logger.debug("Commit..")
    self._connection.commit()
    self._logger.debug("ok")

  def ExecuteOutOfTransaction(self, command):
    """Executes specified command out of transaction block.

    Args:
      command: SQL statement to execute.
    """
    # Store current isolation_level to restore then.
    old_isolation_level = self._connection.isolation_level
    # Set isolation level to execute statement out of transaction block.
    self._connection.set_isolation_level(0)
    # Execute statement.
    self.Modify(command)
    self.Commit()
    # Restore isolation level.
    self._connection.set_isolation_level(old_isolation_level)

  def CopyFromFile(self, file_handle, table_name):
    self.Cursor().copy_from(file_handle, table_name)
    self.Commit()

  def ExecuteThreaded(self, statements):
    """ExecuteThreaded runs sql statements each in its own thread.

    Operation: Fire up up to MAX_THREADS and queue the rest. The connection
      pool object uses lazy initialization, and will be set to None when the
      connection is closed. It is always initialized to MAX_THREADS.
    Limitations: Function will block until all threads complete. This means
      that the function cannot be executed concurrently, the assumption is
      that the process runs in only one thread, or at least that it won't
      attempt to run this function concurrently.

    Args:
      statements: must be a list of strings with two or more items.
    """
    self._logger.debug("ExecuteThreaded started.")
    if not self._connection_pool:
      self._connection_pool = ThreadedConnectionPool(
          PostgresConnection.MIN_THREADS,
          PostgresConnection.MAX_THREADS,
          self._connection.dsn)
    semaphore = threading.BoundedSemaphore(value=PostgresConnection.MAX_THREADS)
    for idx, sql in enumerate(statements):
      thread = threading.Thread(target=_ModifyThreaded, name="Thread%s" % idx,
                                args=(semaphore,
                                      self._connection_pool,
                                      sql,
                                      self._logger))
      thread.setDaemon(0)
      thread.start()
    self._logger.debug("ExecuteTheaded completed.")

  def _Connect(self):
    """Creates a database connection.

    Raises:
      psycopg2.Error/Warning in case of error.
    """
    if not self._connection:
      self._connection = psycopg2.connect(dbname=self._database,
                                          user=self._username,
                                          host=self._host,
                                          password=self._pass,
                                          port=self._port)

  @contextmanager
  def _Cursor(self):
    """Opens a cursor to perform PostgreSQL command in database session.

    Yields:
      context manager that will be bound to cursor object.

    Raises:
      psycopg2.Error/Warning in case of error.
    """
    assert self._connection

    cursor = self._connection.cursor()
    try:
      yield cursor
    except psycopg2.Warning as w:
      self._connection.rollback()
      raise w
    except psycopg2.Error as e:
      self._connection.rollback()
      raise e
    else:
      self._connection.commit()
    finally:
      cursor.close()
