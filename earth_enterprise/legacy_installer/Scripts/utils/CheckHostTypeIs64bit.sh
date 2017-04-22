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

# This script prints an error message returns -1 if the OS is not 64 bit.

if [ $HOSTTYPE != x86_64 -a $HOSTTYPE != x86_64-linux ]
then
    error_message="---------------
Error: Attempt to install on 32 bit operating system.
---------------
   Google Earth Enterprise Fusion is now installable only on 64 bit systems.
   Your HOSTTYPE environment variable is : $HOSTTYPE
   The installer expects HOSTTYPE=x86_64 for installation."
    echo -e "$error_message"
    exit -1
fi
