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
# Simply run this script after building and it will install wsgi modules under
# the /opt/google/gehttpd/wsg-bin/.
#
# Note: to build python modules run 'python2.7 /usr/bin/scons -j8 optimize=1 build_py'

set -x
set -e

gehttpd_dir="/opt/google/gehttpd"

# deploy logging config
cp ./conf/ge_logging.conf $gehttpd_dir/conf/
chmod 644 $gehttpd_dir/conf/ge_logging.conf

# deploy modules
deploy_dir=$gehttpd_dir/wsgi-bin

if [ ! -d $deploy_dir ]
then
  mkdir -p $deploy_dir
  chmod 755 $deploy_dir
fi

modules="common conf cut dashboard fusion search serve wms"
for module in $modules
do
#  rm -rf $deploy_dir/$module
  cp -rf ./$module $deploy_dir
  chmod -R 755 $deploy_dir/$module
done

