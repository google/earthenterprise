Copyright 2017 Google Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
# This directory contains wsgi applications for server:
# common  - common modules for all the applications;
# cut     - cutter modules;
# dashboard - server dashboard modules;
# gee     - GEE publisher modules (old way publisher ported from java);
# publish - publishing modules;
# push    - pushing modules;
#
#
# scripts - scripts to install/reinstall modules (not deployed)
#
# All the modules are installed into /opt/google/gehttpd/wsgi-bin
#
# The following commands can be used to install/reinstall these
# files.
#
# run from directory below
sudo /etc/init.d/geserver stop
sudo ./scripts/updatewsgi.sh
sudo /etc/init.d/geserver start

