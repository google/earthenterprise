#!/bin/bash

# Copyright 2017 the Open GEE Contributors.
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

# Unit tests for the key value store used by the RPMs.  The functions that
# start with "test_" are the unit tests.  Other functions are utility
# functions.

# This is a simple script that builds and installs Open GEE Fusion and Server.
# It's useful for development.

# You can customize this script by changing VERSION to "release", "optimize",
# or (empty) and by changing CORES to the number of concurrent processes you
# want to run.

VERSION="release"
CORES=8

if [ -z ${VERSION} ]; then
  VER_STR=""
else
  VER_STR="${VERSION}=1"
fi

rm -rf /tmp/fusion_os_install
scons -j${CORES} ${VER_STR} build || exit
scons -j${CORES} ${VER_STR} stage_install || exit
cd src/installer
sudo /etc/init.d/gefusion stop
sudo /etc/init.d/geserver stop
sudo ./install_fusion.sh || exit
sudo ./install_server.sh || exit
cd ..
