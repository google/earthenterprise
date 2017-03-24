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

# Usage: RunInstaller.sh product_name installer_binary mode
# where mode is one of:
#   gui
#   console
#   silent_file
# This script will run the specified InstallAnywhere binary using
# the specified mode.
product_name=$1
installer_binary=".installer/$2"
mode=$3

if [ $mode == gui ]
then
    command="$installer_binary -i GUI -f /dev/null"
else
    if [ $mode == console ]
    then
        command="$installer_binary -i CONSOLE -f /dev/null"
    else
        command="$installer_binary -i silent -f $mode"
    fi
fi
echo "$command"
$command
command="rm -f .installer/installer.properties"
echo "$command"
$command
