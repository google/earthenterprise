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

# This script runs basic tests for installation then runs the
# installer for
# Google Earth Enterprise Fusion Internal Tools

installer_binary=GEFTools.bin
product_name="Google Earth Enterprise Fusion Internal Tools"

testresult=`./.installer/CheckDiskSpace.sh 100000 100MB`
echo -e "$testresult"

testresult=`./.installer/CheckHostTypeIs64bit.sh`
if [ -n "$testresult" ]; then
    echo -e "$testresult"
    exit # can't continue
fi

testresult=`./.installer/CheckRootUser.sh "$product_name"`
if [ -n "$testresult" ]; then
    echo -e "$testresult"
    exit # can't continue
fi

# Run the installer
mode=console
if [ $1 ]
then
    mode=$1
fi
echo "Installing $product_name"
./.installer/RunInstaller.sh "$product_name" "$installer_binary" "$mode"
