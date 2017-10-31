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
# It's useful for development.  It  accepts two paramters: the build type
# (release, optimize, or debug - defaults to optimize) and the number of
# processes to run (defaults to twice the number of cores).

# This sets a different default than build-gee.sh and install-gee.sh
BUILD_TYPE=${1:-"optimize"}

PROCS=$2
SCRIPT_DIR=`dirname "$0"`

# Clean up previous builds
rm -rf /tmp/fusion_os_install

# Build
"${SCRIPT_DIR}/build-gee.sh" "${BUILD_TYPE}" "${PROCS}" || exit

# Install
sudo /etc/init.d/gefusion stop
sudo /etc/init.d/geserver stop
"${SCRIPT_DIR}/install-gee.sh" "${BUILD_TYPE}" "${PROCS}" || exit

