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
#


"""Writes current directory as geecheck.py directory in config file.

This tool can be included in a tarball of tests with a test runner.
It will normally be executed with sudo, and then the tests
will be runnable via EarthServer.
"""

import os

CONFIG_FILE = "/opt/google/geecheck/geecheck.conf"


def main():
  if not os.path.exists(os.path.dirname(CONFIG_FILE)):
    os.makedirs(os.path.dirname(CONFIG_FILE))
  geecheck_dir = os.getcwd()
  fp = open(CONFIG_FILE, "w")
  fp.write(geecheck_dir)
  fp.close()

if __name__ == "__main__":
  main()
