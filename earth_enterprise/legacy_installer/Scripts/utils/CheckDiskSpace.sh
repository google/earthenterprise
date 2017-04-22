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

# This script prints a warning message if the minimum disk space is not present
# on /tmp
# Usage: CheckDiskUsage.sh  min_space_kbytes min_space_printable

# First Check for at least 1GB in /tmp, required for installation.
tail=`df /tmp | tail -1`
for i in $tail; do
  if [ `expr index "$i" "%"` == 0 ]; then avail=$i; else break; fi
done
echo "Installer disk space check found $avail KB on /tmp"
message="Disk space check: OK"
if [ $((avail)) -lt $1 ]
then
    message="---------------
Warning: You may not have enough disk space to use this installer.
---------------

    \"df /tmp\" shows less than $2 of available disk space.
    $2 is recommended to run this installer.
    Install will proceed, but if the installation fails you may need
    to free up space in /tmp"
fi
echo "$message"
