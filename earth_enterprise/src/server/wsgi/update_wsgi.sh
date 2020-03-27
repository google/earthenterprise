#!/bin/bash -eu
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

# TODO: High-level file comment.
#!/bin/bash
#
# This is an utility script to help in testing/debugging WSGI modules.
# Simply run this script after building, and it will install WSGI modules and
# their dependencies under the /opt/google/gehttpd/wsg-bin/.
#
# ./update_bin -h|-help|--help reports usage.
#
# Note: Needs to be run with "sudo".
# Note: To build python modules, run 'python2.7 /usr/bin/scons -j8 optimize=1 build_py'

set -e

CHANGED_MIN_AGO_DEFAULT=-60

cmin_val=$CHANGED_MIN_AGO_DEFAULT

usage_print=0
case $1 in
  -h|-help|--help) usage_print=1;;
  *) case $# in
       0) ;;
       1) cmin_val=$1;;
       *) usage_print=1;;
     esac
esac

case $usage_print in
  1)
  cmd=`basename $0`
  echo "Usage   : $cmd [last_changed_minutes_ago]"
  echo "Example :"
  echo "  $cmd"
  echo "    # by default, updates binaries that have been modified not later"
  echo "    # than $CHANGED_MIN_AGO_DEFAULT minutes ago."
  echo "  $cmd -30"
  echo "    # updates binaries that have been modified not later than"
  echo "    # -30 minutes ago."
  echo "Note: Needs to be run with 'sudo'"
  exit 0
  ;;
esac

set -x

# Stop Fusion/Server daemons.
/etc/init.d/gefusion stop
/etc/init.d/geserver stop

# Copy WSGI python scripts.
./scripts/updatewsgi.sh

# Copy dependency modules.
cd ../../OPT-x86_64
find ./lib/libgepublishmanagerhelper.so -cmin $cmin_val -exec cp -v "{}" /opt/google/gehttpd/wsgi-bin/serve/publish/_libgepublishmanagerhelper.so \;
find ./fusion/gepublish/libgepublishmanagerhelper.py -cmin $cmin_val -exec cp -v "{}" /opt/google/gehttpd/wsgi-bin/serve/publish/ \;
cd -

# Start Fusion/Server daemons.
/etc/init.d/gefusion start
/etc/init.d/geserver start

