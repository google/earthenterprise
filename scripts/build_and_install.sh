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

# This is a simple script that builds and installs Open GEE Fusion and Server.
# It's useful for development.

# You can customize this script by changing VERSION to "release", "optimize",
# or (empty) and by changing CORES to the number of concurrent processes you
# want to run.

VERSION="release"
CORES=8

SCRIPT_DIR=`dirname $0`
BASE_DIR_DOTS="${SCRIPT_DIR}/../earth_enterprise"
BASE_DIR=$(cd "${BASE_DIR_DOTS}"; pwd)

if [ -z "${VERSION}" ]; then
  VER_STR=""
else
  VER_STR="${VERSION}=1"
fi

# Clean up previous builds
rm -rf /tmp/fusion_os_install

# Build
cd "${BASE_DIR}"
scons -j${CORES} ${VER_STR} build || exit
scons -j${CORES} ${VER_STR} stage_install || exit

# Install
cd "${BASE_DIR}/src/installer"
sudo /etc/init.d/gefusion stop
sudo /etc/init.d/geserver stop
sudo ./install_fusion.sh || exit
sudo ./install_server.sh || exit

