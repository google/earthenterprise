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

"""Utilities for proto dbroot."""

from serve.snippets.util import dbroot_v2_pb2


def MakeEmptyDbroot():
  """Creates empty proto dbroot.

  We're just keeping other modules from having to include <dbroot_v2_pb2>.
  Returns:
    Default proto dbroot.
  """
  return dbroot_v2_pb2.DbRootProto()


def main():
  pass


if __name__ == "__main__":
  main()
