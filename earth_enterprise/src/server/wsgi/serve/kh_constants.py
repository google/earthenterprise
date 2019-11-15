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

"""kh_constants module.

Module contains constants used in Fusion/Server.
Note: Constants in this module should be synched with
common/khConstants.h
"""

DATABASE_SUFFIX = ".kdatabase"
MAP_DATABASE_SUFFIX = ".kmdatabase"
MERCATOR_MAP_DATABASE_SUFFIX = ".kmmdatabase"

DB_KEY_SUFFIX = ".kda"

MAP_DB_KEY = "mapdb.kda"

# Patterns that are used to match against database path, e.g to check whether
# 2D Fusion Database is Mercator or Flat Map based on database path.
MAP_DATABASE_PATTERN = "{0}/{1}".format(MAP_DATABASE_SUFFIX, MAP_DB_KEY)
MERCATOR_MAP_DATABASE_PATTERN = "{0}/{1}".format(
    MERCATOR_MAP_DATABASE_SUFFIX, MAP_DB_KEY)
