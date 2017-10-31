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

# Common code shared buy the build and install scripts

# Default build type is release
BUILD_TYPE=${1:-'release'}
if [ "$BUILD_TYPE" == "debug" ]; then
  BUILD_TYPE=""
else
  BUILD_TYPE="${BUILD_TYPE}=1"
fi

# Default number of processes is twice the number of cores
PROCS=${2:-$[ 2 * $(nproc) ]}

# Find the earth_enterprise directory
REL_SCRIPT_DIR=`dirname "$0"`
SCRIPT_DIR=$(cd "${REL_SCRIPT_DIR}"; pwd)
if [ `basename "${SCRIPT_DIR}"` = "scripts" ]; then
  # User running from development environment
  BASE_DIR=$(cd "${SCRIPT_DIR}/../earth_enterprise"; pwd)
else
  # Building docker image
  BASE_DIR="earthenterprise/earth_enterprise"
fi
unset REL_SCRIPT_DIR

