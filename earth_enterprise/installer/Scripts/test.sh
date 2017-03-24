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

# This is a test script for testing the various utils shell scripts

# GEE services running test
testresult=`./utils/CheckGEEStopped.sh`
echo -e "$testresult"

# Disk space avail test
testresult=`./utils/CheckDiskSpace.sh 1000000000 1GB`
echo -e "$testresult"

# X11 test
error_title="Error: X11 is not running and is required for this installer."
error_message="    Your X11 DISPLAY variable is not set indicating that X11 is not running.
    X11 is required to run the GUI version of this installer.

    You may try to run the console mode installer instead using:
        sudo ./InstallGEServer.sh"

testresult=`./utils/CheckX11.sh "$error_title" "$error_message"`
if [ -n "$testresult" ]; then
    echo -e "$testresult"
else
    echo X11 Success
fi


testresult=`./utils/CheckHostTypeIs64bit.sh`
if [ -n "$testresult" ]; then
    echo -e "$testresult"
else
    echo Host Type Success
fi


testresult=`./utils/CheckRootUser.sh "test product"`
if [ -n "$testresult" ]; then
    echo -e "$testresult"
else
    echo Root User Success
fi


java_search=`./utils/FindJava.sh`
echo "$java_search"
java_home=`echo "$java_search" | grep Found | sed 's/Found: //'`
if [ -z "$java_home" ]; then
    echo -e "No Java found"
else
    echo "JAVA_HOME=$java_home"
fi

testresult=`./utils/CheckJava.sh $java_home`
if [ -n "$testresult" ]; then
    echo -e "$testresult"
else
    echo 64-bit Java architecture compatibility check completed successfully.
fi

prompt=`./utils/QueryForJavaPrompt.sh "TEST PRODUCT"`
echo "$prompt"
java_home=`./utils/QueryForJava.sh`
if [ -z "$java_home" ]; then
    echo -e "No Java entered"
else
    echo "User entered JAVA_HOME=$java_home"
fi


#Calling the GE Server installer script
#Check for any arguments for silent installation
echo Installing Google Earth Enterprise Fusion using JAVA_HOME=$JAVA_HOME ...

