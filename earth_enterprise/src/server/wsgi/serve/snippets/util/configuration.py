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

"""Configuration module."""

# Enum's values separator.
WIDGET_ENUM_VALUE_SEPARATOR = ","
# Any MANGLING that we do must result in a valid HTML id.
# The fields in the hardmasking (hardmask-template.txt) are mangled,
# so those will have to be changed too if you change this.
# Some code in field_monkeying.py assumes this is a single char.
MANGLING_MARKER = ":"

# Not really user-configurable, but, nowhere else for globals.
# Specific types for parsing.
HONORARY_PRIMITIVES = set(["StringIdOrValueProto"])


def main():
  pass


if __name__ == "__main__":
  main()
