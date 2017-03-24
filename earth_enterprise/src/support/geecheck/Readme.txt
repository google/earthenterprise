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

geecheck.pl is a script for diagnosing installation issues for Google Earth Fusion version 3.0.0 and higher.
It simply reports configuration and potential issues without modifying the system.

It does the following checks:
* File Permissions (including Fusion's asset root, published dbs, and src volume)
* Package versions
* Hostname
* Umask

It also dumps:
* Fusion Logs (including error logs)
* Fusion config files (volumes.xml, systemrc)
* Environment Variables
* CPU Info
* Disk info for the various fusion directories
* other configuration variables

Customer Usage:
1. Google support will send the latest geecheck.tar.gz to them via email:
2. They then need to copy it to /tmp,
3. Run the following commands in the terminal
  cd /tmp
  tar xvzf geecheck.tar.gz
  cd geecheck
  ./geecheck.pl -version=3.0.1 -all > /tmp/geecheck-results-YYYY-MM-DD-CUSTOMERNAME.txt

For usage info run:
  ./geecheck.pl -help
