#!/usr/bin/python
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

"""Postgres database properties loader."""
import utils


class PostgresProperties(object):
  """Helper class to load and hold the Postgres database properties."""

  def __init__(self):
    """Init PostgresProperties object."""
    self.port_number = utils.GetPostgresPortNumber()
    if not self.port_number:
      self.port_number = "5432"

  def GetPortNumber(self):
    """Get the port number for the Postgres database.

    Returns:
      the port number string.
    """
    return self.port_number
