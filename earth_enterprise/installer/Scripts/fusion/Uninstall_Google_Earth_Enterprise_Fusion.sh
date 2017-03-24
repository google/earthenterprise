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

# This script will work only after the installation of the Google Earth
# Enterprise Fusion.

# Check that the GE Fusion Daemons are no longer running.
test=`ps -e | grep geresourceprovi`

# Check if the logged in user is root
if [ `whoami` != root ]
then
        echo ---------------
        echo No Permission
        echo ---------------
        echo
        echo You must have root privileges to uninstall Google Earth Enterprise Fusion.
        echo Log in as the root user or use the "sudo" command to run this uninstaller.
        echo
        :

elif [ ! -d /opt/google/install/GoogleEarthFusion_Uninstaller/ ]
then

echo Google Earth Enterprise Fusion FUSION_VERSION_NUMBER is not installed on this system.

elif [ "$test" != "" ]
then

  echo The Google Earth Enterprise Fusion Daemons are running.
  echo Please stop the daemones using the command:
  echo "   /etc/init.d/gefusion stop"
  echo Then restart the uninstaller.

else

# Start the uninstaller
/opt/google/install/GoogleEarthFusion_Uninstaller/Uninstall_Google_Earth_Enterprise_Fusion_FUSION_VERSION_NUMBER -i CONSOLE

fi
