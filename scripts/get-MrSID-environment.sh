#!/usr/bin/env bash

#
# Copyright 2017 Open GEE Contributors
#
# Licensed under the Apache license, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the license.
#

# This scripts installs packages, downloads and builds software used to
# download the Open GEE source code and build it.
#   It runs on all supported platforms.
#   This script does not share code with other shell scripts so that it can be
# distributed as a single file that can be used to obtain a build environment
# and source code for Open GEE.

SELF_NAME=$(basename "$0")

is_string_value_false()
{
    case $(echo "$1" | tr '[:upper:]' '[:lower:]') in
        false|off|no|0)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}


#===  FUNCTION  ================================================================
#          NAME:  check_root
#   DESCRIPTION:  checks to see if the user has root permissions
#    PARAMETERS:  none
#       RETURNS:  nothing
#===============================================================================
function check_root ()
{
# id -u gives the user ID number. if 0, then root
    if ! [ $(id -u) = 0 ]; then
        echo "$SELF_NAME Error: Must be root!"
	echo
	exit 1
    fi
}

check_root

MRSIDTGZ="MrSID_DSDK-9.5.4.4703-rhel6.x86-64.gcc482.tar.gz"
echo "MRSIDTGZ=$MRSIDTGZ"
wget http://bin.lizardtech.com/download/developer/$MRSIDTGZ -O /tmp/$MRSIDTGZ
tar -xzf /tmp/$MRSIDTGZ -C /opt
echo /opt/MrSID_DSDK-9.5.4.4703-rhel6.x86-64.gcc482/Raster_DSDK/lib >>/etc/ld.so.conf
/sbin/ldconfig

