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
# Google Earth Enterprise Fusion
# Usage: InstallGEServer[GUI].sh [silent_installer_file]
# an argument to this script is interpretted as the silent installer file.
# Note: this line and the X11 test are the only differences between InstallGEServer.sh
# and InstallGEServerGUI.sh. Server does not require X11,
# but the GUI installer does.
mode=console

installer_binary=GEServer.bin
product_name="Google Earth Enterprise Server"

# Check that all GEE Services are stopped.
testresult=`./.installer/CheckGEEStopped.sh`
if [ -n "$testresult" ]; then
    echo -e "$testresult"
    exit # can't continue
fi

testresult=`./.installer/CheckDiskSpace.sh 1000000 1GB`
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
if [ $1 ]
then
    mode=$1
fi
echo "Installing $product_name"
./.installer/RunInstaller.sh "$product_name" "$installer_binary" "$mode"
