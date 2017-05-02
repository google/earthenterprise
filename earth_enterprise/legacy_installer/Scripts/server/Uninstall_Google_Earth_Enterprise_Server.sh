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
# Enterprise Server.
# This script will call the uninstaller script for Google Earth Enterprise
# Server located at /opt/google/install/GoogleEarthServer_Uninstaller
# in GUI mode

ignored_google_processes="Chrome|GoogleTalk|appengine"

# Check that the 'postgres' is no longer running.
is_pg_running=`ps -e | grep postgres`

# Check that the 'gehttpd' is no longer running.
is_httpd_running=`ps -e | grep gehttpd`

# Check that the 'wsgi' is no longer running.
is_wsgi_running=`ps -e | grep wsgi:ge_`

# Check if the logged in user is root
if [ `whoami` != root ]
then
        echo ---------------
        echo No Permission
        echo ---------------
        echo
        echo You must have root privileges to uninstall Google Earth Enterprise Server.
        echo Log in as the root user or use the "sudo" command to run this uninstaller.
        echo
        :
elif [ ! -d /opt/google/install/GoogleEarthServer_Uninstaller/ ]
then

echo Google Earth Enterprise Server FUSION_VERSION_NUMBER is not installed on this system.

elif [ "$is_pg_running" != "" ] || [ "$is_httpd_running" != "" ] || [ "$is_wsgi_running" != "" ]
then

cat <<END_PARA
---------------
Google Earth Enterprise Server must be stopped before uninstallation.
---------------
The Google Earth Enterprise Server is running.
Please stop the server using the command:
"   /etc/init.d/geserver stop"

Then restart the uninstaller.

If you have stopped the Google Earth Enterprise Server as shown above, and you still receive this message, then you may need to manually stop all the Google Earth Enterprise Server processes.
Please run the following script to detect the Google Earth Enterprise Server processes:

END_PARA

echo "(ls -1 \`grep -l "/opt/google\|wsgi:ge_" /proc/[1-9]*/cmdline\` 2>/dev/null | cut -f3 -d'/'; ls -l /proc/*/exe 2>/dev/null | egrep -vi \"$ignored_google_processes\" | grep '/opt/google\|wsgi:ge_' | cut -f3 -d'/') | xargs -r ps -p | egrep -vi  \"$ignored_google_processes\""

else

# Start the uninstaller
/opt/google/install/GoogleEarthServer_Uninstaller/Uninstall_Google_Earth_Enterprise_Server_FUSION_VERSION_NUMBER -i CONSOLE

fi
