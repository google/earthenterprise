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


"""Runs geecheck.py and outputs json result to browser.

Geecheck.py is looked for inside of the directory specified in
the config file (../conf/geecheck.conf).
Expects results from geecheck.py to be a json string,
which it then echoes to the browser.
"""

import json
import os
import common.utils

CONFIG_DIRECTORY = "/opt/google/geecheck/geecheck.conf"
GEE_CHECK_APP = "./geecheck.py"


def GeeCheckNotRun(msg=""):
  """If geecheck was not found, output dict with error msg."""
  # Output empty json.
  msg = "Unable to run geecheck.py. %s" % msg
  print json.dumps({"error": msg})
  raise Exception(msg)


def main():
  common.utils.WriteHeader("application/json")
  try:
    fp = open(CONFIG_DIRECTORY)
    geecheck_dir = fp.readline().strip()
    fp.close()
    os.chdir(geecheck_dir)
    results = common.utils.RunCmd(GEE_CHECK_APP)
    # Results returned as list. Print 0th element (json) for web requests.
    if "REQUEST_METHOD" in os.environ:
      print results[0]
    else:
      print results
  except Exception as e:
    GeeCheckNotRun(e.__str__())

if __name__ == "__main__":
  main()
