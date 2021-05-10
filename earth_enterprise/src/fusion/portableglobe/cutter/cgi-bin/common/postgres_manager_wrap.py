#!/usr/bin/env python3.8
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

"""The postgres manager wrapper module.

Class wraps postgres manager from wsgi-bin to use in cutter.
TODO: it is a temporary solution. Make a refactoring - consider
to put postgres_manager in common place.
"""

import logging
import os
import sys
import importlib.util

'''
In order to import code from wsgi-bin/common, we need to circumvent the normal import process. Python 3 does not like relative imports, 
especially not those that overlap names with other in-use packages. In order to get around this issue, we import the module manually
under a different name (in this case wsgi_common) so that there is no overlap.
'''
wsgi_common_spec = importlib.util.spec_from_file_location('wsgi_common', os.path.join(os.path.dirname(os.path.realpath(__file__)), 
                                                                                      '../../wsgi-bin/common/__init__.pyc'))
wsgi_common_module = importlib.util.module_from_spec(wsgi_common_spec)
sys.modules[wsgi_common_spec.name] = wsgi_common_module
wsgi_common_spec.loader.exec_module(wsgi_common_module)

from wsgi_common import postgres_manager
from wsgi_common import postgres_properties

class PostgresManagerWrap(object):
  DB_PORT = postgres_properties.PostgresProperties().GetPortNumber()
  DB_USER = "geuser"

  @staticmethod
  def Query(db, query, parameters=None):
    """Submits the query to the database and returns tuples.

    Args:
      db: The database being queried.
      query: SQL SELECT statement.
      parameters: sequence of parameters to populate placeholders in SQL
                  statement.
    Returns:
      Results as list of lists (rows of fields).
    """
    db_con = postgres_manager.PostgresConnection(
        db,
        PostgresManagerWrap.DB_USER,
        '/tmp',
        PostgresManagerWrap.DB_PORT,
        logging.getLogger())
    results = db_con.Query(query, parameters)
    db_con.Close()

    return results
