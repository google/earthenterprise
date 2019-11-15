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

"""Extension for logging.FileHandler."""

import logging
import os


class ExtFileHandler(logging.FileHandler):
  """Extension class for logging.FileHandler.

  Extends logging.FileHandler with functionality of creating log
  file whether it does not exist.
  """

  def __init__(self, filename, mode="a", encoding=None, delay=0):
    """Inits ExtFileHandler."""
    if not os.path.exists(os.path.dirname(filename)):
      os.makedirs(os.path.dirname(filename))

    if not os.path.exists(filename):
      fd = open(filename, "w")
      fd.close()

    logging.FileHandler.__init__(self, filename, mode, encoding, delay)


def main():
  pass


if __name__ == "__main__":
  main()
