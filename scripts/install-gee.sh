#! /bin/bash

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

# Installs Open GEE.  You must have built Open GEE previously.  Accepts two
# paramters: the build type (release, optimize, or debug - defaults to release)
# and the number of processes to run (defaults to twice the number of cores).

source common-build-install.sh

# Copy binaries to install package location:
cd "${BASE_DIR}" && \
scons -j${PROCS} "${BUILD_TYPE}" stage_install || exit 1

# Install Fusion:
cd "${BASE_DIR}/src/installer" &&
(echo C; echo C) | sudo ./install_fusion.sh -hnmf || exit 1

# Install Earth Server:
(echo n) | sudo ./install_server.sh -hnmf || exit 1

# Reload PATH:
source /etc/profile || exit 1
