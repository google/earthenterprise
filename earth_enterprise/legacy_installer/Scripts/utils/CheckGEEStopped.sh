#!/bin/bash
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

# This script prints an error message returns -1  if any of the 
# Google Earth Enterprise services are detected to be running.
# Usage: CheckGEEStopped.sh

# Need 2 tests to be safe due to differences in "ps" on the distros
# we support.  First, try to catch anything referring to /opt/google
# such as java or postgresql.
found_process=""
ignored_google_processes="Chrome|GoogleTalk|appengine"
egrep -lvi "$ignored_google_processes" /proc/[1-9]*/cmdline | cut -f3 -d '/' | xargs ps -p | grep -v 'grep' | grep -q '/opt/google'
if [ $? -eq 0 ];
then
    # Found processes that use /opt/google in their command line
    found_process=true
else
    ls -l /proc/*/exe 2>/dev/null  | egrep -vi "$ignored_google_processes" | grep -q '/opt/google'
    if [ $? -eq 0 ];
    then
      # Found processes that use executables from /opt/google
      found_process=true
    fi
fi

# If we found anything, exit with an informative message and return status -1.
if [ -n "$found_process" ]
then
  cat <<END_PARA
---------------
Google Earth Enterprise services must be stopped before installation.
---------------

When installing Google Earth Enterprise software, all GEE services must
be stopped. Use:
  sudo /etc/init.d/gefusion stop
  sudo /etc/init.d/geserver stop

If you have stopped the servers as shown above and you still receive this
message, then you may need to manually stop all processes running from
/opt/google/...
This script detects the following processes running:

END_PARA

  (ls -1 `grep -l "/opt/google" /proc/[1-9]*/cmdline` 2>/dev/null | cut -f3 -d'/'; ls -l /proc/*/exe 2>/dev/null | egrep -vi "$ignored_google_processes" | grep '/opt/google' | cut -f3 -d'/') | xargs ps -p | egrep -vi "$ignored_google_processes"

  exit -1
fi
