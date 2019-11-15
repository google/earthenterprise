#!/usr/bin/env python
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


"""Creates json output of apache access or error log."""

import cgi
import json
import os
import sys

# For import from common.
sys.path.insert(
    1, os.path.dirname(os.path.dirname(os.path.realpath(__file__))))

from common import form_wrap
from common import utils

form = form_wrap.FormWrap(cgi.FieldStorage(), do_sanitize=True)


def main():
  # Print blank line.
  print

  try:
    # Choose an apache log to read.
    log_type = form.getvalue("type")
    if log_type == "access":
      log_file = "/opt/google/gehttpd/logs/access_log"
    else:
      log_file = "/opt/google/gehttpd/logs/error_log"

    # Read log file.
    with open(log_file) as f:
      content = f.readlines()

    # Read and print the last 100 lines from log file.
    log_list = []
    num_lines = len(content)

    entry_count = 0
    while entry_count < 100 and num_lines > 0:
      num_lines -= 1
      cur_line = content[num_lines]
      # Check if we have to skip current line (not informative).
      is_access = cur_line.find("apache_logs.py")
      is_apache_fdb = cur_line.find("ApacheFdbReader")
      is_server_asset = cur_line.find("shared_assets")

      # Add log line to the log list.
      if is_access == -1 and is_apache_fdb == -1 and is_server_asset == -1:
        entry_count += 1
        # Strip unnecessary line breaks and HTML-escape log entry.
        log_list.append(utils.HtmlEscape(cur_line.rstrip("\n")))

    print json.dumps(log_list, ensure_ascii=False, indent=0)
  except Exception, e:
    print json.dumps(["ErrorMsg: %s" % e])


if __name__ == "__main__":
  main()
