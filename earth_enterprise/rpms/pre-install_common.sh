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

# NOTE: requires xmllint from libxml2-utils

set +x

#------------------------------------------------------------------------------
# from common.sh:

BADHOSTNAMELIST=(empty linux localhost dhcp bootp)

# versions and user names
GEE="Google Earth Enterprise"
GEES="$GEE Server"
LONG_VERSION="5.2.1"

# directory locations
BASEINSTALLDIR_OPT="/opt/google"

# Get system info values
NEWLINECLEANER="sed -e s:\\n::g"
HOSTNAME="$(hostname -f | tr [A-Z] [a-z] | $NEWLINECLEANER)"
HOSTNAME_F="$(hostname -f | $NEWLINECLEANER)"

check_bad_hostname()
{
    if [ -z "$HOSTNAME" ] || [[ " ${BADHOSTNAMELIST[*]} " == *"${HOSTNAME,,} "* ]]; then
        show_badhostname
        return 0
    fi
}

show_badhostname()
{
    echo -e "\nYour server [$HOSTNAME] contains an invalid hostname value which typically"
    echo -e "indicates an automatically generated hostname that might change over time."
    echo -e "A subsequent hostname change would cause configuration issues for the "
    echo -e "$SOFTWARE_NAME software.  Invalid values: ${BADHOSTNAMELIST[*]}."
}

check_mismatched_hostname()
{
    if [ $HOSTNAME != $HOSTNAME_F ]; then
        show_mismatchedhostname
        return 0
    fi
}

show_mismatchedhostname()
{
    echo -e "\nThe hostname of this machine does not match the fully-qualified hostname."
    echo -e "$SOFTWARE_NAME requires that they match for local publishing to function properly."
}

check_group()
{
    GROUP_EXISTS=$(getent group $GROUPNAME)

    # add group if it does not exist
    if [ -z "$GROUP_EXISTS" ]; then
        groupadd -r $GROUPNAME &> /dev/null 
        NEW_GEGROUP=true 
    fi
}

#------------------------------------------------------------------------------

# config values
PUBLISHER_ROOT="/gevol/published_dbs"
DEFAULTGROUPNAME="gegroup"
GROUPNAME=$DEFAULTGROUPNAME
DEFAULTGEFUSIONUSER_NAME="gefusionuser"
GEFUSIONUSER_NAME=$DEFAULTGEFUSIONUSER_NAME

# script arguments
BACKUPSERVER=true
BADHOSTNAMEOVERRIDE=false
MISMATCHHOSTNAMEOVERRIDE=false

# user names
GEPGUSER_NAME="gepguser"
GEAPACHEUSER_NAME="geapacheuser"


#-----------------------------------------------------------------
# Main Functions
#-----------------------------------------------------------------
main_preinstall()
{
    check_bad_hostname

    check_mismatched_hostname

    # 8) Check if group and users exist
    check_group
}

#-----------------------------------------------------------------
main_preinstall "$@"
