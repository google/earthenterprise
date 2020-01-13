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


"""Utilities for getting config parameters."""


class Configs(object):
  """Class for loading and provide configuration parameters."""

  def __init__(self, config_file, default_values=None):
    """Load config key/value pairs from the config file."""
    if default_values:
      self._config_values = default_values
    else:
      self._config_values = {}
    try:
      fp = open(config_file)
      for line in fp:
        line = line.strip()
        if line and line[0] != "#":
          (key, value) = line.split(" ")
          self._config_values[key] = value
      fp.close()
    except IOError:
      pass
    except:
      pass

  def GetStr(self, key):
    """Returns the value for the given key from the config file."""
    try:
      return self._config_values[key]
    except KeyError:
      return ""

  def GetInt(self, key):
    """Returns the integer value for the given key from the config file."""
    try:
      return int(self._config_values[key])
    except KeyError:
      return 0

  def GetBool(self, key):
    """Returns the boolean value for the given key from the config file."""
    try:
      return "t" == self._config_values[key].lower()[0]
    except KeyError:
      return False
    except IndexError:
      return False
