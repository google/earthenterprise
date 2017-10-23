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

set +x

#------------------------------------------------------------------------------
# from common.sh:

BADHOSTNAMELIST=(empty linux localhost dhcp bootp)

# versions and user names
GEE="Google Earth Enterprise"
GEES="$GEE Server"
LONG_VERSION="5.2.1"

ROOT_USERNAME="root"

MACHINE_OS=""
MACHINE_OS_VERSION=""
MACHINE_OS_FRIENDLY=""

# directory locations
BASEINSTALLDIR_OPT="/opt/google"
BASEINSTALLDIR_VAR="/var/opt/google"

# derived directories
BASEINSTALLGDALSHAREDIR="$BASEINSTALLDIR_OPT/share/gdal"
GENERAL_LOG="$BASEINSTALLDIR_VAR/log"
INSTALL_LOG_DIR="$BASEINSTALLDIR_OPT/install"
GEHTTPD="$BASEINSTALLDIR_OPT/gehttpd"
GEHTTPD_CONF="$GEHTTPD/conf.d"

# Get system info values
NEWLINECLEANER="sed -e s:\\n::g"
HOSTNAME="$(hostname -f | tr [A-Z] [a-z] | $NEWLINECLEANER)"
HOSTNAME_F="$(hostname -f | $NEWLINECLEANER)"

check_bad_hostname() {
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

check_mismatched_hostname() {
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

check_username() {
    USERNAME_EXISTS=$(getent passwd $1)

    # add user if it does not exist
    if [ -z "$USERNAME_EXISTS" ]; then
        mkdir -p $BASEINSTALLDIR_OPT/.users/$1
        useradd --home $BASEINSTALLDIR_OPT/.users/$1 --system --gid $GROUPNAME $1
        NEW_GEFUSIONUSER=true
    else
        echo "User $1 exists"
        # user already exists -- update primary group
        usermod -g $GROUPNAME $1
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
    # 5a) Check previous installation
    if [ -f "$SERVERBININSTALL" ]; then
        IS_NEWINSTALL=false
    else
        IS_NEWINSTALL=true
    fi

    # 7) Check prerequisite software

    # check to see if GE Server processes are running
    service geserver stop

    # 6) Check valid host properties
    check_bad_hostname
    check_mismatched_hostname

    # 8) Check if users exist
    check_username $GEAPACHEUSER_NAME
    check_username $GEPGUSER_NAME

    # 10) Configure Publish Root
    configure_publish_root
}

#-----------------------------------------------------------------
# Pre-install Functions
#-----------------------------------------------------------------

configure_publish_root()
{
    # Update PUBLISHER_ROOT if geserver already installed
    local STREAM_SPACE="$GEHTTPD_CONF/stream_space"
    if [ -e $STREAM_SPACE ]; then
        PUBLISHER_ROOT=`cat $STREAM_SPACE |cut -d" " -f3 |sed 's/.\{13\}$//'`
    fi
}

#-----------------------------------------------------------------
main_preinstall "$@"
